# 睡眠与跨日阶段验收

日期：2026-11-11。本地分支 stage1，HEAD 为 ff21ee8；本文验证对象包含尚未提交的工作区改动，不是已发布提交。未提交、未推送。保留此前背包、素材及个人配置改动，不将其视为本轮新增交付。

## 范围与责任边界

本阶段完成统一睡眠目标计算、主窗口跨日调度与保存门禁、睡眠过渡画面、失败恢复和相关测试。沿用现有 WorldState 与 SaveManager，没有再造农田、库存或存档服务。

现行 TEAM_MODULES/INTERFACE_CONTRACTS 是工程迁移提案：其中 A 负责时钟与存档，B 负责农田，C 负责主窗口与 UI；历史 E 时间睡眠计划与其责任划分不同。本次按用户明确授权完成现有结构中的睡眠集成，并不表示五人新架构已完成、公共契约已冻结或团队已审查。集成时 A 审查 gameclock 与 CHECK.pro、保存时序，B 审查日切调用，C 审查 mainwindow 与 sleepoverlay。成员实际账号和迁移窗口仍需团队登记。

主要文件：gameclock.h/.cpp、mainwindow.h/.cpp、sleepoverlay.h/.cpp、CHECK.pro、tests/core_tests.cpp、tests/sleep_ui_tests.cpp/.pro；README 与本记录用于操作和验收。原有背包改动及个人配置保持原状。

## 采用的参考逻辑

本机 StardewValleyDecompiled，提交 5225ef4：GameLocation.cs 的 startSleep/doSleep，Game1.cs 的 newDayAfterFade/_newDayAfterFade，Utility.cs 的 CalculateMinutesUntilMorning，以及 StardewValley.Menus/ShippingMenu.cs、StardewValley.Menus/SaveGameMenu.cs 的过渡与保存展示。

采用“淡出到全黑后结算、展示结果与保存状态、玩家确认后淡入”的处理顺序，用 Qt Widgets/QPainter 重新实现。参考是第三方反编译结果，不是官方原始源码；未复制原作资源。原作延续至26:00，本项目保持午夜切日：20:00—23:59 到次日06:00，00:00—05:59 到当天06:00，06:00—19:59 拒绝。当前到站即时出售，因此睡眠面板展示农田摘要，不重复发售卖收益。

## 已实现行为

- 睡眠目标计算独立验证，非法时刻和溢出不跳时；凌晨不多跳一天。
- 加载完成后才启动时间更新；日切按日期顺序处理，拒绝不一致的完成标记，避免重复成长。
- 保存前同步待结算日期；处理中拒绝保存，避免写入半完成检查点。结算失败恢复快照与时间，保留旧存档。
- 睡眠期间冻结时钟、角色与地图输入，显示日期、成长、缺水停滞、空耕地恢复和保存结果。
- 保存失败可重试或明确选择不保存继续；重试保存不重做日结算。结算失败可修复后重试或返回游戏。
- 点击 WAKE UP 或按 Enter/Space 后淡入，恢复时间与交互。

## 构建与自动验证

实际环境：Windows、Qt 6.8.2、MinGW 13.1、Release、C++17。应用最终构建成功。核心结果 24 passed / 0 failed；睡眠集成结果 15 passed / 0 failed（17755ms）。数字包含 Qt Test 数据行和 init/cleanup，不表示同等数量的独立业务函数。

在项目根目录 PowerShell 执行：

```powershell
$env:PATH = 'D:\Qt\6.8.2\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
New-Item -ItemType Directory -Force build_stage1, build_tests, build_sleep_tests
Set-Location build_stage1
qmake ..\CHECK.pro CONFIG+=release
mingw32-make -j2
Set-Location ..\build_tests
qmake ..\tests\tests.pro CONFIG+=release
mingw32-make -j2
.\release\core_tests.exe -o e-time-results.txt,txt
Set-Location ..\build_sleep_tests
qmake ..\tests\sleep_ui_tests.pro CONFIG+=release
mingw32-make -j2
$env:QT_QPA_PLATFORM = 'offscreen'
.\release\sleep_ui_tests.exe -o sleep-results.txt,txt
Remove-Item Env:QT_QPA_PLATFORM
Set-Location ..
```

测试覆盖：午夜及连续多日、保存读回一致性、重复结算、背包打开时跨午夜、结算中保存不改旧文件、日切错误恢复与重试、保存失败重试不重复成长、关闭窗口时检查点、睡眠期间输入门禁、凌晨同日睡眠、白天拒绝。

真实 QWidget 的离屏交互覆盖520×320、1040×640、1920×1080三种客户区；通过 QGraphicsView 的视口坐标点击醒来，验证计时恢复和人物移动。摘要停留期间等待1100ms，时间仍固定在目标分钟。生成8张截图并目视检查；保存失败提示与重试按钮重叠、失败说明行距偏紧已修复，再跑测试通过。

本地日志：build_tests/e-time-results.txt、build_sleep_tests/sleep-results.txt。截图位于 build_sleep_tests/ui-captures（构建目录忽略，不是仓库资源）。

## Windows 实际桌面交互

通过隔离预览使用真实 MainWindow、临时存档和测试农田，不触碰玩家正式存档。正常22:00睡眠：观察到 DAY 0 → DAY 1、06:00、成长数与 PROGRESS SAVED；鼠标点击 WAKE UP，观察淡入完成、角色与地图/HUD恢复、时间继续到06:06；按 B 成功打开背包。随后关闭测试窗口。

此前窗口被最小化的捕获不计为通过；本次重新激活目标后直接观察并操作成功。原生桌面验证覆盖1040×640正常路径；凌晨和保存失败分支、其他尺寸由上述真实控件自动化及截图验证，未声称逐项人工点过。未验证音频、Mac、四季或完整长期游玩。

## 如何运行与复查

正常游戏运行 build_stage1/release/CHECK.exe。靠近床区按 E（不是 F），仅夜间有效；查看摘要后按 Enter 或点击 WAKE UP。保存失败时先看提示，选择重试或不保存继续。

不等待夜晚的隔离预览，在 build_sleep_tests 中运行以下任一命令；预览关闭时临时数据即丢弃：

```powershell
.\release\sleep_ui_tests.exe --preview-sleep
.\release\sleep_ui_tests.exe --preview-sleep --early
.\release\sleep_ui_tests.exe --preview-sleep --save-failure
```

最后一项故意使用不可写为文件的路径展示失败，便于检查 RETRY SAVE / CONTINUE UNSAVED；不是正式存档故障。上述命令需已设置 Qt/MinGW PATH，且未设置 offscreen 平台。

## 阶段结论与剩余事项

本阶段构建、核心与睡眠集成测试、渲染和正常桌面交互已完成，可作为团队审查的本地基线。git diff --check 无空白错误。个人 .pro.user、Tiled session、构建目录及测试存档不应随功能提交。

下一阶段再处理日常昼夜环境、日历/季节、中文字体与团队新架构迁移；不在本轮自动扩展。当前界面仍为英文，地图站点原有文字裁切属于另行 UI 打磨项，不代表已完成全部界面验收。团队所有权审查与提交推送仍需另行安排。
