#pragma once

#include <iostream>

#include <Cocoa/Cocoa.h>


/*
        /+/       <---------------------------
         +                          | GRID_NUMUBER_L*2 + 1
         +      /- GRID_NUMUBER_L   |
         +     /                    |
         +<---v-->|                 |
/+/+/+/+/#/+/+/+/+/                 |
         +                          |
         +                          |
         +                          |
         +                          |
        /+/       <---------------------------
^_________________^
|                 |
|  GRID_NUMUBER_L*2 + 1 |
*/

const int GRID_PIXEL = 9;
const int GRID_NUMUBER_L = 8;

const int GRID_NUMUBER = GRID_NUMUBER_L*2 + 1;

const int CAPTURE_WIDTH = GRID_NUMUBER;
const int CAPTURE_HEIGHT = GRID_NUMUBER;

const int UI_WINDOW_WIDTH = GRID_PIXEL + 2 + \
                            (GRID_NUMUBER_L*GRID_PIXEL + GRID_NUMUBER_L)*2;

const int UI_WINDOW_HEIGHT = GRID_PIXEL + 2 + \
                            (GRID_NUMUBER_L*GRID_PIXEL + GRID_NUMUBER_L)*2;


@interface HostForGrabPictureSurroundCurrentCursor: NSObject
-(void)onTick:(NSTimer* )timer;
@end

@interface Application: NSApplication <NSApplicationDelegate>
@end


@interface MainWindow : NSWindow <NSWindowDelegate>
@end


@interface MainView : NSView
@end


class MouseEventHook
{
public:
    MouseEventHook();
    ~MouseEventHook();
private:
    CFMachPortRef mouse_event_tap;
    CFRunLoopSourceRef event_tap_src_ref;
private:
    static CGEventRef Callback(CGEventTapProxy, CGEventType, CGEventRef, void*);
};

