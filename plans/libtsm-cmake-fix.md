# libtsm 链接问题诊断与修复计划

## 问题现象
`TerminalEmulator` 链接阶段报 `libtsm` 相关符号找不到（libtsm 相关符号未链接上）。

## 根因分析

阅读 [`CMakeLists.txt`](../CMakeLists.txt) 与 [`3rdparty/libtsm/src/tsm/meson.build`](../3rdparty/libtsm/src/tsm/meson.build) 后确认：

1. **meson.build 硬编码 `shared_library`**：[meson.build:29](../3rdparty/libtsm/src/tsm/meson.build:29) 直接写死 `shared_library('tsm', ...)`，因此 CMake 里 `-Ddefault_library=static` 是无效参数，实际产物是 `libtsm.so`。这一点与 `libtsm-imported` 声明为 `SHARED IMPORTED` 一致，**没有 static/shared 不匹配**。

2. **`include(ExternalProject)` 位置太靠后**：[CMakeLists.txt:25](../CMakeLists.txt:25) 才 `include`，而 [line 15](../CMakeLists.txt:15) 已提前定义了 `libtsm-imported`。虽然 imported target 是手动定义不依赖该模块，但 `ExternalProject_Add` 必须在 `include` 之后调用，逻辑上应该提前 include。

3. **最可能的实际错误**：meson 构建失败导致 `libtsm.so` 根本没生成。常见原因：
   - 缺系统依赖 `xkbcommon`（可选，缺则 fallback 到 `external/xkbcommon-keysyms.h`）
   - `MESON_EXECUTABLE` 找不到或版本过旧
   - 构建目录权限问题
   - `werror=true`（[meson.build:11](../3rdparty/libtsm/meson.build:11)）+ 编译器告警 → 构建中断

## 修复步骤

### Step 1: 调整 [CMakeLists.txt](../CMakeLists.txt)
- 把 `include(ExternalProject)` 移到 `project()` 之后（line 3 附近）
- 去掉无效的 `-Ddefault_library=static` 参数（meson 里写死了 shared_library）
- 保留 `add_dependencies(TerminalEmulator libtsm)` 确保构建顺序

### Step 2: 手动验证 meson 能否独立构建成功
```bash
cd 3rdparty/libtsm
rm -rf /tmp/libtsm-build
meson setup --buildtype=release . /tmp/libtsm-build -Dtests=false
meson compile -C /tmp/libtsm-build
ls -l /tmp/libtsm-build/src/tsm/libtsm.so
```
如果失败，根据报错补系统依赖（`libxkbcommon-dev` 等）。

### Step 3: 检查 imported target 的 include 路径
[CMakeLists.txt:19](../CMakeLists.txt:19) 的 `INTERFACE_INCLUDE_DIRECTORIES` 已覆盖 `src/tsm;src/shared;external`，与 [libtsm.h](../3rdparty/libtsm/src/tsm/libtsm.h)、[shl-pty.h](../3rdparty/libtsm/src/shared/shl-pty.h) 位置一致，无需改。

### Step 4: 重新 configure + build
```bash
cmake --build build --target TerminalEmulator
```
观察链接阶段是否还报 `undefined reference to tsm_*` / 找不到 `libtsm.so`。

## 验收
- `meson compile` 产出 `libtsm.so`
- `TerminalEmulator` 链接成功
- 二进制里能 `ldd` 看到 `libtsm.so`（或静态链接时符号已打进去）
