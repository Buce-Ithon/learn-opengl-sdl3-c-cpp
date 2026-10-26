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

## 2. 运行时控制

窗口内 HUD 默认显示在左上角，按 `/` 可以隐藏或重新显示。

| 按键 | 作用 |
| --- | --- |
| `/` | 显示/隐藏 HUD |
| `[` / `]` | 切换到上一个/下一个效果 |
| `P` / `Shift+P` | 选择下一个/上一个可调参数 |
| `,` / `.` | 减小/增大当前参数 |
| `Backspace` | 恢复当前参数的默认值 |
| `R` | 从磁盘重新编译当前效果的 shader |
| `Esc` / `Ctrl+Q` | 退出 |

热重载失败时，旧的 shader 会继续渲染，HUD 中会显示编译错误。

## 3. shader 参数注解

在 fragment shader 中用 `// @param` 声明可调 uniform：

```glsl
// @param uSpeed float 0.1 8.0 1.0 "Rain speed"
uniform float uSpeed;
```

格式为 `// @param <uniform> <类型> <min> <max> <default> "<显示名>"`。

- 支持 `float`、`vec2`、`vec3`、`vec4`、`int`、`bool`。
- 向量类型可以用逗号分隔各分量，例如 `0,0,0`；只写一个值时会对所有分量生效。
- 未注解但处于活动状态的 uniform 会通过 GL 反射显示，并在 HUD 中以 `*` 标记。
- 渲染器保留的 uniform（分辨率、时间、输入纹理、Moon 交互状态等）不会被暴露为可调参数。
