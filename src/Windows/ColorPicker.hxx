#pragma once

/*
 * Writer: Joseph Shen
 */

#include <string>
#include <fstream>
#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <objidl.h>
#include <gdiplus.h>

#include <wincodec.h>
#include <magnification.h>

#include <D3D9.h>

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

struct DIRECT3D_9_INITIALIZER
{
private:
    LPDIRECT3D9 pD3D = nullptr;
public:
    DIRECT3D_9_INITIALIZER()
    :pD3D(::Direct3DCreate9(D3D_SDK_VERSION))
    {
        if( pD3D == nullptr ){
            printf("Direct3DCreate9 Failed %d\n", ::GetLastError());
            throw std::runtime_error("::Direct3DCreate9 Failed");
        }
    }
    ~DIRECT3D_9_INITIALIZER()
    {
        pD3D->Release();
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

