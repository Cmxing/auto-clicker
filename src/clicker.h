#pragma once
#include <windows.h>
#include <cstdint>

// ============================================================
// 点击引擎 — 核心鼠标点击模拟
// ============================================================

namespace Clicker {

// 点击类型
enum class ClickType {
    LeftSingle,     // 左键单击
    LeftDouble,     // 左键双击
    RightSingle,    // 右键单击
    MiddleSingle    // 中键单击
};

// 初始化
void Init();

// 执行一次模拟点击 (在当前位置)
// type: 点击类型
// 返回实际耗时
int PerformClick(ClickType type);

// 移动到指定位置并点击
int ClickAt(POINT target, ClickType type);

// 获取/设置点击计数
uint64_t GetClickCount();
void ResetClickCount();

} // namespace Clicker
