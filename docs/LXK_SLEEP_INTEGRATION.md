# lxk 睡眠行为向主线整合：第一轮

后续背包任务已将站点与床边交互改为 F，E 用于背包；本记录其余内容保留本轮睡眠移植的历史，当前按键与背包验收见[背包交互说明](INVENTORY_INTERACTION.md)。

日期：2026-11-14。整合基线为main的09dfe88，参考lxk的47eea51。本地分支lxk-sleep-integration，工作目录仍名Clover-Cog-main。没有将整个lxk合入，没有提交或推送；原main分支引用和原lxk工作目录保持原样。

## 范围与保留项

本轮完成合并方案第1—5批：主线Windows基线、白天休息、确认/取消、02:00昏迷、安全回床、现有保存失败/面板/暂停/音效联动及回归。第6批成长摘要和季节资源校验增强、第7项lxk旧档迁移不属于本轮，没有暗中替换或迁移。

完整保留主线四种作物、适季与肥料、经济/储物、菜单、固定7天日历、SeasonAppearance精确调色和装饰、DayNightOverlay、SceneTransition、v4存档、WakeSaveState及wakeSaved音效链。不引入lxk的GameCalendar、SeasonVisuals、SleepOverlay、第二套夜幕或音效对象。

参考星露谷本机反编译仓库5225ef4：GameLocation的床询问及Sleep_Yes→startSleep、Game1的timeOfDay>=2600检测、Farmer的performPassOut/passOutFromTired停止操作与恢复路径。借鉴确认/截止门禁和统一恢复顺序；本项目仍午夜切日、无体力扣款或掉物，不照搬26小时日期、原作资源和多人流程。

## 实际行为与接口

- 06:00至23:59休息到次日06:00，00:00—01:59休息到当天06:00；到02:00仍在活动则自动昏迷。02:00—05:59逾期存档进入农场后自动恢复至当天06:00。
- GameClock::sleepTargetMinute为目标时间纯计算；passOutDeadline(from,to)检测第一条02:00截止线并处理逾期起点。保留主线暂停时不足一分钟的余量。
- canSleep/sleepToNextMorning保留原签名供既有消费者编译，旧sleepStartHour参数不再限制白天；它们只做时间计算，交互许可和02:00强制恢复统一在MainWindow调度，不应当用canSleep代替完整命令门禁。
- requestSleep复用主线QMessageBox/showFarmPanel、m_panelOpen和输入屏蔽。确认时再次核对床区，重新计算目标；取消恢复原计时状态。自动重复按键不确认，Esc先取消对话框，不打开暂停页。
- applyClock在提交新时间之前检查首个截止点。延迟采样跨多天时先截到首个02:00，不让玩家绕过昏迷。Load只依据保存时刻是否处于逾期窗口判断，不把合法白天存档当作从新游戏起点自然经过的时间。
- 主动休息和昏迷共用recoverToMorning→applyClock(WakeUp)→已有日结算/保存链。沿用主线“业务状态提交”和“全黑后刷新可见状态”的区分，不在绘制回调另发成长或资产。
- 昏迷先捕获旧画面，关闭业务面板，选择当前新地图床区{25,8,2,2}内的安全格；必要时搜索周边4格。canStandAt复用真实脚部碰撞及地图边界，不复制旧地图坐标。随后一次性提交目标06:00、农田和位置，再保存。
- 完整地图更新不是换季或回床的一部分；不调用loadMap重置农田。家中床区属于同一大地图，尚无独立室内场景切换。
- 无安全落点时暂停并提示重试，不改变旧检查点、位置和农田，不保存半完成状态。修复碰撞后重试才进入早晨。
- 到点关闭背包、商店、收购、储物；业务提交入口acceptPanelCommand再次采样并验证活动面板，防止排队到来的交易越过截止点。真正暂停页不走时，确认框也不走时。
- 保存失败继续沿用WakeSaveState冻结早晨；重试只写同一份时间/位置/农田，不再次成长、施肥或搬运角色。wakeSaved沿用主线一次性通知；不额外创建播放器。只读保护档、备份、退出失败保护继续由主线负责。

本地接口修改需E及直接消费者审查：时钟E、角色查询C、MainWindow由E接入并由D审保存/交易联动，B审日结算调用。未宣称已经完成团队签审。新增friend ClockUiTests仅用于真实主窗口的故障注入和调度测试，没有生产测试开关。

## 基线与验证证据

Windows，Qt6.8.2，MinGW13.1，C++17、Release。应用基线与整合后构建均通过；没有升级框架或依赖。

| 套件 | 原main基线通过 | 整合后通过 | 跳过 |
| --- | ---: | ---: | ---: |
| core_tests | 23 | 23 | 0 |
| economy_tests | 12 | 12 | 0 |
| farm_controls | 21 | 21 | 0 |
| gameplay_panel_tests | 25 | 25 | 1 |
| panel_tests | 7 | 7 | 0 |
| save_tests | 24 | 24 | 0 |
| clock_ui_tests | 28 | 37 | 0 |
| scenetransition_tests | 17 | 17 | 0 |
| seasonappearance_tests | 4 | 4 | 0 |
| 合计 | 161 | 170 | 1 |

