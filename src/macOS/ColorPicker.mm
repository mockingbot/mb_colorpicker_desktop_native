#include "ColorPicker.hxx"

MainWindow* TheMainWindow = nullptr;

CGFloat CAPTUREED_PIXEL_COLOR_R [CAPTURE_HEIGHT][CAPTURE_WIDTH];
CGFloat CAPTUREED_PIXEL_COLOR_G [CAPTURE_HEIGHT][CAPTURE_WIDTH];
CGFloat CAPTUREED_PIXEL_COLOR_B [CAPTURE_HEIGHT][CAPTURE_WIDTH];

CGFloat TheR, TheG, TheB;

@implementation AppDelegate
{
    NSWindow* window;
    MouseEventHook hook;
}
- (id)init
{
    if (self = [super init]) {
        window = [MainWindow new];
    }
    return self;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication* )sender
{
    return YES;
}

@end


@implementation MainWindow
{
    BOOL isESCKeyPressed;
}

- (id)init
{
    CGPoint mouse_pos;
    {
        auto event = CGEventCreate(nullptr);
        mouse_pos = CGEventGetUnflippedLocation(event);
        mouse_pos.x -= UI_WINDOW_WIDTH/2.0f;
        mouse_pos.y -= UI_WINDOW_HEIGHT/2.0f;
        CFRelease(event);
    }

    CGRect wnd_rect;
    wnd_rect.origin = mouse_pos;
    wnd_rect.size.width = UI_WINDOW_WIDTH;
    wnd_rect.size.height = UI_WINDOW_HEIGHT;

    self = [super initWithContentRect: wnd_rect
                  styleMask: NSBorderlessWindowMask
                  backing: NSBackingStoreBuffered
                  defer:NO
                  ];

    self.backgroundColor = NSColor.clearColor;
    // self.backgroundColor = NSColor.redColor;

    [self setLevel: NSStatusWindowLevel];
    [self makeKeyAndOrderFront: nil];
    [self setAlphaValue: 1.0];
    [self setOpaque: NO];
    [self setContentView: [MainView new]];

    auto timer = [NSTimer scheduledTimerWithTimeInterval: 1.0/25.0
                          target: self
                          selector: @selector(onTimerTick:)
                          userInfo: nil
                          repeats: YES];

    [[NSRunLoop currentRunLoop] addTimer:timer forMode: NSDefaultRunLoopMode];

    [NSCursor hide];

    TheMainWindow = self;

    isESCKeyPressed = NO;

    return self;
}

- (void)onTimerTick:(NSTimer* )timer
{
    // refresh first!
    [[self contentView] refreshPictureSurroundCurrentCursor];
    // force redraw
    [[self contentView] display];
}

- (void)keyDown:(NSEvent *)event {
    if (event.keyCode == 53) {
        isESCKeyPressed = YES;
        [self close];
        return;
    }
    [super keyDown:event];
}

- (void)close
{
    [NSCursor unhide];
    if (isESCKeyPressed) {
        printf("");
    } else {
        int r = round(TheR*255.0f);
        int g = round(TheG*255.0f);
        int b = round(TheB*255.0f);
        printf("#%02X%02X%02X\n", r, g, b);
    }
    fflush(stdout);
    [super close];
}

- (BOOL)canBecomeKeyWindow
{
  return YES;
}

@end


@implementation MainView

CGImage* mask_circle;

const uint32_t display_id_list_size = 16; // 16 display is enought

CGDirectDisplayID display_id_list[display_id_list_size] = {};
CGColorSpaceRef color_space_list[display_id_list_size] = {};
CGRect display_bound_list[display_id_list_size] = {};

uint32_t display_count = 0;

CGImage* image_surround_current_cursor = nullptr;

CGImage* zoomed_image_surround_current_cursor = nullptr;

CGColorSpace* current_color_space = nullptr;

