# 把 GTK 版 `gtktsm` 移植到 LVGL 的实施文档

> 本文档面向 AI / 工程师，描述如何把 `3rdparty/libtsm/src/gtktsm` 的 GTK 终端前端改写为 LVGL 版本，并保留底层 `tsm_screen` / `tsm_vte` / `shl_pty` 与 C++ 版 `te::Pty`。

## 0. 工程现状

- **C 语言 GTK 前端**：[`3rdparty/libtsm/src/gtktsm/`](3rdparty/libtsm/src/gtktsm/meson.build)（5 个文件，编译出 `gtktsm` 可执行文件）
- **C++ 版 PTY**：[`src/pty_process.{h,cpp}`](src/pty_process.h)（命名空间 `te::Pty`，已实现 `posix_openpt` + `fork` + `execve`）
- **LVGL v9.2+**：`3rdparty/lvgl`（含 `lv_display_create` / `lv_indev_create` / `lv_timer` / `lv_canvas` / `lv_font_ttf`）
- **底层终端核心**：`3rdparty/libtsm/src/tsm/{tsm-screen.c, tsm-vte.c, tsm-selection.c, tsm-unicode.c}`

## 1. GTK 版 `gtktsm` 三大分区（已梳理完成）

[`gtktsm-terminal.c`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c) 用 3 段大注释块清晰分区：

### 1.1 分区一：Glyph Renderer（行 47–509）

| 结构 | 职责 |
|---|---|
| [`struct gtktsm_font`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:60) | 包装 `PangoFontMap`，引用计数管理 |
| [`struct gtktsm_face`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:65) | 字体 face，含 cell 的 `width/height/baseline/line_thickness/underline_pos/strikethrough_pos`，持有 `shl_htable glyphs` 缓存 |
| [`struct gtktsm_glyph`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:88) | 渲染好的字形位图（A1/A8/XRGB32 三种格式） |
| [`init_pango`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:239) | 配置 Pango 字体描述、抗锯齿、subpixel 顺序 |
| [`measure_pango`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:190) | 渲染一串 ASCII 估算 cell 宽高（Pango 无 monospace 度量） |
| [`create_glyph`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:354) | Pango+Cairo 渲染单字形到 `cairo_image_surface` |
| [`gtktsm_face_render`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:456) | 以 `ch[0]` 为键做 glyph 缓存，未命中时调 `create_glyph` |

### 1.2 分区二：Cell Renderer（行 511–1079）

| 结构 / 函数 | 职责 |
|---|---|
| [`struct gtktsm_renderer`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:519) | ARGB32 影子帧缓冲 + 配套 `cairo_surface` |
| [`struct gtktsm_renderer_ctx`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:528) | 渲染上下文，聚合 `rend / screen / vte / 4 个 face / cell 尺寸 / debug 开关` |
| [`renderer_fill`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:634) | 纯色填充 |
| [`renderer_highlight`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:674) | debug 边框 |
| [`renderer_blend_a1/a8/xrgb32`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:717) | 三种 glyph 格式的整数混合（用 `+0x80` 移位技巧避免 /255 除法，提速 ~20%） |
| [`renderer_draw_cell`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:939) | `tsm_screen_draw` 的每格回调，负责选字体、处理 inverse/underline、调 `renderer_blend` |
| [`gtktsm_renderer_draw`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1043) | "CPU 端渲染到影子缓冲 → `cairo_set_source_surface` blit 到 GTK 绘图区" |

### 1.3 分区三：`GtkTsmTerminal` 部件（行 1081–2023）

- 私有结构 [`GtkTsmTerminalPrivate`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1091)
- 属性：`font / sb-size / anti-aliasing / subpixel-order / show-dirty / debug`（[`terminal_set_property`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1680) / [`terminal_get_property`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1613)）
- 信号：`terminal-changed / terminal-stopped / terminal-title-changed`（[`gtktsm_terminal_class_init`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1849)）
- 事件回调：
  - [`terminal_configure_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1249)：窗口 resize 时重算列/行、resize 渲染器与 screen 与 pty
  - [`terminal_draw_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1298)：调 `gtktsm_renderer_draw`
  - [`terminal_key_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1345)：`gdk_keymap_translate_keyboard_state` → `tsm_vte_handle_keyboard`
  - [`terminal_button_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1421) / [`terminal_motion_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1472)：鼠标选择
  - [`terminal_idle_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1509)：`shl_pty_bridge_dispatch_pty`
  - [`terminal_write_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1526)：`tsm_vte` 产生输出 → `shl_pty_write`
  - [`terminal_osc_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1584)：OSC 2 解析窗口标题
  - [`terminal_bridge_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1598)：`shl_pty_bridge_dispatch`
  - [`terminal_read_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1944)：`shl_pty` 读到数据 → `tsm_vte_input`
  - [`terminal_child_fn`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1956)：子进程退出 → 清理 pty → emit `terminal-stopped`
