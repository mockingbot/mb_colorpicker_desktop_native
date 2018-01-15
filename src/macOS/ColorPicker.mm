#include "ColorPicker.hxx"

MainWindow* TheMainWindow;
MainView* TheMainView;

NSTimer* TheUpdateTimer;

const uint32_t display_id_list_size = 16; // 16 display is enought
uint32_t display_count = 0;

CGDirectDisplayID display_id_list[display_id_list_size] = {};

CGRect display_bound_list[display_id_list_size] = {};
CGColorSpaceRef display_color_space_list[display_id_list_size] = {};


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
    auto empty_event = CGEventCreate(nullptr);
    auto cursor_position = CGEventGetLocation(empty_event);
    CFRelease(empty_event);

    int x = (int)cursor_position.x - UI_WINDOW_WIDTH/2.0;
    int y = (int)cursor_position.y - UI_WINDOW_WIDTH/2.0;

    std::cout<< ">>>>>>>> " << x << " " << y << "\n";

    auto wnd_rect = NSMakeRect(x, y, UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT);

    self = [super initWithContentRect: wnd_rect
                  styleMask: NSBorderlessWindowMask 
                  backing: NSBackingStoreBuffered 
                  defer:NO
                  ];

    self.backgroundColor = NSColor.clearColor;
    // self.backgroundColor = NSColor.redColor;
    [self setLevel: NSStatusWindowLevel];
    [self setAlphaValue: 1.0];
    [self setOpaque: NO];
    [self makeKeyAndOrderFront: nil];

    auto bounds = [self frame];
    bounds.origin = NSZeroPoint;

    auto view = [[MainView alloc] init];
    [super setContentView: view];

    TheMainWindow = self;
    TheMainView = view;

    return self;
}

- (BOOL)canBecomeKeyWindow 
{
  return YES;
}

@end


@implementation MainView

// NSImage* mask_circle_x1;
// NSImage* mask_circle_x2;
CGImage* mask_circle_x1;
CGImage* mask_circle_x2;
// CGImage* image_mask_circle_x1;
// CGImage* image_mask_circle_x2;

- (id)init
{
    self = [super init];

    auto load_image = [](NSString* url) {
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
        // return image;
    };

    mask_circle_x1 = load_image( [[NSBundle mainBundle] 
                                  pathForResource:@"Mask@1" 
                                  ofType:@"png"] );

    mask_circle_x2 = load_image( [[NSBundle mainBundle] 
                                  pathForResource:@"Mask@2" 
                                  ofType:@"png"] );

    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    // NSLog(@"draw");

    auto ctx = [NSGraphicsContext currentContext].CGContext;
    // std::cout << "ctx " << ctx << '\n';

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
    auto cursor_position = CGEventGetLocation(event);
    int x = int(cursor_position.x) - CAPTURE_WIDTH/2.0;
    int y = int(cursor_position.y) - CAPTURE_HEIGHT/2.0;
    auto rect = NSMakeRect(x, y, CAPTURE_WIDTH, CAPTURE_HEIGHT);
    CFRelease(event);

    auto cg_image = CGWindowListCreateImageFromArray(rect, \
                                            window_list_filtered, \
                                            kCGWindowImageNominalResolution);

    auto ns_bitmap_image = [[NSBitmapImageRep alloc] initWithCGImage: cg_image];

    uint32_t display_id_idx = -1;
    for(uint32_t idx=0; idx < display_count; ++idx){
        const auto& rect = display_bound_list[idx];
        if( true == CGRectContainsPoint(rect, cursor_position) ){
            display_id_idx = idx;
            break;
        }
    }
    auto current_display_color_space = display_color_space_list[display_id_idx];

    auto clip_bound = NSMakeRect(4, 4, 163, 163);
    auto clip_path = CGPathCreateWithEllipseInRect(clip_bound, nil);

    CGContextSaveGState(ctx);
    CGContextAddPath(ctx, clip_path);
    CGContextClip(ctx);
    CGContextSetLineWidth(ctx, 1.0f);
    CGContextSetShouldAntialias(ctx, NO);

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

    // /*
    @autoreleasepool {

    for(int idx_y = 0; idx_y < CAPTURE_HEIGHT; ++idx_y)
    {
       for(int idx_x = 0; idx_x < CAPTURE_WIDTH; ++idx_x)
       {
            int x = idx_x;
            int y = CAPTURE_HEIGHT - 1 - idx_y;

            auto ns_color = [ns_bitmap_image colorAtX: x y: y];
            auto red   = [ns_color redComponent  ];
            auto green = [ns_color greenComponent];
            auto blue  = [ns_color blueComponent ];

            CGFloat color_values[] = {red, green, blue, 1.0f};
            auto tmp_color = CGColorCreate(current_display_color_space, color_values);

            CGFloat fixed_red, fixed_blue, fixed_green;
            {
                NSColor* color = [NSColor colorWithCGColor: tmp_color];
                auto color_space_sRGB = [NSColorSpace sRGBColorSpace];
                auto fixed_color = [color colorUsingColorSpace: color_space_sRGB];

                fixed_red   = [fixed_color redComponent  ];
                fixed_green = [fixed_color greenComponent];
                fixed_blue  = [fixed_color blueComponent ];
            }
            auto color = CGColorCreateGenericRGB(fixed_red, fixed_green, fixed_blue, 1.0f);
            // auto color = CGColorCreateGenericRGB(0.0f, 1.0f, 0.0f, 1.0f);
        
            CGRect rect;

            CGContextSetFillColorWithColor(ctx, color);
            rect.origin.x = (GRID_PIXEL+1)*idx_x + 1;
            rect.origin.y = (GRID_PIXEL+1)*idx_y + 1;
            rect.size.width = GRID_PIXEL;
            rect.size.height = GRID_PIXEL;
            CGContextFillRect(ctx, rect);
       }
    }

    } // autorelease
    // */

    CGContextRestoreGState(ctx);

    CGRect mask_bound;
    mask_bound.origin.x = 0;
    mask_bound.origin.y = 0;
    mask_bound.size.width = UI_WINDOW_WIDTH;
    mask_bound.size.height = UI_WINDOW_WIDTH;

    CGContextDrawImage(ctx, mask_bound, mask_circle_x1);

    [ns_bitmap_image release];

    CFRelease(window_list_filtered);
    CFRelease(window_list);
    CGImageRelease(cg_image);

    // [super drawRect:dirtyRect];
}

