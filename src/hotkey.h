#pragma once
#include <windows.h>

// ============================================================
// 热键管理器 — 全局热键注册与处理
// ============================================================

namespace Hotkey {

// 热键 ID
constexpr int HK_TOGGLE = 1;   // 开始/停止
constexpr int HK_EXIT   = 2;   // 紧急退出

// 初始化 (注册默认热键 F6/F7)
bool Init(HWND hwnd);

// 修改切换热键 (vk: 虚键码, modifiers: MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN)
bool UpdateToggleKey(UINT vk, UINT modifiers);

// 注销所有热键
void Shutdown();

// 获取当前热键显示文本
const wchar_t* GetHotkeyText();

// 获取当前热键数据
UINT GetVkCode();
UINT GetModifiers();

// 开始捕获下一次按键 (用于修改热键)
void StartCapture();
bool IsCapturing();
void StopCapture();
void OnKeyCaptured(UINT vk, UINT modifiers);

} // namespace Hotkey
