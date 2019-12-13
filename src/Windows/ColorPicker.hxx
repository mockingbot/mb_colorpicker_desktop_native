#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


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
};


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
};
