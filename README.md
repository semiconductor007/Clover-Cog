# Clover-Cog

基于 **C++17、Qt 6 Widgets 和 qmake** 的像素风传统农场游戏。白天时间正常流逝，玩家亲自锄地、播种、浇水；晚上 20:00 后睡觉推进日期，收获后带着有限背包到收购点出售，再用金币购买下一批种子。

## 当前状态

传统日制农场的第一版闭环已经合入 `main`：

- 小麦、土豆两种作物，生长天数、产量和售价不同；
- 消耗型种子、锄地、播种、每日浇水和睡眠跨天；
- 背包容量、金币、固定收购点和种子商店；
- JSON 存档、备份恢复和核心规则测试。

当前重点是 S2 体验打磨：中文 HUD、新手提示、作物成长表现和边界回归。自动播种、自动收割、运输和离线收益暂不属于当前版本。

## 核心循环

```text
购买种子 → 锄地播种 → 白天照料 → 20:00 后睡眠进入下一天
    ↑                                      ↓
    └──────── 收获成熟作物 → 到收购点出售 ─┘
```

## 操作

| 操作 | 输入 |
| --- | --- |
| 移动 | W / A / S / D |
| 切换工具或种子 | 鼠标滚轮 |
| 锄地、播种、浇水、收获 | 鼠标左键 |
| 睡眠、出售、打开种子商店 | 靠近对应区域后按 E |

## 构建

需要 Qt 6 Desktop Kit、C++17 编译器和 Multimedia；构建测试还需要 Qt Test（testlib）。在 Qt/MinGW 命令行中运行：

```powershell
qmake CHECK.pro CONFIG+=release
mingw32-make -j2
```

核心测试：

```powershell
qmake tests/tests.pro CONFIG+=release
mingw32-make -j2
core_tests.exe -txt
```

## 项目文档

- [项目规划](docs/PROJECT_PLAN.md)：玩法定位、范围和里程碑
- [开发文档](docs/DEVELOPMENT.md)：状态规则、模块职责和当前任务
- [执行计划](docs/IMPLEMENTATION_PLAN.md)：S1–S4 开发顺序与验收门
- [种植需求](docs/FARMING_REQUIREMENTS.md)：首版详细规则
- [工程架构](docs/ARCHITECTURE.md)：代码现状与技术约束
- [素材盘点](docs/ASSET_AUDIT.md)：地图、像素 UI 和字体资源

## 目录提示

- `mainwindow.*`：主窗口、HUD 和交互组装
- `worldstate.*`：土地、树木和作物状态
- `gameclock.*`：游戏时间和睡眠跨天
- `cropdefinition.*`：作物配置
- `inventory.*` / `economyservice.*`：背包、金币、出售和种子购买
- `savemanager.*`：JSON 存档与备份
- `tests/`：Qt Test 核心规则测试

## 许可与素材

代码使用木兰宽松许可证，第 2 版。仓库内像素素材按各自目录的原始许可使用；新增字体和素材必须在文档中记录来源与许可。
