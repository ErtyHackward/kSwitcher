#include "Installation.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <iostream>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")

const wchar_t* Installation::AUTO_START_KEY_PATH = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* Installation::AUTO_START_VALUE_NAME = L"kSwitcher";

bool Installation::IsInstalledLocation() {
    std::wstring currentPath = GetCurrentExecutablePath();
    std::wstring targetPath = GetTargetInstallPath();
    
    // Convert to lowercase for case-insensitive comparison
    std::transform(currentPath.begin(), currentPath.end(), currentPath.begin(), ::towlower);
    std::transform(targetPath.begin(), targetPath.end(), targetPath.begin(), ::towlower);
    
    return currentPath == targetPath;
}

std::wstring Installation::GetTargetInstallPath() {
    std::wstring appDataPath = GetAppDataPath();
    return appDataPath + L"\\kSwitcher\\kSwitcher.exe";
}

std::wstring Installation::GetCurrentExecutablePath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(nullptr, exePath, MAX_PATH);
    return std::wstring(exePath);
}

std::wstring Installation::GetAppDataPath() {
    wchar_t* appDataPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath) == S_OK) {
        std::wstring result(appDataPath);
        CoTaskMemFree(appDataPath);
        return result;
    }
    return L"";
}

bool Installation::ShowInstallDialog() {
    std::wstring message = L"kSwitcher - Keyboard Switcher is not installed yet.\n\n"
                          L"Would you like to install it to your AppData folder and enable auto-start with Windows?\n\n"
                          L"Installation location:\n" + GetTargetInstallPath();
    
    int result = MessageBox(nullptr, message.c_str(), L"kSwitcher Installation", 
                           MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
    
    return result == IDYES;
}

bool Installation::CreateDirectoryRecursive(const std::wstring& path) {
    // Extract directory path (remove filename)
    size_t lastSlash = path.find_last_of(L'\\');
    if (lastSlash == std::wstring::npos) {
        return false;
    }
    
    std::wstring dirPath = path.substr(0, lastSlash);
    
    // Create directory recursively
    if (SHCreateDirectoryEx(nullptr, dirPath.c_str(), nullptr) == ERROR_SUCCESS ||
        GetLastError() == ERROR_ALREADY_EXISTS) {
        return true;
    }
    
    return false;
}

bool Installation::CopyToInstallLocation() {
    try {
        std::wstring sourcePath = GetCurrentExecutablePath();
        std::wstring targetPath = GetTargetInstallPath();
        
        // Create target directory
        if (!CreateDirectoryRecursive(targetPath)) {
            MessageBox(nullptr, L"Failed to create installation directory.", 
                      L"Installation Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        // Copy the executable
        if (!CopyFile(sourcePath.c_str(), targetPath.c_str(), FALSE)) {
            std::wstring errorMsg = L"Failed to copy executable to installation directory.\n"
                                   L"Error code: " + std::to_wstring(GetLastError());
            MessageBox(nullptr, errorMsg.c_str(), L"Installation Error", MB_OK | MB_ICONERROR);
            return false;
        }
        
        // Add to auto-start
        if (!AddToAutoStart(targetPath)) {
            MessageBox(nullptr, L"Executable installed but failed to add to auto-start.\n"
                               L"You can enable this later from the tray menu.", 
                      L"Installation Warning", MB_OK | MB_ICONWARNING);
        }
        
        // Show success message and offer to start installed version
        std::wstring successMsg = L"Installation completed successfully!\n\n"
                                 L"The installed version will now start. You can delete this portable version.";
        MessageBox(nullptr, successMsg.c_str(), L"Installation Complete", MB_OK | MB_ICONINFORMATION);
        
        // Start the installed version
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        
        if (CreateProcess(targetPath.c_str(), nullptr, nullptr, nullptr, FALSE, 0, 
                         nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        
        return true;
    }
    catch (...) {
        MessageBox(nullptr, L"An unexpected error occurred during installation.", 
                  L"Installation Error", MB_OK | MB_ICONERROR);
        return false;
    }
}

bool Installation::AddToAutoStart(const std::wstring& executablePath) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, AUTO_START_KEY_PATH, 0, KEY_WRITE, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    std::wstring quotedPath = L"\"" + executablePath + L"\"";
    result = RegSetValueEx(hKey, AUTO_START_VALUE_NAME, 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(quotedPath.c_str()),
                          static_cast<DWORD>((quotedPath.length() + 1) * sizeof(wchar_t)));
    
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}

bool Installation::RemoveFromAutoStart() {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, AUTO_START_KEY_PATH, 0, KEY_WRITE, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    result = RegDeleteValue(hKey, AUTO_START_VALUE_NAME);
    RegCloseKey(hKey);
    
    return result == ERROR_SUCCESS;
}

bool Installation::IsInAutoStart() {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, AUTO_START_KEY_PATH, 0, KEY_READ, &hKey);
    
    if (result != ERROR_SUCCESS) {
        return false;
    }
    
    DWORD valueType;
    DWORD dataSize = 0;
    result = RegQueryValueEx(hKey, AUTO_START_VALUE_NAME, nullptr, &valueType, nullptr, &dataSize);
    
    RegCloseKey(hKey);
    return result == ERROR_SUCCESS;
}