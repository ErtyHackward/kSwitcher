#include <windows.h>
#include <shellscalingapi.h>
#include "TrayApplication.h"
#include "Installation.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, 
                   _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
    
    // Set DPI awareness programmatically
    // Try the newer API first (Windows 10 v1703+)
    HMODULE hUser32 = GetModuleHandle(L"user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(HANDLE);
        SetProcessDpiAwarenessContextProc setProcessDpiAwarenessContext =
            (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContextW");
        
        if (setProcessDpiAwarenessContext) {
            setProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            // Fallback to older API (Windows 8.1+)
            HMODULE hShcore = LoadLibrary(L"Shcore.dll");
            if (hShcore) {
                typedef HRESULT(WINAPI* SetProcessDpiAwarenessProc)(PROCESS_DPI_AWARENESS);
                SetProcessDpiAwarenessProc setProcessDpiAwareness =
                    (SetProcessDpiAwarenessProc)GetProcAddress(hShcore, "SetProcessDpiAwareness");
                
                if (setProcessDpiAwareness) {
                    setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
                }
                FreeLibrary(hShcore);
            } else {
                // Fallback to oldest API
                SetProcessDPIAware();
            }
        }
    }
    
    // Check if app needs to be installed
    if (!Installation::IsInstalledLocation()) {
        if (Installation::ShowInstallDialog()) {
            bool installSuccess = Installation::CopyToInstallLocation();
            // Exit after installation attempt (success or failure)
            return installSuccess ? 0 : 1;
        }
        // User declined installation, continue running from current location
    }
    
    // Prevent multiple instances
    HANDLE hMutex = CreateMutex(nullptr, TRUE, L"kSwitcherMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 0;
    }
    
    int result = -1;
    try {
        TrayApplication app;
        result = app.Run();
    }
    catch (...) {
        result = -1;
    }
    
    CloseHandle(hMutex);
    return result;
}