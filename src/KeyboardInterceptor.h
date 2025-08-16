#pragma once
#include <windows.h>
#include <vector>
#include <memory>

class KeyboardInterceptor {
public:
    KeyboardInterceptor();
    ~KeyboardInterceptor();
    
    void StartIntercepting();
    void StopIntercepting();

private:
    struct KeystrokeInfo {
        int virtualKey;
        bool shift;
        bool capsLock;
    };
    
    static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    void RecordKeystroke(int vkCode);
    void PerformLayoutCorrection();
    void SwitchKeyboardLayout();
    void ReplayKeystroke(const KeystrokeInfo& keystroke);
    bool IsCharacterKey(int vkCode) const;
    void ClearBuffer();
    
    HHOOK _keyboardHook;
    HHOOK _mouseHook;
    std::vector<KeystrokeInfo> _keystrokeBuffer;
    std::vector<KeystrokeInfo> _lastCorrectedBuffer;
    HWND _lastActiveWindow;
    bool _isProcessingCorrection;
    bool _lastCharWasSpace;
    int _correctionCount;
    
    static KeyboardInterceptor* _instance;
};