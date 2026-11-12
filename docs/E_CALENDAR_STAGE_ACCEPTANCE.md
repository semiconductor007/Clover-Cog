# E-04 日历与换季阶段验收

日期：2026-11-12。分支stage1，HEAD为ff21ee8；验收对象包含工作区尚未提交的多轮改动，不是已发布版本。本阶段完成日历基础、日期呈现、换季刷新与通用地图回退、睡眠/保存恢复联动。未提交、未推送。

## 规则、参考与职责

先查阅本机StardewValleyDecompiled提交5225ef4中的`Stardew Valley/StardewValley/WorldDate.cs`，采用TotalDays按累计日期集中换算年份、季节、季内日的组织方式。该仓库为第三方反编译结果；原作每季28天，本项目季节长度由CalendarRules传入，未照搬28天或原作跨季枯萎规则。

- 内部absoluteDay从0开始，显示为第1年春季第1天。
- 春、夏、秋、冬依次循环，冬季最后一天结束后进入下一年春季第1天。
- 每季默认7天沿用开发计划中的原型建议，不是团队最终平衡结论。支持1—366的整数季节长度，366为本轮工程校验上限，并非真实季节规定。
- 午夜仍是日切边界，凌晨02:00睡到同日06:00，不重复结算。已有作物跨季保留，仍使用原有浇水/成长服务。
- 年份、季节、季内日从统一累计日期派生，不分别累计或保存三套日期状态。

用户本次授权包含实现上一阶段分析中的日历跨模块支援。GameCalendar及规则由B审查；MainWindow、SleepOverlay与字体呈现由C审查；存档规则元数据与v1/v2兼容由D审查；构建配置由A审查。旧TEAM_MODULES与新版E分工矛盾仍存在，本次没有冒充团队已冻结契约、登记成员或完成签审。

## 实际接口

接口为本地实现v1，尚待团队审查；不是将旧契约草案宣布冻结。

```cpp
struct CalendarRules {
    int daysPerSeason; // 新档默认7；保存后以该档数据为准
    bool isValid() const;
};
struct CalendarDate {
    qint64 year;       // 从1开始
    Season season;    // Spring, Summer, Autumn, Winter
    int dayOfSeason;  // 从1开始
};
std::optional<CalendarDate> GameCalendar::forDay(qint64 absoluteDay,
                                                CalendarRules rules);
// MainWindow已有构造调用保持兼容，新增第三参数仅配置新档。
MainWindow(QWidget *parent, const QString &savePath, CalendarRules newGameCalendar);
// 日期成功结算并刷新后，若显示年份/季节改变则发出通知。
void MainWindow::seasonChanged(qint64 year, int season);
```

GameCalendar只做纯计算，负日期或非法规则返回空值，不写时钟、农田、背包或存档。超大累计日期在纯接口中使用qint64运算。当前游戏日结算标记仍为int，主窗口读取超过int日期范围的存档时拒绝加载，避免窄化后溢出；未改造整个农田标记类型。

newGameCalendar合法时应用于新档，非法时构造使用原型默认规则；纯接口与存档接口对非法规则明确拒绝。规则目前通过C++配置注入，不提供玩家调季节长度的设置界面。修改新档默认值不会改变v2已有存档。

seasonChanged是显示刷新通知，不是每个被跳过季节的业务事件队列。多日推进仍先按日结算农田，再向界面发布最终日期；B不得依赖显示信号代替逐日日结算。

## 呈现与缺图回退

- 日常HUD以两行显示年份/季节与季内日；完整日期另有tooltip。时间标签初始化和resize布局已统一，避免首次绘制时跑出右侧面板。
- 睡眠面板使用同一日期格式函数：上方显示醒来日期与06:00，下方显示睡前日期及换季提示；凌晨睡眠提示同日休息。
- 依次尝试系统Microsoft YaHei UI、Microsoft YaHei、Noto Sans CJK SC、PingFang SC，逐字检查所需汉字。通过则日期用中文；没有合适字体时使用英文日期，避免缺字方框。
- 本机Windows字体检查选择Microsoft YaHei UI。字体不复制到仓库，未引入新字体下载、QRC字体资源或许可分发决定；跨机器字形风格仍需后续资源负责人统一。
- 目前没有登记四季专属地图，因此各季使用当前通用地图。刷新环境层的seasonId及usingSharedSeasonVisuals展示元数据，换季记录缺口日志，不加载/初始化另一张地图，不清空现有作物或耕地。
- 缺图是开发记录，不在玩家HUD常驻显示调试标签。日常昼夜继续使用已验证的固定室外曲线；季节配色、四季图集切换和室内外场景仍属E-07后续工作。

## 保存兼容

新写入格式schemaVersion=2，新增`calendarDaysPerSeason`。只保存规则与累计时间，不另存派生年份/季节，防止读回矛盾。

读取v2必须包含1—366的整数季节长度；缺失、零、负数、小数、字符串或超上限均失败，调用方候选对象保持不变。非法规则写入在文件操作前被拒绝，旧文件字节保持不变。

读取v1时固定赋予LegacyDaysPerSeason=7。v1原先只显示DAY n，没有已保存的季节长度；此规则为首次引入日历的明确兼容约定，不宣称从旧档恢复了不存在的季节字段。累计时间、库存、农田和结算标记保持原语义。旧档成功保存时写为v2，沿用原有`.bak`轮转备份，首次升级可读回v1备份；备份不是永久归档。

