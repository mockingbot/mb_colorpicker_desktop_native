#pragma once

/*
 * Writer: Joseph Shen
 */

#include <string>
#include <fstream>
#include <iostream>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <objidl.h>
#include <gdiplus.h>

#include <wincodec.h>
#include <magnification.h>


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


struct GDI_PLUS_INITIALIZER
{
private:
    ULONG_PTR gdiplusToken;
    ::Gdiplus::GdiplusStartupInput gdiplusStartupInput;
public:
    GDI_PLUS_INITIALIZER()
    {
        ::Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    }
    ~GDI_PLUS_INITIALIZER()
    {
        ::Gdiplus::GdiplusShutdown(gdiplusToken);
    }
};


struct MAGNIFICATION_INITIALIZER
{
public:
    MAGNIFICATION_INITIALIZER()
    {
        if( FALSE == ::MagInitialize() ){
            printf("MagInitialize Failed %d\n", ::GetLastError());
            throw std::runtime_error("::MagInitialize Failed");
        }
    }
    ~MAGNIFICATION_INITIALIZER()
    {
        if( FALSE == ::MagUninitialize() ){
            printf("MagUninitialize Failed %d\n", ::GetLastError());
        }
    }
};


int ALL_MONITOR_INFO_COUNT = 0;
RECT ALL_MONITOR_RECT_INFO[32] = {0};
SIZE ALL_MONITOR_SIZE_INFO[32] = {0};

struct MONITOR_INFO_INITIALIZER
{
private:
    static BOOL CALLBACK EnumProc(HMONITOR hMonitor,
                                  HDC      hdcMonitor,
                                  LPRECT   lprcMonitor,
                                  LPARAM   dwData)
    {
        SIZE size = {0, 0};
        size.cx = lprcMonitor->right - lprcMonitor->left,
        size.cy = lprcMonitor->bottom - lprcMonitor->top,

        ALL_MONITOR_RECT_INFO[ALL_MONITOR_INFO_COUNT] = *lprcMonitor;
        ALL_MONITOR_SIZE_INFO[ALL_MONITOR_INFO_COUNT] = size;
        ALL_MONITOR_INFO_COUNT++;

        return TRUE;
    }
public:
    MONITOR_INFO_INITIALIZER()
    {
        ::EnumDisplayMonitors(NULL, NULL, EnumProc, 0);
    }
    ~MONITOR_INFO_INITIALIZER()
    {
    }
};

