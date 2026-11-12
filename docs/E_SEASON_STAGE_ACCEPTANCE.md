# E-07 季节环境首版验收

日期：2026-11-12。stage1，HEAD ff21ee8 加工作区多轮未提交改动。本轮实现四季运行时植被调色、安全纹理切换、资源失败回退和季节晨昏参数；没有提交推送，没有启动桌面捕获。

## 参考、边界与职责

参考本机StardewValleyDecompiled提交5225ef4（第三方反编译结果）：GameLocation.cs的updateSeasonalTileSheets加载目标图集，失败恢复原资源；Game1.cs的getStartingToGetDarkTime/getTrulyDarkTime按季节和场景选择变暗时间。采用资源失败保留可用画面、光照参数集中配置的顺序，以Qt重新实现，不复制原作资源或原作地形/NPC更新规则。

本次是E季节表现任务下对C地图/主窗口的授权支援。MapWidget和MainWindow接口由C审查，图集和美术参数由资源负责人审查，A审构建登记，B审季节输入，D复核保存恢复回归。未改写既有农田规则、库存或存档格式；没有宣称团队已确认原型参数或完成签审。

## 资源盘点与实际登记

默认map6.tmj使用10张图集，每格16×16。以下尺寸与GID通过真实MapLoader日志及本地PNG元数据核对：

| 图集（sprout目录中的同名PNG） | 首GID | 像素尺寸 | 本轮处理 |
| --- | ---: | --- | --- |
| Grass | 1 | 176×112 | 绿色像素调色 |
| Basic_Grass_Biom_things | 78 | 144×80 | 绿色像素调色 |
| Tilled_Dirt_Wide_v2 | 123 | 176×112 | 保持原样，湿润叠色仍由现有渲染处理 |
| Farming Plants | 200 | 80×240 | 保持原样，不改变作物阶段或颜色 |
| Mushrooms, Flowers, Stones | 275 | 192×80 | 保持原样 |
| Trees, stumps and bushes | 335 | 192×112 | 绿色像素调色 |
| Water | 419 | 64×16 | 保持原样，不增加结冰行为 |
| Wood_Bridge | 423 | 80×48 | 保持原样 |
| Fences | 438 | 128×64 | 保持原样 |
| Grass_tiles_v2 | 470 | 176×112 | 绿色像素调色 |

已实际查看原草地图集、树木图集和Premium包Darker_Grass_Tiles_v2候选；后者也是176×112，但单个深色草地候选不能证明四季整套资源或语义映射兼容，因此没有只凭尺寸相同便替换。当前未登记可直接覆盖默认地图全部相关图集的专属四季套装，不据文件名搜索断言素材包绝对不存在这些内容。

实际资源登记在SeasonVisuals::vegetationAtlases和profile中：使用既有QRC图集作为不可变原图，运行时生成调色版本，不覆盖原PNG，不新增付费素材副本，不新增QRC资源。所有派生纹理仍以原Sprout Lands图集为基础，原素材来源与条款说明继续适用。

## 四季表现与原型参数

春季保留原色；夏季更浓绿；秋季暖橙；冬季低饱和冷色。调色仅处理登记图集中色相55—170度、饱和度至少0.16且非全透明的像素，保留每个像素位置和透明度。图集中的绿色装饰也可能被选中，本轮不是逐个物体语义分割，不保证所有绿色果实/装饰都独立保色。非绿色细节和未登记图集逐像素保持一致。

| 季节 | 目标色相 | 饱和度倍率 | 明度倍率/提升 | 清晨开始/中点/结束 | 黄昏开始/中点/结束 |
| --- | ---: | ---: | --- | --- | --- |
| 春 | 原色 | 1 | 1 / 0 | 05:00 / 06:00 / 07:00 | 17:00 / 19:00 / 21:00 |
| 夏 | 98° | 1.12 | 0.95 / 0 | 04:00 / 05:00 / 06:00 | 18:00 / 20:00 / 22:00 |
| 秋 | 32° | 1.12 | 0.98 / 0 | 05:30 / 06:30 / 07:30 | 16:00 / 18:00 / 20:00 |
| 冬 | 195° | 0.25 | 0.85 / 0.14 | 06:00 / 07:00 / 08:00 | 15:00 / 17:00 / 19:00 |

夜色RGBA：春(24,34,72,135)，夏(19,34,62,128)，秋(34,29,58,132)，冬(20,30,58,125)。冬季晨昏色偏冷，其余沿用暖色。按游戏分钟插值，午夜两侧保持连续。冬季06:00仍处清晨开始阶段，睡醒较暗是该原型参数的结果，不改变睡眠目标时间。

以上均为可调整的试玩值，不是最终季节或天文学规则。冬季浅色草地属于调色表现，不等于雪层、落雪、裸树或专属四季美术。未实现天气和水面结冰。

## 安全切换与缓存接口

```cpp
SeasonVisualProfile SeasonVisuals::profile(Season season);
bool SeasonVisuals::build(const QVector<Tileset> &base,
                         const SeasonVisualProfile &profile,
                         QVector<Tileset> *result, QString *error);
bool MapWidget::setSeasonVisuals(const SeasonVisualProfile &profile,
                                QString *error);
QColor DayNightEnvironment::tintAt(qint64 minutes, bool outdoors, Season season);
```

接口为本地实现，待相关成员审查；既有tintAt和setGameTime调用仍可用默认春季参数。

每次从不可变原图构建候选纹理，避免多次换季累积变色。整套候选通过后才替换m_tilesets；不调用loadMap、WorldState::initialize，不修改图层、GID、碰撞、成长、库存或人物位置。原地图真正重新加载时才重建基础纹理并清空缓存。

