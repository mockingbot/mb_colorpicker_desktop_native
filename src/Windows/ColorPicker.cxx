#include "ColorPicker.hxx"

#include "Resource.h"

const wchar_t WND_CLASS_NAME[] = L"WINDOW_CLASS_FOR_COLOR_PICKER";

HWND HWND_UI;

HWND HWND_MAGNIFIER_HOST;
HWND HWND_MAGNIFIER;

HDC HDC_FOR_UI_WND;
HDC HDC_FOR_UI_WND_CANVASE;

Gdiplus::Bitmap* BITMAP_MASK_CIRCLE;

Gdiplus::Color CAPTURED_IMAGE[GRID_NUMUBER][GRID_NUMUBER];
const auto& CAPTURED_COLOR = CAPTURED_IMAGE[GRID_NUMUBER_L][GRID_NUMUBER_L];

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

const UINT UPDATE_TIMER_INTERVAL = 16; // close to the refresh rate @60hz

// the magnification api can only exclude some window, 16 is enough for us
uint32_t EXCLUDE_WINDOW_COUNT = 0;
HWND EXCLUDE_WINDOW_LIST[16] = {0};

//
// HBITMAP ALL_MONITOR_CAPTURED_BITMAP[32] = {NULL};
Gdiplus::Bitmap* ALL_MONITOR_CAPTURED_BITMAP[32] = {NULL};

BOOL WINAPI MagnifierUpdateCallback(HWND hWnd, \
                                    void* srcdata, MAGIMAGEHEADER srcheader, \
                                    void* destdata, MAGIMAGEHEADER destheader, \
                                    RECT unclipped, RECT clipped, HRGN dirty);

void DrawZoomedCanvas();

int main(int argc, char* argv[])
{
    struct GDI_PLUS_INITIALIZER GDI_PLUS_INITIALIZER;
    struct MONITOR_INFO_INITIALIZER MONITOR_INFO_INITIALIZER;
    struct MAGNIFICATION_INITIALIZER MAGNIFICATION_INITIALIZER;

    auto hInstance = ::GetModuleHandle(nullptr);

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
                                    WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
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
        FLUSH_TIMER(HWND hWnd)
        :timer_id(::SetTimer(hWnd, 0, UPDATE_TIMER_INTERVAL, nullptr))
        {
        }
        ~FLUSH_TIMER()
        {
           ::KillTimer(NULL, timer_id);
        }
    } FLUSH_TIMER(HWND_UI);

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
    EXCLUDE_WINDOW_LIST[EXCLUDE_WINDOW_COUNT] = HWND_UI;
    EXCLUDE_WINDOW_COUNT++;

    // EXCLUDE_WINDOW_LIST[EXCLUDE_WINDOW_COUNT] = HWND_MAGNIFIER_HOST;
    // EXCLUDE_WINDOW_COUNT++;

    //*************************************************************************//
    if( FALSE == ::MagSetImageScalingCallback(HWND_MAGNIFIER, \
                                                MagnifierUpdateCallback) )
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

    printf("#%02X%02X%02X\n", \
            CAPTURED_COLOR.GetR(),
            CAPTURED_COLOR.GetG(),
            CAPTURED_COLOR.GetB());
    fflush(stdout);

    return (int) msg.wParam;
}


/*******************************************************************************/

extern void SnapshotBasedRefreshCallback();
extern void MagnifierBasedRefreshCallback();
extern std::function<void(void)> TheRefreshCallback;

