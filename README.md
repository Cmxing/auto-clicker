# PC Auto Clicker

轻量级 Windows 自动连点器，C++17 / Win32 API 原生实现，具备进阶级反检测能力。

> 编译后单文件仅 **30KB**，无任何运行时依赖。

---

## 功能特性

### 点击模拟
- **SendInput API 底层注入** — 使用用户态最底层的鼠标事件模拟
- 支持左键单击 / 左键双击 / 右键单击 / 中键单击
- 可调 CPS（每秒点击次数）范围，每次间隔随机化

### 反检测能力
| 维度 | 实现 |
|------|------|
| 点击间隔 | 截断正态分布随机化，避免机械式等间隔 |
| 按压时长 | 随机 30~120ms，模拟人类手指按压行程 |
| 点击位置 | 目标坐标 ±3px 微抖动 |
| 鼠标轨迹 | 二次贝塞尔曲线 + ease-out 缓动，路径自然弯曲 |
| 事件指纹 | 每次 SendInput 的 extraInfo 随机化 |
| 窗口感知 | 检测到调试/反作弊工具窗口时自动暂停 |

### 使用方式
- **热键切换** — 按 F6 开始/停止连点（可自定义组合键）
- **定时模式** — 设定目标时间自动开始，可设持续时长（0=无限）
- **紧急退出** — Ctrl+Shift+F12 立即终止

---

## 界面截图

```
┌──────────────────────────────────┐
│  PC Auto Clicker        [─][□][×]│
├──────────────────────────────────┤
│  Click Speed (CPS):              │
│  [8] ~ [15]          Cur: ~12/s  │
│  Click Type: [Left Click    ▼]   │
│  Toggle Hotkey: [F6]    [Set]    │
│  ┌─ Timer Mode ──────────────┐   │
│  │ ☐ Enable Timer            │   │
│  │ Target [12]:[00]  Dur [0]  │   │
│  │ min (0=infinite)          │   │
│  └───────────────────────────┘   │
│  [ ● Start (F6) ]  [ ○ Stop ]   │
│  Status: ● Running | Clicks: 234 │
│  Anti-Detect: SendInput | Bezier │
└──────────────────────────────────┘
```

---

## 快速开始

### 下载运行

从 [Releases](../../releases) 下载 `auto-clicker.exe`，双击运行即可，无需安装任何依赖。

### 从源码编译

**前置要求：**
- Visual Studio 2022 Build Tools（或完整 VS 2022）
- CMake 3.10+

**编译步骤：**

```bash
# 配置 (使用 NMake)
cmake -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build

# 输出: build/auto-clicker.exe (~30KB)
```

或直接运行项目根目录下的 `build.bat` 一键编译。

---

## 项目结构

```
auto-clicker/
├── CMakeLists.txt              # CMake 构建配置
├── build.bat                   # 一键编译脚本
└── src/
    ├── main.cpp                # WinMain 入口 + GUI 对话框逻辑
    ├── resource.h               # 控件 ID 定义
    ├── resource.rc              # 对话框模板
    ├── clicker.h / .cpp         # 点击引擎 (SendInput 封装)
    ├── anti_detect.h / .cpp     # 反检测模块 (贝塞尔/正态分布/窗口感知)
    ├── hotkey.h / .cpp          # 全局热键管理 (RegisterHotKey)
    └── scheduler.h / .cpp       # 定时调度器 (Windows Timer)
```

---

## 技术栈

| 项目 | 选择 | 说明 |
|------|------|------|
| 语言 | C++17 | 零开销抽象 |
| GUI | 原生 Win32 Dialog | 无第三方 UI 框架 |
| 输入模拟 | `SendInput` API | 用户态最底层标准注入 |
| 热键 | `RegisterHotKey` | 系统级全局热键 |
| 随机数 | `std::mt19937` (梅森旋转) | 高质量伪随机数生成 |
| 静态链接 | `/MT` + `/LTCG` | 单文件 EXE，不依赖 VC Runtime DLL |

---

## 注意事项

- 本工具仅供合法用途（自动化测试、重复性操作辅助、无障碍辅助等）
- 请勿在违反游戏或平台服务条款的场景下使用
- 反检测功能旨在模拟正常人类操作行为，不能保证 100% 规避所有反作弊系统

---

## License

MIT
