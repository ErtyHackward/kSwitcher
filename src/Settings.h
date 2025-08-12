#pragma once
#include <windows.h>
#include <string>

class Settings {
public:
    // Settings data
    bool textCorrectionEnabled = true;
    bool layoutSwitchEnabled = true;
    bool autoStartWithWindows = false;

    // Static methods
    static Settings Load();
    void Save() const;

private:
    static std::wstring GetSettingsPath();
    static std::wstring GetSettingsDirectory();
    void SyncAutoStartRegistry();
    void UpdateAutoStartRegistry() const;
    static const wchar_t* APP_NAME;
    static const wchar_t* SETTINGS_FILENAME;
    static const wchar_t* REGISTRY_RUN_KEY;
};