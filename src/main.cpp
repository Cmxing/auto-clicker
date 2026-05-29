// ============================================================
// PC Auto Clicker — 轻量级连点器
// C++17 / Win32 API / SendInput / 贝塞尔轨迹反检测
// ============================================================

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <cstdio>
#include "resource.h"
#include "clicker.h"
#include "anti_detect.h"
#include "hotkey.h"
#include "scheduler.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// ---- 全局状态 ----
static HINSTANCE s_hInst = nullptr;
static HWND      s_hDlg  = nullptr;
static bool      s_timerUISuppress = false; // 抑制 UI 更新触发的控件事件

// ---- 辅助函数 ----

// 从 Edit 控件读取整数
static int GetEditInt(HWND hDlg, int ctrlId, int defaultVal = 0) {
    BOOL ok;
    int val = static_cast<int>(GetDlgItemInt(hDlg, ctrlId, &ok, FALSE));
    return ok ? val : defaultVal;
}

// 将整数写入 Edit 控件
static void SetEditInt(HWND hDlg, int ctrlId, int val) {
    SetDlgItemInt(hDlg, ctrlId, val, FALSE);
}

// 更新状态栏显示
static void UpdateStatus(HWND hDlg) {
    wchar_t buf[256];
    if (Scheduler::IsRunning()) {
        swprintf_s(buf, L"状态: ● 运行中  |  已点击 %llu 次  |  下次点击 ~%dms",
            Clicker::GetClickCount(),
            Scheduler::GetTimeUntilNextClick());
    } else {
        swprintf_s(buf, L"状态: ○ 已停止  |  累计点击 %llu 次",
            Clicker::GetClickCount());
    }
    SetDlgItemTextW(hDlg, IDC_STATUS, buf);

    // 更新当前 CPS 显示
    if (Scheduler::IsRunning()) {
        wchar_t cpsBuf[64];
        int avgCps = (Scheduler::GetMinCps() + Scheduler::GetMaxCps()) / 2;
        swprintf_s(cpsBuf, L"当前: ~%d/s", avgCps);
        SetDlgItemTextW(hDlg, IDC_CPS_CURRENT, cpsBuf);
    } else {
        SetDlgItemTextW(hDlg, IDC_CPS_CURRENT, L"当前: --");
    }
}

// 设置控件启用状态
static void UpdateControlState(HWND hDlg) {
    bool running = Scheduler::IsRunning();
    bool timerEnabled = (IsDlgButtonChecked(hDlg, IDC_ENABLE_TIMER) == BST_CHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_CPS_MIN), !running);
    EnableWindow(GetDlgItem(hDlg, IDC_CPS_MAX), !running);
    EnableWindow(GetDlgItem(hDlg, IDC_CLICK_TYPE), !running);
    EnableWindow(GetDlgItem(hDlg, IDC_HOTKEY_SET), !running);
    EnableWindow(GetDlgItem(hDlg, IDC_ENABLE_TIMER), !running);
    EnableWindow(GetDlgItem(hDlg, IDC_TIMER_HOUR), !running && timerEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_TIMER_MINUTE), !running && timerEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_TIMER_DURATION), !running && timerEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_START), !running);
    EnableWindow(GetDlgItem(hDlg, IDC_BTN_STOP), running);
}

// 从 UI 读取参数并启动
static void DoStart(HWND hDlg) {
    // 读取 CPS 设置
    int minCps = GetEditInt(hDlg, IDC_CPS_MIN, 8);
    int maxCps = GetEditInt(hDlg, IDC_CPS_MAX, 15);
    if (minCps < 1) minCps = 1;
    if (maxCps < minCps) maxCps = minCps;
    if (maxCps > 1000) maxCps = 1000;

    Scheduler::SetCps(minCps, maxCps);

    // 读取点击类型
    int clickType = static_cast<int>(
        SendDlgItemMessageW(hDlg, IDC_CLICK_TYPE, CB_GETCURSEL, 0, 0));
    if (clickType < 0) clickType = 0;
    Scheduler::SetClickType(clickType);

    // 定时模式
    if (IsDlgButtonChecked(hDlg, IDC_ENABLE_TIMER) == BST_CHECKED) {
        int hour = GetEditInt(hDlg, IDC_TIMER_HOUR, 0);
        int minute = GetEditInt(hDlg, IDC_TIMER_MINUTE, 0);
        int duration = GetEditInt(hDlg, IDC_TIMER_DURATION, 0);
        Scheduler::SetTimerMode(true, hour, minute, duration);
    } else {
        Scheduler::SetTimerMode(false, 0, 0, 0);
    }

    Scheduler::Start();
    UpdateControlState(hDlg);
}

