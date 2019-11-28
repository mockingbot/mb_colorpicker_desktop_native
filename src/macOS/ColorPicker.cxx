#include "ColorPicker.hxx"

#include "../Instance.hxx"
#include "../Predefined.hxx"


@implementation AppDelegate
{
    NSWindow* window;
}

-(id) init
{
    if (self = [super init]) {
        window = [MainWindow new];
    }
    return self;
}

-(BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication*)sender
{
    return NO;
}

@end

// CGImageRef image_caputured;

@implementation MainWindow
{
    NSTimer* fresh_timer;
    WindowIDList excluded_window_list;
    struct OffScreenRenderPixel* off_screen_render_data;
    uint32_t off_screen_render_data_fresh_ratio_count;
}

-(BOOL) canBecomeMainWindow
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    return YES;
}

-(BOOL) canBecomeKeyWindow
{
    // make sure our window can recive keyboard event
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    return YES;
}

-(id) init
{
    float x = 0.f, y = 0.f;
    GetCurrentCursorPosition(&x, &y);

    CGRect wnd_rect;
    wnd_rect.origin.x    = x - UI_WINDOW_SIZE/2.0f;
    wnd_rect.origin.y    = y - UI_WINDOW_SIZE/2.0f;
    wnd_rect.size.width  = UI_WINDOW_SIZE;
    wnd_rect.size.height = UI_WINDOW_SIZE;

    self = [super initWithContentRect: wnd_rect
                  styleMask: NSBorderlessWindowMask
                  backing: NSBackingStoreBuffered
                  defer: NO];

    [self setBackgroundColor: [NSColor clearColor]];
    // [self setBackgroundColor: [NSColor redColor]];
    // [self setBackgroundColor: [NSColor grayColor]];
    // [self setBackgroundColor: [NSColor whiteColor]];

    [self setLevel: NSStatusWindowLevel];
    [self makeKeyAndOrderFront: nil];
    [self setAlphaValue: 1.0];
    [self setOpaque: NO];
    [self setContentView: [MainView new]];

    auto fresh_time_interval = 1.0f/CURSOR_REFRESH_FREQUENCY;

    fresh_timer = [NSTimer scheduledTimerWithTimeInterval: fresh_time_interval
                          target: self
                          selector: @selector(onRefreshTimerTick:)
                          userInfo: nil
                          repeats: YES];

    [[NSRunLoop currentRunLoop] addTimer: fresh_timer
                                forMode: NSDefaultRunLoopMode];

    auto main_window_id = (WindowID)(uintptr_t)self.windowNumber;
    fprintf(stderr, "MainWindowID %u\n", main_window_id);
    excluded_window_list.push_back(main_window_id);

    auto data_size = CAPTURE_WIDTH*CAPTURE_HEIGHT;
    off_screen_render_data = new struct OffScreenRenderPixel[data_size]();

    off_screen_render_data_fresh_ratio_count = 0;

    return self;
}

-(void) onRefreshTimerTick: (NSTimer*)timer;
{
    // fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    float x = 0.f, y = 0.f;
    GetCurrentCursorPosition(&x, &y);
    // fprintf(stderr, "Current Cursor Position: %9.4f %9.4f\n", x, y);

    if( off_screen_render_data_fresh_ratio_count == 0 )
    {
        RefreshOffScreenRenderPixelWithinBound( \
                x, y, CAPTURE_WIDTH, CAPTURE_HEIGHT, \
                    excluded_window_list, off_screen_render_data );

        // refresh first!
        // [[self contentView] ];
        // force redraw
        [[self contentView] display];
    }

    off_screen_render_data_fresh_ratio_count += 1;
    off_screen_render_data_fresh_ratio_count %= \
        SCREEN_CAPTURE_FREQUENCY_TO_CURSOR_REFRESH_RATIO;

    CGPoint mouse_pos;
    mouse_pos.x = x - UI_WINDOW_SIZE/2.0f;
    mouse_pos.y = y - UI_WINDOW_SIZE/2.0f;
    [self setFrameOrigin: mouse_pos];
}

-(void) keyDown: (NSEvent*)event
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    switch(event.keyCode)
    {
        case 0x35: // escape
        {
            // TODO:
            [self close];
        }
        break;
        case 0x43: // enter
        case 0x24: // return
        {
            // TODO:
            [self close];
        }
        break;
        default:
        break;
    }
    [super keyDown:event];
}

