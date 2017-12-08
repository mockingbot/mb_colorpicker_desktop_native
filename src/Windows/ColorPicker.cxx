#include "ColorPicker.hxx"

#include "Resource.h"

const wchar_t WND_CLASS_NAME[] = L"WINDOW_CLASS_FOR_COLOR_PICKER";

HWND HWND_UI;

HWND HWND_MAGNIFIER_HOST;
HWND HWND_MAGNIFIER;

HDC HDC_FOR_UI_WND;
HDC HDC_FOR_UI_WND_CANVASE;

Gdiplus::Bitmap* BITMAP_MASK_CIRCLE;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

const UINT UPDATE_TIMER_INTERVAL = 16; // close to the refresh rate @60hz
void CALLBACK FlushTimerCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);


// the magnification api can only exclude some window, 16 is enough for us
uint32_t EXCLUDE_WINDOW_COUNT = 0;
HWND EXCLUDE_WINDOW_LIST[16] = {0};


int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   using namespace Gdiplus;

   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}


BOOL WINAPI MagnifierWndCallback(HWND hWnd, \
                                 void* srcdata, MAGIMAGEHEADER srcheader, \
                                 void* destdata, MAGIMAGEHEADER destheader, \
                                 RECT unclipped, RECT clipped, HRGN dirty);

