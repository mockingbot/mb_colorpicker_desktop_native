#include "ColorPicker.hxx"

#include "../Instance.hxx"
#include "../Predefined.hxx"


BOOL should_log_out_central_pixel_color = TRUE;
ScreenPixelData* recorded_screen_render_data_buffer = nullptr;

WindowIDList excluded_window_list;


void
GetCurrentCursorPosition
(
    int* const x, int* const y
)
{
    POINT mousePos;
    ::GetCursorPos(&mousePos);

    *x = mousePos.x;
    *y = mousePos.y;
}


ScreenLens::ScreenLens()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    module_handle_ = ::LoadLibrary(L"Magnification.dll");

    reinterpret_cast<BOOL(WINAPI*)(void)>( \
        ::GetProcAddress(module_handle_, "MagInitialize") )();

    auto runtime_host = Instance::RuntimeHost();
    auto instance_handle = (HINSTANCE)(((void**)runtime_host)[2]);

    window_class_name_ = L"MG_WINCLS#???????"; // ?? TODO:

    WNDCLASSEX wcex = {};

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = ::DefWindowProc;
    wcex.hInstance      = instance_handle;
    wcex.lpszClassName  = window_class_name_;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(1 + COLOR_BTNFACE);

    if( 0 == ::RegisterClassEx(&wcex) ) {
        fprintf(stderr, "RuntimeHostNative Constructor Error 1\n");
        throw std::runtime_error("RuntimeHostNative Constructor Error 1");
    }

    hwnd_magnifier_host_ = ::CreateWindow( \
                                        window_class_name_,
                                        L"MagnifierWindowHost",
                                        WS_POPUP,
                                        0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT,
                                        nullptr,
                                        nullptr,
                                        instance_handle,
                                        nullptr);
    if( hwnd_magnifier_host_ == NULL )
    {
        fprintf(stderr, "ScreenLens Constructor Error 1 %d\n", ::GetLastError());
        throw std::runtime_error("ScreenLens Constructor Error 1");
    }

    hwnd_magnifier_ = ::CreateWindow( \
                                    L"Magnifier",
                                    L"MagnifierWindow",
                                    WS_CHILD | WS_VISIBLE,
                                    0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT,
                                    hwnd_magnifier_host_,
                                    nullptr,
                                    instance_handle,
                                    nullptr);
    if( hwnd_magnifier_ == NULL )
    {
        fprintf(stderr, "ScreenLens Constructor Error 2 %d\n", ::GetLastError());
        throw std::runtime_error("ScreenLens Constructor Error 2");
    }

    // set len magnification factor
    typedef struct
    {
        float v[3][3];
    } transform_matrix_t;

    transform_matrix_t matrix = {};
    matrix.v[0][0] = 1.00;
    matrix.v[1][1] = 1.00;
    matrix.v[2][2] = 1.00;

    BOOL(WINAPI *set_lens_transform_matrix)(HWND, transform_matrix_t*);

    set_lens_transform_matrix = \
            reinterpret_cast<decltype(set_lens_transform_matrix)>( \
                ::GetProcAddress(module_handle_, "MagSetWindowTransform") );

    if( FALSE == set_lens_transform_matrix(hwnd_magnifier_, &matrix) )
    {
        fprintf(stderr, "ScreenLens Constructor Error 3 %d\n", ::GetLastError());
        throw std::runtime_error("ScreenLens Constructor Error 3");
    }

    setSourceRect = \
            reinterpret_cast<decltype(setSourceRect)>( \
                ::GetProcAddress(module_handle_, "MagSetWindowSource") );

    setFilterList = \
            reinterpret_cast<decltype(setFilterList)>( \
                ::GetProcAddress(module_handle_, "MagSetWindowFilterList") );

    setZoomCallback = \
            reinterpret_cast<decltype(setZoomCallback)>( \
                ::GetProcAddress(module_handle_, "MagSetImageScalingCallback") );


    static auto who = this;

    static auto callback =
    []
    (
        HWND hwnd,
        void* src_data, DataInfo src_info,
        void* dest_data, DataInfo dest_info,
        RECT unclipped, RECT clipped, HRGN dirty
    )
    {
        return who->ZoomCallback(
            hwnd,
            src_data, src_info,
            dest_data, dest_info,
            unclipped, clipped, dirty
        );
        return TRUE;
    };

    if( TRUE != setZoomCallback(hwnd_magnifier_, callback) )
    {
        fprintf(stderr, "ScreenLens Constructor Error 4 %d\n", ::GetLastError());
        throw std::runtime_error("ScreenLens Constructor Error 4");
    }
}