@end

@implementation ScreenCaptureHost 

- (id)init
{
    self = [super init];

    auto timer = [NSTimer scheduledTimerWithTimeInterval: 1.0/25.0
                          target: self
                          selector: @selector(onTick:)
                          userInfo: nil
                          repeats: YES];
    TheUpdateTimer = timer;

    [[NSRunLoop currentRunLoop] addTimer:timer forMode: NSDefaultRunLoopMode];

    if( kCGErrorSuccess != CGGetActiveDisplayList(16, display_id_list, &display_count) ){
        throw std::runtime_error("CGGetActiveDisplayList Failed\n");
    }

    for(uint32_t idx=0; idx < display_count; ++idx){
        display_bound_list[idx] = CGDisplayBounds(display_id_list[idx]);
        display_color_space_list[idx] = CGDisplayCopyColorSpace(display_id_list[idx]);
    }

    // new std::thread([](){

    // });

    return self;
}

- (void)onTick:(NSTimer* )timer 
{
    if( TheMainWindow != nullptr ) 
    {
        [TheMainView setNeedsDisplay: YES];
    }
}

@end

CGEventRef MouseEventCallback(CGEventTapProxy proxy, CGEventType type, \
                                            CGEventRef event, void * refcon)
{
    auto mouseLocation = CGEventGetUnflippedLocation(event);

    int x = int(mouseLocation.x) - 1 - UI_WINDOW_WIDTH/2;
    int y = int(mouseLocation.y) + 1 - UI_WINDOW_HEIGHT/2;
   
    std::cout << "kCGEventMouseMoved " << int(mouseLocation.x)  << " " <<  int(mouseLocation.y) << "\n";
    std::cout << "kCGEventMouseMoved " << mouseLocation.x  << " " << mouseLocation.y << "\n";
    std::cout << "kCGEventMouseMoved " << x  << " " << y << "\n";

    switch(type)
    {
        case kCGEventMouseMoved:
        {
            std::cout << "kCGEventMouseMoved " << x  << " " << y << "\n";
            std::cout << TheMainWindow << "\n";
            if( TheMainWindow != nullptr ) 
            {
                [TheMainWindow setFrameOrigin: NSMakePoint(x, y)];
            }
        }
        break;
        case kCGEventLeftMouseDown:
        case kCGEventRightMouseDown:
        {
            NSLog(@"kCGEventMouseDown");
            [TheUpdateTimer invalidate];
            [TheMainWindow close];
            TheMainWindow = nullptr;
        }
        break;
        case kCGEventRightMouseUp:
        case kCGEventLeftMouseUp:
            NSLog(@"kCGEventMouseUp");
        break;
        default:
        break;
    }

    return event;
}

MouseEventHook::MouseEventHook()
{
    CGEventMask emask = {0};
    emask |= CGEventMaskBit(kCGEventRightMouseDown);
    emask |= CGEventMaskBit(kCGEventRightMouseUp);
    emask |= CGEventMaskBit(kCGEventLeftMouseDown);
    emask |= CGEventMaskBit(kCGEventLeftMouseUp);
    emask |= CGEventMaskBit(kCGEventMouseMoved);

    mouse_event_tap = CGEventTapCreate (
        kCGSessionEventTap,
        kCGTailAppendEventTap,
        kCGEventTapOptionListenOnly,
        emask,
        &MouseEventCallback,
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
        [[Application alloc] init];
        
        [[MainWindow alloc] init];
        [[ScreenCaptureHost alloc] init];

        [NSApp run];
    }
    return 0;
}
