#include "TrayApplication.h"
#include "resource.h"
#include <algorithm>
#include <shellapi.h>

TrayApplication* TrayApplication::_instance = nullptr;
const wchar_t* TrayApplication::WINDOW_CLASS_NAME = L"kSwitcherWindow";

TrayApplication::TrayApplication() 
    : _hWnd(nullptr), _hIcon(nullptr), _layoutSwitchHook(nullptr), 
      _altPressed(false), _shiftPressed(false) {
    _instance = this;
}

TrayApplication::~TrayApplication() {
    CleanupLayoutSwitchHook();
    
    if (_hIcon) {
        DestroyIcon(_hIcon);
    }
    
    if (_hWnd) {
        DestroyWindow(_hWnd);
    }
    
    _instance = nullptr;
}

int TrayApplication::Run() {
    try {
        // Load settings
        _settings = std::make_unique<Settings>(Settings::Load());
        
        // Create hidden window
        CreateHiddenWindow();
        
        // Create tray icon
        _hIcon = CreateTrayIcon();
        
        // Initialize tray icon
        _trayIcon = std::make_unique<NativeTrayIcon>(
            _hWnd, _hIcon,
            L"kSwitcher - Alt+Shift to switch, Pause to correct text",
            [this](int menuId) { OnMenuItemSelected(menuId); }
        );
        
        // Build menu
        _trayIcon->AddMenuItem(NativeTrayIcon::MENU_TEXT_CORRECTION, 
                             L"Text Correction (Pause key)", 
                             _settings->textCorrectionEnabled);
        _trayIcon->AddMenuItem(NativeTrayIcon::MENU_LAYOUT_SWITCH, 
                             L"Layout Switch (Alt+Shift)", 
                             _settings->layoutSwitchEnabled);
        _trayIcon->AddSeparator();
        _trayIcon->AddMenuItem(NativeTrayIcon::MENU_AUTO_START, 
                             L"Start with Windows", 
                             Installation::IsInAutoStart());
        _trayIcon->AddSeparator();
        _trayIcon->AddMenuItem(NativeTrayIcon::MENU_EXIT, L"Exit");
        
        // Initialize keyboard interceptor
        _keyboardInterceptor = std::make_unique<KeyboardInterceptor>();
        if (_settings->textCorrectionEnabled) {
            _keyboardInterceptor->StartIntercepting();
        }
        
        // Initialize layout switch hook
        InitializeLayoutSwitchHook();
        
        // Message loop
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        return static_cast<int>(msg.wParam);
    }
    catch (...) {
        return -1;
    }
}

void TrayApplication::CreateHiddenWindow() {
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = GetModuleHandle(nullptr);
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    
    RegisterClassEx(&wcex);
    
    _hWnd = CreateWindowEx(0, WINDOW_CLASS_NAME, L"kSwitcher",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                          nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
}

HICON TrayApplication::CreateTrayIcon() {
    // Decide light/dark based on system setting
    auto isSystemDark = []() -> bool {
        try {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER,
                             L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                             0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD value = 0, size = sizeof(value), type = 0;
                LONG res = RegQueryValueEx(hKey, L"AppsUseLightTheme", nullptr, &type,
                                           reinterpret_cast<BYTE*>(&value), &size);
                RegCloseKey(hKey);
                return (res == ERROR_SUCCESS && type == REG_DWORD && value == 0);
            }
        } catch (...) {}
        return false;
    }();

    // Get DPI-aware icon size
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);
    
    // Calculate proper icon size based on DPI
    // Standard is 16px at 96 DPI, scale accordingly
    int iconSize = MulDiv(16, dpi, 96);
    
    // Round to nearest standard size for better quality
    if (iconSize <= 16) iconSize = 16;
    else if (iconSize <= 24) iconSize = 24;
    else if (iconSize <= 32) iconSize = 32;
    else if (iconSize <= 48) iconSize = 48;
    else if (iconSize <= 64) iconSize = 64;
    else iconSize = 128;

    int resId = isSystemDark ? IDI_TRAYICON_DARK : IDI_TRAYICON_LIGHT;
    HICON hIcon = static_cast<HICON>(
        LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(resId),
                  IMAGE_ICON, iconSize, iconSize, LR_DEFAULTCOLOR));
    if (!hIcon) hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    return hIcon;
}

bool TrayApplication::IsSystemInDarkMode() {
    try {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                         0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD value = 0, size = sizeof(value), type = 0;
            LONG res = RegQueryValueEx(hKey, L"AppsUseLightTheme", nullptr, &type,
                                       reinterpret_cast<BYTE*>(&value), &size);
            RegCloseKey(hKey);
            return (res == ERROR_SUCCESS && type == REG_DWORD && value == 0);
        }
    } catch (...) {}
    return false;
}

