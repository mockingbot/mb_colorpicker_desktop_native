#pragma once

#include <vector>

#include <AppKit/AppKit.h>


@interface AppDelegate: NSObject <NSApplicationDelegate>

@end


@interface MainWindow : NSWindow

-(id) init;
-(void) onRefreshTimerTick: (NSTimer*)timer;
-(void) keyDown: (NSEvent*)event;
-(BOOL) canBecomeMainWindow;
-(BOOL) canBecomeKeyWindow;

@end


@interface MainView : NSView

-(id) init;
-(void) drawRect: (NSRect)dirt_rect;
-(void) mouseDown: (NSEvent*)event;

@end


void
GetCurrentCursorPosition
(
    float* const x, float* const y
);

typedef CGWindowID WindowID ;
typedef std::vector<CGWindowID> WindowIDList;

//! always using this format: (r, g, b, x) = (float, float, float, float)
struct OffScreenRenderPixel
{
    float r = 0, g = 0, b = 0, a = 0;
};

bool
RefreshOffScreenRenderPixelWithinBound
(
    float central_x, float central_y,
    float bound_width, float bound_height,
    const WindowIDList& excluded_window_list,
    struct OffScreenRenderPixel* const off_screen_render_data
);

namespace MonitorInfo
{
    const uint32_t LIST_SIZE = 16; //!! 16 is enought

    auto ID_LIST = new UInt32[LIST_SIZE]();
    auto BOUND_LIST = new CGRect[LIST_SIZE]();
    auto COLOR_SPACE_LIST = new CGColorSpaceRef[LIST_SIZE]();
    auto NS_COLOR_SPACE_LIST = new NSColorSpace*[LIST_SIZE]();

    uint32_t AVAILABLE_COUNT = 0;

    struct Initializer
    {
        Initializer();
        ~Initializer();
    } Initialized;

    Initializer::Initializer()
    {
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
        if( auto result = ::CGGetActiveDisplayList(LIST_SIZE, ID_LIST, \
                                                        &AVAILABLE_COUNT);
            result != kCGErrorSuccess )
        {
            fprintf(stderr, "::CGGetActiveDisplayList Failed\n");
            throw std::runtime_error("::CGGetActiveDisplayList Failed\n");
        }

        for(uint32_t idx=0; idx < AVAILABLE_COUNT; ++idx)
        {
            fprintf(stderr, "monitor %u id: 0x%08X ", idx, ID_LIST[idx]);
            //! in the global display coordinate space !//
            BOUND_LIST[idx] = ::CGDisplayBounds(ID_LIST[idx]);
            fprintf(stderr, "bound: %+6.0f %+6.0f %+6.0f %+6.0f ",
                    BOUND_LIST[idx].origin.x, BOUND_LIST[idx].origin.y, \
                    BOUND_LIST[idx].size.width, BOUND_LIST[idx].size.height\
                );
            //! need this to correct color value !//
            COLOR_SPACE_LIST[idx] = ::CGDisplayCopyColorSpace(ID_LIST[idx]);
            NS_COLOR_SPACE_LIST[idx] = [[NSColorSpace alloc]
                               initWithCGColorSpace: COLOR_SPACE_LIST[idx]];

            auto ns_cs_name = [NS_COLOR_SPACE_LIST[idx] localizedName];
            if( ns_cs_name != nil )
            {
                auto cs_name = (CFStringRef)ns_cs_name;
                auto str_len = ::CFStringGetLength(cs_name);
                auto max_str_len = 4 * str_len + 1;
                auto cs_name_str = new char[max_str_len]();
                ::CFStringGetCString(cs_name, cs_name_str, max_str_len, \
                                                    kCFStringEncodingUTF8);
                fprintf(stderr, "colorspace: %s\n", cs_name_str);
                delete[] cs_name_str;
            }
            else
            {
                fprintf(stderr, "colorspace: unknow\n");
            }
        }
    }

    Initializer::~Initializer()
    {
        for(uint32_t idx=0; idx < AVAILABLE_COUNT; ++idx)
        {
            [NS_COLOR_SPACE_LIST[idx] dealloc];
            ::CGColorSpaceRelease(COLOR_SPACE_LIST[idx]);
        }
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    }

    auto ID(const uint32_t idx) { return ID_LIST[idx]; }
    auto Bound(const uint32_t idx) { return BOUND_LIST[idx]; }
    auto ColorSpace(const uint32_t idx) { return COLOR_SPACE_LIST[idx]; }

} // namespace MonitorInfo