- [`gtktsm_terminal_fork`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1976)：`shl_pty_open` + `shl_pty_bridge_add` + `g_child_watch_add`，父进程返回非 0 pid，子进程返回 0
- [`gtktsm_terminal_kill`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:2011)：`shl_pty_signal(pty, sig)`

## 2. 三大分区 → LVGL 对应实现

### 2.1 分区一：Glyph Renderer

| 原 GTK 实现 | LVGL 需要实现 |
|---|---|
| `gtktsm_font` 包装 `PangoFontMap` | LVGL 没有 Pango，用 `lv_font_ttf`（或内置 `lv_font_montserrat_*`）；封装 `term_font` 结构 + 引用计数 |
| `gtktsm_face` + `measure_pango` | LVGL 没有"按 ASCII 串估算 cell 宽高"，自己写 `term_face`：从 `lv_font` 读 `line_height` 和 `adv_w` 算 cell 尺寸；bold/italic 需手动准备 TTF |
| `gtktsm_glyph` + `create_glyph`（Pango+Cairo 渲染单字形） | 用 `lv_canvas` 当单字渲染画布：`lv_canvas_init` → `lv_canvas_set_draw` 回调里 `lv_draw_label` 画单字符；用 `shl_htable`（[`3rdparty/libtsm/src/shared/shl-htable.h`](3rdparty/libtsm/src/shared/shl-htable.h)）做 glyph 缓存 |
| `renderer_blend_a1/a8/xrgb32` | LVGL 自带 `LV_COLOR_DEPTH=16/32` 像素格式，**不需要**自己写整数混合；`lv_canvas` 内部已做 alpha 混合 |

### 2.2 分区二：Cell Renderer

| 原 GTK 实现 | LVGL 需要实现 |
|---|---|
| `gtktsm_renderer` 影子缓冲 + `cairo_set_source_surface` blit | LVGL 自带"脏矩形 + `lv_refr`"机制（[`lv_refr.c`](3rdparty/lvgl/src/core/lv_refr.c)），**不需要**再写影子缓冲；用 `lv_canvas` 作为终端画布 |
| `renderer_draw_cell`（`tsm_screen_draw` 回调） | 保留为 `lv_canvas` 的 draw 回调：遍历 dirty cell，按 `tsm_screen_attr` 取前景/背景色，用 `lv_draw_label` 画字符 + `lv_draw_rect` 画下划线 |
| `gtktsm_renderer_draw` 的 padding 填充 | 用 `lv_obj_set_style_bg_color` 给父容器设背景色 |

### 2.3 分区三：`lv_term` 部件

| 原 GTK 实现 | LVGL 需要实现 |
|---|---|
| `GtkTsmTerminal` 继承 `GtkDrawingArea` | 写 `lv_term` widget，继承 `lv_obj`（参考 [`lv_obj.c`](3rdparty/lvgl/src/core/lv_obj.c) 模式），内部挂 `lv_canvas` 子对象 |
| GProperty（`font / sb-size / anti-aliasing / subpixel-order / show-dirty / debug`） | LVGL 没有 GProperty，用 `lv_obj_set_user_data` + 自定义 getter/setter；或直接结构体字段 |
| GSignal（`terminal-changed / stopped / title-changed`） | 用 `lv_event_send(obj, LV_EVENT_..., param)` |
| `terminal_configure_fn`（窗口 resize） | `LV_EVENT_RESIZED` 回调：`tsm_screen_resize` + `Pty::resize` + 重设 canvas 尺寸 |
| `terminal_key_fn`（`gdk_keymap` → `tsm_vte_handle_keyboard`） | `lv_indev`（[`lv_indev.c`](3rdparty/lvgl/src/indev/lv_indev.c)）轮询模型，自己写 keymap（可复用 xkbcommon keysym 表） |
| `terminal_button_fn` / `terminal_motion_fn`（鼠标选择） | `LV_EVENT_PRESS` / `LV_EVENT_PRESSING` / `LV_EVENT_RELEASE`；短按 `tsm_screen_selection_reset`，长按复制到剪贴板 |
| `terminal_bridge_fn`（`g_io_add_watch` 监听 pty bridge fd） | LVGL 没有 GLib IO channel，用 `lv_timer`（[`lv_timer.c`](3rdparty/lvgl/src/misc/lv_timer.c)）周期调用 `Pty::dispatch` |
| `terminal_child_fn`（`g_child_watch_add`） | LVGL 没有 `g_child_watch`，用 `signalfd(SIGCHLD)` + `waitpid` 在 `lv_timer` 里检测子进程退出 |
| `gtktsm_terminal_fork` | 复用 [`Pty::open`](src/pty_process.h:20)；父进程返回非 0，子进程返回 0 |