- (id)init
{
    self = [super init];

    auto load_image = [](NSString* url)->CGImage*
    {
        auto image = [[NSImage alloc] initWithContentsOfFile: url];
        auto repre = [[image representations] objectAtIndex:0];
        auto pixel_width = [repre pixelsWide];
        auto pixel_hight = [repre pixelsHigh];

        [repre setSize: NSMakeSize(pixel_width, pixel_hight)];
        [image setSize: NSMakeSize(pixel_width, pixel_hight)];

        [image addRepresentation: repre];
        [image removeRepresentation:[[image representations] objectAtIndex:0]];

        auto context = [NSGraphicsContext currentContext];
        auto imageCGRect = CGRectMake(0, 0, pixel_width, pixel_hight);
        auto imageRect = NSRectFromCGRect(imageCGRect);

        return [image CGImageForProposedRect:&imageRect context:context hints:nil];
    };

    mask_circle = load_image([[NSBundle mainBundle] pathForResource:@"Mask" \
                                                    ofType:@"png"]);

    if( kCGErrorSuccess != CGGetActiveDisplayList(display_id_list_size, \
                                                  display_id_list, \
                                                  &display_count) )
    {
        throw std::runtime_error("CGGetActiveDisplayList Failed\n");
    }

    for(uint32_t idx=0; idx < display_count; ++idx){
        color_space_list[idx] = CGDisplayCopyColorSpace(display_id_list[idx]);
        // in the global display coordinate space
        display_bound_list[idx] = CGDisplayBounds(display_id_list[idx]);
    }

    return self;
}

