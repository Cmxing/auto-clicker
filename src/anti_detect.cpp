#include "anti_detect.h"
#include <cmath>
#include <algorithm>
#include <chrono>

namespace AntiDetect {

// ---- 随机数引擎 (线程安全) ----
static std::mt19937 s_rng;
static bool s_initialized = false;

void Init() {
    if (s_initialized) return;
    // 使用高精度时钟 + 系统时间 + 进程 ID 混合种子
    auto now = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    uint64_t seed = static_cast<uint64_t>(ns) ^
                    static_cast<uint64_t>(GetTickCount64()) ^
                    static_cast<uint64_t>(GetCurrentProcessId()) ^
                    static_cast<uint64_t>(__rdtsc());
    s_rng.seed(static_cast<unsigned int>(seed & 0xFFFFFFFF));
    s_initialized = true;
}

// ---- 截断正态分布随机延迟 ----
// 使用 Box-Muller 变换 + 拒绝采样
int RandomDelay(double meanMs, double sigmaMs, int minMs, int maxMs) {
    std::normal_distribution<double> dist(0.0, 1.0);

    double val;
    do {
        // Box-Muller
        double u1 = std::uniform_real_distribution<double>(0.0, 1.0)(s_rng);
        double u2 = std::uniform_real_distribution<double>(0.0, 1.0)(s_rng);
        // 只用一个独立正态样本来简化
        double z = std::sqrt(-2.0 * std::log(std::max(u1, 1e-10))) *
                   std::cos(2.0 * 3.14159265358979323846 * u2);
        val = meanMs + z * sigmaMs;
    } while (val < minMs || val > maxMs); // 截断

    return static_cast<int>(val);
}

// ---- 均匀随机 ----
int RandomRange(int min, int max) {
    if (min >= max) return min;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(s_rng);
}

// ---- 贝塞尔曲线路径生成 ----
// 使用二次贝塞尔: B(t) = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2
std::vector<POINT> GenerateBezierPath(POINT start, POINT end, int controlOffset) {
    // 生成随机控制点 (偏移中点)
    int midX = (start.x + end.x) / 2;
    int midY = (start.y + end.y) / 2;
    int dx = end.x - start.x;
    int dy = end.y - start.y;

    // 垂直于移动方向的随机偏移
    double dist = std::sqrt(static_cast<double>(dx * dx + dy * dy));
    double perpX = 0, perpY = 0;
    if (dist > 1.0) {
        perpX = -dy / dist;  // 垂直方向
        perpY = dx / dist;
    }

    double offsetMag = RandomRange(-controlOffset, controlOffset);
    int ctrlX = midX + static_cast<int>(perpX * offsetMag);
    int ctrlY = midY + static_cast<int>(perpY * offsetMag);

    // 沿路径采样
    std::vector<POINT> path;
    int steps = std::max(10, static_cast<int>(dist / 3.0));
    steps = std::min(steps, 60); // 最多 60 步

    for (int i = 0; i <= steps; i++) {
        double t = static_cast<double>(i) / steps;
        // 应用 ease-out 缓动
        double et = EaseOutCubic(t);

        // 二次贝塞尔: B(et) = (1-et)^2*P0 + 2*(1-et)*et*P1 + et^2*P2
        double oneMinusT = 1.0 - et;
        double x = oneMinusT * oneMinusT * start.x +
                   2.0 * oneMinusT * et * ctrlX +
                   et * et * end.x;
        double y = oneMinusT * oneMinusT * start.y +
                   2.0 * oneMinusT * et * ctrlY +
                   et * et * end.y;

        POINT pt;
        pt.x = static_cast<int>(x);
        pt.y = static_cast<int>(y);
        path.push_back(pt);
    }
    return path;
}

// ---- 缓动函数: ease-out cubic ----
double EaseOutCubic(double t) {
    double t1 = t - 1.0;
    return t1 * t1 * t1 + 1.0;
}

// ---- 模拟人类鼠标移动 ----
int HumanMouseMove(POINT target) {
    POINT current;
    GetCursorPos(&current);

    // 如果已经很近，直接点击
    int dx = target.x - current.x;
    int dy = target.y - current.y;
    if (dx * dx + dy * dy < 4) {
        SetCursorPos(target.x, target.y);
        return RandomRange(5, 20);
    }

    // 生成贝塞尔路径
    double dist = std::sqrt(static_cast<double>(dx * dx + dy * dy));
    int controlOffset = static_cast<int>(dist * 0.35);
    auto path = GenerateBezierPath(current, target, controlOffset);

    // 沿路径移动，每步间隔随机变化
    int totalTime = 0;
    for (size_t i = 0; i < path.size(); i++) {
        SetCursorPos(path[i].x, path[i].y);

        // 移动速度变化: 中间快，两端慢
        double progress = static_cast<double>(i) / path.size();
        double speedFactor;
        if (progress < 0.15) {
            speedFactor = 2.5; // 开始阶段: 加速
        } else if (progress > 0.85) {
            speedFactor = 2.0; // 接近目标: 减速
        } else {
            speedFactor = 0.8; // 中间: 最快
        }

        int stepDelay = RandomRange(
            static_cast<int>(2 * speedFactor),
            static_cast<int>(6 * speedFactor)
        );
        Sleep(stepDelay);
        totalTime += stepDelay;
    }

    // 到达后微停顿 (模拟人类瞄准)
    int settleDelay = RandomRange(10, 50);
    Sleep(settleDelay);
    totalTime += settleDelay;

    return totalTime;
}

// ---- 前景窗口安全检查 ----
// 检测常见的反作弊/调试工具窗口
bool IsForegroundWindowSafe() {
    HWND fg = GetForegroundWindow();
    if (!fg) return true;

    wchar_t title[256];
    GetWindowTextW(fg, title, 256);

    // 检测已知调试/反作弊工具窗口标题 (可根据需要扩展)
    const wchar_t* unsafeKeywords[] = {
        L"Cheat Engine",
        L"x64dbg",
        L"x32dbg",
        L"OllyDbg",
        L"IDA",
        L"Process Hacker",
        L"Process Monitor",
        L"Wireshark",
        L"dnSpy",
        L"Sysinternals",
    };

    for (const auto* kw : unsafeKeywords) {
        if (wcsstr(title, kw) != nullptr) {
            return false;
        }
    }

    return true;
}

// ---- 位置抖动 ----
int PositionJitter(int range) {
    return RandomRange(-range, range);
}

} // namespace AntiDetect