跳过项是wakeSoundWithoutOutput：本机存在输出设备，不能在该环境假装“无输出设备”。同套件保留正常播放通知等检查。计数包含初始化、清理和数据行，不等同于业务函数数。

原测试twoAmRemainsPlayableAndSleepsWithinSameDay已改成02:00昏迷；白天床边拒绝改成确认/取消和白天休息。整局newFarmFinancesItsNextCrop只补点击新确认按钮，原金币27、肥料消耗、胡萝卜成长2日及保存重开断言保留。

新增真实窗口测试：中午取消检查点字节不变、01:59确认等待不跨截止点及长按防护、确认时床区位置复查、四种面板到点关闭、延迟跨多天只到首个截止点、非零金币/背包/种子/肥料/箱内资产守恒、03:00载入恢复及再次读档、全阻挡床区失败不改档与重试、昏迷保存失败及Esc恰好到截止点时保持暂停。原跨季/跨年、过渡中退出、暂停说明、失败保存继续/重试和备份恢复用例继续通过。

额外QT_SCALE_FACTOR=1.5离屏交互子集8项通过，覆盖确认、取消、四种面板昏迷和位置复查。Windows offscreen不自动枚举系统中文字体，测试fixture只读加载本机C:/Windows/Fonts/msyh.ttc并验证汉字字形，没有复制字体入库或更改游戏字体设置。

已检查Qt实际控件截图captures/sleep-confirmation.png与passout-bed.png：中文确认、按钮和暂停说明可读；回床后角色位于房屋床区，主线四季/HUD/工具栏保留。截图由控件grab生成，没有启动OS桌面捕获。测试执行键鼠交互，但未声称完成主观试听、长期原生桌面试玩或Mac新增功能验收。

## 复现与产物

构建目录在源码目录外：../build_main_app及../build_main_baseline。每个测试目录内baseline-results.txt记录旧基线，integration-results.txt记录当前结果，另有构建与运行日志；clock_ui/clock_ui内dpi-results.txt及captures。它们不是待提交资源。

```powershell
$env:PATH = 'D:\Qt\6.8.2\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
# 在Clover-Cog-main目录
New-Item -ItemType Directory -Force ../build_main_app
Set-Location ../build_main_app
qmake ../Clover-Cog-main/CHECK.pro CONFIG+=release CONFIG-=debug_and_release
mingw32-make -j2
.\CHECK.exe
```

本机Python Launcher未发现Python运行时，因此没有安装依赖，也没有把失败的py命令写成通过。九套件由PowerShell顺序调用qmake、make并直接运行exe。make check本机曾返回0但没有Totals输出，验收以直接运行的结果文件为准。

有Python环境时可继续使用仓库tests/run_all.py；本机无Python的等价执行方式：

```powershell
# 从Clover-Cog-main根目录运行，已设置Qt/MinGW PATH。
$source = (Get-Location).Path
$testRoot = Join-Path $source 'tests'
$buildRoot = [IO.Path]::GetFullPath((Join-Path $source '../build_main_baseline'))
$env:QT_QPA_PLATFORM = 'offscreen'
foreach ($project in Get-ChildItem $testRoot -Recurse -Filter *.pro) {
    $relative = $project.FullName.Substring($testRoot.Length + 1)
    $directory = Join-Path $buildRoot ($relative -replace '\.pro$', '')
    New-Item -ItemType Directory -Force $directory | Out-Null
    Push-Location $directory
    try {
        qmake $project.FullName CONFIG+=release CONFIG-=debug_and_release
        if ($LASTEXITCODE -ne 0) { throw "qmake failed: $relative" }
        mingw32-make -j2
        if ($LASTEXITCODE -ne 0) { throw "build failed: $relative" }
        $executable = Get-ChildItem -Filter *.exe | Select-Object -First 1
        & $executable.FullName -o integration-results.txt,txt
        if ($LASTEXITCODE -ne 0) { throw "tests failed: $relative" }
        Get-Content integration-results.txt | Select-String '^Totals:'
    } finally { Pop-Location }
}
Remove-Item Env:QT_QPA_PLATFORM
```

## 文件与剩余工作

修改src/core/gameclock.h/.cpp、src/app/mainwindow.h/.cpp、src/player/rpg.h/.cpp、tests/core_tests.cpp、tests/clock_ui/clock_ui_tests.cpp、tests/gameplay_panel_tests.cpp、README.md、README.en.md、docs/IMPLEMENTATION_PLAN.md和本记录。没有改地图、QRC、作物、经济/储物实现、季节资源、光照代码、存档格式或构建源码清单。

原lxk的旧地图v2档仍不能当作主线历史v2直接承诺兼容；旧档迁移留作独立任务。完整成长摘要、额外季节资源校验也留在后续批次。当前结果是可审查的本地整合实现，尚未成为main已提交或已发布版本。
