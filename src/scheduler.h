#pragma once
#include <windows.h>
#include <cstdint>

// ============================================================
// 定时调度器 — 管理点击循环与定时启动
// ============================================================

namespace Scheduler {

// 初始化 (hwnd 用于接收 WM_TIMER)
bool Init(HWND hwnd);

// 设置点击参数: cps 范围
void SetCps(int minCps, int maxCps);

// 获取当前参数
int GetMinCps();
int GetMaxCps();

// 设置/获取点击类型
void SetClickType(int type);    // 0=左键单击, 1=左键双击, 2=右键单击, 3=中键单击
int GetClickType();

// 启动/停止点击
void Start();
void Stop();
bool IsRunning();

// 定时模式
void SetTimerMode(bool enabled, int hour, int minute, int durationMinutes);
bool IsTimerModeEnabled();
void GetTimerConfig(int& hour, int& minute, int& durationMinutes);

// 点击循环回调 (由主窗口 WM_TIMER 驱动)
void OnTick();

// 定时器消息分发 (由主窗口 WM_TIMER 调用)
void OnTimer(UINT_PTR timerId);

// 关闭
void Shutdown();

// 获取下次点击剩余时间 (ms)
int GetTimeUntilNextClick();

// 点击计数 (转发自 Clicker)
uint64_t GetTotalClicks();

} // namespace Scheduler
