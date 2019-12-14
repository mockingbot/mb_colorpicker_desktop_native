#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>

typedef HWND WindowID ;
typedef std::vector<WindowID> WindowIDList;

//! always using this format: (r, g, b, x) = (float, float, float, float)
struct ScreenPixelData
{
    float r = 0, g = 0, b = 0, a = 0;

    static constexpr auto BitsPerChannel()
    {
        return sizeof(struct ScreenPixelData)*8/4;
    }

    static constexpr auto BitsAllChannel()
    {
        return sizeof(struct ScreenPixelData)*8;
    }
};

static_assert( ScreenPixelData::BitsPerChannel() == sizeof(float)*8 );
static_assert( ScreenPixelData::BitsAllChannel() == sizeof(float)*8*4 );


class ScreenLens
{
public:
    ScreenLens();
    ~ScreenLens();
private:
    HMODULE module_handle_ = nullptr;
private:
    HWND hwnd_magnifier_host_ = nullptr;
    HWND hwnd_magnifier_ = nullptr;
private:
    typedef struct
    {
        UINT width;
        UINT height;
        GUID format;
        UINT stride;
        UINT offset;
        SIZE_T cbSize;
    } DataInfo;
private:
    typedef
    BOOL
    (CALLBACK* ZoomCallbackT)
    (
        HWND hwnd,
        void* src_data, DataInfo src_info,
        void* dest_data, DataInfo dest_info,
        RECT unclipped, RECT clipped, HRGN dirty
    );
private:
    BOOL
    (WINAPI* setSourceRect)
    (
        HWND,
        RECT rect
    );
private:
    BOOL
    (WINAPI* setFilterList)
    (
        HWND,
        DWORD filter_mode, int hwnd_list_count, const HWND* hwnd_list
    );
private:
    BOOL (WINAPI* setZoomCallback)
    (
        HWND hwnd,
        ZoomCallbackT callback
    );
private:
    BOOL CALLBACK ZoomCallback
    (
        HWND hwnd,
        void* src_data, DataInfo src_info,
        void* dest_data, DataInfo dest_info,
        RECT unclipped, RECT clipped, HRGN dirty
    );
public:
    // bool
    // RefreshScreenPixelDataWithinBound
    // (
    //     int central_x, int central_y,
    //     int bound_width, int bound_height,
    //     const WindowIDList& excluded_window_list,
    //     struct ScreenPixelData* const off_screen_render_data
    // );
public:
    bool
    SetScreenZoomBound(RECT rect)
    {
        return setSourceRect(hwnd_magnifier_, rect) == TRUE ? true : false;
    }
public:
    bool
    SetExcludedWindowList(const WindowIDList& list)
    {
        return setFilterList(hwnd_magnifier_, 0, \
                    (int)list.size(), list.data() ) == TRUE ? true : false;
    }
// public:
private:
    void* current_screen_data_ = nullptr;
public:
    bool Refresh();
};


bool
RefreshScreenPixelDataWithinBound
(
    int central_x, int central_y,
    int bound_width, int bound_height,
    const WindowIDList& excluded_window_list,
    struct ScreenPixelData* const off_screen_render_data
);


class MainWindow
{
public:
    MainWindow();
    ~MainWindow();
private:
    LRESULT CALLBACK Callback(UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    decltype(&Callback) window_callback_fun_ptr_ = nullptr;
    HWND handle_ = nullptr;
public:
    void Show();
    void Hide();
private:
    void onRefreshTimerTick();
private:
    UINT_PTR refresh_timer_;
};
