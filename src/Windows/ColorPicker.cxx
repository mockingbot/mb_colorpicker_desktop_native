#include "ColorPicker.hxx"

#include "../Instance.hxx"
#include "../Predefined.hxx"


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

    // set len magnification factor.
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
}


ScreenLens::~ScreenLens()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    ::DestroyWindow(hwnd_magnifier_);
    ::DestroyWindow(hwnd_magnifier_host_);

    reinterpret_cast<BOOL(WINAPI*)(void)>( \
            ::GetProcAddress(module_handle_, "MagUninitialize") )();

}


MainWindow::MainWindow()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

    auto runtime_host = Instance::RuntimeHost();

    auto runtime_host_data = (void**)runtime_host;
    auto instance_handle = (HINSTANCE)runtime_host_data[2];
    auto class_name = (wchar_t*)runtime_host_data[3];

    window_callback_fun_ptr_ = &MainWindow::Callback;
    printf("callback %p\n", window_callback_fun_ptr_);

    CREATESTRUCT create_struct = {};
    create_struct.lpCreateParams = this;
    printf("this %p\n", this);

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
}


MainWindow::~MainWindow()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
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
    }
    return ::DefWindowProc(handle_, uMsg, wParam, lParam);
}




void
PreRun_Mode_Normal()
{
    fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
    fprintf(stderr, "screen record size: %4u %4u\n", \
                                CAPTURE_WIDTH, CAPTURE_HEIGHT);

    // MainWindow main_window;
    auto screen_lens = new ScreenLens();
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

