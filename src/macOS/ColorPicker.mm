#include "ColorPicker.hxx"

MainWindow* TheMainWindow = nullptr;

@implementation Application

- (id)init
{
    self = [super init];
    self.delegate = self;
    return self;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication* )sender
{
    return YES;
}

@end


@implementation MainWindow

- (id)init
{
    int x, y;
    // CGPoint mouse_pos
    {
        auto event = CGEventCreate(nullptr);
        auto mouse_pos = CGEventGetUnflippedLocation(event);
        mouse_pos.x -= UI_WINDOW_WIDTH/2.0f;
        mouse_pos.y -= UI_WINDOW_HEIGHT/2.0f;
        CFRelease(event);
    }

    auto wnd_rect = NSMakeRect(x, y, UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT);
    // auto wnd_rect = NSMakeRect(0, 0, UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT);

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

    // [self setFrameOrigin: mouse_pos];
    TheMainWindow = self;

    [HostForGrabPictureSurroundCurrentCursor new];

    return self;
}

- (BOOL)canBecomeKeyWindow
{
  return YES;
}

@end


@implementation MainView

CGImage* mask_circle_x1;
CGImage* mask_circle_x2;

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

    mask_circle_x1 = load_image([[NSBundle mainBundle]
                                  pathForResource:@"Mask@1"
                                  ofType:@"png"]);

    mask_circle_x2 = load_image([[NSBundle mainBundle]
                                  pathForResource:@"Mask@2"
                                  ofType:@"png"]);

    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    printf("ccccccccccccccccccccccccccccccccccccccc\n");
    auto ctx = [NSGraphicsContext currentContext].CGContext;

    auto clip_bound = NSMakeRect(4, 4, 163, 163);
    auto clip_path = CGPathCreateWithEllipseInRect(clip_bound, nil);

    CGContextSaveGState(ctx);
    CGContextAddPath(ctx, clip_path);
    CGContextClip(ctx);
    CGContextSetLineWidth(ctx, 1.0f);
    CGContextSetShouldAntialias(ctx, NO);

    // TODO:
    {
        auto color = CGColorCreateGenericRGB(0.62f, 0.62f, 0.62f, 0.92f);
        CGContextSetFillColorWithColor(ctx, color);
        CGRect rect;
        rect.origin.x = 0;
        rect.origin.y = 0;
        rect.size.width = UI_WINDOW_WIDTH;
        rect.size.height = UI_WINDOW_HEIGHT;
        CGContextFillRect(ctx, rect);
    }

    CGContextRestoreGState(ctx);

    CGRect mask_bound;
    mask_bound.origin.x = 0;
    mask_bound.origin.y = 0;
    mask_bound.size.width = UI_WINDOW_WIDTH;
    mask_bound.size.height = UI_WINDOW_WIDTH;

    CGContextDrawImage(ctx, mask_bound, mask_circle_x1);
}

@end


@implementation HostForGrabPictureSurroundCurrentCursor


const uint32_t display_id_list_size = 16; // 16 display is enought

CGDirectDisplayID display_id_list[display_id_list_size] = {};
CGColorSpaceRef color_space_list[display_id_list_size] = {};
CGRect display_bound_list[display_id_list_size] = {};

uint32_t display_count = 0;

- (id)init
{
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

    self = [super init];
    auto timer = [NSTimer scheduledTimerWithTimeInterval: 1.0/25.0
                          target: self
                          selector: @selector(onTick:)
                          userInfo: nil
                          repeats: YES];

    [[NSRunLoop currentRunLoop] addTimer:timer forMode: NSDefaultRunLoopMode];


    return self;
}

- (void)onTick:(NSTimer* )timer
{
    if( TheMainWindow == nullptr ){
        return;
    }

    auto window_list = CGWindowListCreate(kCGWindowListOptionOnScreenOnly, \
                                                                kCGNullWindowID);
    auto window_list_size = CFArrayGetCount(window_list);

    auto window_list_filtered = CFArrayCreateMutableCopy(kCFAllocatorDefault, \
                                                  window_list_size, window_list);

    auto main_window_id = TheMainWindow.windowNumber;
    for (int idx = window_list_size - 1; idx >= 0; --idx)
    {
        auto window = CFArrayGetValueAtIndex(window_list, idx);
        if( main_window_id == (CGWindowID)(uintptr_t)window ) {
            CFArrayRemoveValueAtIndex(window_list_filtered, idx);
        }
    }

    auto event = CGEventCreate(NULL);
    // global display coordinates.
    auto cursor_position = CGEventGetLocation(event);
    CFRelease(event);

    printf("cursor_position % 12.4f % 12.4f\n", cursor_position.x, cursor_position.y);

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

    printf("display_id_idx %d\n", display_id_idx);

    auto current_color_space = color_space_list[display_id_idx];

    [[TheMainWindow contentView] display];
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

int main(int argc, const char * argv[])
{
    MouseEventHook hook;

    @autoreleasepool {
        [Application new];
        [MainWindow new];
        [NSApp run];
    }
    return 0;
}