-(void) close
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    [fresh_timer invalidate];
    [super close];
    [NSApp stop: nil];
}

@end


@implementation MainView
{

    CGImageRef image_cicle_mask;
}


-(id) init
{
    image_cicle_mask = [](auto data_buffer, auto data_buffer_size)
    {
        auto data_provide = ::CGDataProviderCreateWithData( \
                            NULL, data_buffer, data_buffer_size, NULL);

        return ::CGImageCreateWithPNGDataProvider(\
                        data_provide, NULL, NO, kCGRenderingIntentDefault);

    }((char*)RES_Circle_Mask, RES_Circle_Mask_len);

    self = [super init];
    return self;
}

-(void) drawRect: (NSRect)dirt_rect
{
    // fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    auto ctx = [[NSGraphicsContext currentContext] CGContext];

    CGRect wnd_rect;
    wnd_rect.origin.x = 0;
    wnd_rect.origin.y = 0;
    wnd_rect.size.width = UI_WINDOW_SIZE;
    wnd_rect.size.height = UI_WINDOW_SIZE;

    CGContextDrawImage(ctx, wnd_rect, image_cicle_mask);

    wnd_rect.size.width = CAPTURE_WIDTH;
    wnd_rect.size.height = CAPTURE_HEIGHT;

    // CGContextDrawImage(ctx, wnd_rect, image_caputured);
    // CGContextDrawImage(ctx, wnd_rect, patched_image);
}

-(void) mouseDown: (NSEvent*)event
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    [[self window] close];
    [super mouseDown: event];
}

@end


void
GetCurrentCursorPosition
(
    float* const x, float* const y
)
{
    CGPoint mouse_pos;
    {
        auto event = ::CGEventCreate(nullptr);
        mouse_pos = ::CGEventGetUnflippedLocation(event);
        ::CFRelease(event);
    }
    *x = mouse_pos.x;
    *y = mouse_pos.y;
}


