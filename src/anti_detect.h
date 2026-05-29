#pragma once
#include <windows.h>
#include <random>
#include <chrono>
#include <vector>

// ============================================================
// 反检测模块 — 让点击行为模拟真实人类
// ============================================================

namespace AntiDetect {

// 初始化随机数生成器
void Init();

// 生成符合截断正态分布的随机延迟 (毫秒)
// mean: 平均延迟, sigma: 标准差, minVal/maxVal: 截断边界
int RandomDelay(double meanMs, double sigmaMs, int minMs, int maxMs);

// 生成范围内的均匀随机整数 [min, max]
int RandomRange(int min, int max);

// 生成贝塞尔曲线路径点 (从 start 到 end)
// controlOffset: 中间控制点偏移幅度
// 返回: 路径上的采样点列表
std::vector<POINT> GenerateBezierPath(POINT start, POINT end, int controlOffset);

// 获取缓动函数值 (ease-out cubic: 开始快、结束慢)
double EaseOutCubic(double t);

// 模拟人类鼠标移动: 沿着贝塞尔曲线移动鼠标到目标位置
// 返回移动耗时(ms)
int HumanMouseMove(POINT target);

// 检查当前前景窗口是否安全 (避免被反作弊检测窗口捕获)
bool IsForegroundWindowSafe();

// 生成随机微偏移 (点击位置抖动)
int PositionJitter(int range);

} // namespace AntiDetect