旧二进制不支持v2，不能用旧程序反复打开新档。既有主窗口加载失败门禁继续保留。未新增离线时间或离线收益。

## 验证证据

Windows，Qt6.8.2，MinGW13.1，Release，C++17。应用最终构建通过；核心27 passed / 0 failed；完整集成24 passed / 0 failed（34524ms）；Windows原生字体专项3 passed / 0 failed（9008ms）。数量包含Qt Test数据行和init/cleanup，不表示同等数量业务函数。

| 验收项 | 本轮证据 |
| --- | --- |
| 日期起点、季末与跨年 | 1/7/28/366天配置遍历三年，固定季节边界断言及qint64上界测试 |
| 新季睡醒 | 春夏、夏秋、秋冬、冬春五组真实MainWindow集成，含28天配置 |
| 不重复结算 | 睡醒重复刷新、同日02:00再睡、保存读回均保持一次成长 |
| 规则随档保存 | 28天档用1天新档配置启动，仍按档中28天恢复 |
| 旧档与失败不变 | v1读取、保存升级、v1备份可读；坏规则读取不改候选、写入不改旧文件 |
| 自然午夜与背包 | 背包打开跨春夏午夜，日期更新、成长一次，关闭后交互恢复 |
| 多季跳时 | 第6日跳至第29日，23个结束日依序处理，仅首日沿用浇水，最终第2年春季第2天 |
| 日切错误 | 注入矛盾结算标记，不发布新季、不更新日期标签、不覆盖成功检查点 |
| 地图回退 | 换季前后保留地图尺寸、作物与实际成长，环境元数据记录当前季节和通用资源 |
| 布局与输入 | 520×320、1040×640、1920×1080截图、日期宽高断言；原有视口醒来与移动测试保持通过 |
| 中文和英文回退 | Windows原生字体检测及隐藏控件渲染；offscreen无系统CJK字体时英文回退通过 |

截图位于build_sleep_tests/ui-captures：calendar-7-6-sleep.png、calendar-7-13/20/27系列、calendar-28-111系列、calendar-chinese.png、calendar-chinese-sleep.png等。已目视检查小窗口跨季摘要、大窗口HUD、28天跨年和中文日期/跨年摘要。原生中文截图为780×480、对应520×320逻辑画布的150%缩放；没有启动Windows屏幕捕获或黄色捕获边框工具，也没有读写个人正式存档。原生字体专项在未显示桌面窗口的真实控件上渲染，不等同于逐项人工桌面试玩。

日志：build_tests/calendar-core-results.txt、build_sleep_tests/calendar-ui-results.txt、build_sleep_tests/calendar-native-font-results.txt，以及两个构建目录的calendar-build.log。它们是本地忽略的构建产物，不作为素材入库。

## 复现与预览

在项目根目录执行，其他机器替换Kit路径：

```powershell
$env:PATH = 'D:\Qt\6.8.2\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
New-Item -ItemType Directory -Force build_stage1, build_tests, build_sleep_tests
Set-Location build_stage1
qmake ..\CHECK.pro CONFIG+=release
mingw32-make -j2
Set-Location ..\build_tests
qmake ..\tests\tests.pro CONFIG+=release
mingw32-make -j2
.\release\core_tests.exe -o calendar-core-results.txt,txt
Set-Location ..\build_sleep_tests
qmake ..\tests\sleep_ui_tests.pro CONFIG+=release
mingw32-make -j2
.\release\sleep_ui_tests.exe calendarFontAndFallbackText -platform windows -o calendar-native-font-results.txt,txt
$env:QT_QPA_PLATFORM = 'offscreen'
.\release\sleep_ui_tests.exe -o calendar-ui-results.txt,txt
Remove-Item Env:QT_QPA_PLATFORM
```

正常游戏运行build_stage1/release/CHECK.exe。不想等待季末，可在build_sleep_tests目录运行隔离预览（Qt/MinGW PATH已设置且未启用offscreen）：

```powershell
.\release\sleep_ui_tests.exe --preview-calendar
```

预览从春季最后一天22:00进入睡眠，显示夏季第1天摘要。它使用临时目录与真实MainWindow，关闭后临时存档丢弃；不会加入正式游戏的调时键。本轮提供该入口但没有自动打开桌面预览；自动验收通过真实集成用例完成。

## 文件及交接

新增gamecalendar.h/.cpp、calendarpresentation.h/.cpp；修改mainwindow.h/.cpp、sleepoverlay.h/.cpp、savemanager.h/.cpp、CHECK.pro、tests/core_tests.cpp、tests/tests.pro、tests/sleep_ui_tests.cpp/.pro、README及E计划/本记录。未修改worldstate或背包业务，未修改素材与QRC。CHECK.pro.user、Tiled session及此前未提交素材/背包/睡眠/昼夜工作保持原状，不应夹带进本轮提交。

阶段功能及本地验证已完成。团队仍需审查日历契约、7天原型默认值和旧档兼容规则；Mac、全套中文、字体打包、音效、适种限制及季节专属美术没有在本轮完成，也未声称通过。后续优先与B确认最终日历参数，再与资源负责人接E-07实际季节景观。