SeasonVisualProfile::textureOverrides允许按精确TSX文件名指定替代图片，便于未来接入已审查美术。当前内置配置不依赖外部替代文件。替代资源必须可解码、尺寸与原图一致、源网格合法、透明布局一致；未知图集或非法参数也拒绝。透明布局检查不能证明完全不透明图集的语义图块顺序，未来美术仍须人工审查GID映射，不以尺寸和透明度检查冒充语义证明。

失败时不提交任何一张候选图，保留上次成功季节纹理并返回错误；MainWindow在失败时记录日志和seasonVisualFallback状态。日期与季节光照仍按日历更新，不回滚时间或农田。失败结果不缓存，修复资源后可再次请求同一配置。成功资源按完整配置键缓存，QCache最多4套；当前不提供成功文件同路径修改的热重载功能。

MainWindow初次加载及季节变化时更新纹理；自然时间、睡眠目标、失败时间恢复和读档仍使用同一刷新入口。世界资源与昼夜只在现有环境层组合，没有新增第二个全屏夜幕；HUD、工具栏、背包和睡眠界面层次保持原约定。

## 验证结果

Windows，Qt6.8.2，MinGW13.1，Release。应用构建通过。核心28 passed / 0 failed；完整集成27 passed / 0 failed（34595ms）；Windows字体专项3 passed / 0 failed（7104ms）。数字包含数据行和init/cleanup。

| 范围 | 证据 |
| --- | --- |
| 四季光照 | 全日逐分钟连续性、周期性、午夜连续、透明室内分支及冬夏晨昏差异 |
| 图块与颜色 | 4季×10图集，首GID/网格/尺寸/每像素透明度不变；非选中像素及作物等独立图集不变 |
| 真实季节差异 | 四季中午截图的相同世界区域两两不同，工具栏不透明内框完全一致 |
| 资源失败 | 缺文件、坏图片、错尺寸、错透明布局、未知图集均拒绝，整幅画面与活动资源季节不变 |
| 原子性与重试 | 第一张资源合法、后续资源失败时不部分提交；补齐文件后同一请求成功 |
| 状态保留 | 成熟小麦、未成熟土豆、湿润、耕地、碰撞与结算标记在8轮四季切换中保持；worldChanged零次 |
| 缓存 | 正常四季仅构建4套，反复切换不重复构建；额外配置也不突破4套缓存 |
| 睡眠/读档 | 四季边界及跨年真实主窗口用例通过，读回的资源季节与目标光照一致，原有失败/重复结算回归通过 |
| UI与缩放 | 小/双倍/宽窗口睡眠回归通过；四季中午/17:00/23:00、冬季背包截图已生成 |

截图位于build_sleep_tests/ui-captures：season-spring/summer/autumn/winter-noon.png、对应dusk/night图、season-winter-backpack.png，以及calendar系列的跨季睡眠/读档图。已目视检查四季中午、冬季夜晚和背包，确认世界可辨认、界面清晰。透明工具栏角落会透出季节地面，不能要求整块含透明区的截图逐像素不变。

截图来自Qt真实控件渲染，不是设计稿；本轮未启动桌面捕获，也没有声称逐项人工桌面试玩、音频、Mac或真实室内外场景验收通过。缓存测试证明数量有界，不代替长期进程内存分析。

日志：build_tests/season-core-results.txt、build_sleep_tests/season-ui-results.txt、build_sleep_tests/season-native-font-results.txt，以及构建目录中的season-build.log。构建截图和日志为忽略产物，不入素材目录。

## 复现与使用

从项目根目录运行，其他电脑替换Kit路径：

```powershell
$env:PATH = 'D:\Qt\6.8.2\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
New-Item -ItemType Directory -Force build_stage1, build_tests, build_sleep_tests
Set-Location build_stage1
qmake ..\CHECK.pro CONFIG+=release
mingw32-make -j2
Set-Location ..\build_tests
qmake ..\tests\tests.pro CONFIG+=release
mingw32-make -j2
.\release\core_tests.exe -o season-core-results.txt,txt
Set-Location ..\build_sleep_tests
qmake ..\tests\sleep_ui_tests.pro CONFIG+=release
mingw32-make -j2
$env:QT_QPA_PLATFORM = 'offscreen'
.\release\sleep_ui_tests.exe -o season-ui-results.txt,txt
Remove-Item Env:QT_QPA_PLATFORM
.\release\sleep_ui_tests.exe calendarFontAndFallbackText -platform windows -o season-native-font-results.txt,txt
```

正常运行build_stage1/release/CHECK.exe，根据存档日历自动应用季节。仍可用隔离的`build_sleep_tests/release/sleep_ui_tests.exe --preview-calendar`查看春夏睡眠切换；这是临时存档，不修改玩家正式存档。当前没有新增正式游戏调季节的作弊键。

## 文件与待决事项

新增seasonvisuals.h/.cpp；修改mapwidget.h/.cpp、daynightenvironment.h/.cpp、daynightoverlay.h/.cpp、mainwindow.cpp、CHECK.pro、tests/tests.pro、tests/core_tests.cpp、tests/sleep_ui_tests.cpp/.pro、README.md、E开发计划及本记录。未修改WorldState、SaveManager、原始素材和QRC；已有其他未提交工作及个人配置保持原状。

本阶段季节环境首版完成。专属四季美术、准确的物体级调色遮罩、真实室内外场景、中文全面统一、字体打包、音效和Mac仍待后续任务。团队需审查原型配色、晨昏参数和C文件支援，再决定合入；没有将本地通过视为已推送或已获团队签审。