- (void)refreshPictureSurroundCurrentCursor
{
    if( image_surround_current_cursor != nullptr ){
        CGImageRelease(image_surround_current_cursor);
    }
    if( zoomed_image_surround_current_cursor != nullptr ){
        CGImageRelease(zoomed_image_surround_current_cursor);
    }

    auto window_list = CGWindowListCreate(kCGWindowListOptionOnScreenOnly, \
                                                                kCGNullWindowID);

    const auto window_list_size = CFArrayGetCount(window_list);

    auto window_list_filtered = CFArrayCreateMutableCopy(kCFAllocatorDefault, \
                                                  window_list_size, window_list);

    auto main_window_id = self.window.windowNumber;
    for (int idx = window_list_size - 1; idx >= 0; --idx)
    {
        auto window = CFArrayGetValueAtIndex(window_list, idx);
        if( main_window_id == (CGWindowID)(uintptr_t)window ) {
            CFArrayRemoveValueAtIndex(window_list_filtered, idx);
        }
    }

    auto event = CGEventCreate(NULL);
    // global display coordinates.
    const auto cursor_position = CGEventGetLocation(event);
    CFRelease(event);

    uint32_t display_id_idx = 0xFFFF;
    for(uint32_t idx = 0; idx < display_count; ++idx)
    {
        const auto& rect = display_bound_list[idx];
        if( (true == CGRectContainsPoint(rect, cursor_position)) )
        {
            display_id_idx = idx;
            break;
        }
    }

    if( display_id_idx == 0xFFFF )
    {
        CGPoint check_points[4];
        check_points[0].x = cursor_position.x - 10;
        check_points[0].y = cursor_position.y - 10;
        check_points[1].x = cursor_position.x + 10;
        check_points[1].y = cursor_position.y - 10;
        check_points[2].x = cursor_position.x - 10;
        check_points[2].y = cursor_position.y + 10;
        check_points[3].x = cursor_position.x + 10;
        check_points[3].y = cursor_position.y + 10;

        auto check_marks = new int[display_count];
        for(uint32_t idx = 0; idx < display_count; ++idx)
        {
            int mark = 0;
            const auto& rect = display_bound_list[idx];
            if( (true == CGRectContainsPoint(rect, check_points[0])) ){
                mark++;
            }
            if( (true == CGRectContainsPoint(rect, check_points[1])) ){
                mark++;
            }
            if( (true == CGRectContainsPoint(rect, check_points[2])) ){
                mark++;
            }
            if( (true == CGRectContainsPoint(rect, check_points[3])) ){
                mark++;
            }
            check_marks[idx] = mark;
        }

        int max_mark = -1;
        for(uint32_t idx = 0; idx < display_count; ++idx)
        {
            if( check_marks[idx] > max_mark ){
                max_mark= check_marks[idx];
                display_id_idx = idx;
            }
        }

        delete[] check_marks;
    }

    current_color_space = color_space_list[display_id_idx];

    CGRect rect;
    rect.origin.x = int(cursor_position.x) - CAPTURE_WIDTH/2;
    rect.origin.y = int(cursor_position.y) - CAPTURE_HEIGHT/2;
    rect.size.width = CAPTURE_WIDTH;
    rect.size.height = CAPTURE_HEIGHT;

    auto cg_image = CGWindowListCreateImageFromArray(rect, window_list_filtered, \
                                                    kCGWindowImageNominalResolution);
    auto ns_bitmap_image = [[NSBitmapImageRep alloc] initWithCGImage: cg_image];

    // fix color space here
    for(int y = 0; y < CAPTURE_HEIGHT; ++y)
    {
       for(int x = 0; x < CAPTURE_WIDTH; ++x)
       {
            CGFloat color_values[] = {0/255.f, 0/255.f, 0/255.f, 1.0f};
            @autoreleasepool
            {
                auto color = [ns_bitmap_image colorAtX: x y: y];
                color_values[0] = [color redComponent  ];
                color_values[1] = [color greenComponent];
                color_values[2] = [color blueComponent ];
            }

            CGFloat fixed_red, fixed_blue, fixed_green;
            @autoreleasepool
            {
                auto color = CGColorCreate(current_color_space, color_values);
                auto ns_color = [NSColor colorWithCGColor: color];
                auto color_space_sRGB = [NSColorSpace sRGBColorSpace];
                auto fixed_color = [ns_color colorUsingColorSpace: color_space_sRGB];
                fixed_red   = [fixed_color redComponent  ];
                fixed_green = [fixed_color greenComponent];
                fixed_blue  = [fixed_color blueComponent ];
                CFRelease(color);
            }
            CAPTUREED_PIXEL_COLOR_R[CAPTURE_HEIGHT-1-y][x] = fixed_red;
            CAPTUREED_PIXEL_COLOR_G[CAPTURE_HEIGHT-1-y][x] = fixed_green;
            CAPTUREED_PIXEL_COLOR_B[CAPTURE_HEIGHT-1-y][x] = fixed_blue;
       }
    }

    [ns_bitmap_image release];
    CFRelease(window_list_filtered);
    CFRelease(window_list);

    image_surround_current_cursor = cg_image;

    ////////////////////////////////////////////////////////////////////////////
    auto color_space = CGColorSpaceCreateDeviceRGB();
    auto ctx = CGBitmapContextCreate(nullptr, \
                 UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT, 32, 0, color_space, \
                 kCGImageAlphaPremultipliedLast|kCGBitmapFloatComponents);

    CGRect wnd_rect;
    wnd_rect.origin.x = 0;
    wnd_rect.origin.y = 0;
    wnd_rect.size.width = UI_WINDOW_WIDTH;
    wnd_rect.size.height = UI_WINDOW_HEIGHT;

    CGRect mask_bound;
    mask_bound.origin.x = 8 + 2;
    mask_bound.origin.y = 8 + 2;
    mask_bound.size.width = UI_WINDOW_WIDTH - (8+2)*2;
    mask_bound.size.height = UI_WINDOW_HEIGHT - (8+2)*2;

    CGContextSaveGState(ctx);

    auto clip_path = CGPathCreateWithEllipseInRect(mask_bound, nil);
    CGContextAddPath(ctx, clip_path);
    CGContextClip(ctx);
    CGContextSetLineWidth(ctx, 1.0f);
    CGContextSetShouldAntialias(ctx, NO);

    /*********************************************************************/
    // draw background for grid
    auto grid_color = CGColorCreateGenericRGB(0.72f, 0.72f, 0.72f, 0.98f);
    CGContextSetFillColorWithColor(ctx, grid_color);
    CGContextFillRect(ctx, wnd_rect);
    CFRelease(grid_color);

    /*********************************************************************/
    // draw each pixel
    CGFloat color_values[] = {0/255.f, 0/255.f, 0/255.f, 1.0f};
    for(int y = 0; y < CAPTURE_HEIGHT; ++y)
    {
       for(int x = 0; x < CAPTURE_WIDTH; ++x)
       {
            auto r = CAPTUREED_PIXEL_COLOR_R[y][x];
            auto g = CAPTUREED_PIXEL_COLOR_G[y][x];
            auto b = CAPTUREED_PIXEL_COLOR_B[y][x];
            CGContextSetRGBFillColor(ctx, r, g, b, 1.0);

            CGRect rect;
            rect.origin.x = 8 + 1 + (1+GRID_PIXEL)*x;
            rect.origin.y = 8 + 1 + (1+GRID_PIXEL)*y;
            rect.size.width = GRID_PIXEL;
            rect.size.height = GRID_PIXEL;
            CGContextFillRect(ctx, rect);
       }
    }

    { // black and white and the center pixel color
        int x = GRID_NUMUBER_L;
        int y = GRID_NUMUBER_L;
        CGRect rect;
        rect.origin.x = 8 + 1 + (1+GRID_PIXEL)*x - 1;
        rect.origin.y = 8 + 1 + (1+GRID_PIXEL)*y - 1;
        rect.size.width = GRID_PIXEL + 2;
        rect.size.height = GRID_PIXEL + 2;

        CGContextSetRGBFillColor(ctx, 0.0f, 0.0f, 0.0f, 1.0f);
        CGContextFillRect(ctx, rect);

        rect.origin.x = 8 + 1 + (1+GRID_PIXEL)*x;
        rect.origin.y = 8 + 1 + (1+GRID_PIXEL)*y;
        rect.size.width = GRID_PIXEL;
        rect.size.height = GRID_PIXEL;

        CGContextSetRGBFillColor(ctx, 1.0f, 1.0f, 1.0f, 1.0f);
        CGContextFillRect(ctx, rect);


        rect.origin.x = 8 + 1 + (1+GRID_PIXEL)*x + 1;
        rect.origin.y = 8 + 1 + (1+GRID_PIXEL)*y + 1;
        rect.size.width = GRID_PIXEL - 2;
        rect.size.height = GRID_PIXEL - 2;

        TheR = CAPTUREED_PIXEL_COLOR_R[y][x];
        TheG = CAPTUREED_PIXEL_COLOR_G[y][x];
        TheB = CAPTUREED_PIXEL_COLOR_B[y][x];
        CGContextSetRGBFillColor(ctx, TheR, TheG, TheB, 1.0);
        CGContextFillRect(ctx, rect);
    }

    CFRelease(clip_path);
    CGContextRestoreGState(ctx);

    zoomed_image_surround_current_cursor = CGBitmapContextCreateImage(ctx);

    CGContextRelease(ctx);
    CGColorSpaceRelease(color_space);
}

