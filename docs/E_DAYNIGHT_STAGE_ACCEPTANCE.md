# E-06 昼夜环境阶段验收

记录日期：2026-11-11。分支 stage1，HEAD ff21ee8 加未提交工作区。本阶段仅交付固定室外参数的昼夜首版与睡眠/读档衔接；没有提交或推送。

## 交付与边界

- 昼夜色调直接从 GameClock 累计分钟推导，日内取模，不另建时钟或每帧累加亮度。
- 地图、人物、作物共同受影响。环境控件位于地图子控件中的人物上方、工具栏下方；画布级HUD、背包和睡眠界面位于世界子树上方。
- 环境控件不接收鼠标、不获取焦点。夜间依旧能移动、切换工具并把地图点击交给原有处理链。
- 原有 updateTime 统一刷新：自然时间、加载完成、睡眠跳时和失败时间恢复均走同一入口。不在绘制过程中结算农田、资产或保存。
- 不修改农田、背包、购买、出售、存档格式或季节规则。没有接入天气、灯具和四季地图。

## 原型参数

| 时间 | 环境 |
| --- | --- |
| 00:00—05:00 | 夜色，RGBA(24,34,72,135) |
| 05:00—06:00 | 夜色插值到清晨暖色 |
| 06:00—07:00 | 暖色逐渐透明 |
| 07:00—17:00 | 透明，保留原素材色彩 |
| 17:00—19:00 | 透明逐渐过渡到暖色 |
| 19:00—21:00 | 暖色逐渐过渡到夜色 |
| 21:00—24:00 | 夜色，与午夜后连续 |

暖色为RGBA(158,85,55,38)。参数集中在 daynightenvironment.cpp，属于可试玩调整的首轮值；尚未经团队确认，不是最终日出日落规则。最多135/255透明度的覆盖保留世界可辨认亮度。按游戏分钟插值，每秒对应一个游戏分钟，沿用现有1秒刷新，不新增刷新定时器。

纯计算支持 outdoors=false 返回透明色，但当前地图入口只有室外，未虚构室内场景状态或声称完成室内外切换。后续由C提供真实场景信息再接入。季节日期由B提供，E不复制季节长度或年份公式。

## 星露谷参考

先阅读本机 StardewValleyDecompiled，提交5225ef4（第三方反编译结果）：

- Stardew Valley/StardewValley/Game1.cs：UpdateGameClock，使用变暗阈值和时间插值计算outdoorLight。
- 同文件DrawLighting：集中处理世界光照，区分ambientLight与outdoorLight。

采用时间驱动、集中世界光照及显示层分离的原则，以C++17/Qt Widgets重写。没有复制原作完整光照贴图、局部灯源、天气或美术资源；QPainter SourceOver与原作混合方式不同，因此没有照抄透明度数值。

## 文件与跨模块交接

| 文件 | 本轮用途 |
| --- | --- |
| daynightenvironment.h/.cpp | 无业务状态的时间到色调计算 |
| daynightoverlay.h/.cpp | 只绘制环境色、输入穿透 |
| mainwindow.h/.cpp | 组装环境层，在统一时间刷新入口更新 |
| CHECK.pro | 应用注册新增实现与头文件 |
| tests/core_tests.cpp、tests/tests.pro | 连续性、周期性、边界与室内回退测试 |
| tests/sleep_ui_tests.cpp/.pro | 真实控件分层、输入、保存读回、睡眠联动及隔离预览 |
| README.md、E_DEVELOPMENT_PLAN.md、本记录 | 当前状态、边界与复查说明 |

依据用户本次明确开发授权实施，未改写旧五人所有权文件。A需审构建注册，C需审主窗口与绘制层，B提供季节日历后才能继续季节差异。尚未取得团队签审，不能把本地验证等同于已合并的共同基线。已有未提交背包、睡眠、素材和个人配置改动保留，不冒充本轮新增内容。

## 构建与测试结果

Windows，Qt6.8.2，MinGW13.1，Release。应用qmake/make构建成功；核心25 passed / 0 failed；完整睡眠与昼夜集成16 passed / 0 failed（20032ms）。计数包含数据行与init/cleanup。

核心新增测试覆盖全天逐分钟相邻色差、午夜连续、跨多日周期性、清晨/黄昏趋势、室内透明、负值回退和qint64最大值。真实MainWindow集成覆盖人物和世界变暗、HUD白色文字及工具栏不被覆盖、夜间视口点击穿透与人物移动、背包保持可读、环境刷新不改变金币/库存/种子/成长/湿润状态、存档读回夜色、睡眠目标06:00更新光照。原有午夜、失败恢复、重试保存及重复结算测试也通过。

520×320、1040×640、1920×1080客户区均完成夜景与睡醒截图及醒来按钮坐标测试。截图位于 build_sleep_tests/ui-captures；目视检查白天、黄昏、夜晚、夜间背包、大窗口夜景和小窗口清晨。HUD半透明背景会透出变暗的世界，文字和边框不被环境层染色；不能要求整块半透明HUD的合成像素完全不变。

日志：build_tests/daynight-core-results.txt、build_sleep_tests/daynight-ui-results.txt、build_stage1/daynight-build.log、build_sleep_tests/daynight-build.log。均为本地忽略的构建产物。

在项目根目录复查（其他电脑需替换Kit路径）：

```powershell
$env:PATH = 'D:\Qt\6.8.2\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
New-Item -ItemType Directory -Force build_stage1, build_tests, build_sleep_tests
Set-Location build_stage1
qmake ..\CHECK.pro CONFIG+=release
mingw32-make -j2
Set-Location ..\build_tests
qmake ..\tests\tests.pro CONFIG+=release
mingw32-make -j2
.\release\core_tests.exe -o daynight-core-results.txt,txt
Set-Location ..\build_sleep_tests
qmake ..\tests\sleep_ui_tests.pro CONFIG+=release
mingw32-make -j2
$env:QT_QPA_PLATFORM = 'offscreen'
.\release\sleep_ui_tests.exe -o daynight-ui-results.txt,txt
Remove-Item Env:QT_QPA_PLATFORM
```

## 桌面交互与隔离预览

使用真实MainWindow与临时存档启动 `sleep_ui_tests.exe --preview-daynight`。原先窗口激活连续失败；用户最小化全屏应用后重试成功。实际观察22:00夜景、按F6切到12:00白天、按F7切到19:00黄昏，地图与人物一起改变，HUD及工具栏正常。后续F8输入因工具检测到用户操作被拦下，没有记为成功；保留预览供用户检查。桌面未逐项人工操作睡眠/背包，相关联动由上述真实控件集成测试验证。

在build_sleep_tests目录、已设置Qt/MinGW PATH且未设置offscreen平台时运行：

```powershell
.\release\sleep_ui_tests.exe --preview-daynight
```

仅该测试预览提供F6白天、F7黄昏、F8夜晚、F9清晨，不向正式游戏加入调时作弊键。预览关闭后临时数据丢弃，不写个人正式存档。正常游戏运行build_stage1/release/CHECK.exe，随游戏时间自动变化。

## 剩余事项

本阶段固定室外昼夜目标已实现并验证。未完成B日历接入、季节HUD与季节配色、室内外真实场景切换、中文字体和音效、灯具/天气、Mac验证。夜间配色仍需团队试玩确认，站点原有文字裁切另属UI打磨；没有将这些未验证项记为通过。下一阶段先与B确定日历接口，再推进E-04和E-07。
