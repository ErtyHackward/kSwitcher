#pragma once
#include <windows.h>
#include <shellapi.h>
#include <functional>
#include <string>

class NativeTrayIcon {
public:
    using MenuItemCallback = std::function<void(int)>;
    
    NativeTrayIcon(HWND hWnd, HICON hIcon, const std::wstring& tooltip, MenuItemCallback callback);
    ~NativeTrayIcon();
    
    void AddMenuItem(int id, const std::wstring& text, bool isChecked = false, bool isEnabled = true);
    void AddSeparator();
    void UpdateMenuItem(int id, bool isChecked);
    void ShowContextMenu();
    void ProcessWindowMessage(UINT message, WPARAM wParam, LPARAM lParam);
    
    // Constants for menu IDs
    static const int MENU_TEXT_CORRECTION = 1;
    static const int MENU_LAYOUT_SWITCH = 2;
    static const int MENU_AUTO_START = 3;
    static const int MENU_EXIT = 4;

private:
    void EnableDarkMode();
    bool IsSystemInDarkMode() const;
    
    // Dark mode function types
    enum class PreferredAppMode {
        Default = 0,
        AllowDark = 1,
        ForceDark = 2,
        ForceLight = 3,
        Max = 4
    };
    
    using fnSetPreferredAppMode = int(*)(PreferredAppMode appMode);
    using fnAllowDarkModeForWindow = bool(*)(HWND hWnd, bool allow);
    using fnFlushMenuThemes = void(*)();
    
    static const UINT WM_TRAYICON = WM_USER + 1;
    
    NOTIFYICONDATA _notifyIconData;
    HWND _hWnd;
    HMENU _hMenu;
    MenuItemCallback _menuItemCallback;
    
    // Dark mode function pointers
    fnSetPreferredAppMode _setPreferredAppMode = nullptr;
    fnAllowDarkModeForWindow _allowDarkModeForWindow = nullptr;
    fnFlushMenuThemes _flushMenuThemes = nullptr;
};