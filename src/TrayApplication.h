#pragma once
#include <windows.h>
#include <memory>
#include "Settings.h"
#include "NativeTrayIcon.h"
#include "KeyboardInterceptor.h"
#include "Installation.h"

class TrayApplication {
public:
    TrayApplication();
    ~TrayApplication();
    
    int Run();

private:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    void CreateHiddenWindow();
    HICON CreateTrayIcon();
    void OnMenuItemSelected(int menuId);
    void InitializeLayoutSwitchHook();
    void CleanupLayoutSwitchHook();
    void UpdateTrayIcon();
    bool IsSystemInDarkMode();
    
    HWND _hWnd;
    HICON _hIcon;
    std::unique_ptr<Settings> _settings;
    std::unique_ptr<NativeTrayIcon> _trayIcon;
    std::unique_ptr<KeyboardInterceptor> _keyboardInterceptor;
    
    // Layout switch hook data
    HHOOK _layoutSwitchHook;
    bool _altPressed;
    bool _shiftPressed;
    
    static TrayApplication* _instance;
    static const wchar_t* WINDOW_CLASS_NAME;
};