bool
RefreshOffScreenRenderPixelWithinBound
(
    float central_x, float central_y,
    float bound_width, float bound_height,
    const WindowIDList& excluded_window_list,
    struct OffScreenRenderPixel* const off_screen_render_data
)
{
    struct cf_object_releaser
    {
        CFTypeRef who;
    public:
        cf_object_releaser(CFTypeRef w): who(w) {}
    public:
        ~cf_object_releaser() { ::CFRelease(who); }
    };

    auto window_list = \
    []()
    {
        auto list = ::CGWindowListCreate(kCGWindowListOptionOnScreenOnly, \
                                                             kCGNullWindowID);
        struct cf_object_releaser releaser(list);
        return ::CFArrayCreateMutableCopy(NULL, ::CFArrayGetCount(list), list);
    }();
    struct cf_object_releaser window_list_releaser(window_list);

    const auto window_list_size = ::CFArrayGetCount(window_list);
    const auto invalida_window_id = CGWindowID(kCGNullWindowID);
    for(const auto filter_wnd_id : excluded_window_list )
    {
        for( int idx = window_list_size - 1; idx >= 0; --idx )
        {
            auto crt_wnd_id = (CGWindowID)(uintptr_t)\
                    ::CFArrayGetValueAtIndex(window_list, idx);
            if( crt_wnd_id == filter_wnd_id )
            {
                ::CFArraySetValueAtIndex(window_list, idx, &invalida_window_id);
                break;
            }
        }
    }
    // fprintf(stderr, "window list size %lu\n", window_list_size);

    static auto main_display_height = []()
    {
        auto bound = ::CGDisplayBounds(::CGMainDisplayID());
        return bound.size.height;
    }();

    CGRect bound;
    bound.origin.x = central_x - bound_width/2.0;
    //! need this to correct cursor position !//
    bound.origin.y = main_display_height - central_y - bound_height/2.0;
    bound.size.width = bound_width;
    bound.size.height = bound_height;

    auto image = ::CGWindowListCreateImageFromArray(bound, \
                        window_list, kCGWindowImageNominalResolution);
    struct cf_object_releaser image_releaser(image);

    /*! cache this !*/ static const auto image_size_ok = \
    [](auto image, auto bound)
    {
        auto width = ::CGImageGetWidth(image);
        auto height = ::CGImageGetHeight(image);
        if( (width != bound.width) || (height != bound.height) )
        {
            fprintf(stderr, "captured image size error %lu %lu  \n", \
                                                            width, height);
            return NO;
        }
        return YES;
    }(image, bound.size);

    if( image_size_ok != YES )
    {
        return false; // return early
    }

    /*! cache this !*/ static const auto bitmap_info = \
    [](auto image)
    {
        auto bitmap_info = ::CGImageGetBitmapInfo(image);
        if( bitmap_info & kCGBitmapFloatComponents )
        {
            fprintf(stderr, "is kCGBitmapFloatComponents\n");
        }
        else
        {
            fprintf(stderr, "not kCGBitmapFloatComponents\n");
        }

        switch( bitmap_info & kCGBitmapByteOrderMask )
        {
            case kCGBitmapByteOrderDefault:
                fprintf(stderr, "is kCGBitmapByteOrderDefault\n");
            break;
            case kCGBitmapByteOrder16Little:
                fprintf(stderr, "is kCGBitmapByteOrder16Little\n");
            break;
            case kCGBitmapByteOrder32Little:
                fprintf(stderr, "is kCGBitmapByteOrder32Little\n");
            break;
            case kCGBitmapByteOrder16Big:
                fprintf(stderr, "is kCGBitmapByteOrder16Big\n");
            break;
            case kCGBitmapByteOrder32Big:
                fprintf(stderr, "is kCGBitmapByteOrder32Big\n");
            break;
            // ??????????? fucked !!
            default:
                fprintf(stderr, "?? kCGBitmapByteOrderMask Unknow\n");
            break;
        }

        return bitmap_info;
    }(image);

    /*! cache this !*/ static const auto alpha_info = \
    [](auto image)
    {
        auto alpha_info = ::CGImageGetAlphaInfo(image);
        switch( alpha_info )
        {
            // For example, RGB.
            case kCGImageAlphaNone:
                fprintf(stderr, "is kCGImageAlphaNone\n");
            break;
            // For example, premultiplied RGBA
            case kCGImageAlphaPremultipliedLast:
                fprintf(stderr, "is kCGImageAlphaPremultipliedLast\n");
            break;
            // For example, premultiplied ARGB
            case kCGImageAlphaPremultipliedFirst:
                fprintf(stderr, "is kCGImageAlphaPremultipliedFirst\n");
            break;
            // For example, non-premultiplied RGBA
            case kCGImageAlphaLast:
                fprintf(stderr, "is kCGImageAlphaLast\n");
            break;
            // For example, non-premultiplied ARGB
            case kCGImageAlphaFirst:
                fprintf(stderr, "is kCGImageAlphaFirst\n");
            break;
            // For example, RBGX.
            case kCGImageAlphaNoneSkipLast:
                fprintf(stderr, "is kCGImageAlphaNoneSkipLast\n");
            break;
            // For example, XRGB.
            case kCGImageAlphaNoneSkipFirst:
                fprintf(stderr, "is kCGImageAlphaNoneSkipFirst\n");
            break;
            // No color data, alpha data only
            case kCGImageAlphaOnly:
                fprintf(stderr, "is kCGImageAlphaOnly\n");
            break;
            // ??????????? fucked !!
            default:
                fprintf(stderr, "is kCGImageAlphaInfo Unknow\n");
            break;
        }
        return alpha_info;
    }(image);

    //* right now our compiler don't support unpack to static *//
    // static const auto [bits_per_pixel, bits_per_component] = [](auto image)
    /*! cache this !*/ static const auto bits_info = \
    [](auto image)
    {
        const auto bpp = ::CGImageGetBitsPerPixel(image);
        const auto bpc = ::CGImageGetBitsPerComponent(image);
        fprintf(stderr, "is %lu bpp %lu bpc\n", bpp, bpc);
        struct bits_info_t { size_t bpp; size_t bpc;};
        return bits_info_t { .bpp = bpp, .bpc = bpc };
    }(image);

    /*! cache this !*/ static const auto image_raw_data_format_ok = \
    [](auto bitmap_info, auto alpha_info, auto bits_info)
    {
        auto state = YES;
        if( (bitmap_info&kCGBitmapByteOrderMask) != kCGBitmapByteOrder32Little)
        {
            fprintf(stderr, "captured image bytes order error\n");
            state = NO;
        }
        if( (bitmap_info&kCGBitmapFloatInfoMask) == kCGBitmapFloatComponents )
        {
            fprintf(stderr, "captured image bytes type error\n");
            state = NO;
        }
        if( ((alpha_info) != kCGImageAlphaPremultipliedFirst ) )
        {
            fprintf(stderr, "captured image alpha info error\n");
            state = NO;
        }
        if( (bits_info.bpp != 32) || (bits_info.bpc != 8 ) )
        {
            fprintf(stderr, "captured image bpp/bpc error\n");
            state = NO;
        }
        return state;
    }(bitmap_info, alpha_info, bits_info);

    if( image_raw_data_format_ok != YES )
    {
        return false; // return early
    }

    auto calculate_then_flush_result_to_the_buffer = \
    [](auto image, auto off_screen_render_data)
    {
        const auto src_width = ::CGImageGetWidth(image);
        const auto src_height = ::CGImageGetHeight(image);

        const auto src_data_provider = ::CGImageGetDataProvider(image);
        const auto src_data = ::CGDataProviderCopyData(src_data_provider);
        const auto src_data_len = ::CFDataGetLength(src_data);
        const auto src_data_buf = std::make_unique<uint8_t[]>(src_data_len);
        ::CFDataGetBytes(src_data, ::CFRangeMake(0, src_data_len), \
                                                    src_data_buf.get());
        struct cf_object_releaser src_data_releaser(src_data);

        const auto src_bytes_per_row = ::CGImageGetBytesPerRow(image);
        const auto src_colorspace = ::CGImageGetColorSpace(image);

        const auto tmp_bytes_per_component = \
                    OffScreenRenderPixel::BitsPerChannel()/8;
        const auto tmp_bytes_per_pixel =
                    OffScreenRenderPixel::BitsAllChannel()/8;

        const auto tmp_bytes_per_row = src_width*tmp_bytes_per_pixel;
        const auto tmp_data_len = tmp_bytes_per_row*src_height;
        const auto tmp_data_buf = std::make_unique<uint8_t[]>(tmp_data_len);

        auto tmp_data_cursor = (struct OffScreenRenderPixel*)tmp_data_buf.get();
        for( size_t y = 0; y < src_height; ++y )
        {
            for( size_t x = 0; x < src_width; ++x )
            {
                uint8_t b = src_data_buf[y*src_bytes_per_row+x*4+0];
                uint8_t g = src_data_buf[y*src_bytes_per_row+x*4+1];
                uint8_t r = src_data_buf[y*src_bytes_per_row+x*4+2];
                uint8_t a = src_data_buf[y*src_bytes_per_row+x*4+3];
                // printf("%3u %3u %3u %3u \n", r, g, b, a);

                tmp_data_cursor[y*src_width+x].b = b/255.0;
                tmp_data_cursor[y*src_width+x].g = g/255.0;
                tmp_data_cursor[y*src_width+x].r = r/255.0;
                tmp_data_cursor[y*src_width+x].a = a/255.0;
            }
        }

        // dump
        for( size_t y = 0; y < src_height; ++y )
        {
            for( size_t x = 0; x < src_width; ++x )
            {
                uint8_t b = src_data_buf[y*src_bytes_per_row+x*4+0];
                uint8_t g = src_data_buf[y*src_bytes_per_row+x*4+1];
                uint8_t r = src_data_buf[y*src_bytes_per_row+x*4+2];
                uint8_t a = src_data_buf[y*src_bytes_per_row+x*4+3];

                uint8_t b_t = tmp_data_cursor[y*src_width+x].b*255.0;
                uint8_t g_t = tmp_data_cursor[y*src_width+x].g*255.0;
                uint8_t r_t = tmp_data_cursor[y*src_width+x].r*255.0;
                uint8_t a_t = tmp_data_cursor[y*src_width+x].a*255.0;

                // fprintf(stderr, "%3u %3u %3u %3u -> %3u %3u %3u %3u \n",
                //                            r, g, b, a, r_t, g_t, b_t, a_t);
            }
        }

        static const auto tmp_bitmap_info = kCGBitmapFloatInfoMask |
                                            kCGBitmapByteOrder32Little |
                                            kCGImageAlphaPremultipliedLast |
                                            0;

        const auto tmp_data_provider = ::CGDataProviderCreateWithData( \
                               NULL, tmp_data_buf.get(), tmp_data_len, NULL);
        struct cf_object_releaser tmp_data_provider_releaser(tmp_data_provider);

        const auto tmp_image = ::CGImageCreate( \
                        src_width,                    // width
                        src_height,                   // height
                        8*tmp_bytes_per_component,    // bitsPerComponent
                        8*tmp_bytes_per_pixel,        // bitsPerPixel
                        tmp_bytes_per_row,            // bytesPerRow
                        src_colorspace,               // colorspace
                        tmp_bitmap_info,              // bitmapInfo
                        tmp_data_provider,            // CGDataProvider
                        NULL,                         // decode array
                        NO,                           // shouldInterpolate
                        kCGRenderingIntentDefault     // intent
                    );
        struct cf_object_releaser tmp_image_releaser(tmp_image);

        static const auto colorspace_sRGB = \
                        ::CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

        static const auto context = ::CGBitmapContextCreate( \
                off_screen_render_data,     // data
                src_width,                  // width
                src_height,                 // height
                8*tmp_bytes_per_component,  // bitsPerComponent
                tmp_bytes_per_row,          // bytesPerRow
                colorspace_sRGB,            // space
                tmp_bitmap_info             // bitmapInfo
              );

        ::CGContextDrawImage(context, \
                ::CGRectMake(0, 0, src_width, src_height), tmp_image);

        // fprintf(stderr, "---------------------------------------------------\n");

        auto sRGB_data_cursor = off_screen_render_data;
        for( size_t y = 0; y < src_height; ++y )
        {
            for( size_t x = 0; x < src_width; ++x )
            {
                uint8_t r_t = tmp_data_cursor[y*src_width+x].r*255.0;
                uint8_t g_t = tmp_data_cursor[y*src_width+x].g*255.0;
                uint8_t b_t = tmp_data_cursor[y*src_width+x].b*255.0;
                uint8_t a_t = tmp_data_cursor[y*src_width+x].a*255.0;

                uint8_t r_d = sRGB_data_cursor[y*src_width+x].r*255.0;
                uint8_t g_d = sRGB_data_cursor[y*src_width+x].g*255.0;
                uint8_t b_d = sRGB_data_cursor[y*src_width+x].b*255.0;
                uint8_t a_d = sRGB_data_cursor[y*src_width+x].a*255.0;

                fprintf(stderr, "%3u %3u %3u %3u -> %3u %3u %3u %3u \n", \
                                    r_t, g_t, b_t, a_t, r_d, g_d, b_d, a_d);
            }
        }
    };

    calculate_then_flush_result_to_the_buffer(image, off_screen_render_data);

    return true;
}


