# 项目说明

## 1. 编译与运行

```bash
# 配置项目（使用 MinGW Debug 预设，其他配置类似）
cmake --preset mingw-debug

# 编译
cmake --build build/mingw/debug

# 运行
./build/mingw/debug/${CMAKE_PROJECT_NAME}.exe
```

其他预设：
```bash
cmake --preset mingw-release    # MinGW Release
cmake --preset ninja-debug      # Ninja Debug
cmake --preset msvc             # Visual Studio 2022
```