static void DoStop(HWND hDlg) {
    Scheduler::Stop();
    UpdateControlState(hDlg);
}

// ---- 热键捕获 ----
// 使用 WH_KEYBOARD_LL 低级钩子捕获组合键
static HHOOK s_llHook = nullptr;

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        // 忽略单独的修饰键
        UINT vk = kb->vkCode;
        if (vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT ||
            vk == VK_LWIN || vk == VK_RWIN || vk == VK_CAPITAL) {
            return CallNextHookEx(s_llHook, nCode, wParam, lParam);
        }

        // 获取修饰键状态
        UINT mods = 0;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mods |= MOD_CONTROL;
        if (GetAsyncKeyState(VK_MENU)    & 0x8000) mods |= MOD_ALT;
        if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) mods |= MOD_SHIFT;
        if (GetAsyncKeyState(VK_LWIN)    & 0x8000) mods |= MOD_WIN;
        if (GetAsyncKeyState(VK_RWIN)    & 0x8000) mods |= MOD_WIN;

        // 通知主窗口
        PostMessageW(s_hDlg, WM_APP, vk, mods);

        // 立即取消钩子
        if (s_llHook) {
            UnhookWindowsHookEx(s_llHook);
            s_llHook = nullptr;
        }
        return 1; // 吞掉这条消息
    }
    return CallNextHookEx(s_llHook, nCode, wParam, lParam);
}

static void BeginHotkeyCapture() {
    s_llHook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        s_hInst,
        0
    );
    if (s_llHook) {
        Hotkey::StartCapture();
        SetDlgItemTextW(s_hDlg, IDC_HOTKEY_SET, L"等待按键...");
        EnableWindow(GetDlgItem(s_hDlg, IDC_HOTKEY_SET), FALSE);
    }
}

static void EndHotkeyCapture() {
    if (s_llHook) {
        UnhookWindowsHookEx(s_llHook);
        s_llHook = nullptr;
    }
    Hotkey::StopCapture();
    SetDlgItemTextW(s_hDlg, IDC_HOTKEY_SET, L"修改");
    SetDlgItemTextW(s_hDlg, IDC_HOTKEY, Hotkey::GetHotkeyText());
    EnableWindow(GetDlgItem(s_hDlg, IDC_HOTKEY_SET), TRUE);
}

