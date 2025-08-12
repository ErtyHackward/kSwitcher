#pragma once
#include <windows.h>
#include <string>

class Installation {
public:
    static bool IsInstalledLocation();
    static std::wstring GetTargetInstallPath();
    static std::wstring GetCurrentExecutablePath();
    static bool ShowInstallDialog();
    static bool CopyToInstallLocation();
    static bool AddToAutoStart(const std::wstring& executablePath);
    static bool RemoveFromAutoStart();
    static bool IsInAutoStart();

private:
    static bool CreateDirectoryRecursive(const std::wstring& path);
    static std::wstring GetAppDataPath();
    static const wchar_t* AUTO_START_KEY_PATH;
    static const wchar_t* AUTO_START_VALUE_NAME;
};