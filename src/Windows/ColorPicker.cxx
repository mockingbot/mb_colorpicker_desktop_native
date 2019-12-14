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

    auto runtime_host = Instance::RuntimeHost();

    auto runtime_host_data = (void**)runtime_host;
    auto instance_handle = (HINSTANCE)runtime_host_data[2];
    auto class_name = (wchar_t*)runtime_host_data[3];

    module_handle_ = ::LoadLibrary(L"Magnification.dll");

    reinterpret_cast<BOOL(WINAPI*)(void)>( \
        ::GetProcAddress(module_handle_, "MagInitialize") )();

    hwnd_magnifier_host_ = ::CreateWindow( \
                                        class_name,
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
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    static auto rgba_data_to_raw_data = []
    (void* rgba_data, DataInfo rgba_data_info, void* raw_data)
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
                printf("0x%02X%02X%02X%02X\n", r, g, b, a);
            }
        }

    };

    if( IsEqualGUID(src_info.format, GUID_WICPixelFormat32bppRGBA) )
    {
        printf("GUID_WICPixelFormat32bppRGBA\n");
        // rgba_data_to_raw_data(src_data, src_info, nullptr);
        current_screen_data_ = src_data;
    }
    else
    if( IsEqualGUID(src_info.format, GUID_WICPixelFormat32bppBGR) )
    {
        printf("GUID_WICPixelFormat32bppBGR\n");
    }
    else
    {
        printf("GUID_WICPixelFormat Unkwown\n");
    }

    printf("FFFFFFFFFFFFFFF %d\n", ::GetCurrentThreadId() );
    printf("FFFFFFFFFFFFFFF %p\n", current_screen_data_ );

    return TRUE;
}


bool
ScreenLens::Refresh
(
)
{
    // current_screen_data_ = nullptr;
    ::InvalidateRect(hwnd_magnifier_, NULL, TRUE);
    if (current_screen_data_ == nullptr)
    {
        printf("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n");
    }

    return true;
}

// bool
// ScreenLens::RefreshScreenPixelDataWithinBound
// (
//     int central_x, int central_y,
//     int bound_width, int bound_height,
//     const WindowIDList& excluded_window_list,
//     struct ScreenPixelData* const off_screen_render_data
// )
// {
//     RECT bound_box;
//     bound_box.left = central_x - bound_width/2;
//     bound_box.right = bound_width - bound_box.left;
//     bound_box.top = central_y - bound_height/2;
//     bound_box.bottom = bound_height - bound_box.top;

//     if( TRUE != setSourceRect(hwnd_magnifier_, bound_box) )
//     {
//         fprintf(stderr, "%s Error 1\n", __PRETTY_FUNCTION__);
//     }

//     ::InvalidateRect(hwnd_magnifier_, NULL, TRUE);
// }


MainWindow::MainWindow()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto runtime_host = Instance::RuntimeHost();

    auto runtime_host_data = (void**)runtime_host;
    auto instance_handle = (HINSTANCE)runtime_host_data[2];
    auto class_name = (wchar_t*)runtime_host_data[3];

    window_callback_fun_ptr_ = &MainWindow::Callback;

    CREATESTRUCT create_struct = {};
    create_struct.lpCreateParams = this;

    handle_ = ::CreateWindowEx( \
                        // WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
                        // WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                        0x0000,
                        class_name,
                        L"ColorPicker",
                        WS_OVERLAPPEDWINDOW,
                        400, 400, UI_WINDOW_SIZE, UI_WINDOW_SIZE,
                        nullptr,
                        nullptr,
                        instance_handle,
                        &create_struct);
    if( handle_ == NULL)
    {
        fprintf(stderr, "MainWindow Constructor Error 1 %d\n", ::GetLastError());
        throw std::runtime_error("MainWindow Constructor Error 1");
    }

    const auto fresh_time_interval = 1.0f/CURSOR_REFRESH_FREQUENCY;
    refresh_timer_ = ::SetTimer(handle_, \
                        (UINT_PTR)this, fresh_time_interval*1000, nullptr);

    // cache excluded window list
    excluded_window_list.push_back(handle_);

    // alloc data buffer
    const auto data_size = CAPTURE_WIDTH*CAPTURE_HEIGHT;
    recorded_screen_render_data_buffer = new struct ScreenPixelData[data_size];
}


MainWindow::~MainWindow()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    ::KillTimer(handle_, refresh_timer_);
    ::DestroyWindow(handle_);
}


void
MainWindow::Show()
{
    ::ShowWindow(handle_, SW_SHOW);
}


void
MainWindow::Hide()
{
    ::ShowWindow(handle_, SW_HIDE);
}


LRESULT CALLBACK
MainWindow::Callback(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // printf("%s\n", __PRETTY_FUNCTION__);
    switch(uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
    break;
    case WM_TIMER:
        onRefreshTimerTick();
    break;
    }
    return ::DefWindowProc(handle_, uMsg, wParam, lParam);
}


void
MainWindow::onRefreshTimerTick()
{
    printf("%s\n", __PRETTY_FUNCTION__);

    int x = 0, y = 0;
    GetCurrentCursorPosition(&x, &y);
    fprintf(stderr, "Current Cursor Position: %4d %4d\n", x, y);

    RefreshScreenPixelDataWithinBound( \
            x, y, CAPTURE_WIDTH, CAPTURE_HEIGHT, \
                excluded_window_list, recorded_screen_render_data_buffer );
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

    screen_lens->SetScreenZoomBound(bound_box);
    screen_lens->SetExcludedWindowList(excluded_window_list);
    screen_lens->Refresh();
    printf("SSSSSSSSSSSSSS %d\n", ::GetCurrentThreadId() );

    // screen_lens_->RefreshScreenPixelDataWithinBound( \
    //         central_x, central_y, bound_width, bound_height, \
    //         excluded_window_list, recorded_screen_render_data_buffer );

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