- (void)drawRect:(NSRect)dirtyRect
{
    auto ctx = [NSGraphicsContext currentContext].CGContext;

    CGRect wnd_rect;
    wnd_rect.origin.x = 0;
    wnd_rect.origin.y = 0;
    wnd_rect.size.width = UI_WINDOW_WIDTH;
    wnd_rect.size.height = UI_WINDOW_HEIGHT;

    // draw the zoomed image suround cursor
    CGContextDrawImage(ctx, wnd_rect, zoomed_image_surround_current_cursor);
    // draw the mask
    CGContextDrawImage(ctx, wnd_rect, mask_circle);
}

@end


CGEventRef
MouseEventHook::Callback(CGEventTapProxy proxy, \
                            CGEventType type, CGEventRef event, void* refcon)
{
    if( TheMainWindow == nullptr ) {
        return event;
    }
    // relative to the lower-left corner of the main display.
    auto mouse_pos = CGEventGetUnflippedLocation(event);
    mouse_pos.x -= UI_WINDOW_WIDTH/2.0f;
    mouse_pos.y -= UI_WINDOW_HEIGHT/2.0f;

    switch(type)
    {
        case kCGEventMouseMoved:
        {
            // NSLog(@"move");
            [NSApp activateIgnoringOtherApps: YES];
            [TheMainWindow setFrameOrigin: mouse_pos];
        }
        break;
        case kCGEventLeftMouseDown:
        case kCGEventRightMouseDown:
        // case kCGEventRightMouseUp:
        // case kCGEventLeftMouseUp:
        {
            [TheMainWindow close];
        }
        break;
        default:
        break;
    }

    return event;
}

MouseEventHook::MouseEventHook()
{
    CGEventMask emask = {0};
    emask |= CGEventMaskBit(kCGEventMouseMoved);
    emask |= CGEventMaskBit(kCGEventRightMouseDown);
    emask |= CGEventMaskBit(kCGEventLeftMouseDown);
    // emask |= CGEventMaskBit(kCGEventRightMouseUp);
    // emask |= CGEventMaskBit(kCGEventLeftMouseUp);

    mouse_event_tap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGTailAppendEventTap,
        kCGEventTapOptionListenOnly,
        emask,
        &MouseEventHook::Callback,
        NULL
    );

    event_tap_src_ref = CFMachPortCreateRunLoopSource(
        kCFAllocatorDefault,
        mouse_event_tap,
        0
    );

    CFRunLoopAddSource(
        CFRunLoopGetCurrent(),
        event_tap_src_ref,
        kCFRunLoopDefaultMode
    );
}

MouseEventHook::~MouseEventHook()
{
    CFRunLoopRemoveSource(
        CFRunLoopGetCurrent(),
        event_tap_src_ref,
        kCFRunLoopDefaultMode
    );

    CFRelease(mouse_event_tap);
}

int main(int argc, const char * argv[]) {
    NSApplication * app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
    app.delegate = [AppDelegate new];
    [app run];
    return 0;
}
