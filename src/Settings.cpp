#include "Settings.h"
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <map>

const wchar_t* Settings::APP_NAME = L"LayoutSwitcher";
const wchar_t* Settings::SETTINGS_FILENAME = L"settings.yml";
const wchar_t* Settings::REGISTRY_RUN_KEY = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

std::wstring Settings::GetSettingsDirectory() {
    wchar_t* appDataPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath))) {
        std::wstring dir = appDataPath;
        CoTaskMemFree(appDataPath);
        dir += L"\\";
        dir += APP_NAME;
        return dir;
    }
    return L"";
}

std::wstring Settings::GetSettingsPath() {
    auto dir = GetSettingsDirectory();
    if (!dir.empty()) {
        dir += L"\\";
        dir += SETTINGS_FILENAME;
    }
    return dir;
}

// Convert between std::string and std::wstring
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], sizeNeeded, nullptr, nullptr);
    return str;
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return {};
    
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}

// Simple YAML parser for key: value pairs
std::map<std::string, std::string> ParseSimpleYAML(const std::string& content) {
    std::map<std::string, std::string> result;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Remove leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos || line[start] == '#') continue;
        
        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);
        
        // Find colon separator
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            result[key] = value;
        }
    }
    
    return result;
}

bool ParseBool(const std::string& value) {
    return value == "true" || value == "True" || value == "TRUE" || value == "1";
}

Settings Settings::Load() {
    Settings settings;
    
    try {
        auto settingsPath = GetSettingsPath();
        if (!settingsPath.empty() && GetFileAttributes(settingsPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            std::ifstream file(settingsPath);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
                
                auto config = ParseSimpleYAML(content);
                
                if (config.find("textCorrectionEnabled") != config.end()) {
                    settings.textCorrectionEnabled = ParseBool(config["textCorrectionEnabled"]);
                }
                if (config.find("layoutSwitchEnabled") != config.end()) {
                    settings.layoutSwitchEnabled = ParseBool(config["layoutSwitchEnabled"]);
                }
                if (config.find("autoStartWithWindows") != config.end()) {
                    settings.autoStartWithWindows = ParseBool(config["autoStartWithWindows"]);
                }
            }
        }
    }
    catch (...) {
        // Use default settings on any error
    }
    
    settings.SyncAutoStartRegistry();
    return settings;
}

void Settings::Save() const {
    try {
        auto settingsDir = GetSettingsDirectory();
        if (settingsDir.empty()) return;
        
        // Create directory if it doesn't exist
        CreateDirectoryW(settingsDir.c_str(), nullptr);
        
        auto settingsPath = GetSettingsPath();
        if (settingsPath.empty()) return;
        
        // Create YAML content manually
        std::ostringstream yaml;
        yaml << "textCorrectionEnabled: " << (textCorrectionEnabled ? "true" : "false") << "\n";
        yaml << "layoutSwitchEnabled: " << (layoutSwitchEnabled ? "true" : "false") << "\n";
        yaml << "autoStartWithWindows: " << (autoStartWithWindows ? "true" : "false") << "\n";
        
        // Write to file
        std::ofstream file(settingsPath);
        if (file.is_open()) {
            file << yaml.str();
        }
        
        UpdateAutoStartRegistry();
    }
    catch (...) {
        // Ignore save errors
    }
}

void Settings::SyncAutoStartRegistry() {
    try {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_RUN_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t buffer[MAX_PATH];
            DWORD bufferSize = sizeof(buffer);
            DWORD type;
            
            LONG result = RegQueryValueEx(hKey, APP_NAME, nullptr, &type, 
                                        reinterpret_cast<BYTE*>(buffer), &bufferSize);
            
            autoStartWithWindows = (result == ERROR_SUCCESS && type == REG_SZ);
            RegCloseKey(hKey);
        }
    }
    catch (...) {
        autoStartWithWindows = false;
    }
}

void Settings::UpdateAutoStartRegistry() const {
    try {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_RUN_KEY, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            if (autoStartWithWindows) {
                wchar_t exePath[MAX_PATH];
                if (GetModuleFileName(nullptr, exePath, MAX_PATH) > 0) {
                    std::wstring quotedPath = L"\"";
                    quotedPath += exePath;
                    quotedPath += L"\"";
                    
                    RegSetValueEx(hKey, APP_NAME, 0, REG_SZ, 
                                reinterpret_cast<const BYTE*>(quotedPath.c_str()),
                                static_cast<DWORD>((quotedPath.length() + 1) * sizeof(wchar_t)));
                }
            } else {
                RegDeleteValue(hKey, APP_NAME);
            }
            RegCloseKey(hKey);
        }
    }
    catch (...) {
        // Ignore registry errors
    }
}