void
PreRun_Mode_Normal(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    fprintf(stderr, "screen record size: %4u %4u\n", \
                                CAPTURE_WIDTH, CAPTURE_HEIGHT);

    [NSApplication sharedApplication];

    // application does not appear in the Dock and does not have a menu bar
    [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];
    [NSApp activateIgnoringOtherApps: YES];
    [NSApp setDelegate: [AppDelegate new]];
    // [NSCursor hide];
}

void
PostRun_Mode_Normal(class Instance* instance)
{
    // [NSCursor unhide];
}

void
PreRun_Mode_Check(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    float central_x = 200, central_y = 200;
    float bound_width = 100, bound_height = 100;

    WindowIDList no_exclude_window_list;

    auto off_screen_render_data = new \
                OffScreenRenderPixel[bound_width*bound_height];

    RefreshOffScreenRenderPixelWithinBound( \
        central_x, central_y, bound_width, bound_height, \
                no_exclude_window_list, off_screen_render_data );

    delete[] off_screen_render_data;
}

void
PreRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto inst_info = instance->InstanceInfo();
    auto exec_mode = inst_info->CommandLineParameter<int>("--mode=");
    auto exec_time = inst_info->CommandLineParameter<int>("--time=");

    switch(exec_mode)
    {
        case 0:
            PreRun_Mode_Normal(instance);
        break;
        case 1:
        {
            while(exec_time--)
            {
                PreRun_Mode_Check(instance);
            }
        }
        break;
        case 2:
        {
            while(exec_time--)
            {
                struct MonitorInfo::Initializer init;
            }
        }
        default:
            // pass
        break;
    }
}


void
PostRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

