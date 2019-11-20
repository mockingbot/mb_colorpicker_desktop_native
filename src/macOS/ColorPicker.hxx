#pragma once

#include <iostream>

#include <Cocoa/Cocoa.h>


@interface AppDelegate: NSObject <NSApplicationDelegate>

@end


@interface MainWindow : NSWindow <NSWindowDelegate>

-(void)onTimerTick:(NSTimer* )timer;

@end


@interface MainView : NSView

-(void)refreshPictureSurroundCurrentCursor;

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

