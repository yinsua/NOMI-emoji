# NOMI 眼睛表情仿真器

基于 LVGL 9.5.0 的 240 x 240 圆屏眼睛表情系统。屏幕保持纯黑背景，仅眼睛属于五官；问号、汗滴、爱心、星光、眼泪、气团等均为独立的情绪辅助符号。

## 运行

```powershell
.\tools\run.cmd
```

默认每个表情停留约 2.2 秒，窗口标题显示当前序号和名称。快速验收模式：

```powershell
.\build\nomi_eye_simulator.exe --fast
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