void MagnifierBasedRefreshCallback()
{
    // printf("%s\n", __FUNCTION__);

    if( FALSE == ::MagSetWindowFilterList(HWND_MAGNIFIER,
                                          MW_FILTERMODE_EXCLUDE,
                                          EXCLUDE_WINDOW_COUNT,
                                          EXCLUDE_WINDOW_LIST) )
    {
        std::cout << "::MagSetWindowFilterList Failed" << ::GetLastError() << "\n";
        throw std::runtime_error("::MagSetWindowFilterList Failed");
    }

    POINT mousePos;
    ::GetCursorPos(&mousePos);

    RECT sourceRect;
    sourceRect.left   = mousePos.x - CAPTURE_WIDTH / 2;
    sourceRect.top    = mousePos.y - CAPTURE_HEIGHT / 2;
    sourceRect.right  = CAPTURE_WIDTH;
    sourceRect.bottom = CAPTURE_HEIGHT;

    // Reclaim topmost status
    ::SetWindowPos(HWND_UI, HWND_TOPMOST, 0, 0, 0, 0, \
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

    // Set the source rectangle for the magnifier control.
    ::MagSetWindowSource(HWND_MAGNIFIER, sourceRect);

    // Force redraw.
    ::InvalidateRect(HWND_MAGNIFIER, NULL, TRUE);
}

BOOL WINAPI MagnifierUpdateCallback(HWND hWnd, \
                                    void* srcdata, MAGIMAGEHEADER srcheader, \
                                    void* destdata, MAGIMAGEHEADER destheader, \
                                    RECT unclipped, RECT clipped, HRGN dirty)
{
    // printf("%s\n", __FUNCTION__);

    auto capture_all_screen_bitmap = []()
    {
        auto hDesktopWnd = ::GetDesktopWindow();
        auto hDesktopDC  = ::GetDC(hDesktopWnd);
        auto hCaptureDC  = ::CreateCompatibleDC(hDesktopDC);
        auto hBrushBlack = ::CreateSolidBrush(RGB(0, 0, 0));

        for (int idx = 0; idx < ALL_MONITOR_INFO_COUNT; ++idx)
        {
            const int display_width = ALL_MONITOR_SIZE_INFO[idx].cx;
            const int display_height = ALL_MONITOR_SIZE_INFO[idx].cy;

            const int display_top = ALL_MONITOR_RECT_INFO[idx].top;
            const int display_left = ALL_MONITOR_RECT_INFO[idx].left;

            auto hCaptureBitmap = ::CreateCompatibleBitmap(hDesktopDC, \
                                            display_width + GRID_NUMUBER_L*2,
                                            display_height + GRID_NUMUBER_L*2);

            ::SelectObject(hCaptureDC, hCaptureBitmap);

            const auto size = ALL_MONITOR_SIZE_INFO[idx];
            const auto rect = RECT{ 0, 0, size.cx, size.cy};

            ::FillRect(hCaptureDC, &rect, hBrushBlack);

            ::BitBlt(hCaptureDC,
                     GRID_NUMUBER_L, GRID_NUMUBER_L,
                     display_width, display_height,
                     hDesktopDC,
                     display_left, display_top,
                     SRCCOPY|CAPTUREBLT);

            ALL_MONITOR_CAPTURED_BITMAP[idx] = \
                Gdiplus::Bitmap::FromHBITMAP(hCaptureBitmap, nullptr);
        }

        ::DeleteObject(hBrushBlack);
        ::DeleteDC(hCaptureDC);
        ::DeleteDC(hDesktopDC);
        ::ReleaseDC(hDesktopWnd, hDesktopDC);
    };

    static bool initd = [&srcheader, &capture_all_screen_bitmap](){
        /*  */ if(srcheader.format == GUID_WICPixelFormat32bppRGBA ) {
            // printf("GUID_WICPixelFormat32bppRGBA\n");
            TheRefreshCallback = MagnifierBasedRefreshCallback;
        } else if(srcheader.format == GUID_WICPixelFormat32bppBGR ){
            // printf("GUID_WICPixelFormat32bppBGR\n");
            capture_all_screen_bitmap();
            TheRefreshCallback = SnapshotBasedRefreshCallback;
        } else {
            // printf("GUID_WICPixelFormat Unkwown\n");
            capture_all_screen_bitmap();
            TheRefreshCallback = SnapshotBasedRefreshCallback;
        }
        return true;
    }();


    /**************************************************************************/
    // Update Zoomed Image
    const auto src_buffer = (BYTE*)srcdata;

    for(int idx_y = 0; idx_y < GRID_NUMUBER; ++idx_y)
    {
        for(int idx_x = 0; idx_x < GRID_NUMUBER; ++idx_x)
        {
            const auto src_pixel = &src_buffer[idx_y*GRID_NUMUBER*4 + idx_x*4];
            const auto r = src_pixel[2];
            const auto g = src_pixel[1];
            const auto b = src_pixel[0];
            // const auto a = src_pixel[3];
            CAPTURED_IMAGE[idx_y][idx_x] = Gdiplus::Color(0xFF, r, g, b);
        }
    }

    DrawZoomedCanvas();

    return TRUE;
}

void SnapshotBasedRefreshCallback()
{
    // printf("%s\n", __FUNCTION__);

    POINT mousePos;
    ::GetCursorPos(&mousePos);

    int mousePosX, mousePosY;
    Gdiplus::Bitmap* current_capture_bitmap = nullptr;
    for (int idx = 0; idx < ALL_MONITOR_INFO_COUNT; ++idx)
    {
        if( PtInRect(&ALL_MONITOR_RECT_INFO[idx], mousePos) )
        {
            current_capture_bitmap = ALL_MONITOR_CAPTURED_BITMAP[idx];
            const auto rect = ALL_MONITOR_RECT_INFO[idx];
            mousePosX = mousePos.x - rect.left;
            mousePosY = mousePos.y - rect.top;
            break;
        }
    }

    // Reclaim topmost status
    ::SetWindowPos(HWND_UI, HWND_TOPMOST, 0, 0, 0, 0, \
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

    /**************************************************************************/
    // Update Zoomed Image
    for(int idx_y = 0; idx_y < GRID_NUMUBER; ++idx_y)
    {
        for(int idx_x = 0; idx_x < GRID_NUMUBER; ++idx_x)
        {
            int x = 1 + (GRID_PIXEL + 1)*idx_x;
            int y = 1 + (GRID_PIXEL + 1)*idx_y;

            int p_x = mousePosX + idx_x;
            int p_y = mousePosY + idx_y;

            Gdiplus::Color color;
            current_capture_bitmap->GetPixel(p_x, p_y, &color);

            CAPTURED_IMAGE[idx_y][idx_x] = color;
        }
    }

    DrawZoomedCanvas();
}

void CheckWhichModeShouldRun()
{
    POINT mousePos;
    ::GetCursorPos(&mousePos);

    RECT sourceRect;
    sourceRect.left   = mousePos.x - CAPTURE_WIDTH / 2;
    sourceRect.top    = mousePos.y - CAPTURE_HEIGHT / 2;
    sourceRect.right  = CAPTURE_WIDTH;
    sourceRect.bottom = CAPTURE_HEIGHT;

    // Set the source rectangle for the magnifier control.
    ::MagSetWindowSource(HWND_MAGNIFIER, sourceRect);

    // Force redraw. The MagnifierUpdateCallback will get called immediately!!
    ::InvalidateRect(HWND_MAGNIFIER, NULL, TRUE);
}

std::function<void(void)> TheRefreshCallback = CheckWhichModeShouldRun;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TIMER:
        TheRefreshCallback();
    break;
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


void DrawZoomedCanvas()
{
    Gdiplus::Graphics graphics(HDC_FOR_UI_WND_CANVASE);
    Gdiplus::Bitmap bitmap(UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT, &graphics);

    Gdiplus::Graphics painter(&bitmap);

    // Paint zoomed capture image inside cliped circle
    auto graphics_state = painter.Save();
    {
        // clip the circle
        Gdiplus::GraphicsPath path(Gdiplus::FillMode::FillModeWinding);
        path.AddEllipse(4, 4, 163, 163);
        Gdiplus::Region region(&path);
        painter.SetClip(&region, Gdiplus::CombineMode::CombineModeReplace);

        // draw zoomed image
        Gdiplus::Pen grid_pen(Gdiplus::Color(0xFF*0.25, 0xCF, 0xCF, 0xCF), 1);
        // Gdiplus::Pen grid_pen(Gdiplus::Color(0xFF*0.05, 0x4F, 0x4F, 0x4F), 1);
        for(int idx_y = 0; idx_y < GRID_NUMUBER; ++idx_y)
        {
            for(int idx_x = 0; idx_x < GRID_NUMUBER; ++idx_x)
            {
                int x = 1 + (GRID_PIXEL + 1)*idx_x;
                int y = 1 + (GRID_PIXEL + 1)*idx_y;

                const auto& pixel_color = CAPTURED_IMAGE[idx_y][idx_x];
                Gdiplus::SolidBrush brush(pixel_color);
                painter.FillRectangle(&brush, x, y, GRID_PIXEL+1, GRID_PIXEL+1);
                painter.DrawRectangle(&grid_pen, x, y, GRID_PIXEL+1, GRID_PIXEL+1);
            }
        }
        // draw center grid
        int x = 1 + (GRID_PIXEL + 1)*GRID_NUMUBER_L;
        int y = 1 + (GRID_PIXEL + 1)*GRID_NUMUBER_L;

        Gdiplus::Pen black_pen(Gdiplus::Color(0xFF, 0x00, 0x00, 0x00), 1);
        Gdiplus::Pen white_pen(Gdiplus::Color(0xFF, 0xFF, 0xFF, 0xFF), 1);

        painter.DrawRectangle(&black_pen, x, y, GRID_PIXEL+1, GRID_PIXEL+1);
        painter.DrawRectangle(&white_pen, x+1, y+1, GRID_PIXEL-1, GRID_PIXEL-1);
    }
    painter.Restore(graphics_state);

    // mask circle
    painter.DrawImage(BITMAP_MASK_CIRCLE, 0, 0, \
                        UI_WINDOW_WIDTH, UI_WINDOW_HEIGHT);

    /**************************************************************************/
    HBITMAP hBitmap;
    bitmap.GetHBITMAP(Gdiplus::Color(), &hBitmap);

    auto hPrevObj = ::SelectObject(HDC_FOR_UI_WND_CANVASE, hBitmap);

    POINT mousePos;
    ::GetCursorPos(&mousePos);

    // POINT ptDest = {800, 400};
    POINT ptDest = { mousePos.x - UI_WINDOW_WIDTH/2, mousePos.y - UI_WINDOW_HEIGHT/2};

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
}