void TrayApplication::UpdateTrayIcon() {
    if (!_trayIcon || !_hWnd) return;
    
    // Get DPI-aware icon size
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);
    
    // Calculate proper icon size based on DPI
    int iconSize = MulDiv(16, dpi, 96);
    
    // Round to nearest standard size for better quality
    if (iconSize <= 16) iconSize = 16;
    else if (iconSize <= 24) iconSize = 24;
    else if (iconSize <= 32) iconSize = 32;
    else if (iconSize <= 48) iconSize = 48;
    else if (iconSize <= 64) iconSize = 64;
    else iconSize = 128;
    
    int resId = IsSystemInDarkMode() ? IDI_TRAYICON_DARK : IDI_TRAYICON_LIGHT;
    HICON newIcon = static_cast<HICON>(
        LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(resId),
                  IMAGE_ICON, iconSize, iconSize, LR_DEFAULTCOLOR));
    
    if (newIcon) {
        // Update the tray icon
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(nid);
        nid.hWnd = _hWnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON;
        nid.hIcon = newIcon;
        
        Shell_NotifyIcon(NIM_MODIFY, &nid);
        
        // Clean up old icon if it exists
        if (_hIcon && _hIcon != newIcon) {
            DestroyIcon(_hIcon);
        }
        _hIcon = newIcon;
    }
}

void TrayApplication::OnMenuItemSelected(int menuId) {
    switch (menuId) {
        case NativeTrayIcon::MENU_TEXT_CORRECTION:
            _settings->textCorrectionEnabled = !_settings->textCorrectionEnabled;
            _trayIcon->UpdateMenuItem(NativeTrayIcon::MENU_TEXT_CORRECTION, 
                                    _settings->textCorrectionEnabled);
            
            if (_settings->textCorrectionEnabled) {
                _keyboardInterceptor->StartIntercepting();
            } else {
                _keyboardInterceptor->StopIntercepting();
            }
            
            _settings->Save();
            break;
            
        case NativeTrayIcon::MENU_LAYOUT_SWITCH:
            _settings->layoutSwitchEnabled = !_settings->layoutSwitchEnabled;
            _trayIcon->UpdateMenuItem(NativeTrayIcon::MENU_LAYOUT_SWITCH, 
                                    _settings->layoutSwitchEnabled);
            _settings->Save();
            break;
            
        case NativeTrayIcon::MENU_AUTO_START: {
            bool currentlyEnabled = Installation::IsInAutoStart();
            bool success = false;
            
            if (currentlyEnabled) {
                success = Installation::RemoveFromAutoStart();
            } else {
                std::wstring currentPath = Installation::GetCurrentExecutablePath();
                success = Installation::AddToAutoStart(currentPath);
            }
            
            if (success) {
                _trayIcon->UpdateMenuItem(NativeTrayIcon::MENU_AUTO_START, 
                                        !currentlyEnabled);
            } else {
                MessageBox(nullptr, 
                          currentlyEnabled ? 
                              L"Failed to remove from auto-start. Check registry permissions." :
                              L"Failed to add to auto-start. Check registry permissions.",
                          L"Auto-start Error", MB_OK | MB_ICONERROR);
            }
            break;
        }
            
        case NativeTrayIcon::MENU_EXIT:
            PostQuitMessage(0);
            break;
    }
}

void TrayApplication::InitializeLayoutSwitchHook() {
    _layoutSwitchHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc,
                                       GetModuleHandle(nullptr), 0);
}

void TrayApplication::CleanupLayoutSwitchHook() {
    if (_layoutSwitchHook) {
        UnhookWindowsHookEx(_layoutSwitchHook);
        _layoutSwitchHook = nullptr;
    }
}

LRESULT CALLBACK TrayApplication::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (_instance && _instance->_trayIcon) {
        _instance->_trayIcon->ProcessWindowMessage(message, wParam, lParam);
    }
    
    switch (message) {
        case WM_SETTINGCHANGE:
            // Check if theme changed
            if (lParam && wcscmp(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0) {
                if (_instance) {
                    _instance->UpdateTrayIcon();
                }
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    
    return 0;
}

LRESULT CALLBACK TrayApplication::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && _instance && _instance->_settings->layoutSwitchEnabled) {
        KBDLLHOOKSTRUCT* pKbdStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        int vkCode = pKbdStruct->vkCode;
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool isKeyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        
        bool isAlt = (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU);
        bool isShift = (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT);
        
        if (isAlt) {
            _instance->_altPressed = isKeyDown;
        } else if (isShift) {
            _instance->_shiftPressed = isKeyDown;
        }
        
        // Trigger layout switch on Alt+Shift
        if (isKeyDown && _instance->_altPressed && _instance->_shiftPressed) {
            HWND hWnd = GetForegroundWindow();
            if (hWnd) {
                PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0x02, 0);
            }
            
            _instance->_altPressed = false;
            _instance->_shiftPressed = false;
            return 1; // Suppress the key
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
