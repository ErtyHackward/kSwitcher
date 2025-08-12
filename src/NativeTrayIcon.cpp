#include "NativeTrayIcon.h"
#include <uxtheme.h>

NativeTrayIcon::NativeTrayIcon(HWND hWnd, HICON hIcon, const std::wstring& tooltip, MenuItemCallback callback)
    : _hWnd(hWnd), _menuItemCallback(callback) {
    
    // Enable dark mode support
    EnableDarkMode();
    
    // Create popup menu
    _hMenu = CreatePopupMenu();
    
    // Initialize notify icon data
    ZeroMemory(&_notifyIconData, sizeof(_notifyIconData));
    _notifyIconData.cbSize = sizeof(_notifyIconData);
    _notifyIconData.hWnd = _hWnd;
    _notifyIconData.uID = 1;
    _notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    _notifyIconData.uCallbackMessage = WM_TRAYICON;
    _notifyIconData.hIcon = hIcon;
    
    // Copy tooltip (limited to 127 characters)
    wcscpy_s(_notifyIconData.szTip, 
             tooltip.length() > 127 ? tooltip.substr(0, 127).c_str() : tooltip.c_str());
    
    // Add tray icon
    Shell_NotifyIcon(NIM_ADD, &_notifyIconData);
}

NativeTrayIcon::~NativeTrayIcon() {
    // Remove tray icon
    Shell_NotifyIcon(NIM_DELETE, &_notifyIconData);
    
    // Destroy menu
    if (_hMenu) {
        DestroyMenu(_hMenu);
    }
}

void NativeTrayIcon::AddMenuItem(int id, const std::wstring& text, bool isChecked, bool isEnabled) {
    UINT flags = MF_STRING;
    if (isChecked) flags |= MF_CHECKED;
    if (!isEnabled) flags |= MF_GRAYED;
    
    AppendMenu(_hMenu, flags, id, text.c_str());
}

void NativeTrayIcon::AddSeparator() {
    AppendMenu(_hMenu, MF_SEPARATOR, 0, nullptr);
}

void NativeTrayIcon::UpdateMenuItem(int id, bool isChecked) {
    CheckMenuItem(_hMenu, id, isChecked ? MF_CHECKED : MF_UNCHECKED);
}

void NativeTrayIcon::ShowContextMenu() {
    POINT pt;
    GetCursorPos(&pt);
    
    // This ensures the menu is dismissed when clicking outside
    SetForegroundWindow(_hWnd);
    
    // Refresh dark mode for menus before showing
    try {
        if (_flushMenuThemes) {
            _flushMenuThemes();
        }
    }
    catch (...) {
        // Ignore if function not available
    }
    
    // Show the menu at cursor position and get the selected item
    int cmd = TrackPopupMenu(_hMenu, 
                           TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                           pt.x, pt.y, 0, _hWnd, nullptr);
    
    if (cmd != 0 && _menuItemCallback) {
        _menuItemCallback(cmd);
    }
    
    // Send a dummy message to ensure proper cleanup
    PostMessage(_hWnd, WM_NULL, 0, 0);
}

void NativeTrayIcon::ProcessWindowMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_TRAYICON) {
        switch (LOWORD(lParam)) {
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                ShowContextMenu();
                break;
        }
    }
}

void NativeTrayIcon::EnableDarkMode() {
    try {
        if (IsSystemInDarkMode()) {
            // Load uxtheme.dll and get function pointers
            HMODULE hUxtheme = LoadLibrary(L"uxtheme.dll");
            if (hUxtheme) {
                // Use ordinal numbers for undocumented functions
                _setPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(
                    GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
                    
                _allowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(
                    GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
                    
                _flushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(
                    GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
                
                // Enable dark mode
                if (_setPreferredAppMode) {
                    _setPreferredAppMode(PreferredAppMode::AllowDark);
                }
                
                if (_allowDarkModeForWindow) {
                    _allowDarkModeForWindow(_hWnd, true);
                }
            }
        }
    }
    catch (...) {
        // Ignore errors - fallback to default appearance
    }
}

bool NativeTrayIcon::IsSystemInDarkMode() const {
    try {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            
            DWORD value = 0;
            DWORD valueSize = sizeof(value);
            DWORD type;
            
            LONG result = RegQueryValueEx(hKey, L"AppsUseLightTheme", nullptr, 
                                        &type, reinterpret_cast<BYTE*>(&value), &valueSize);
            
            RegCloseKey(hKey);
            
            // Dark mode is enabled when AppsUseLightTheme is 0
            return (result == ERROR_SUCCESS && type == REG_DWORD && value == 0);
        }
    }
    catch (...) {
        // Default to light mode on error
    }
    
    return false;
}