#include "clicker.h"
#include "anti_detect.h"

namespace Clicker {

static uint64_t s_clickCount = 0;

void Init() {
    AntiDetect::Init();
    s_clickCount = 0;
}

// ---- 内部: 发送单个鼠标事件 ----
static void SendMouseInput(DWORD flags, DWORD extraData = 0) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;
    input.mi.dwExtraInfo = extraData;
    input.mi.time = 0;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = 0;

    // 有一定概率在 SendInput 之间插入极短延迟
    if (AntiDetect::RandomRange(1, 100) <= 15) {
        Sleep(AntiDetect::RandomRange(1, 5));
    }

    SendInput(1, &input, sizeof(INPUT));
}

// ---- 执行一次点击 ----
int PerformClick(ClickType type) {
    int totalDelay = 0;

    // 模拟按压前微停顿
    int preDelay = AntiDetect::RandomRange(5, 25);
    Sleep(preDelay);
    totalDelay += preDelay;

    // 随机化 extraInfo (增加识别难度)
    DWORD extra = static_cast<DWORD>(
        AntiDetect::RandomRange(0x10000, 0x7FFFFFFF));

    switch (type) {
    case ClickType::LeftSingle: {
        // 模拟人类按压时长: 30~120ms
        int pressDuration = AntiDetect::RandomDelay(60.0, 20.0, 30, 120);

        // 按下
        SendMouseInput(MOUSEEVENTF_LEFTDOWN, extra);

        // 按压保持
        Sleep(pressDuration);
        totalDelay += pressDuration;

        // 释放
        SendMouseInput(MOUSEEVENTF_LEFTUP, extra);
        break;
    }

    case ClickType::LeftDouble: {
        // 双击: 两次点击之间有随机间隔
        for (int i = 0; i < 2; i++) {
            int pressDuration = AntiDetect::RandomDelay(55.0, 15.0, 25, 100);
            SendMouseInput(MOUSEEVENTF_LEFTDOWN, extra);

            if (i == 0 && AntiDetect::RandomRange(0, 1)) {
                // 第一次按压后有时会有微小抖动
                int jitterMs = AntiDetect::RandomRange(3, 12);
                Sleep(jitterMs);
                totalDelay += jitterMs;
            }

            Sleep(pressDuration);
            totalDelay += pressDuration;

            SendMouseInput(MOUSEEVENTF_LEFTUP, extra);

            // 双击间隔 (人类通常 80~250ms)
            if (i == 0) {
                int doubleDelay = AntiDetect::RandomDelay(140.0, 40.0, 80, 250);
                Sleep(doubleDelay);
                totalDelay += doubleDelay;
            }
        }
        break;
    }

    case ClickType::RightSingle: {
        int pressDuration = AntiDetect::RandomDelay(65.0, 22.0, 35, 130);
        SendMouseInput(MOUSEEVENTF_RIGHTDOWN, extra);
        Sleep(pressDuration);
        totalDelay += pressDuration;
        SendMouseInput(MOUSEEVENTF_RIGHTUP, extra);
        break;
    }

    case ClickType::MiddleSingle: {
        int pressDuration = AntiDetect::RandomDelay(55.0, 18.0, 28, 110);
        SendMouseInput(MOUSEEVENTF_MIDDLEDOWN, extra);
        Sleep(pressDuration);
        totalDelay += pressDuration;
        SendMouseInput(MOUSEEVENTF_MIDDLEUP, extra);
        break;
    }
    }

    // 模拟点击后微停顿
    int postDelay = AntiDetect::RandomRange(8, 35);
    Sleep(postDelay);
    totalDelay += postDelay;

    s_clickCount++;
    return totalDelay;
}

// ---- 移动到目标位置并点击 ----
int ClickAt(POINT target, ClickType type) {
    // 先做位置抖动
    target.x += AntiDetect::PositionJitter(3);
    target.y += AntiDetect::PositionJitter(3);

    // 限制在屏幕内
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    target.x = (std::max)(1L, (std::min)(static_cast<LONG>(target.x), static_cast<LONG>(screenW - 1)));
    target.y = (std::max)(1L, (std::min)(static_cast<LONG>(target.y), static_cast<LONG>(screenH - 1)));

    // 模拟人类鼠标移动
    int moveTime = AntiDetect::HumanMouseMove(target);

    // 执行点击
    int clickTime = PerformClick(type);

    return moveTime + clickTime;
}

uint64_t GetClickCount() {
    return s_clickCount;
}

void ResetClickCount() {
    s_clickCount = 0;
}

} // namespace Clicker