ScreenLens::~ScreenLens()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    ::DestroyWindow(hwnd_magnifier_);
    ::DestroyWindow(hwnd_magnifier_host_);

    auto runtime_host = Instance::RuntimeHost();
    auto instance_handle = (HINSTANCE)(((void**)runtime_host)[2]);
    if( 0 == ::UnregisterClass(window_class_name_, instance_handle) )
    {
        fprintf(stderr, "RuntimeHostNative Deconstructor Error\n");
    }

    reinterpret_cast<BOOL(WINAPI*)(void)>( \
            ::GetProcAddress(module_handle_, "MagUninitialize") )();

}


DEFINE_GUID(GUID_WICPixelFormat32bppRGBA, \
    0xf5c7ad2d, 0x6a8d, 0x43dd, 0xa7, 0xa8, 0xa2, 0x99, 0x35, 0x26, 0x1a, 0xe9);

DEFINE_GUID(GUID_WICPixelFormat32bppBGR, \
    0x6fddc324, 0x4e03, 0x4bfe, 0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0e);

BOOL
CALLBACK
ScreenLens::ZoomCallback
(
    HWND hwnd,
    void* src_data, DataInfo src_info,
    void* dest_data, DataInfo dest_info,
    RECT unclipped, RECT clipped, HRGN dirty
)
{
    // fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    current_screen_data_ = src_data;
    current_screen_data_info_ = src_info;
    return TRUE;
}


bool
ScreenLens::RefreshScreenPixelDataWithinBound
(
    RECT bound_box
)
{
    /*
     * call this will trigeer zoom callback, therefor the
     * current_screen_data_** will get updated
     */
    if( TRUE != setSourceRect(hwnd_magnifier_, bound_box) )
    {
        fprintf(stderr, "%s Error 1\n", __PRETTY_FUNCTION__);
        return false;
    }

    static auto rgba_data_to_raw_data = []
    (void* rgba_data, DataInfo rgba_data_info)
    {
        const auto cursor_base = (uint8_t*)rgba_data + rgba_data_info.offset;
        for(UINT y = 0; y < rgba_data_info.height ; ++y)
        {
            for(UINT x = 0; x < rgba_data_info.height ; ++x)
            {
                auto cursor = cursor_base + y*rgba_data_info.stride + x*4;
                auto b = cursor[0];
                auto g = cursor[1];
                auto r = cursor[2];
                auto a = cursor[3];
                // printf("0x%02X%02X%02X%02X\n", r, g, b, a);
            }
        }
    };

    static auto bgr_data_to_raw_data = []
    (void* rgba_data, DataInfo rgba_data_info)
    {
        const auto cursor_base = (uint8_t*)rgba_data + rgba_data_info.offset;
        for(UINT y = 0; y < rgba_data_info.height ; ++y)
        {
            for(UINT x = 0; x < rgba_data_info.height ; ++x)
            {
                auto cursor = cursor_base + y*rgba_data_info.stride + x*4;
                auto b = cursor[0];
                auto g = cursor[1];
                auto r = cursor[2];
                auto a = cursor[3];
                // printf("0x%02X%02X%02X%02X\n", r, g, b, a);
            }
        }
    };

    if( IsEqualGUID(current_screen_data_info_.format, \
                                GUID_WICPixelFormat32bppRGBA) )
    {
        rgba_data_to_raw_data(current_screen_data_, current_screen_data_info_);
        return true;
    }
    else
    if( IsEqualGUID(current_screen_data_info_.format, \
                                GUID_WICPixelFormat32bppBGR) )
    {
        bgr_data_to_raw_data(current_screen_data_, current_screen_data_info_);
        return true;
    }
    else
    {
        fprintf(stderr, "%s Error 2\n", __PRETTY_FUNCTION__);
        return false;
    }
}


