# TerminalEmulator

##

### update submodules, install dependencies

```shell
git submodule update --init --recursive
apt install libx11-dev
```

### build

Required tools: cmake >= 3.31, ninja (or make), meson >= 1.1, plus a C/C++ compiler.

```shell
cmake -S . -B build -G Ninja
cmake --build build
```
