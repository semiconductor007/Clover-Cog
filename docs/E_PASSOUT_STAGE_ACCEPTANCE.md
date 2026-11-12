# 02:00昏迷与床区恢复验收

日期：2026-11-12。stage1，HEAD ff21ee8加现有多轮未提交工作区。用户要求：06:00至次日02:00前的休息都到同一个次日06:00；02:00还未休息则自动昏迷，在床上醒来。本轮不提交推送、不启动桌面捕获、不扩展中文设置。

## 规则与参考

| 时刻 | 行为 |
| --- | --- |
| 第N天06:00—23:59 | 床边确认后进入第N+1天06:00 |
| 第N+1天00:00—01:59 | 床边确认后进入当天06:00，不再增加一天 |
| 第N+1天02:00 | 无需确认，强制昏迷，进入当天06:00并回床区 |
| 读档已处于02:00—05:59 | 初始化完成后立即昏迷恢复到当天06:00 |

新游戏从第0天06:00开始。旧档保留原累计时间；日期仍在午夜切换，HUD显示00:00—02:00，不改成原作24:00—26:00。农田仍在午夜结算一次，02:00到06:00不会再成长一次。确认框维持原有暂停规则：01:59打开确认且暂停后尚未到02:00；取消再继续计时，抵达02:00才强制昏迷。

先查看本机StardewValleyDecompiled提交5225ef4：Game1.Update中timeOfDay>=2600检测并调用startToPassOut；Farmer.performPassOut停止动作、执行倒下表现；passOutFromTired关闭菜单并进入恢复流程。采用到点停止操作、退出菜单、统一恢复与保存的顺序，用本项目时钟/Widgets实现。没有复制原作动画、体力、邮件、医院、扣钱或掉物规则；本项目无昏迷扣款和物品损失。

## 实际实现

- GameClock::passOutDeadline(from,to)返回区间中第一个02:00；若起点已经在02:00—06:00，立即处理。负值、倒退区间拒绝，qint64上界检查通过。
- MainWindow记录上次检查的游戏分钟。普通更新、交互和保存入口检查截止点；定时器延迟跨过02:00甚至跨多天时，先截住第一个截止点，不结算截止点之后尚未经历的多天。
- 昏迷直接进入既有淡出/结算/保存/早晨摘要，不弹确认。停止移动、屏蔽地图输入、关闭背包和种子商店；商店的晚到购买请求也检查忙碌/截止状态。
- 昏迷画面显示YOU PASSED OUT，早晨摘要显示PASSED OUT - WOKE AT BED。未添加与角色图集不匹配的倒地帧；不是原作完整角色倒地动画。
- 成功结算后，在床区内寻找可站立位置；床区被挡时检查周围4格范围。ImageMovementWidget::canStandAt复用实际脚部碰撞矩形和当前CollisionDetector，包含地图边界检查。通过setInitialPosition同时更新角色位置和镜头。
- 先完成结算及安全落点，再保存06:00、位置和农田状态。保存失败可以重试同一检查点，不再成长或再次搬运角色。
- 若床区及附近全被挡住，回滚本次候选日结算，恢复昏迷开始时间，保留原位置与旧存档，保持暂停并只允许重试；不传送进墙，也不允许取消昏迷后继续越过截止点。该失败阶段自动保存也被拒绝。
- 正常早晨完成时重置检查锚点，醒来不立即再次昏迷。新构造窗口先完成所有UI再检查旧档，避免载入过期时间时访问未构造控件。

上次检查时间只是运行期调度锚点，不新增第二个时钟，不写入存档。读取完整存档后以其累计时间初始化锚点，再依据02:00—06:00窗口恢复。没有离线时间增长或存档格式变更。

## 文件与责任

修改gameclock.h/.cpp（截止点纯计算）、mainwindow.h/.cpp（调度、商店关闭、落点、保存门禁）、sleepoverlay.h/.cpp（昏迷原因与失败重试）、rpg.h/.cpp（安全站立检查），以及tests/core_tests.cpp、tests/sleep_ui_tests.cpp、README中英文、E计划和本记录。没有修改WorldState、库存、经济规则或SaveManager格式。