LRESULT CALLBACK
WindowProcess
(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
)
{
    class MainWindow* window = nullptr;

    if( uMsg == WM_NCCREATE )
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        if( cs->lpCreateParams != nullptr )
        {
            window = *(class MainWindow**)(cs->lpCreateParams);
            window->window_handle_ = hWnd;

            ::SetLastError(ERROR_SUCCESS);
            auto result = ::SetWindowLongPtr(hWnd, \
                    GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            if( result == 0 && ::GetLastError() != ERROR_SUCCESS )
            {
                fprintf(stderr, "RuntimeHostNative::WndProc Error 1\n");
            }
        }
    }
    else
    {
        window = (class MainWindow*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if( window != nullptr )
    {
        return window->Callback(uMsg, wParam, lParam);
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}


MainWindow::MainWindow()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    auto runtime_host = Instance::RuntimeHost();
    auto instance_handle = (HINSTANCE)(((void**)runtime_host)[2]);

    window_class_name_ = L"WINCLS#???????"; // ?? TODO:

    WNDCLASSEX wcex = {};

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc    = WindowProcess;
    wcex.hInstance      = instance_handle;
    wcex.lpszClassName  = window_class_name_;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(1 + COLOR_BTNFACE);

    if( 0 == ::RegisterClassEx(&wcex) ) {
        fprintf(stderr, "RuntimeHostNative Constructor Error 1\n");
        throw std::runtime_error("RuntimeHostNative Constructor Error 1");
    }

    CREATESTRUCT create_struct = {};
    create_struct.lpCreateParams = this;
    printf("this %p\n", this);
    printf("ccccccccccc %p\n", &create_struct);

    window_handle_ = ::CreateWindowEx( \
                        // WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
                        // WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                        0x0000,
                        window_class_name_,
                        L"ColorPicker",
                        WS_OVERLAPPEDWINDOW,
                        400, 400, UI_WINDOW_SIZE, UI_WINDOW_SIZE,
                        nullptr,
                        nullptr,
                        instance_handle,
                        &create_struct);

    if( window_handle_ == NULL)
    {
        fprintf(stderr, "MainWindow Constructor Error 1 %d\n", ::GetLastError());
        throw std::runtime_error("MainWindow Constructor Error 1");
    }

    const auto fresh_time_interval = 1.0f/CURSOR_REFRESH_FREQUENCY;
    refresh_timer_ = ::SetTimer(window_handle_, \
                        (UINT_PTR)this, fresh_time_interval*1000, nullptr);

    // cache excluded window list
    excluded_window_list.push_back(window_handle_);

    // alloc data buffer
    const auto data_size = CAPTURE_WIDTH*CAPTURE_HEIGHT;
    recorded_screen_render_data_buffer = new struct ScreenPixelData[data_size];
}


MainWindow::~MainWindow()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    ::KillTimer(window_handle_, refresh_timer_);

    ::DestroyWindow(window_handle_);

    auto runtime_host = Instance::RuntimeHost();
    auto instance_handle = (HINSTANCE)(((void**)runtime_host)[2]);
    if( 0 == ::UnregisterClass(window_class_name_, instance_handle) )
    {
        fprintf(stderr, "RuntimeHostNative Deconstructor Error\n");
    }
}


LRESULT CALLBACK
MainWindow::Callback(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
    break;
    case WM_TIMER:
        onRefreshTimerTick();
    break;
    }
    return ::DefWindowProc(window_handle_, uMsg, wParam, lParam);
}


void
MainWindow::onRefreshTimerTick()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    int x = 0, y = 0;
    GetCurrentCursorPosition(&x, &y);
    // fprintf(stderr, "Current Cursor Position: %4d %4d\n", x, y);

    static uint32_t record_screen_render_data_fresh_ratio_counter = 0;

    if( record_screen_render_data_fresh_ratio_counter == 0 )
    {
        RefreshScreenPixelDataWithinBound( \
            x, y, CAPTURE_WIDTH, CAPTURE_HEIGHT, \
                excluded_window_list, recorded_screen_render_data_buffer );
    }

    record_screen_render_data_fresh_ratio_counter += 1;
    record_screen_render_data_fresh_ratio_counter %= \
        SCREEN_CAPTURE_FREQUENCY_TO_CURSOR_REFRESH_RATIO;

}


bool
RefreshScreenPixelDataWithinBound
(
    int central_x, int central_y,
    int bound_width, int bound_height,
    const WindowIDList& excluded_window_list,
    struct ScreenPixelData* const off_screen_render_data
)
{
    RECT bound_box;
    bound_box.left = central_x - bound_width/2;
    bound_box.right = bound_width - bound_box.left;
    bound_box.top = central_y - bound_height/2;
    bound_box.bottom = bound_height - bound_box.top;

    static auto screen_lens = new class ScreenLens;

    screen_lens->SetExcludedWindowList(excluded_window_list);
    screen_lens->RefreshScreenPixelDataWithinBound(bound_box);

    return true;
}


void
PreRun_Mode_Normal()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    fprintf(stderr, "screen record size: %4u %4u\n", \
                                CAPTURE_WIDTH, CAPTURE_HEIGHT);

    // MainWindow main_window;
    auto main_window = new MainWindow();
    main_window->Show();
}


void
PostRun_Mode_Normal()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}


void
PreRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto inst_info = instance->InstanceInfo();
    auto exec_mode = inst_info->CommandLineParameter<int>(L"--mode=");

    switch(exec_mode)
    {
        case 0:
            PreRun_Mode_Normal();
        break;
        default:
            // pass
        break;
    }
}


void
PostRun(class Instance* instance)
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto inst_info = instance->InstanceInfo();
    auto exec_mode = inst_info->CommandLineParameter<int>(L"--mode=");

    switch(exec_mode)
    {
        case 0:
            PostRun_Mode_Normal();
        break;
        default:
            // pass
        break;
    }
}

