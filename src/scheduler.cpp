#include "scheduler.h"
#include "clicker.h"
#include "anti_detect.h"

namespace Scheduler {

static HWND   s_hwnd            = nullptr;
static bool   s_running         = false;
static int    s_minCps          = 8;
static int    s_maxCps          = 15;
static int    s_clickType       = 0;  // 0=左键单击
static int    s_nextDelayMs     = 0;
static bool   s_timerEnabled    = false;
static int    s_timerHour       = 0;
static int    s_timerMinute     = 0;
static int    s_timerDuration   = 0;  // 分钟, 0=无限
static DWORD  s_timerStartTick  = 0;

// 定时检查使用的 timer ID
static constexpr UINT_PTR TIMER_CLICK   = 1;
static constexpr UINT_PTR TIMER_CHECK   = 2;  // 每秒检查定时启动
static constexpr UINT_PTR TIMER_UPDATE  = 3;  // GUI 更新

bool Init(HWND hwnd) {
    s_hwnd = hwnd;

    // 启动定时检查 (每秒)
    SetTimer(s_hwnd, TIMER_CHECK, 1000, nullptr);

    // 启动 GUI 更新定时器 (每 200ms)
    SetTimer(s_hwnd, TIMER_UPDATE, 200, nullptr);

    return true;
}

void SetCps(int minCps, int maxCps) {
    s_minCps = std::max(1, minCps);
    s_maxCps = std::max(s_minCps, maxCps);
}

int GetMinCps() { return s_minCps; }
int GetMaxCps() { return s_maxCps; }

void SetClickType(int type) {
    s_clickType = std::max(0, std::min(type, 3));
}

int GetClickType() { return s_clickType; }

void Start() {
    if (s_running) return;

    s_running = true;
    s_timerStartTick = GetTickCount();

    // 立即安排第一次点击
    s_nextDelayMs = AntiDetect::RandomDelay(
        1000.0 / ((s_minCps + s_maxCps) / 2.0),
        1000.0 / ((s_minCps + s_maxCps) / 2.0) * 0.25,
        std::max(10, 1000 / s_maxCps),
        std::min(500,  1000 / s_minCps)
    );
    SetTimer(s_hwnd, TIMER_CLICK, s_nextDelayMs, nullptr);
}

void Stop() {
    s_running = false;
    KillTimer(s_hwnd, TIMER_CLICK);
}

bool IsRunning() { return s_running; }

void SetTimerMode(bool enabled, int hour, int minute, int durationMinutes) {
    s_timerEnabled   = enabled;
    s_timerHour      = hour;
    s_timerMinute    = minute;
    s_timerDuration  = durationMinutes;
}

bool IsTimerModeEnabled() { return s_timerEnabled; }
void GetTimerConfig(int& hour, int& minute, int& durationMinutes) {
    hour = s_timerHour;
    minute = s_timerMinute;
    durationMinutes = s_timerDuration;
}

// 检查是否到了定时启动的时间
static bool ShouldAutoStart() {
    if (!s_timerEnabled) return false;

    SYSTEMTIME st;
    GetLocalTime(&st);
    int nowMinutes = st.wHour * 60 + st.wMinute;
    int targetMinutes = s_timerHour * 60 + s_timerMinute;

    // 允许 1 分钟的误差窗口
    return (nowMinutes >= targetMinutes && nowMinutes < targetMinutes + 1);
}

// 检查是否已经超时
static bool HasTimedOut() {
    if (!s_timerEnabled || s_timerDuration <= 0) return false;

    DWORD elapsed = (GetTickCount() - s_timerStartTick) / 60000; // 转分钟
    return elapsed >= static_cast<DWORD>(s_timerDuration);
}

void OnTick() {
    if (!s_running) return;

    // 安全检查
    if (!AntiDetect::IsForegroundWindowSafe()) {
        // 不安全窗口，暂停 (不停止，保留状态)
        return;
    }

    // 定时模式超时检查
    if (HasTimedOut()) {
        Stop();
        PostMessageW(s_hwnd, WM_COMMAND, MAKEWPARAM(0, 0x8001), 0); // 自定义通知
        return;
    }

    // 在当前位置执行点击
    POINT pt;
    GetCursorPos(&pt);

    Clicker::ClickType ct;
    switch (s_clickType) {
        case 1: ct = Clicker::ClickType::LeftDouble;   break;
        case 2: ct = Clicker::ClickType::RightSingle;  break;
        case 3: ct = Clicker::ClickType::MiddleSingle; break;
        default: ct = Clicker::ClickType::LeftSingle;  break;
    }

    Clicker::PerformClick(ct);

    // 计算下一次点击间隔
    double avgCps = (s_minCps + s_maxCps) / 2.0;
    double avgInterval = 1000.0 / avgCps;
    double sigma = avgInterval * 0.25;

    int minInterval = std::max(10, 1000 / s_maxCps);
    int maxInterval = std::min(500, 1000 / s_minCps);

    s_nextDelayMs = AntiDetect::RandomDelay(
        avgInterval, sigma, minInterval, maxInterval
    );

    // 重新设置定时器
    SetTimer(s_hwnd, TIMER_CLICK, s_nextDelayMs, nullptr);
}

void Shutdown() {
    Stop();
    KillTimer(s_hwnd, TIMER_CHECK);
    KillTimer(s_hwnd, TIMER_UPDATE);
}

int GetTimeUntilNextClick() {
    return s_nextDelayMs;
}

uint64_t GetTotalClicks() {
    return Clicker::GetClickCount();
}

// ---- 外部: 定时器消息分发 ----
// 由主窗口 WM_TIMER 调用
void OnTimer(UINT_PTR timerId) {
    switch (timerId) {
    case TIMER_CLICK:
        OnTick();
        break;

    case TIMER_CHECK:
        // 定时模式检查
        if (s_timerEnabled && !s_running && ShouldAutoStart()) {
            Start();
        }
        break;

    case TIMER_UPDATE:
        // 触发 GUI 刷新 (通知主窗口更新状态文本)
        InvalidateRect(s_hwnd, nullptr, FALSE);
        break;
    }
}

} // namespace Scheduler