A审时钟接口，C审主窗口、角色与睡眠呈现，B/D审结算/保存回归。本次为用户明确授权的跨域支援，不代表团队签审完成。此前未提交文件和个人配置保持原状。

## 验证

环境：Windows、Qt6.8.2、MinGW13.1、C++17、Release。应用构建通过。核心29 passed / 0 failed；完整集成38 passed / 0 failed；原生高DPI渲染专项3 passed / 0 failed。数量包括数据行、init/cleanup。具体输出位于本地构建日志，不作为素材提交。

| 验收点 | 实际测试 |
| --- | --- |
| 精确边界 | 01:59未触发，02:00触发，06:00不触发；00:00—01:59主动睡眠到同日06:00 |
| 跨过截止点 | 23:59直接延迟到第三天09:00，恢复只到首个凌晨对应的06:00，农田只经历一次午夜 |
| 不重复成长 | 午夜成长后02:00昏迷不追加成长；重复更新和醒来后更新不再次昏迷 |
| 关闭面板 | 背包隐藏；种子商店嵌套exec事件循环中到点被关闭，输入恢复到睡眠流程 |
| 床区与保存 | 角色从远处移回床区，可通过实际碰撞检查；落点、06:00和成长一致保存并读回 |
| 资产不变 | 非零金币、背包作物和种子数量在昏迷前后相同 |
| 读旧状态 | 03:00、尚未结算午夜的合法存档启动后自动恢复，无构造期崩溃，午夜只结算一次 |
| 无安全落点 | 全阻挡地图失败保留旧文件字节、时间和原位置；修复碰撞后重试成功 |
| 保存失败 | 昏迷后IO失败仍保留内存床区位置，重试保存读回同一位置/时间/成长 |
| 原功能 | 确认/取消、全天截止前休息、日历、季节、音效、视口醒来与缩放回归通过 |

少数旧测试直接注入已经结算的多日状态，现显式重置测试锚点，并使用09:00等非逾期时间，避免把独立农田/渲染fixture误当作自然游玩时间。真实截止测试不重置区间锚点，明确验证延迟无法跳过02:00；生产代码没有测试用的跳过开关。

截图：build_sleep_tests/ui-captures/passout-morning.png、passout-awake.png及既有日历/睡眠系列。使用真实Qt控件渲染；没有启动OS屏幕捕获，不声称实际声音试听或Mac验证。多场景/室内床仍需C提供场景接口，当前单地图床区正常落点优先位于床标记内；阻挡时安全邻格属于明确回退。

日志：build_tests/passout-core-results.txt、build_sleep_tests/passout-results.txt、passout-native-results.txt，以及各构建目录的passout-build.log和运行期日志。

## 操作与复现

正常运行build_stage1/release/CHECK.exe。06:00—次日02:00前在床边按E确认；到02:00会自动昏迷，摘要中按Enter返回床区。没有新增正式调时键。

在项目根目录执行（替换本机Kit路径）：

```powershell
$env:PATH = 'D:\Qt\6.8.2\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
New-Item -ItemType Directory -Force build_stage1, build_tests, build_sleep_tests
Set-Location build_stage1
qmake ..\CHECK.pro CONFIG+=release
mingw32-make -j2
Set-Location ..\build_tests
qmake ..\tests\tests.pro CONFIG+=release
mingw32-make -j2
.\release\core_tests.exe -o passout-core-results.txt,txt
Set-Location ..\build_sleep_tests
qmake ..\tests\sleep_ui_tests.pro CONFIG+=release
mingw32-make -j2
$env:QT_QPA_PLATFORM = 'offscreen'
.\release\sleep_ui_tests.exe -o passout-results.txt,txt
Remove-Item Env:QT_QPA_PLATFORM
$env:QT_SCALE_FACTOR = '1.5'
.\release\sleep_ui_tests.exe calendarFontAndFallbackText -platform windows -o passout-native-results.txt,txt
Remove-Item Env:QT_SCALE_FACTOR
```

在build_sleep_tests中手动运行隔离预览：

```powershell
.\release\sleep_ui_tests.exe --preview-passout
```

预览从远离床的01:59进入02:00昏迷，醒来回床区；仅使用临时存档、默认静音，不影响正式存档。本轮未自动启动桌面预览。