## 3. LVGL 程序的标准初始化与运行流程

### 3.1 初始化阶段（一次性）

| 步骤 | API | 说明 |
|---|---|---|
| 1 | `lv_init()` | 初始化 `lv_global` + `lv_timer_core_init`（见 [`lv_timer.c:55`](3rdparty/lvgl/src/misc/lv_timer.c:55)） |
| 2 | 时钟 | 二选一：`lv_tick_set_cb(my_tick_cb)`（[`lv_tick.h:96`](3rdparty/lvgl/src/tick/lv_tick.h:96)）或主循环里每 1ms 调 `lv_tick_inc(1)`（[`lv_tick.c:38`](3rdparty/lvgl/src/tick/lv_tick.c:38)） |
| 3 | `lv_display_create(w, h)` | 创建显示对象（V9 起取代旧 `lv_init_disp`），见 [`lv_display.c:74`](3rdparty/lvgl/src/display/lv_display.c:74) |
| 4 | `lv_indev_create(type)` | 创建输入设备（`LV_INDEV_TYPE_POINTER` / `LV_INDEV_TYPE_KEYPAD`），见 [`lv_indev.c`](3rdparty/lvgl/src/indev/lv_indev.c) |
| 5 | `lv_indev_set_read_cb(indev, cb, user_data)` | 绑定读回调（LVGL 是轮询模型，不是事件回调） |
| 6 | `lv_indev_set_group(indev, group)` | 把输入设备加入焦点组 |
| 7 | 创建 `lv_term` widget 并挂到 `lv_display_get_screen(disp)` | 你的终端 widget |

### 3.2 主循环（周期性）

```cpp
while (pty.is_open()) {
    lv_tick_inc(1);           // 推进 LVGL 时钟 1ms
    lv_timer_handler();       // 跑所有 lv_timer
    pty.dispatch();           // PTY 桥接：read → input_func_ → lv_term_feed_input
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
```

或更简洁：

```cpp
while (pty.is_open()) lv_timer_periodic_handler();  // 见 lv_timer.h:66
```

### 3.3 `main.cpp` 最小骨架

```cpp
#include <thread>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <csignal>
#include "lvgl.h"
#include "pty_process.h"
#include "terminal/lv_term.h"   // 待实现

static uint32_t my_tick_cb() {
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main() {
    lv_init();
    lv_tick_set_cb(my_tick_cb);

    lv_display_t * disp = lv_display_create(1024, 600);
    if (!disp) return 1;
    lv_obj_t * scr = lv_display_get_screen(disp);

    // 输入设备
    lv_group_t * grp = lv_group_create();

    lv_indev_t * keybd = lv_indev_create(LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keybd, keyboard_read_cb, keybd);   // 自己实现 keymap
    lv_indev_set_group(keybd, grp);

    lv_indev_t * ptr = lv_indev_create(LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(ptr, pointer_read_cb, ptr);

    // 终端 widget
    lv_obj_t * term = lv_term_create(scr);
    lv_term_set_font(term, "fonts/DejaVuSansMono.ttf");
    lv_term_set_sb_size(term, 2000);
    lv_group_add_obj(grp, term);

    // PTY + shell
    te::Pty pty;
    pid_t pid = pty.open([](auto pty, auto opaque, auto data) {
        lv_term_feed_input((lv_obj_t*)opaque, data);
    }, term, 80, 24);

    if (pid == 0) {
        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "lvtsm", 1);
        char *shell = getenv("SHELL") ? : (char*)"/bin/sh";
        execl(shell, shell, (char*)nullptr);
        _exit(127);
    }

    // 主循环
    while (pty.is_open()) {
        lv_timer_handler();
        pty.dispatch();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    lv_deinit();
    return 0;
}
```

## 4. 与 GTK 版的关键差异

