cmake_minimum_required(VERSION 3.10)
project(AutoClicker LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 静态链接 CRT — 生成独立 EXE，无需运行时 DLL
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(auto-clicker
    src/main.cpp
    src/clicker.cpp
    src/anti_detect.cpp
    src/hotkey.cpp
    src/scheduler.cpp
    src/resource.rc
)

# MSVC: 强制 UTF-8 编码 (解决中文系统 GBK 编码问题)
target_compile_options(auto-clicker PRIVATE /utf-8)

# Release 优化: 体积优先
target_compile_options(auto-clicker PRIVATE
    $<$<CONFIG:Release>:/O2 /GL /GS- /Gy>
)
target_link_options(auto-clicker PRIVATE
    $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
)

target_link_libraries(auto-clicker user32 gdi32 comctl32)

# 启用 ComCtl32 v6 视觉样式
target_compile_definitions(auto-clicker PRIVATE
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
