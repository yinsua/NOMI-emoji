# NOMI 眼睛表情仿真器

基于 LVGL 9.5.0 的 240 x 240 圆屏眼睛表情系统。屏幕保持纯黑背景，仅眼睛属于五官；问号、汗滴、爱心、星光、眼泪、气团等均为独立的情绪辅助符号。

## 构建

### 环境要求

- **Windows**（仿真器使用 Win32 后端：`user32` / `gdi32` / `shell32` 等）。
- **Python 3 + pip**：用于把自包含的构建工具链下载到 `.tools/`（该目录已在 `.gitignore` 中忽略）。
- 无需预装 Visual Studio、CMake、Ninja 或编译器——工具链全部由 pip 提供，编译器使用 [Zig](https://ziglang.org/) 作为 C/C++ 前端。

### 1. 获取源码（含 LVGL 子模块）

LVGL 以 git 子模块形式引用，克隆时务必带 `--recursive`：

```powershell
git clone --recursive https://github.com/yinsua/NOMI-emoji.git
cd NOMI-emoji
```

若已克隆但忘了加 `--recursive`：

```powershell
git submodule update --init --recursive
```

### 2. 准备工具链

把 CMake、Ninja、Zig 安装到项目内的 `.tools/` 目录（版本与开发环境一致）：

```powershell
pip install --target .tools cmake==4.3.4 ninja==1.13.0 ziglang==0.16.0
```

安装后目录结构应为：`.tools\cmake\data\bin\cmake.exe`、`.tools\bin\ninja.exe`、`.tools\ziglang\zig.exe`。

### 3. 编译

```powershell
.\tools\build.cmd
```

该脚本会用上述工具链执行 CMake 配置 + Ninja 构建（Release），产物为 `build\nomi_eye_simulator.exe`。

## 运行

一步完成构建并启动：

```powershell
.\tools\run.cmd
```

默认每个表情停留约 2.2 秒，窗口标题显示当前序号和名称。快速验收模式（跳过停留、快速轮播全部表情）：

```powershell
.\build\nomi_eye_simulator.exe --fast
```

## 项目结构

```
├─ src/                 主程序与眼睛渲染
│  ├─ main.c            入口、窗口与轮播逻辑
│  ├─ nomi_eyes.c       64 个表情的绘制与动画
│  └─ nomi_eyes.h       表情接口
├─ tools/               构建与运行脚本（build/run + zig 编译器包装）
├─ third_party/lvgl/    LVGL v9.5.0（git 子模块）
├─ lv_conf.h            LVGL 裁剪配置
├─ CMakeLists.txt       构建定义
├─ .tools/              pip 安装的工具链（cmake/ninja/zig，未纳入版本管理）
└─ build/              构建产物（未纳入版本管理）
```

## 64 个表情

### 系统与基础 01-18

1. 待机 Idle
2. 放松 Relaxed
3. 唤醒 Wake
4. 聆听 Listening
5. 思考 Thinking
6. 回答 Responding
7. 确认 Confirm
8. 生气 Angry
9. 高兴 Happy
10. 兴奋 Excited
11. 好奇 Curious
12. 无语 Speechless
13. 惊讶 Surprised
14. 害羞 Shy
15. 叹气 Sigh
16. 困倦 Sleepy
17. 睡眠 Sleep
18. 警觉 Alert

### 积极与互动 19-36

19. 微笑 Gentle Smile
20. 大笑 Big Grin
21. 欢笑 Laugh
22. 喜极而泣 Tears of Joy
23. 眨眼 Wink
24. 调皮 Playful
25. 酷 Cool
26. 心动 In Love
27. 星星眼 Starstruck
28. 飞吻 Kissy
29. 温暖拥抱 Warm Hug
30. 幸福 Blissful
31. 得意 Proud
32. 恶作剧 Mischievous
33. 搞怪 Silly
34. 庆祝 Party
35. 神奇 Magical
36. 感谢 Thankful

### 中性与疑问 37-48

37. 怀疑 Skeptical
38. 不爽 Unamused
39. 翻白眼 Eye Roll
40. 疑问 Doubtful
41. 尴尬 Awkward
42. 窘迫 Embarrassed
43. 无聊 Bored
44. 专注 Focused
45. 坚定 Determined
46. 警惕 Suspicious
47. 寻找 Searching
48. 机器人 Robot

### 负面与悲伤 49-58

49. 担心 Worried
50. 焦虑 Anxious
51. 紧张 Nervous
52. 害怕 Afraid
53. 惊恐 Panic
54. 难过 Sad
55. 哭泣 Crying
56. 恳求 Pleading
57. 失望 Disappointed
58. 孤独 Lonely

### 极端与状态 59-64

59. 抓狂 Frustrated
60. 眩晕 Dizzy
61. 晕倒 Knocked Out
62. 不舒服 Sick
63. 冻僵 Freezing
64. 暴怒 Rage

## 绘制词汇

- 眼形：胶囊眼、上月牙、下月牙、圆环眼、爱心眼、星形眼、方形眼、X 眼、菱形眼、旋涡眼。
- 辅助符号：`#`、`?`、`!`、省略点、汗滴、气团、`Zzz`、爱心、星光、眼泪、彩带、蒸汽、压力线、扫描线、雪花和爆发线。
- 循环动作：呼吸、点头、摇头、扫视、弹跳、脉动、游移、颤抖和高频警觉。

## 交互

- 鼠标按下并移动：眼睛追踪触点。
- 单击：立即眨眼。
- 双击或长按：切换到高兴表情，然后继续轮播。

