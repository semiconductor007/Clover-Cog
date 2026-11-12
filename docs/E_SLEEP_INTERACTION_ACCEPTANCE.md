# E-08 睡眠交互与全天休息验收

> 历史记录：用户随后增加02:00昏迷规则，主动休息窗口现为06:00—次日02:00前。当前规则和新游戏起始时间以 [昏迷阶段验收](E_PASSOUT_STAGE_ACCEPTANCE.md) 为准，下文保留当时全天休息实现的证据。

日期：2026-11-12。stage1，基线HEAD ff21ee8加现有多轮工作区改动。用户本次明确要求暂不做中文设置，并允许白天睡觉；该要求覆盖旧文档的20:00限制。本轮未提交推送，未启动桌面捕获。

## 本轮交付

- 床边按E打开休息确认，显示目标日期和06:00。确认期间暂停唯一GameClock，停止移动、屏蔽地图及其他面板请求。
- SLEEP/Enter确认，CANCEL/Esc取消。Enter在Cancel或Retry按钮上执行对应按钮动作；长按自动重复不触发多次。
- 取消恢复打开前的计时状态：原本运行则恢复运行，原本暂停则保持暂停。取消不额外跳时、成长或写入睡眠结果；确认期间自动保存门禁阻止落盘。
- 确认时复查当前床区、初始化状态和结算状态，再重算睡眠目标。确认后仍走原有淡出、统一结算、保存、摘要和淡入流程。
- 返回游戏恢复地图与角色焦点。床区有独立的E休息提示，不覆盖保存错误等状态消息。
- 首次完成结算且保存成功时消费一次成功音效；重试保存首次成功可消费一次，再次重试或刷新不重复。
- 本轮不翻译睡眠正文、按钮、错误提示或增加语言菜单；已有日历中文与英文回退维持原样。没有顺带修改背包、交易或作物业务。

## 最新时间规则

目标是严格晚于当前时刻的下一次06:00，而不是固定增加一天：

| 当前时刻 | 目标 |
| --- | --- |
| 00:00、02:00、05:59 | 同一天06:00，不新增午夜 |
| 06:00、12:00、19:59、20:00、23:59 | 次日06:00，经过一次午夜 |

醒来后可以重新发起并确认下一次休息。重复输入防护依靠确认/睡眠进行中状态，而不再依靠白天禁止睡眠。同一天没浇水就反复睡觉，不会重复获得成长。非法时间、唤醒小时或目标溢出仍拒绝，集成入口拒绝越过现有int日期范围。

发起确认前同步已经发生的自然午夜；这与确认取消导致的额外成长不同。关闭确认中的整个游戏会先取消，再沿用正常关闭保存检查点，未把关闭操作当作完成睡眠。

## 星露谷参考与项目差异

查阅本机StardewValleyDecompiled提交5225ef4的GameLocation.cs：床交互通过FarmHouse_Bed_GoToSleep创建Yes/No问题，Sleep_Yes再调用startSleep，随后doSleep进入新日。采用确认在前、统一睡眠提交在后的顺序，不复制原作C#、音效、美术或多人准备系统。

本项目按用户新要求全天可睡，午夜结算，凌晨到当天06:00；不改成原作26小时游戏日。当前只有单地图床区，尚无真实场景ID接口；没有声称已完成多场景室内/室外床验证。

## 音效来源与接口

wav/sleep_success.wav为本轮原创合成占位短音效：PCM16、单声道、22050Hz、约0.36秒，两个柔和正弦音，带短起音和衰减。由tools/generate_sleep_chime.ps1确定性生成；不使用原作声音、不下载第三方素材。作为项目新增资源登记，不代表团队已确认最终听感。

SleepSoundCue仅管理一个QSoundEffect及单次播放策略，不管理时间、资产、成长或保存。构造后预加载，beginAttempt重置本轮，complete(settled,saved)只在两者为真时接受一次成功，finishAttempt禁止离开摘要后迟到的播放。

- 结算失败和保存失败不播放成功音效；恢复后第一次保存成功才接受。
- 静音或音量0会丢弃该次提示，不在稍后解除静音时补播旧成功音。
- 资源缺失、解码失败或无输出设备，不等待音频完成，也不阻止睡眠或保存。
- 成功事件successAccepted不等于听见声音；playbackRequested只表示向Qt发出播放请求。

MainWindow提供setEffectsVolume(float)和setEffectsMuted(bool)，供C后续设置界面或全局静音意图调用。效果音默认0.35，背景音乐仍用原有独立对象。本轮不新增设置菜单或另一套全局音频管理器。

## 验证与局限

环境：Windows、Qt6.8.2、MinGW13.1、C++17、Release。应用构建通过；核心28 passed / 0 failed；完整集成32 passed / 0 failed；Windows原生高DPI渲染专项3 passed / 0 failed。数字包含Qt Test数据行和init/cleanup。