| GTK 版 | LVGL 版 |
|---|---|
| `g_application_run`（[`gtktsm.c:40`](3rdparty/libtsm/src/gtktsm/gtktsm.c:40)） | `while(lv_tick_inc; lv_timer_handler; pty.dispatch; sleep)` 主循环 |
| `GtkWindow`（[`gtktsm-win.c:38`](3rdparty/libtsm/src/gtktsm/gtktsm-win.c:38)） | `lv_display_create` |
| `gdk_keymap_translate_keyboard_state`（[`gtktsm-terminal.c:1375`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:1375)） | `lv_indev_set_read_cb`（轮询模型） |
| `g_io_add_watch` + `g_idle_add` 监听 pty bridge | `lv_timer_create` 周期调 `Pty::dispatch` |
| `g_child_watch_add`（[`gtktsm-terminal.c:2006`](3rdparty/libtsm/src/gtktsm/gtktsm-terminal.c:2006)） | `signalfd(SIGCHLD)` + `waitpid`（放 `lv_timer` 里轮询） |

## 5. 实现顺序建议（对应 todo）

```
[ ] 1. 新建 src/terminal/ 目录，定义 lv_term widget 骨架（继承 lv_obj，挂 lv_canvas 子对象）
[ ] 2. 实现 term_font / term_face / term_glyph：用 lv_font_ttf 加载 TTF，按 ch[0] 做 shl_htable 缓存，lv_canvas 渲染单字形
[ ] 3. 实现 term_canvas draw 回调：遍历 tsm_screen 的 dirty cell，按 tsm_screen_attr 画字符 + underline + inverse
[ ] 4. 实现 lv_indev 键盘处理：keymap 翻译 → tsm_vte_handle_keyboard（复用 xkbcommon）
[ ] 5. 实现 lv_indev 鼠标选择：PRESS 记录起点，PRESSING 调 tsm_screen_selection_start/target，短按 reset，长按复制
[ ] 6. 实现 lv_timer 周期调用 Pty::dispatch，补上 pty_process.cpp:235 的 input_func_ → tsm_vte_input
[ ] 7. 实现子进程退出检测：signalfd(SIGCHLD) + waitpid，emit terminal-stopped 事件
[ ] 8. 实现窗口 resize 回调：LV_EVENT_RESIZED → tsm_screen_resize + Pty::resize + 重设 canvas 尺寸
[ ] 9. 把 main.cpp 接起来：LVGL 初始化（lv_init + lv_tick + lv_display_create + lv_indev_create）→ 创建 lv_term → Pty::open → 子进程 execve shell → 主循环 while(lv_tick_inc; lv_timer_handler; pty.dispatch; sleep)
```

## 6. 执行策略（给 AI 看的"分步验证"路径）

1. **先跑通最小骨架**：`lv_init` + `lv_tick_set_cb` + `lv_display_create` + 一个 `lv_label` 显示 "hello" + 主循环 → 确认 LVGL 渲染管线工作正常
2. **再叠终端 widget**：`lv_term_create` + `lv_canvas` 子对象 + `tsm_screen` / `tsm_vte` / `te::Pty`
3. **最后接输入**：`lv_indev` 键盘/鼠标 → `tsm_vte_handle_keyboard` / `tsm_screen_selection_*`
4. **主循环里周期** `Pty::dispatch` 把子进程输出喂给 `tsm_vte_input`（补上 [`src/pty_process.cpp:235`](src/pty_process.cpp:235) 的 TODO）

每一步都独立可验证，避免一上来就写 2000 行 `gtktsm-terminal.c` 等价物。

## 7. 关键 API 速查

| 功能 | LVGL API | 参考文件 |
|---|---|---|
| 初始化 | `lv_init()` | `3rdparty/lvgl/src/lv_init.c` |
| 时钟 | `lv_tick_set_cb` / `lv_tick_inc` | `3rdparty/lvgl/src/tick/lv_tick.c` |
| 显示 | `lv_display_create(w,h)` | `3rdparty/lvgl/src/display/lv_display.c` |
| 输入 | `lv_indev_create` + `lv_indev_set_read_cb` + `lv_indev_set_group` | `3rdparty/lvgl/src/indev/lv_indev.c` |
| 定时器 | `lv_timer_create(cb, period, user_data)` | `3rdparty/lvgl/src/misc/lv_timer.c` |
| 主循环 | `lv_timer_handler()` / `lv_timer_periodic_handler()` | `3rdparty/lvgl/src/misc/lv_timer.c` |
| 画布 | `lv_canvas` 子对象 | `3rdparty/lvgl/src/widgets/canvas/lv_canvas.c` |
| 字体 | `lv_font_ttf` | `3rdparty/lvgl/src/font/` |
| 事件 | `lv_event_send` / `LV_EVENT_*` | `3rdparty/lvgl/src/misc/lv_event.c` |
| 重绘 | `lv_obj_invalidate` + `lv_refr` | `3rdparty/lvgl/src/core/lv_refr.c` |