int main(int argc, char* argv[])
{
    class GDI_PLUS_INITIALIZER GDI_PLUS_INITIALIZER;
    class MONITOR_INFO_INITIALIZER MONITOR_INFO_INITIALIZER;
    class MAGNIFICATION_INITIALIZER MAGNIFICATION_INITIALIZER;

    auto hInstance = ::GetModuleHandle(nullptr);

    for (int idx = 0; idx < ALL_MONITOR_RECT_INFO_COUNT; ++idx)
    {
        auto rect = ALL_MONITOR_RECT_INFO[idx];
        auto size = ALL_MONITOR_SIZE_INFO[idx];
        printf("%d %d %d %d\n", rect.top, rect.left, rect.right, rect.bottom);
        printf("%d %d \n", size.cx, size.cy);
    }

    struct WND_CLASS_INITIALIZER
    {
    private:
        HINSTANCE hInstance;
    public:
        WND_CLASS_INITIALIZER(HINSTANCE instance)
        :hInstance(instance)
        {
            WNDCLASSEX wcex     = {};

            wcex.cbSize         = sizeof(WNDCLASSEX);
            wcex.style          = CS_HREDRAW | CS_VREDRAW;
            wcex.lpfnWndProc    = WindowProc;
            wcex.hInstance      = hInstance;
            wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
            wcex.hbrBackground  = (HBRUSH)(1 + COLOR_BTNFACE);
            wcex.lpszClassName  = WND_CLASS_NAME;

            ::RegisterClassEx(&wcex);
        }
        ~WND_CLASS_INITIALIZER()
        {
            ::UnregisterClass(WND_CLASS_NAME, hInstance);
        }
    } WND_CLASS_INITIALIZER(hInstance);

    struct UI_WINDOW_INITIALIZER
    {
    public:
        UI_WINDOW_INITIALIZER(HINSTANCE hInstance)
        {
            HWND_UI = ::CreateWindowEx(\
                                    WS_EX_TOPMOST | WS_EX_LAYERED,
                                    WND_CLASS_NAME,
                                    L"ColorPicker",
                                    WS_POPUP,
                                    400,
                                    0,
                                    UI_WINDOW_WIDTH,
                                    UI_WINDOW_HEIGHT,
                                    nullptr,
                                    nullptr,
                                    hInstance,
                                    nullptr);
            if( HWND_UI == NULL)
            {
                printf("CreateWindowEx HWND_UI Failed %d\n", ::GetLastError());
                throw std::runtime_error("::Create UI Window Failed");
            }
        }
        ~UI_WINDOW_INITIALIZER()
        {
            ::DestroyWindow(HWND_UI);
        }
    } UI_WINDOW_INITIALIZER(hInstance);

    struct MAGNIFICATION_WINDOW_INITIALIZER
    {
    public:
        MAGNIFICATION_WINDOW_INITIALIZER(HINSTANCE hInstance)
        {
            HWND_MAGNIFIER_HOST = ::CreateWindow(\
                                        WND_CLASS_NAME,
                                        TEXT("MagnifierWindowHost"),
                                        WS_POPUP,
                                        0,
                                        0,
                                        CAPTURE_WIDTH,
                                        CAPTURE_HEIGHT,
                                        nullptr,
                                        nullptr,
                                        hInstance,
                                        nullptr);
            if( HWND_MAGNIFIER_HOST == NULL )
            {
                printf("CreateWindow HWND_MAGNIFIER_HOST Failed %d\n", ::GetLastError());
                throw std::runtime_error("::CreateWindowEx HWND_MAGNIFIER_HOST Failed");
            }

            HWND_MAGNIFIER = ::CreateWindow(\
                                    WC_MAGNIFIER,
                                    TEXT("MagnifierWindow"),
                                    WS_CHILD | WS_VISIBLE,
                                    0,
                                    0,
                                    CAPTURE_WIDTH,
                                    CAPTURE_HEIGHT,
                                    HWND_MAGNIFIER_HOST,
                                    nullptr,
                                    hInstance,
                                    nullptr);
            if( HWND_MAGNIFIER == NULL )
            {
                printf("CreateWindow HWND_MAGNIFIER Failed %d\n", ::GetLastError());
                throw std::runtime_error("::CreateWindow HWND_MAGNIFIER Failed");
            }


            // Set the magnification factor.
            MAGTRANSFORM matrix;
            ::memset(&matrix, 0, sizeof(matrix));
            matrix.v[0][0] = 1.0f;
            matrix.v[1][1] = 1.0f;
            matrix.v[2][2] = 1.0f;

            if( FALSE == ::MagSetWindowTransform(HWND_MAGNIFIER, &matrix) )
            {
                printf("MagSetWindowTransform %d\n", ::GetLastError());
                throw std::runtime_error("::MagSetWindowTransform Failed");
            }
        }
        ~MAGNIFICATION_WINDOW_INITIALIZER()
        {
            if( 0 == ::DestroyWindow(HWND_MAGNIFIER) ){
                printf("DestroyWindow HWND_MAGNIFIER Failed\n");
            }
            if( 0 == ::DestroyWindow(HWND_MAGNIFIER_HOST) ){
                printf("DestroyWindow HWND_MAGNIFIER_HOST Failed\n");
            }
        }
    } MAGNIFICATION_WINDOW_INITIALIZER(hInstance);

    struct FLUSH_TIMER
    {
    private:
        UINT_PTR timer_id;
    public:
        FLUSH_TIMER(HWND hWnd, TIMERPROC lpTimerFunc)
        :timer_id(::SetTimer(hWnd, 0, UPDATE_TIMER_INTERVAL, lpTimerFunc))
        {
        }
        ~FLUSH_TIMER()
        {
           ::KillTimer(NULL, timer_id);
        }
    } FLUSH_TIMER(HWND_UI, FlushTimerCallback);

    //*************************************************************************//
    struct HDC_INITIALIZER
    {
        HDC_INITIALIZER()
        {
            HDC_FOR_UI_WND = ::GetDC(HWND_UI);
            HDC_FOR_UI_WND_CANVASE = ::CreateCompatibleDC(nullptr);
        }
        ~HDC_INITIALIZER()
        {
            ::DeleteDC(HDC_FOR_UI_WND_CANVASE);
            ::ReleaseDC(HWND_UI, HDC_FOR_UI_WND);
        }
    } HDC_INITIALIZER;

    //*************************************************************************//
    struct BITMAP_MASK_INITIALIZER
    {
        BITMAP_MASK_INITIALIZER(HINSTANCE hInstance)
        {
            BITMAP_MASK_CIRCLE = LoadBitmapFromResPNG(hInstance, \
                                            RES_INDEX_FOR_Circle_Mask_x1);
        }
        ~BITMAP_MASK_INITIALIZER()
        {
            delete BITMAP_MASK_CIRCLE;
        }
    } BITMAP_MASK_INITIALIZER(hInstance);

    //*************************************************************************//

    CLSID pngClsid;
    GetEncoderClsid(L"image/bmp", &pngClsid);

    auto hDesktopWnd = ::GetDesktopWindow();
    auto hDesktopDC = ::GetDC(hDesktopWnd);
    auto hCaptureDC = ::CreateCompatibleDC(hDesktopDC);

    for (int idx = 0; idx < ALL_MONITOR_RECT_INFO_COUNT; ++idx)
    {
        int display_width = ALL_MONITOR_SIZE_INFO[idx].cx;
        int display_height = ALL_MONITOR_SIZE_INFO[idx].cy;

        int display_top = ALL_MONITOR_RECT_INFO[idx].top;
        int display_left = ALL_MONITOR_RECT_INFO[idx].left;

        auto hCaptureBitmap = ::CreateCompatibleBitmap(hDesktopDC, \
                                        display_width, display_height);

        ::SelectObject(hCaptureDC, hCaptureBitmap);

        ::BitBlt(hCaptureDC,
                 0, 0, display_width, display_height,
                 hDesktopDC,
                 display_left, display_top,
                 SRCCOPY|CAPTUREBLT);

        auto bitmap = Gdiplus::Bitmap::FromHBITMAP(hCaptureBitmap, nullptr);
        bitmap->Save(L"Test.bmp", &pngClsid, NULL);
        delete bitmap;

        ::DeleteObject(hCaptureBitmap);
    }

    ::DeleteDC(hCaptureDC);
    ::DeleteDC(hDesktopDC);
    ::ReleaseDC(hDesktopWnd, hDesktopDC);


    //*************************************************************************//
    EXCLUDE_WINDOW_LIST[EXCLUDE_WINDOW_COUNT] = HWND_UI;
    EXCLUDE_WINDOW_COUNT++;

    // EXCLUDE_WINDOW_LIST[EXCLUDE_WINDOW_COUNT] = HWND_MAGNIFIER_HOST;
    // EXCLUDE_WINDOW_COUNT++;

    //*************************************************************************//
    if( FALSE == ::MagSetImageScalingCallback(HWND_MAGNIFIER, MagnifierWndCallback) )
    {
        printf("MagSetImageScalingCallback %d\n", ::GetLastError());
        throw std::runtime_error("::MagSetImageScalingCallback Failed");
    }

    //*************************************************************************//
    ::ShowWindow(HWND_UI, SW_SHOW);
    // ::ShowWindow(HWND_MAGNIFIER_HOST, SW_SHOW);

    //*************************************************************************//
    MSG msg;
    while(::GetMessage(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return (int) msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
    break;
    case WM_LBUTTONDOWN:
        PostQuitMessage(0);
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void CALLBACK FlushTimerCallback(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    if( FALSE == ::MagSetWindowFilterList(HWND_MAGNIFIER,
                                          MW_FILTERMODE_EXCLUDE,
                                          EXCLUDE_WINDOW_COUNT,
                                          EXCLUDE_WINDOW_LIST) )
    {
        std::cout << "::MagSetWindowFilterList Failed" << ::GetLastError() << "\n";
        throw std::runtime_error("::MagSetWindowFilterList Failed");
    }

    POINT mousePoint;
    ::GetCursorPos(&mousePoint);

    RECT sourceRect;
    sourceRect.left   = mousePoint.x - CAPTURE_WIDTH / 2;
    sourceRect.top    = mousePoint.y - CAPTURE_HEIGHT / 2;
    sourceRect.right  = CAPTURE_WIDTH;
    sourceRect.bottom = CAPTURE_HEIGHT;

    // Reclaim topmost status
    ::SetWindowPos(HWND_UI, HWND_TOPMOST, 0, 0, 0, 0, \
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

    // Set the source rectangle for the magnifier control.
    ::MagSetWindowSource(HWND_MAGNIFIER, sourceRect);

    // Force redraw.
    ::InvalidateRect(HWND_MAGNIFIER, NULL, TRUE);
    // ::InvalidateRect(HWND_UI, NULL, TRUE);
}

BOOL WINAPI MagnifierWndCallback(HWND hWnd, \
                                 void* srcdata, MAGIMAGEHEADER srcheader, \
                                 void* destdata, MAGIMAGEHEADER destheader, \
                                 RECT unclipped, RECT clipped, HRGN dirty)
{
    auto const src_buffer = (BYTE*)srcdata;
    // return TRUE;

    static bool initd = [&srcheader](){
        /*  */ if(srcheader.format == GUID_WICPixelFormat32bppRGBA ) {
            std::cout << "GUID_WICPixelFormat32bppRGBA\n";
        } else if(srcheader.format == GUID_WICPixelFormat32bppBGR ){
            std::cout << "GUID_WICPixelFormat32bppBGR\n";
        } else {
            std::cout << "GUID_WICPixelFormat Unkwown\n";
        }
        return true;
    }();

    Gdiplus::Graphics graphics(HDC_FOR_UI_WND_CANVASE);
    Gdiplus::Bitmap bitmap(UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT, &graphics);

    Gdiplus::Graphics painter(&bitmap);

    auto graphics_state = painter.Save();
    {
        Gdiplus::GraphicsPath path(Gdiplus::FillMode::FillModeWinding);
        path.AddEllipse(4, 4, 163, 163);

        Gdiplus::Region region(&path);
        painter.SetClip(&region, Gdiplus::CombineMode::CombineModeReplace);
    }

    for(int y = 0; y < GRID_NUMUBER; ++y)
    {
        for(int x = 0; x < GRID_NUMUBER; ++x)
        {
            int dst_x_l = 1 + (GRID_PIXEL + 1)*x;
            int dst_y_t = 1 + (GRID_PIXEL + 1)*y;

            const auto src_pixel = &src_buffer[y*GRID_NUMUBER*4 + x*4];
            const auto r = src_pixel[2];
            const auto g = src_pixel[1];
            const auto b = src_pixel[0];
            // const auto a = src_pixel[3];

            Gdiplus::SolidBrush brush(Gdiplus::Color(0xFF, r, g, b));
            painter.FillRectangle(&brush, dst_x_l, dst_y_t, GRID_PIXEL+1, GRID_PIXEL+1);

            // Gdiplus::Pen l_pen(Gdiplus::Color(0xFF*0.05, 0x4F, 0x4F, 0x4F), 1);
            Gdiplus::Pen h_pen(Gdiplus::Color(0xFF*0.25, 0xCF, 0xCF, 0xCF), 1);

            // painter.DrawRectangle(&l_pen, dst_x_l, dst_y_t, GRID_PIXEL+1, GRID_PIXEL+1);
            painter.DrawRectangle(&h_pen, dst_x_l, dst_y_t, GRID_PIXEL+1, GRID_PIXEL+1);
        }
    }
    {
        int dst_x_l = 1 + (GRID_PIXEL + 1)*GRID_NUMUBER_L;
        int dst_y_t = 1 + (GRID_PIXEL + 1)*GRID_NUMUBER_L;

        Gdiplus::Pen black_pen(Gdiplus::Color(0xFF, 0x00, 0x00, 0x00), 1);
        Gdiplus::Pen white_pen(Gdiplus::Color(0xFF, 0xFF, 0xFF, 0xFF), 1);

        painter.DrawRectangle(&black_pen, dst_x_l, dst_y_t, GRID_PIXEL+1, GRID_PIXEL+1);
        painter.DrawRectangle(&white_pen, dst_x_l+1, dst_y_t+1, GRID_PIXEL-1, GRID_PIXEL-1);
    }
    painter.Restore(graphics_state);

    painter.DrawImage(BITMAP_MASK_CIRCLE, 0, 0);

    /**************************************************************************/
    HBITMAP hBitmap;
    if( bitmap.GetHBITMAP(Gdiplus::Color(), &hBitmap) != Gdiplus::Status::Ok)
    {
        printf("-----------------\n");
        return TRUE;
    }

    auto hPrevObj = ::SelectObject(HDC_FOR_UI_WND_CANVASE, hBitmap);

    POINT mousePoint;
    ::GetCursorPos(&mousePoint);

    // POINT ptDest = {800, 400};
    POINT ptDest = { mousePoint.x - UI_WINDOW_WIDTH/2, mousePoint.y - UI_WINDOW_HEIGHT/2};

    POINT ptSrc = {0, 0};
    SIZE client = {UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT};
    BLENDFUNCTION blend_func = {AC_SRC_OVER, 0, 0xFF, AC_SRC_ALPHA};

    ::UpdateLayeredWindow(\
                        HWND_UI,
                        HDC_FOR_UI_WND,
                        &ptDest,
                        &client,
                        HDC_FOR_UI_WND_CANVASE,
                        &ptSrc,
                        0,
                        &blend_func,
                        ULW_ALPHA
                        );

    ::SelectObject(HDC_FOR_UI_WND_CANVASE, hPrevObj);
    ::DeleteObject(hBitmap);

    return TRUE;
}