| 行为 | 已验证证据 |
| --- | --- |
| 全天可睡 | 遍历全天1440分钟，目标始终晚于当前且落在06:00；原边界表、负值及溢出回归 |
| 白天成长 | 中午休息到次日06:00，一次成长；06:00重新确认再睡，未浇水不追加成长 |
| 床边实际输入 | 角色E键进入确认；Enter激活SLEEP，Cancel按钮Enter及Esc取消 |
| 取消不变 | 已暂停/运行两组，确认等待1100ms时间冻结，存档字节和成长不变，取消后可以移动 |
| 位置与重入 | 确认期间程序改变位置后拒绝；自动重复Enter、重复确认及重复请求不多结算 |
| 保存失败区分 | 结算失败及保存失败无成功提示，修复重试后一次提示；重试不再成长 |
| 音频故障 | WAV头与长度验证；静音、音量0、坏资源、离开后结果、重复结果；缺音频时主窗口仍完成睡眠并恢复操作 |
| 播放请求 | 本机资源Ready，极低增益下验证一次真实Qt播放请求，没有把它记为主观听感验收 |
| 布局 | 520×320、双倍、宽窗口原有回归；Windows额外设置QT_SCALE_FACTOR=1.5，实际控件devicePixelRatio=2.25的隐藏渲染通过 |
| 原有功能 | 日历、季节、保存恢复、错误恢复、视口醒来及人物移动回归继续通过 |

测试使用临时存档。普通流程测试静音；真实播放分派专项以极低增益执行。本机音频后端日志另有FFmpeg/SWR输出布局警告，故障未阻塞流程；实际扬声器听感和物理拔插设备没有验收。不能把信号通过或资源Ready宣称为已试听通过。

截图：build_sleep_tests/ui-captures/sleep-confirmation.png、sleep-confirmation-native.png，以及既有morning/awake/save-failure/settlement-failure系列。已检查确认框的目标日期、暂停说明及两个按钮无重叠；没有启动Windows屏幕捕获，不会为本轮验收创建黄色捕获边框。

日志：build_tests/sleep-polish-core-results.txt、build_sleep_tests/sleep-polish-results.txt、sleep-polish-native-results.txt和相应build/runtime日志。构建目录为忽略产物，不进入资源清单。

## 复现与操作

正常游戏：运行build_stage1/release/CHECK.exe，在床区按E，确认或取消。界面仍沿用当前语言。确认框暂停时间，背包原有打开期间继续计时规则不变。

根目录PowerShell，其他电脑替换Kit路径：

```powershell
$env:PATH = 'D:\Qt\6.8.2\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
New-Item -ItemType Directory -Force build_stage1, build_tests, build_sleep_tests
Set-Location build_stage1
qmake ..\CHECK.pro CONFIG+=release
mingw32-make -j2
Set-Location ..\build_tests
qmake ..\tests\tests.pro CONFIG+=release
mingw32-make -j2
.\release\core_tests.exe -o sleep-polish-core-results.txt,txt
Set-Location ..\build_sleep_tests
qmake ..\tests\sleep_ui_tests.pro CONFIG+=release
mingw32-make -j2
$env:QT_QPA_PLATFORM = 'offscreen'
.\release\sleep_ui_tests.exe -o sleep-polish-results.txt,txt
Remove-Item Env:QT_QPA_PLATFORM
$env:QT_SCALE_FACTOR = '1.5'
.\release\sleep_ui_tests.exe calendarFontAndFallbackText -platform windows -o sleep-polish-native-results.txt,txt
Remove-Item Env:QT_SCALE_FACTOR
```

在build_sleep_tests中可手动运行隔离预览：

```powershell
.\release\sleep_ui_tests.exe --preview-confirm
.\release\sleep_ui_tests.exe --preview-confirm --with-sound
```

默认从中午床边打开确认，预览静音；第二条开启效果音，背景音乐仍静音，方便用户试听。两条命令任选一条启动，关闭后临时存档丢弃，不影响正式存档。本轮没有自动打开该预览。

## 文件和审查边界

修改gameclock.h/.cpp、mainwindow.h/.cpp、sleepoverlay.h/.cpp，新增sleepsoundcue.h/.cpp、tools/generate_sleep_chime.ps1及wav/sleep_success.wav；更新character.qrc、CHECK.pro、tests/core_tests.cpp、tests/sleep_ui_tests.cpp/.pro、README中英文、E开发计划及本记录。时钟/构建A审，主窗口/确认和资源接入C审，保存回归D审。没有改农田、背包业务、字体资源或存档格式。

其他轮次未提交改动、CHECK.pro.user与Tiled session保持原状，不能夹带提交。原计划中“夜间限制”和“06:00不能再次睡眠”已由用户新要求覆盖；旧阶段验收记录属于历史证据，以本记录与更新后的README为准。
