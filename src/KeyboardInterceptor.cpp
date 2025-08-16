#include "KeyboardInterceptor.h"
#include <algorithm>

KeyboardInterceptor* KeyboardInterceptor::_instance = nullptr;

KeyboardInterceptor::KeyboardInterceptor() 
    : _keyboardHook(nullptr), _mouseHook(nullptr), _lastActiveWindow(nullptr),
      _isProcessingCorrection(false), _lastCharWasSpace(false), _correctionCount(0) {
    _instance = this;
}

KeyboardInterceptor::~KeyboardInterceptor() {
    StopIntercepting();
    _instance = nullptr;
}

void KeyboardInterceptor::StartIntercepting() {
    if (!_keyboardHook) {
        _keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, 
                                       GetModuleHandle(nullptr), 0);
    }
    
    if (!_mouseHook) {
        _mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, 
                                    GetModuleHandle(nullptr), 0);
    }
}

void KeyboardInterceptor::StopIntercepting() {
    if (_keyboardHook) {
        UnhookWindowsHookEx(_keyboardHook);
        _keyboardHook = nullptr;
    }
    
    if (_mouseHook) {
        UnhookWindowsHookEx(_mouseHook);
        _mouseHook = nullptr;
    }
}

LRESULT CALLBACK KeyboardInterceptor::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && _instance && !_instance->_isProcessingCorrection) {
        int vkCode = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam)->vkCode;
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        
        HWND currentWindow = GetForegroundWindow();
        if (currentWindow != _instance->_lastActiveWindow) {
            _instance->ClearBuffer();
            _instance->_lastActiveWindow = currentWindow;
        }
        
        if (vkCode == VK_PAUSE && isKeyDown) {
            _instance->PerformLayoutCorrection();
            return 1; // Suppress the key
        }
        
        if (isKeyDown) {
            _instance->RecordKeystroke(vkCode);
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardInterceptor::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && _instance && !_instance->_isProcessingCorrection) {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN) {
            _instance->ClearBuffer();
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void KeyboardInterceptor::RecordKeystroke(int vkCode) {
    // Clear buffer on navigation keys (excluding space)
    if (vkCode == VK_RETURN || vkCode == VK_TAB ||
        vkCode == VK_LEFT || vkCode == VK_RIGHT || vkCode == VK_UP || vkCode == VK_DOWN ||
        vkCode == VK_HOME || vkCode == VK_END || vkCode == VK_PRIOR || vkCode == VK_NEXT ||
        vkCode == VK_ESCAPE) {
        ClearBuffer();
        return;
    }
    
    // Handle backspace
    if (vkCode == VK_BACK && !_keystrokeBuffer.empty()) {
        _keystrokeBuffer.pop_back();
        // Update space flag based on the last character in buffer
        _lastCharWasSpace = !_keystrokeBuffer.empty() && 
                           _keystrokeBuffer.back().virtualKey == VK_SPACE;
        return;
    }
    
    // Record character keys
    if (IsCharacterKey(vkCode)) {
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool capsLock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        
        if (!ctrl && !alt) {
            // If last character was space and current is not space, clear buffer
            if (_lastCharWasSpace && vkCode != VK_SPACE) {
                ClearBuffer();
                _lastCharWasSpace = false;
            }
            
            KeystrokeInfo keystroke;
            keystroke.virtualKey = vkCode;
            keystroke.shift = shift;
            keystroke.capsLock = capsLock;
            _keystrokeBuffer.push_back(keystroke);
            
            // Track if current character is space
            _lastCharWasSpace = (vkCode == VK_SPACE);
        } else {
            ClearBuffer();
            _lastCharWasSpace = false;
        }
    }
}

void KeyboardInterceptor::PerformLayoutCorrection() {
    _isProcessingCorrection = true;
    
    try {
        std::vector<KeystrokeInfo> bufferToCorrect;
        
        if (!_keystrokeBuffer.empty()) {
            bufferToCorrect = _keystrokeBuffer;
            _correctionCount = 1;
        } else if (!_lastCorrectedBuffer.empty()) {
            bufferToCorrect = _lastCorrectedBuffer;
            _correctionCount++;
        } else {
            _isProcessingCorrection = false;
            return;
        }
        
        // Delete the typed text
        for (size_t i = 0; i < bufferToCorrect.size(); ++i) {
            keybd_event(VK_BACK, 0, 0, 0);
            keybd_event(VK_BACK, 0, KEYEVENTF_KEYUP, 0);
        }
        
        // Switch keyboard layout
        SwitchKeyboardLayout();
        
        Sleep(50);
        
        // Replay keystrokes
        for (const auto& keystroke : bufferToCorrect) {
            ReplayKeystroke(keystroke);
        }
        
        _lastCorrectedBuffer = bufferToCorrect;
        _keystrokeBuffer.clear();
    }
    catch (...) {
        // Handle any errors
    }
    
    _isProcessingCorrection = false;
}

void KeyboardInterceptor::SwitchKeyboardLayout() {
    HWND hWnd = GetForegroundWindow();
    if (hWnd) {
        PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0x02, 0);
    }
}

void KeyboardInterceptor::ReplayKeystroke(const KeystrokeInfo& keystroke) {
    INPUT inputs[4] = {}; // Max: shift down, key down, key up, shift up
    int inputCount = 0;
    
    // Press shift if needed
    if (keystroke.shift) {
        inputs[inputCount].type = INPUT_KEYBOARD;
        inputs[inputCount].ki.wVk = VK_SHIFT;
        inputs[inputCount].ki.dwFlags = 0;
        inputCount++;
    }
    
    // Press key
    inputs[inputCount].type = INPUT_KEYBOARD;
    inputs[inputCount].ki.wVk = static_cast<WORD>(keystroke.virtualKey);
    inputs[inputCount].ki.dwFlags = 0;
    inputCount++;
    
    // Release key
    inputs[inputCount].type = INPUT_KEYBOARD;
    inputs[inputCount].ki.wVk = static_cast<WORD>(keystroke.virtualKey);
    inputs[inputCount].ki.dwFlags = KEYEVENTF_KEYUP;
    inputCount++;
    
    // Release shift if needed
    if (keystroke.shift) {
        inputs[inputCount].type = INPUT_KEYBOARD;
        inputs[inputCount].ki.wVk = VK_SHIFT;
        inputs[inputCount].ki.dwFlags = KEYEVENTF_KEYUP;
        inputCount++;
    }
    
    SendInput(inputCount, inputs, sizeof(INPUT));
}

bool KeyboardInterceptor::IsCharacterKey(int vkCode) const {
    return (vkCode >= 'A' && vkCode <= 'Z') ||
           (vkCode >= '0' && vkCode <= '9') ||
           (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) ||
           (vkCode >= VK_OEM_1 && vkCode <= VK_OEM_3) ||
           (vkCode >= VK_OEM_4 && vkCode <= VK_OEM_8) ||
           vkCode == VK_SPACE;
}

void KeyboardInterceptor::ClearBuffer() {
    _keystrokeBuffer.clear();
    _lastCorrectedBuffer.clear();
    _correctionCount = 0;
    _lastCharWasSpace = false;
}