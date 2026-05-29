#include "hotkey.h"
#include <string>
#include <cstdio>

namespace Hotkey {

static HWND  s_hwnd          = nullptr;
static UINT  s_toggleVk      = VK_F6;
static UINT  s_toggleMod     = 0;
static bool  s_capturing     = false;
static wchar_t s_hotkeyText[32] = L"F6";

bool Init(HWND hwnd) {
    s_hwnd = hwnd;

    // 注册默认热键
    if (!RegisterHotKey(s_hwnd, HK_TOGGLE, s_toggleMod, s_toggleVk)) {
        return false;
    }
    RegisterHotKey(s_hwnd, HK_EXIT, MOD_CONTROL | MOD_SHIFT, VK_F12);

    return true;
}

bool UpdateToggleKey(UINT vk, UINT modifiers) {
    // 注销旧热键
    UnregisterHotKey(s_hwnd, HK_TOGGLE);

    s_toggleVk = vk;
    s_toggleMod = modifiers;

    // 注册新热键
    if (!RegisterHotKey(s_hwnd, HK_TOGGLE, s_toggleMod, s_toggleVk)) {
        // 回退到默认
        s_toggleVk = VK_F6;
        s_toggleMod = 0;
        RegisterHotKey(s_hwnd, HK_TOGGLE, 0, VK_F6);
        return false;
    }

    // 生成显示文本
    std::wstring text;
    if (modifiers & MOD_CONTROL) text += L"Ctrl+";
    if (modifiers & MOD_ALT)     text += L"Alt+";
    if (modifiers & MOD_SHIFT)   text += L"Shift+";
    if (modifiers & MOD_WIN)     text += L"Win+";

    // 虚键码转名称
    wchar_t keyName[32] = {};
    UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    if (vk >= VK_F1 && vk <= VK_F24) {
        swprintf_s(keyName, L"F%d", vk - VK_F1 + 1);
    } else {
        GetKeyNameTextW(scanCode << 16, keyName, 32);
        if (keyName[0] == L'\0') {
            swprintf_s(keyName, L"VK_%u", vk);
        }
    }
    text += keyName;

    wcscpy_s(s_hotkeyText, 32, text.c_str());
    return true;
}

void Shutdown() {
    UnregisterHotKey(s_hwnd, HK_TOGGLE);
    UnregisterHotKey(s_hwnd, HK_EXIT);
}

const wchar_t* GetHotkeyText() {
    return s_hotkeyText;
}

UINT GetVkCode()    { return s_toggleVk; }
UINT GetModifiers() { return s_toggleMod; }

void StartCapture()       { s_capturing = true; }
bool IsCapturing()        { return s_capturing; }
void StopCapture()        { s_capturing = false; }

void OnKeyCaptured(UINT vk, UINT modifiers) {
    UpdateToggleKey(vk, modifiers);
    s_capturing = false;
}

} // namespace Hotkey