// ---- 对话框消息处理 ----
static INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_INITDIALOG: {
        s_hDlg = hDlg;

        // ---- 初始化 CPS 编辑框 ----
        SetEditInt(hDlg, IDC_CPS_MIN, 8);
        SetEditInt(hDlg, IDC_CPS_MAX, 15);

        // ---- 创建 Spin 控件 (动态创建) ----
        HWND hSpinMin = CreateWindowExW(
            0, L"msctls_updown32", nullptr,
            WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_AUTOBUDDY |
            UDS_ARROWKEYS | UDS_ALIGNRIGHT,
            0, 0, 0, 0,
            hDlg, (HMENU)(UINT_PTR)IDC_CPS_MIN_SPIN, s_hInst, nullptr);
        SendMessageW(hSpinMin, UDM_SETRANGE32, 1, 1000);
        SendMessageW(hSpinMin, UDM_SETBUDDY,
            (WPARAM)GetDlgItem(hDlg, IDC_CPS_MIN), 0);

        HWND hSpinMax = CreateWindowExW(
            0, L"msctls_updown32", nullptr,
            WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_AUTOBUDDY |
            UDS_ARROWKEYS | UDS_ALIGNRIGHT,
            0, 0, 0, 0,
            hDlg, (HMENU)(UINT_PTR)IDC_CPS_MAX_SPIN, s_hInst, nullptr);
        SendMessageW(hSpinMax, UDM_SETRANGE32, 1, 1000);
        SendMessageW(hSpinMax, UDM_SETBUDDY,
            (WPARAM)GetDlgItem(hDlg, IDC_CPS_MAX), 0);

        // ---- 填充点击类型下拉框 ----
        HWND cb = GetDlgItem(hDlg, IDC_CLICK_TYPE);
        const wchar_t* clickTypes[] = {
            L"左键单击", L"左键双击", L"右键单击", L"中键单击"
        };
        for (int i = 0; i < 4; i++) {
            SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)clickTypes[i]);
        }
        SendMessageW(cb, CB_SETCURSEL, 0, 0); // 默认左键单击

        // ---- 热键显示 ----
        SetDlgItemTextW(hDlg, IDC_HOTKEY, Hotkey::GetHotkeyText());

        // ---- 定时默认值 ----
        SetEditInt(hDlg, IDC_TIMER_HOUR, 12);
        SetEditInt(hDlg, IDC_TIMER_MINUTE, 0);
        SetEditInt(hDlg, IDC_TIMER_DURATION, 0);

        // ---- 初始化状态 ----
        UpdateControlState(hDlg);
        UpdateStatus(hDlg);

        // ---- 初始化热键 ----
        if (!Hotkey::Init(hDlg)) {
            MessageBoxW(hDlg,
                L"Hotkey registration failed!\n"
                L"F6 may be in use by another app.\n"
                L"Click [Set] to change the hotkey.",
                L"Warning", MB_ICONWARNING);
        }

        // ---- 初始化调度器 ----
        Scheduler::Init(hDlg);

        return TRUE;
    }

    case WM_APP: {
        // 接收到捕获的热键 (wParam=VK, lParam=modifiers)
        UINT vk  = static_cast<UINT>(wParam);
        UINT mod = static_cast<UINT>(lParam);
        Hotkey::OnKeyCaptured(vk, mod);
        EndHotkeyCapture();
        return TRUE;
    }

    case WM_HOTKEY: {
        switch (wParam) {
        case Hotkey::HK_TOGGLE:
            // F6: 切换开始/停止
            if (Scheduler::IsRunning()) {
                DoStop(hDlg);
            } else {
                DoStart(hDlg);
            }
            UpdateStatus(hDlg);
            break;

        case Hotkey::HK_EXIT:
            // Ctrl+Shift+F12: 紧急退出
            Scheduler::Stop();
            Hotkey::Shutdown();
            EndDialog(hDlg, 0);
            break;
        }
        return TRUE;
    }

    case WM_TIMER: {
        // 转发到调度器
        Scheduler::OnTimer(static_cast<UINT_PTR>(wParam));
        UpdateStatus(hDlg);
        return TRUE;
    }

    case WM_COMMAND: {
        WORD ctrlId = LOWORD(wParam);
        WORD notify = HIWORD(wParam);

        switch (ctrlId) {

        case IDC_BTN_START:
            DoStart(hDlg);
            UpdateStatus(hDlg);
            break;

        case IDC_BTN_STOP:
            DoStop(hDlg);
            UpdateStatus(hDlg);
            break;

        case IDC_HOTKEY_SET:
            // 开始热键捕获
            if (!Hotkey::IsCapturing()) {
                BeginHotkeyCapture();
            }
            break;

        case IDC_ENABLE_TIMER:
            // 定时模式勾选切换
            UpdateControlState(hDlg);
            break;

        case IDC_CPS_MIN:
        case IDC_CPS_MAX:
            if (notify == EN_CHANGE && !s_timerUISuppress) {
                // 确保 min <= max
                int minCps = GetEditInt(hDlg, IDC_CPS_MIN, 1);
                int maxCps = GetEditInt(hDlg, IDC_CPS_MAX, 1);
                if (minCps > maxCps) {
                    s_timerUISuppress = true;
                    SetEditInt(hDlg, IDC_CPS_MAX, minCps);
                    s_timerUISuppress = false;
                }
            }
            break;
        }
        return TRUE;
    }

    case WM_CLOSE:
        Scheduler::Stop();
        Hotkey::Shutdown();
        EndHotkeyCapture();
        Scheduler::Shutdown();
        EndDialog(hDlg, 0);
        return TRUE;

    case WM_DESTROY:
        return TRUE;
    }

    return FALSE;
}

// ---- 主入口 ----
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // 初始化公共控件 (Spin, ComboBox 视觉样式)
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    s_hInst = hInstance;

    // 初始化各模块
    AntiDetect::Init();
    Clicker::Init();

    // 创建模态对话框 (DialogBox 自带消息循环)
    INT_PTR result = DialogBoxParamW(
        s_hInst,
        MAKEINTRESOURCEW(IDD_MAIN),
        nullptr,
        MainDlgProc,
        0
    );

    if (result == -1) {
        DWORD err = GetLastError();
        wchar_t buf[512];
        swprintf_s(buf, L"DialogBox failed. Error code: %lu (0x%lX)\n"
            L"IDD_MAIN=%d, hInstance=%p",
            err, err, IDD_MAIN, (void*)s_hInst);
        MessageBoxW(nullptr, buf, L"Error", MB_ICONERROR);
        return 1;
    }

    Scheduler::Shutdown();
    Hotkey::Shutdown();
    EndHotkeyCapture();

    return static_cast<int>(result);
}
