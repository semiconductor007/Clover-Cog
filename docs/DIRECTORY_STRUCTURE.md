# 当前目录、目标文件与渐进迁移

基线 `8ddf705`；当前目录由 Git 跟踪文件生成，完整清单见文末附录。本轮仅新增 Markdown，不创建下列目标源文件或移动资产。`*.h/.cpp` 表示同名头与实现两个文件，表中的目录通配归属在 [文件所有权](FILE_OWNERSHIP.md) 中定义。

## 1. 当前实际结构

```text
Clover-Cog/
├── .gitignore
├── Basic_Grass_Biom_things.tsx
├── CHECK.pro
├── CHECK.pro.user
├── Grass.tsx
├── Grass11.tsx
├── Grass222.tsx
├── Idle.png
├── LICENSE
├── Premium Charakter Spritesheet.png
├── README.en.md
├── README.md
├── Walk.png
├── Water.tsx
├── character.h
├── character.qrc
├── collisiondetector.cpp
├── collisiondetector.h
├── gameclock.cpp
├── gameclock.h
├── gameconstants.h
├── itembar.cpp
├── itembar.h
├── mainwindow.cpp
├── mainwindow.h
├── mainwindow.ui
├── map.tiled-session
├── map6.tmj
├── map7.tiled-project
├── map7.tiled-session
├── map7.tmj
├── map8.tmj
├── mapdata.cpp
├── mapdata.h
├── maprenderer.cpp
├── maprenderer.h
├── mapwidget.cpp
├── mapwidget.h
├── playercontroller.cpp
├── playercontroller.h
├── rpg.cpp
├── rpg.h
├── tooltype.h
├── worldstate.cpp
├── worldstate.h
├── .trae/  (1 个已追踪文件)
│   ├── .ignore
├── Farm RPG FREE 16x16 - Tiny Asset Pack/  (21 个已追踪文件)
├── Sprout Lands - Sprites - Basic pack/  (11 个已追踪文件)
├── build/  (538 个已追踪文件)
├── docs/  (4 个已追踪文件)
│   ├── ASSET_AUDIT.md
│   ├── DEVELOPMENT.md
│   ├── GAMEPLAY_DESIGN.md
│   ├── PROJECT_PLAN.md
├── itembar/  (6 个已追踪文件)
│   ├── itembar0.png
│   ├── itembar1.png
│   ├── itembar2.png
│   ├── itembar3.png
│   ├── itembar4.png
│   ├── itembar5.png
├── sprout/  (268 个已追踪文件)
├── tests/  (2 个已追踪文件)
│   ├── core_tests.cpp
│   ├── tests.pro
├── tsx/  (10 个已追踪文件)
│   ├── Basic_Grass_Biom_things.tsx
│   ├── Farming Plants.tsx
│   ├── Fences.tsx
│   ├── Grass.tsx
│   ├── Grass_Hill_Tiles_Slopes v.2.tsx
│   ├── Grass_tiles_v2.tsx
│   ├── Mushrooms, Flowers, Stones.tsx
│   ├── Tilled_Dirt_Wide_v2.tsx
│   ├── Trees, stumps and bushes.tsx
│   ├── Wood_Bridge.tsx
├── wav/  (3 个已追踪文件)
│   ├── Audacity_Installer_via_MuseHub.exe
│   ├── On_the_Farm.mp3
│   ├── On_the_Farm.wav
```

共 909 个跟踪文件。资源/历史构建的大目录在树中折叠，完整路径在文末展开；这是本轮新文档写入前的提交快照。

## 2. 目标工程树

下列是推荐落地位置，不是已实现文件。src 下仍是五责任域：A 同时拥有 core/persistence/controller；C 同时拥有 ui/player。原资源目录先保留；resources 只收新增配置/资源，不开展素材全量搬家。

```text
Clover-Cog/
├── CHECK.pro                         # A，保留 qmake 应用入口
├── character.qrc                     # C，保留旧 URL 和旧资源目录
├── AGENTS.md                         # A，统一协作规范
├── .gitignore                        # A，后续单独清理提案
├── .github/                          # 未来配置，本轮不创建
│   ├── pull_request_template.md      # A，模板正文见 GIT_WORKFLOW
│   ├── CODEOWNERS                    # A，真实账号确定后建立
│   └── workflows/windows-qt.yml      # A，Qt/qmake 最小 CI
├── src/
│   ├── core/                         # A
│   │   ├── core.pri
│   │   ├── types.h                   # ID、数值单位、坐标、错误码、Result
│   │   ├── commands.h                # 命令、上下文、结果、变更通知
│   │   ├── playerstate.h             # 玩家持久化状态
│   │   ├── farmstate.h               # B 领域数据，共同审查，A 主责
│   │   ├── inventorystate.h          # D 领域数据
│   │   ├── economystate.h            # D 等级/解锁/累计进度
│   │   ├── logisticsstate.h          # E 设施/路线/货舱引用（M3）
│   │   ├── mapdefinition.h           # 无 QPixmap 的地图/市场/区域定义
│   │   ├── definitions.h             # 作物、升级、效果、设施定义
│   │   ├── gamestate.h/.cpp          # 活动状态封装/只读快照
│   │   ├── transaction.h/.cpp        # 候选事务和受限子域写句柄
│   │   ├── definitioncatalog.h/.cpp  # JSON 加载与校验入口
│   │   ├── gameclock.h/.cpp          # 单调增量、暂停、恢复、日历
│   │   └── gameruntime.h/.cpp        # 服务组装和统一时间推进
│   ├── gameplay/
│   │   ├── controller.pri           # A
│   │   ├── gamecontroller.h/.cpp     # A，命令权限和跨域提交
│   │   ├── farm/                    # B
│   │   │   ├── farm.pri
│   │   │   ├── worldstate.h/.cpp     # 旧类过渡，规则迁出后只留语义状态适配，不保留第二份 FarmState
│   │   │   └── farmsimulation.h/.cpp # 生长、种收、再生、范围/扩地规则
│   │   └── player/                  # C，纯规则，无 QWidget
│   │       ├── player.pri
│   │       ├── moverules.h/.cpp      # 按真实位置、边界、碰撞计算位移
│   │       └── collisiondetector.h/.cpp
│   ├── economy/                     # D
│   │   ├── economy.pri
│   │   ├── inventoryservice.h/.cpp   # 所有库存增减/容量/保留/转移
│   │   └── economyservice.h/.cpp     # 出售、购买、效果及累计进度
│   ├── logistics/                   # E，M3 再实现业务
│   │   ├── logistics.pri
│   │   ├── automationsystem.h/.cpp   # 自动采收与补种调度/覆盖
│   │   └── transportsystem.h/.cpp    # 代售批次与固定路线状态机
│   ├── persistence/                 # A
│   │   ├── persistence.pri
│   │   └── savemanager.h/.cpp        # 版本、原子写/备份、候选恢复
│   └── ui/                          # C
│       ├── ui.pri
│       ├── mainwindow.h/.cpp        # 保留 main()、窗口、装配调用
│       ├── map/
│       │   ├── mapdata.h/.cpp       # MapData/MapLoader，资源与语义导入
│       │   ├── maprenderer.h/.cpp   # 只读状态到绘制
│       │   ├── mapwidget.h/.cpp     # 视口/事件，兼容旧调用期后收窄 API
│       │   └── viewtransform.h/.cpp # 唯一坐标转换
│       ├── player/
│       │   ├── rpg.h/.cpp           # 保留 ImageMovementWidget 动画表现
│       │   ├── playercontroller.h/.cpp # 输入适配，不保存业务规则
│       │   └── toolvisuals.h        # ToolAnimation/资源帧
│       ├── panels/
│       │   ├── itembar.h/.cpp       # 当前工具/选种视图
│       │   ├── hud.h/.cpp           # ClockBgWidget 与金币/容量展示
│       │   └── gamepanels.h/.cpp    # 背包、交易、商店最小面板
│       └── assets/
│           └── assetcatalog.h/.cpp # 图集/字体加载与 cropId→帧映射
├── data/                            # 未来配置，无实际数值变动
│   ├── crops.json                   # B，作物行为
│   ├── upgrades.json                # D，价格/前置/效果
│   ├── economy.json                 # D，经济/背包参数
│   ├── facilities.json              # E，机器/物流参数
│   └── render_mapping.json          # C，逻辑 ID 到视觉资源
├── resources/
│   ├── config.qrc                   # A，新 data 的稳定 alias
│   ├── ui.qrc                       # C，未来新增 UI/字体
│   └── fonts/                       # C，选定字体及其许可证，文件名待选
├── tests/
│   ├── tests.pro                    # A，先保留现有单目标，随后 SUBDIRS 汇总
│   ├── core_tests.cpp               # A，原 5 测试保留到替代测试等价
│   ├── legacy/legacy.pro            # A，未来保留旧测试目标时加入
│   ├── core/core.pro + tst_core.cpp # A，事务/定义/时钟
│   ├── farm/farm.pro + tst_farm.cpp # B，生长/种收/范围
│   ├── player/player.pro + tst_player.cpp # C，坐标/移动/碰撞
│   ├── economy/economy.pro + tst_economy.cpp # D，库存/经济/升级
│   ├── logistics/logistics.pro + tst_logistics.cpp # E，设施/运输
│   ├── persistence/persistence.pro + tst_persistence.cpp # A，存档
│   ├── ui/ui.pro + tst_ui.cpp       # C，输入/资源/面板
│   ├── integration/integration.pro + tst_integration.cpp # E，跨域
│   └── fixtures/                   # 各域自有子目录；公共 fixture 由 A 管
├── docs/                            # 现有 4 篇 + 本轮 8 篇工程文档
├── map6.tmj / map7.tmj / map8.tmj   # C，当前原位保留
├── tsx/、sprout/、itembar/、wav/     # C，当前原位保留
├── Farm RPG FREE 16x16 - Tiny Asset Pack/ # 保留
├── Sprout Lands - Sprites - Basic pack/   # 保留
├── character.h / mainwindow.ui     # C，未接工程占位，暂留根目录
└── LICENSE、编辑器配置和历史 build/ # 暂留，清理另行评估
```

“+”表示该测试目录中的两个文件。纯数据头没有空 .cpp；暂不增加 Repository/Manager/Service 三层包装。GamePanels 可在真实复杂度增加时由 C 再拆，不能为每个按钮建一个业务服务。

## 3. 迁移映射（每行同时是核心文件职责表）

W1=输入收敛窗口，W2=状态/业务移交窗口，W3=逐模块路径迁移窗口。窗口按依赖顺序，不是日历天数。只有主责可编辑源文件，其他成员提供接口/需求 diff，不同时重构。

| 原路径 | 目标路径/模块/负责人 | 保留/拆分与理由 | 前置条件 | 验证 |
| --- | --- | --- | --- | --- |
| mainwindow.cpp/.h | src/ui/mainwindow.*，C；组装逻辑→core/gameruntime.*，A | 保留 MainWindow 和 main()；小麦计数→D 库存；ClockBgWidget→ui/panels/hud.* | W2 控制器/库存接口稳定，C 主迁移，A 审 | 启动、HUD、音频、关闭销毁顺序、种收反馈 |
| mapwidget.cpp/.h | src/ui/map/mapwidget.*，C | 保留 MapWidget；生产 timer→A，持有 WorldState→A GameState，命令→B | W1 一次点击；W2 状态单一化，B 不直接改此文件 | 负坐标/视口、无重复收种、成长可见、换图指针有效 |
| worldstate.cpp/.h | src/gameplay/farm/worldstate.*，B；语义类型→core/farmstate.h，A | 先保留类/原位提取 FarmSimulation，再成为 GameState 子状态适配；移除 GID 业务判定 | W2 A 受限写接口、C 无图像 MapDefinition | 原耕作/树木测试、两作物、成熟与渲染解耦 |
| playercontroller.cpp/.h | src/ui/player/playercontroller.*，C | 保留类作输入适配；工具规则→B、请求编排→A | W1/2 命令 facade | 同次输入仅一动作、工具索引边界、无 QWidget 业务依赖 |
| rpg.cpp/.h | src/ui/player/rpg.*，C；位移规则→gameplay/player/moverules.* | 保留 ImageMovementWidget/帧资产；位置真实状态→A | W2 PlayerState/MoveIntent/只读碰撞 | WASD、动画、地图边界、面板焦点、加载后位置一致 |
| gameclock.cpp/.h | src/core/gameclock.*，A | 保留 GameClock，扩暂停/有效 ms/恢复；日历换算不变 | DEV-001、IC-06 | 现有换算、暂停恢复、大增量、无时间跳变 |
| mapdata.cpp/.h | src/ui/map/mapdata.*，C；公共定义→core/mapdefinition.h，A | 保留 MapLoader/MapData 渲染层；无 QPixmap 语义导出 | W2 B 需要的地块/市场字段明确 | 非法地图、TSX 错误、默认图、objectgroup 与区域可达性 |
| maprenderer.cpp/.h | src/ui/map/maprenderer.*，C | 保留类，从 const snapshot+AssetCatalog 绘制 | C 帧映射、W2 状态接口 | 图层顺序/树移除/生长图、资源 URL 不变 |
| collisiondetector.cpp/.h | src/gameplay/player/collisiondetector.*，C | 保留检测类，修边界并替换长寿命裸借用方式 | W1 精确矩形测试、W2 快照 | 一像素边界/负坐标/重载动态障碍 |
| itembar.cpp/.h | src/ui/panels/itembar.*，C | 保留选择信号，后续组合槽位取代整图切换 | W3 先纯搬移，DEV-009 再 UI 升级 | 六槽/滚轮/工具同步/焦点 |
| tooltype.h | core/types.h，A；ui/player/toolvisuals.h，C | ToolType 共用、ToolAnimation/QPixmap 留 UI；ToolProperty 规则与资源拆开 | W2 A/C/B/D 审字段与过渡 include | 全应用构建，业务头无图像类型 |
| gameconstants.h | core/types.h/definitions.h，A；data/render_mapping.json，C | 时间常量保留默认；GID 变表现映射；业务参数进配置 | B 两作物测试、C 视觉等价 | 原默认速度/外观等价，非法配置拒绝 |
| tests/core_tests.cpp | 原位 A；后来分域 tests/*/tst_*.cpp | 不删除断言；按所有权移交相关场景 | 目标测试先通过，A 单一窗口迁移 | 同样 5 个功能测试仍存在且可运行 |
| tests/tests.pro | 原位，A | 从单 app 逐步变 SUBDIRS，legacy 注册旧目标 | 分域测试有独立 pro | 每个 exe 都实际执行，不只 make 汇总 |
| CHECK.pro | 原位，A | 保留 qmake；逐模块 include .pri，移除对应旧条目避免双编译 | 每次目录迁移同行 PR | 干净应用+测试构建，无重复符号 |
| character.qrc | 原位，C | 保留全部旧 prefix/alias；新配置单独 QRC | 新资源接入清单 | rcc、所有 URL 加载、字体/音频冒烟 |
| mainwindow.ui / character.h | 原位，C | 未编译占位，先保留，不新建重复入口 | 确认全仓引用后另提清理 | CHECK.pro FORMS 和 SOURCES 比对 |
| *.tmj、*.tsx、tsx/、map7.tiled-project | 原位，C | 保留 Tiled 文件；后续加语义对象需版本化 | DEV-005/010 地块与市场约定 | Loader、可达性、QRC 路径 |
| sprout/、itembar/、wav/、根 PNG、两套素材包 | 原位，C | 保留资产及来源；wav 安装程序单列候选清理，不运行 | 素材引用/许可记录核对 | 完整 QRC 本地路径扫描 |

## 4. qmake 与资源迁移操作约定

A 在 CHECK.pro 逐批加入 `include($$PWD/src/core/core.pri)` 等；模块 .pri 用 `$$PWD/文件.cpp` 和显式 HEADERS/SOURCES，禁止通配把未完成代码全部编进程序。某文件从根迁移时，同一个 PR 更新 include、.pri、应用和测试工程引用，旧路径删除仅在那次已授权的迁移任务执行。本轮不删文件。

tests 的 .pro 选择测试所需纯模块 .pri，不包含 mainwindow.cpp，避免多个 main；QTEST_APPLESS_MAIN 的旧测试不加载真实 QPixmap 图集，资源冒烟另用带 QGuiApplication/QApplication 的目标。资源大包继续 resources_big；不能简单在每个单元测试都链接所有图片音频。

保留旧 `:/map/map6.tmj`、`:/tilesets/tsx/...`、`:/tilesets/sprout/...`、`:/characte/...`、`:/ui/...` 和 `qrc:/sounds/...`。若未来移动某个素材，QRC 用 alias 保持 URL，实体相对路径按 QRC 所在目录重算；同时测试 MapLoader 当前依赖的 TSX basename→PNG 映射。禁止同时大改路径、业务与图集。

短期兼容转发方法必须注释所属迁移任务和删除条件：它只能调用新命令，不复制旧算法。WorldState 旧 GID 层可短期当派生渲染缓存，不能与语义状态分别写入。W2 同一 PR 移交所有权，防止一半代码引用旧实例、一半引用新实例。

## 5. 追踪文件快照附录

以下由 `git ls-files` 生成，包含已追踪的历史构建输出，不等于建议目录。未追踪 build-readme-check/ 不属于此清单，原样保留。

<details>
<summary>展开全部 909 个已跟踪路径（含历史构建文件）</summary>

```text
.gitignore
.trae/.ignore
Basic_Grass_Biom_things.tsx
CHECK.pro
CHECK.pro.user
Farm RPG FREE 16x16 - Tiny Asset Pack/Character/Idle.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Character/Portrait.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Character/Walk.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Farm Animals/Baby Chicken Yellow.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Farm Animals/Chicken Blonde  Green.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Farm Animals/Chicken Red.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Farm Animals/Female Cow Brown.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Farm Animals/Male Cow Brown.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Objects/Fence's copiar.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Objects/House.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Objects/Interior.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Objects/Maple Tree.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Objects/Plates.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Objects/Road copiar.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Objects/Spring Crops.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Objects/Sprite-0002.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Tileset/Fall Tile.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Tileset/Spring Tile.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Tileset/Summer Tile.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Tileset/Walls and Floors copiar.png
Farm RPG FREE 16x16 - Tiny Asset Pack/Tileset/Winter Fall.png
Grass.tsx
Grass11.tsx
Grass222.tsx
Idle.png
LICENSE
Premium Charakter Spritesheet.png
README.en.md
README.md
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Characters/Basic Charakter Actions.png
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Characters/Basic Charakter Spritesheet.png
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Characters/Egg_And_Nest.png
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Characters/Free Chicken Sprites.png
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Characters/Free Cow Sprites.png
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Characters/Tools.png
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Sprout Lands color pallet/Sprout Lands defautlt palette snip.PNG
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Sprout Lands color pallet/Sprout Lands defautlt palette.aseprite
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Sprout Lands color pallet/Sprout Lands defautlt palette.png
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/Tilesets/Wooden_House_Walls_Tilset.png
Sprout Lands - Sprites - Basic pack/Sprout Lands - Sprites - Basic pack/read_me.txt
Walk.png
Water.tsx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qmake.stash
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QApplication.7CCA7E633C1AAFFE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QCoreApplication.BF03ED095091EFE0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QDebug.1A1A6C62331291F6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QFile.483D1421089D9A40.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QFileInfo.17AFEF997A9B81BD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QJsonArray.05D3AF9D64E81B21.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QJsonDocument.55F24140483DE508.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QJsonObject.5CE4927CA386A4E9.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QKeyEvent.D02F240E7272309D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QLabel.402AFA2B8974AC65.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QList.D9C5ED5D939B567E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QObject.03522C5D7D33A816.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QPainter.6A89A9F71D617C5D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QPixmap.CE30A7DE31269A3C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QRect.3A0A06716DAEABF9.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QSize.F4786088A7066D01.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QSizeF.B1D5C4BE620644FA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QString.1859A97BDD865117.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QTimer.81117902F7DA1659.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QTransform.C0BA28C17AA4B4D6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QVariant.5E49797166A3B6E3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QVector.75185F808D4DAC2A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QWidget.E6DAD9EFD261B9F2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/QXmlStreamReader.7795E7BC5E70174C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stdarg___gnuc_va_list.h.F007904B7013DB64.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stdarg___va_copy.h.B3FB8917CC738B40.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stdarg_header_macro.h.AAA3AE53C628B048.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stdarg_va_arg.h.6E3C2D60ACB684B5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stdarg_va_copy.h.6548FFB5061F6272.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stdarg_va_list.h.E9B1122378D29F3F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_header_macro.h.ED60E3DAE26E2266.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_max_align_t.h.BC86EBE177602CC5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_null.h.4F832BC422BA6F05.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_nullptr_t.h.9CCC80B709ABE5F7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_offsetof.h.731B85590794C0A3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_ptrdiff_t.h.BD1C0C3F297E99B2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_size_t.h.FE9A4DD378894963.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_wchar_t.h.096E4A45410E540A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/__stddef_wint_t.h.C711FB61E41E0D25.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/_mingw.h.B0EE091DD885983E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/_mingw_ddk.h.442DC10EC03B1338.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/_mingw_mac.h.304C145B79C161A6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/_mingw_off_t.h.FF2E38318D677908.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/_mingw_secapi.h.A064B872FAB78740.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/_mingw_stat64.h.E53C5780F411405D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/_timeval.h.7EEB48B1D7CCF292.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/algorithm.4DEE9B9EACE6AF7C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/algorithmfwd.h.0B497762543C8985.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/align.h.F1552BD3DD305725.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/aligned_buffer.h.9BB7484036823CE5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/alloc_traits.h.85867FE5A67E5A49.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/alloc_traits.h.A11883B677330512.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/allocated_ptr.h.CDF8C532A1A9AC14.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/allocator.h.F385DBC9005FF1C7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/array.26B74003B85546B3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/assert.h.68D6D6EF94A32832.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/assertions.h.AFC18645B49C45E2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/atomic.F62AF69E231766FB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/atomic_base.h.615439FAE55D4FCD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/atomic_lockfree_defines.h.DCC03259313824D9.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/atomic_wait.h.0C470531397A3E96.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/atomic_word.h.F7AB0AC4E119999A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/atomicity.h.E6AF11B710FA6A56.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/auto_ptr.h.1474AAD0CD1A143C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/basic_ios.h.E56986FC75EBE157.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/basic_ios.tcc.26616710529391B5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/basic_string.h.0E229E5FB8F36D82.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/basic_string.tcc.891DDB6C6F398B45.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/bessel_function.tcc.8E23C988593AF43C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/beta_function.tcc.F1D2B2C076C392D0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/binders.h.017DE93138A9F045.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/bit.A9A2511053D8DC1A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/c++allocator.h.C5A7807D3317991A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/c++config.h.B1F6AB130BA307C5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/c++locale.h.1E3B5B90B274A032.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cassert.7B8041861C616F4C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cctype.285B797895EC42BA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cerrno.F8C92C2FE6F50D8B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/char_traits.h.FDD012BA98ADB0FD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/character.cpp.47B7493AE05800E8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/character.h.EDC2443D07100CDC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/charconv.E3F9A32AD9F24D9C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/charconv.h.96C840BC9CAABE1E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/chrono.0A3972BFDA2D439B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/chrono.h.F790C9FF8E5A00C9.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/chrono_io.h.4D3DD03F78D67C7A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/climits.DA2E22DDC75CCB83.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/clocale.DF22B8538C54C3ED.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cmath.FC9CE6783A6CF08D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/codecvt.728A4414B23FD89D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/codecvt.h.F39BADC8D834365B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/collisiondetector.cpp.4237095D8F76D869.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/collisiondetector.h.74E9F5E5272C22F5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/compare.5D2180EC539AD733.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/concept_check.h.A92008FF98BE09E6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/concepts.1125FCAE79F7EA72.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/concurrence.h.676E5B67AC761AF2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/corecrt.h.DC10C62DAD596B9E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/corecrt_startup.h.50C1230E329616FA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/corecrt_stdio_config.h.6DEF148248573F20.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/corecrt_wstdlib.h.DFCCD196B0D54568.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cpp_type_traits.h.1255BF6B9C2E47A5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cpu_defines.h.42E91D4A8C78655C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/crtdefs.h.B0DE51648AA94F6E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cstddef.75A7818CC72C6640.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cstdint.782BC67A3703D126.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cstdio.89A20A9754FD651B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cstdlib.9CE82F6989F6117C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cstring.4AA026347F3CF771.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ctime.FCC46399592EB6AE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ctype.h.01DF00AC0BC1A936.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ctype_base.h.B1347DAAC7ED6F13.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ctype_inline.h.2A5F38F50D6F98DE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cwchar.776F2E3545319B50.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cwctype.961B500D582A9348.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cxxabi_forced.h.0A5039C6228D443D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/cxxabi_init_exception.h.4159A85F401934BF.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/debug.h.798455F6FE2AC08E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ell_integral.tcc.F4F027B20CBD127F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/enable_special_members.h.524955B07D619D69.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/erase_if.h.B9E4AA64C84C63FC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/errno.h.43D3020B5D7292AB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/error_constants.h.195501146EFBF93D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/exception.F4110AAD38FE49A1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/exception.h.2A9651983302B3CD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/exception_defines.h.39F1D50679F8DFC4.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/exception_ptr.h.D856965727F611E0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/execution_defs.h.90BBE444E34D4054.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/exp_integral.tcc.F9DAD445B33C7502.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/filesystem.3026D86F314E1C19.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/format.549B6319B3A4F68F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/fs_dir.h.E798AA0514F2B3A0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/fs_fwd.h.055B1B66B3F5654C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/fs_ops.h.3DFD500BCAF01E63.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/fs_path.h.B445556847A20F00.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/functexcept.h.12CCFABDF67F4460.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/functional.37EA17A6A664873A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/functional_hash.h.4E719005803AD176.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/gamma.tcc.88A5555BE7ABDD24.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/glue_algorithm_defs.h.71BB831B01670F96.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/glue_memory_defs.h.F26B660F9A3F16E4.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/glue_numeric_defs.h.F9174BDA4C8A3C7C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/gthr-default.h.ACB691495DBDFDB5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/gthr.h.4ECCE1C9BDB1427D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/hash_bytes.h.BBBB1142DC68BC57.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/hashtable.h.3AA895569B129D9A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/hashtable_policy.h.37E8F9255387718D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/hypergeometric.tcc.961440F86360CCDD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/image_movement_widget.cpp.C991EC080CE479FE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/image_movement_widget.h.6CF46E2A29BFAE3A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/initializer_list.2BA437BE1531E285.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/invoke.h.47EED4590895C57A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/iomanip.43D14F6B6CDA34A9.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ios.DF0F8FC125E9DA05.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ios_base.h.BFD16D6E378B39E1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/iosfwd.9340073C5109DE02.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/istream.C85CEAF05ECC288D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/istream.tcc.D51E8569006BA9B7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/iterator.AF06A9A0E0671584.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/iterator_concepts.h.CF404F64F7A894B7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/legendre_function.tcc.31868A682204A4FA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/limits.D451CBEFD886976F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/limits.h.19CF69E111D296D2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/limits.h.C41708D448ABFA1C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/list.0CA27D43B3F57D34.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/list.tcc.A2674ED28FEC1A09.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale.E458612ADC7E5636.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale.h.807566621B5B7E9B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale_classes.h.592A51CE6E868713.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale_classes.tcc.8C106B02C2B98181.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale_conv.h.9080CBDBC4C6C641.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale_facets.h.19BC8E5331B657DC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale_facets.tcc.21FD61C03DF7D230.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale_facets_nonio.h.59468C1C7FBA1703.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/locale_facets_nonio.tcc.63E79E9E243B3BD9.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/localefwd.h.11DAF501161E56C8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/main.cpp.2AFE55D3196FBFBC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/malloc.h.329AFFA5039D8A83.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/map.E1FD9F43D8016195.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/mapwidget.cpp.FC45F9C456E39E5B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/mapwidget.h.DC4BB8853FF5DCC3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/math.h.09AE8666EAEE972E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/max_size_type.h.FF3074791D28C396.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/memory.B738C39CF3FE4D41.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/memory_resource.h.2C7E595FE70BB7C6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/memoryfwd.h.2F5F536B7E11442E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/messages_members.h.0227E3D461BE1D57.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/mm_malloc.h.3CCA3A1387BFFB32.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/modified_bessel_func.tcc.83A9DA56B27CBC22.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/mofunc_impl.h.1D725E989C1D0F1C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/move.h.716C08CA8C8EAE84.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/move_only_function.h.649F4F2021CD177D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/nested_exception.h.40FF4B1E5FB719FB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/new.D5B2BD61B462EF50.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/new_allocator.h.A06CC8464190FD96.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/node_handle.h.C8866507F88AD379.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/numbers.272957FCCDE53D76.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/numeric.1F9C92E24736C13F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/numeric_traits.h.CD7E65BB2C8D4FB5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/optional.3433DAB3C31B89D5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/os_defines.h.16984C92EBE45BCE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ostream.FAF3A469D77B9229.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ostream.tcc.A05F77452439D03B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ostream_insert.h.C9683995656E76D3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/parse_numbers.h.3E385C2440884A98.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/poly_hermite.tcc.BF7204C7C637172F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/poly_laguerre.tcc.16A37177699E29BE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/postypes.h.77699A2E176B9615.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/predefined_ops.h.2542983083F4B942.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/process.h.98058895BE3521C1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/pstl_config.h.1B8AFF09EA6FA94F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/pthread.h.7321656405FBDE3D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/pthread_compat.h.1A0F527992D03D7E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/pthread_signal.h.170F5B0DCB137599.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/pthread_time.h.15F66372A160793E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/pthread_unistd.h.836D238BBBE42D1B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ptr_traits.h.2082866D50BCF120.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/q20functional.h.80F508C9D3F93FA9.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/q20iterator.h.C9BBC1D861393ECC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/q20memory.h.5549914AF1189FAE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/q20type_traits.h.9D54A4620573ED86.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/q23utility.h.EC11ACA11558670E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qaction.h.15D9DB42A910CDFD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qalgorithms.h.EA2D50FFB96CA948.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qanystringview.h.D936CA314612CCF1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qapplication.h.DD2044ED5818BF9F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qarraydata.h.E7864E7453FD2624.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qarraydataops.h.4E3F77603B4DFD12.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qarraydatapointer.h.B8DA0EE7648BA675.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qassert.h.FA304E8D42ACD077.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qatomic.h.205DD18751559C04.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qatomic_cxx11.h.C354E29303389C6D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbasicatomic.h.7434478627F3279A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbasictimer.h.A41EABFD1866668B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbindingstorage.h.BE0D47757F60D3B4.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbitmap.h.5353948FF931BA34.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbrush.h.3DC209A583CD44A0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbytearray.h.3A4F9640CA470EB5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbytearrayalgorithms.h.E43FAC8A4317BC45.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbytearraylist.h.93F96308C6D8F066.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qbytearrayview.h.329C42B0488CBC3A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcalendar.h.17571AE1AA70C7A5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcborcommon.h.384592051276DFE0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcborvalue.h.32CD2C0026751B56.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qchar.h.26A453F2F7311D3C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcolor.h.B96A93785F9716ED.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcompare.h.50B5204150FA09B1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcompare_impl.h.7C16ADF8A758D7CC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcomparehelpers.h.FB903A5603F43232.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcompilerdetection.h.BB22B21D58E3B93C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qconfig.h.014EEFF084E2829A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qconstructormacros.h.C4E279052EB44298.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcontainerfwd.h.A8B5560DA8D1731D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcontainerinfo.h.70D5FD6E236F8DCA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcontainertools_impl.h.3CA33911D4449911.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcontiguouscache.h.3C2B7FAAF3024535.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcoreapplication.h.7B2504D1136BD8F8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcoreapplication_platform.h.05A0063DB337F544.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcoreevent.h.3BC6BF5373358E8E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qcursor.h.860334C167526538.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qdarwinhelpers.h.A836EAE7C05CBC05.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qdatastream.h.C3D8BDDA66629C92.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qdatetime.h.371E57379C0FDD8C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qdeadlinetimer.h.3B90011FD7781F46.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qdebug.h.47538069A1ED52C3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qelapsedtimer.h.765F6D59A889F491.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qendian.h.309791605901704C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qevent.h.F6E44761BE138395.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qeventloop.h.A8EFE763607509FD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qeventpoint.h.761373A5E4E72D54.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qexceptionhandling.h.D7F794A6EC64C14B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfile.h.DA14440B1808E9F3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfiledevice.h.D78808A4A57D2D26.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfileinfo.h.5A9F19A91FBECF72.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qflags.h.E9722C78807AC673.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfloat16.h.E86F5D34D46D7EA5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfont.h.74D920E67241C375.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfontinfo.h.F10479385FA00437.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfontmetrics.h.2673B33460718554.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qforeach.h.96AD8F7B1488A21F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qframe.h.5D4ADE564CBB170E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfunctionaltools_impl.h.22056B6E7B34305F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qfunctionpointer.h.1066AE75864DE266.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qgenericatomic.h.B23F40CB3B8403EE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qglobal.h.708AE8EE01A2B246.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qglobalstatic.h.992898381A53EA15.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qguiapplication.h.9EA1344CE99F42BD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qguiapplication_platform.h.974FE0151457BBA6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qhash.h.90C41B72DAF3FA9A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qhashfunctions.h.505CF33B68252E5E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qicon.h.8CA9A97E8E8A546D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qimage.h.B7A8410F6F614391.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qinputdevice.h.89E2CFCA9A5EE745.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qinputmethod.h.2EA6790C8ED31E76.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qiodevice.h.46E762CCCBF22359.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qiodevicebase.h.950AC8D2C6CCB9F7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qiterable.h.68B88B656C1BA248.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qiterator.h.309E99593ADCA2DD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qjsonarray.h.EE588369BC127502.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qjsondocument.h.13467571D2EAE402.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qjsonobject.h.8A8450903D335A6A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qjsonvalue.h.5FA8240A4FA8B264.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qkeysequence.h.1EBFBDC90B2FD515.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qlabel.h.20EDDF5B0FA1C1C8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qlatin1stringview.h.E78491AC8AD5CD8E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qline.h.39B24829172140A3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qlist.h.A0D0D6FFE05BEF28.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qlocale.h.795E73D99C2F6B15.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qlogging.h.C99665610449DF98.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qmalloc.h.AEB75040104D225E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qmap.h.0DCAFBB6991AAA14.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qmargins.h.0984782C26BE3EE7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qmath.h.F0C4CD05016A6A05.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qmetacontainer.h.0ED4CCF788DAC6C2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qmetatype.h.D38B45918312649C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qminmax.h.514A6D61C6D120FB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qnamespace.h.A481BF624B74F528.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qnativeinterface.h.F12F5CF553A2B7D2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qnumeric.h.42D4F5AC1DF9B8C7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qobject.h.2C5062FBCD80CE3E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qobject_impl.h.4B2626CE4DCB2E0C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qobjectdefs.h.AA0E53E345178842.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qobjectdefs.h.D8599C372842B5A4.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qobjectdefs_impl.h.AEB79260FC886DD2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qoverload.h.0F00ED87B82AF3CF.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpaintdevice.h.E53EB2CEECC3FE23.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpainter.h.912D2A82306BA2E8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpair.h.A2A09C9AF219E4E5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpalette.h.D3D939286044F24F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpen.h.6448356374510264.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpicture.h.740C4E1FF10330D8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpixelformat.h.75679C1D61B216E2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpixmap.h.2EE27FDC9540DF5E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpoint.h.FCDC300A69C38469.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpointingdevice.h.E2595E601887D096.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qpolygon.h.87261112EE82D353.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qprocessordetection.h.BFC244A60E81D8A5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qrect.h.7BDF348BD5D3FF22.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qrefcount.h.B817D513BBBE2FDB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qregion.h.7FA7CA4EA6DA06EA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qregularexpression.h.678319B23108944D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qrgb.h.E37CC972C0D5A85B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qrgba64.h.CA37770B69F9BFC6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qscopedpointer.h.7F05EE22E1E58C54.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qscopeguard.h.99E40207899DDABD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qscreen.h.9998B0BADD42BCD6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qscreen_platform.h.AE0FDFDC713D81C5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qset.h.C084E713A592562F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qshareddata.h.9565D9C15EA61E31.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qshareddata_impl.h.955D92549FEB6D98.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qsharedpointer.h.29854E61615B2341.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qsharedpointer_impl.h.FB269B4890E288A2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qsize.h.28F69F9F0ABAB3F1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qsizepolicy.h.8C3614D7241620FA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qspan.h.34942DAC424046A5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstring.h.32B1AEFC643444ED.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringalgorithms.h.ED41AABFF28308C0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringbuilder.h.DE83AD91BA30FBA2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringconverter.h.1A66225780F80113.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringconverter_base.h.07E4AF9247F3EE42.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringfwd.h.22C7D40506D0E4FA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringlist.h.E794296BEB4BDBA7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringliteral.h.1A423E5332ECE350.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringmatcher.h.E143098F7FFD3561.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringtokenizer.h.0C787C821E641207.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qstringview.h.5B3341C43817F50A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qswap.h.D33A3C0A95ABE461.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qsysinfo.h.71B7207860E0A725.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qsystemdetection.h.44C8505694F2CB08.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtaggedpointer.h.E2D8B9DD403F74E7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtclasshelpermacros.h.C453C5BEDACA7270.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtconfiginclude.h.5E6166D192D9856A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtconfigmacros.h.5AD8933415C98579.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtcore-config.h.0F7C04938EB9A0E0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtcoreexports.h.64F668716E3E06E5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtdeprecationdefinitions.h.1C6C470FA8618D7C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtdeprecationmarkers.h.1E708A4966C044CC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtenvironmentvariables.h.D336383DBE28CDDA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtextdocument.h.57E24F3B8D55AD50.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtextoption.h.0C6743D0B7BB2217.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtextstream.h.E57EF3238E9489EA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtgui-config.h.F56F802837D4715D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtguiexports.h.E972C5488CEC2162.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtguiglobal.h.876F8E3B214933EB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtimer.h.8D5D068E8AB125FC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtimezone.h.DCBDB5E133E9E469.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtmetamacros.h.D8CB690A6CB87359.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtnoop.h.C9A858064D321539.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtpreprocessorsupport.h.BEE458FE4BA55567.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtransform.h.C300979E97EDFAA2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtresource.h.0C4758484A5E6B73.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qttranslation.h.9958E910C4BA1D5D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qttypetraits.h.C4C772B67BD50E3A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtversion.h.B0C147F49A7C62BB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtversionchecks.h.812AD6FE891E4496.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtwidgets-config.h.F8226543F02C44C1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtwidgetsexports.h.B0E20F9CE1754BD1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtwidgetsglobal.h.D869C50E0CA1AB68.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtypeinfo.h.D925CCD977E8E83F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qtypes.h.FB8AFA9DD1D710C3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/quoted_string.h.0CF8E6187E67DECF.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qurl.h.7D47313C095503A8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qutf8stringview.h.903D59B7308A7A62.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/quuid.h.643A2CCDE52020F7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qvariant.h.4FF723458AEBBC7D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qvarlengtharray.h.7C09EBAA68E2CA44.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qvector.h.19EF874040233364.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qvector2d.h.4F43845F257774CF.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qvectornd.h.0FA061665DDA10B8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qversiontagging.h.7E5BF22ACB5D6768.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qwidget.h.99A3836E99102BE3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qwindowdefs.h.32D667226B342EE8.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qwindowdefs_win.h.C7DEEB806E81F7B3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qxmlstream.h.CD2E662483676200.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qxptype_traits.h.A6CBB8B38F7CB62B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/qyieldcpu.h.77B86AB451852216.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/range_access.h.E43B39B30A98150C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ranges_algo.h.4548781C0B1B4567.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ranges_algobase.h.2F7B717F6AA36A4A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ranges_base.h.F60DB2F9F8D8BEE7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ranges_cmp.h.0CE13F03DE620284.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ranges_uninitialized.h.72CF713724BE6338.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ranges_util.h.76CB834A07FFE72A.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ratio.E5A6EF43017FB912.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/refwrap.h.37AB7E06935CFA2C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/requires_hosted.h.3B4BC5C7D813A41B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/riemann_zeta.tcc.A1352DD294B30770.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/rpg.cpp.F357CDB28A9CA6B5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/rpg.h.D3A1EEFC1AE611F0.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/shared_ptr.h.DA67F7F8659F42BD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/shared_ptr_atomic.h.BA5EDB2E910A6B69.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/shared_ptr_base.h.D10A3AE9585197F9.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/signal.h.29BFE3FE5D93F48E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/span.784A525709F06426.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/specfun.h.F02557FA03D8DB49.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/special_function_util.h.D99F0AF9419A5733.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/sstream.48158D4B3AA9E76D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/sstream.tcc.B8DC57BC3E8CF329.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/std_abs.h.34577278CB89DFCA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/std_function.h.FD55234689A29346.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/std_mutex.h.4090DF23BF647C95.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdarg.h.AFAB9E67AE1745B4.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdbool.h.359737F0AB4A9FC7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stddef.h.445A0E908FD14E68.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdexcept.4707C8A1F3C2DFD4.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdint.h.14CB15B9F7CA95F3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdint.h.487BA7BDCF10ECE1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdio.h.8A4F2ED9132D66B6.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdio_s.h.6EE9F51D48AC4746.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdlib.h.18862E50B727D4E1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdlib.h.3D3F0F92ED7C377B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stdlib_s.h.EC3DED0E5CA82733.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_algo.h.5F2E0710EEEA4817.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_algobase.h.627C83A1D850F6B2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_bvector.h.57EA764E89F13177.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_construct.h.D740D9A1DE80244B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_function.h.59A70F32F7A369CD.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_heap.h.D63C25B1DCE66C3C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_iterator.h.F3CA840818D341DB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_iterator_base_funcs.h.19559D9A5D66602E.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_iterator_base_types.h.BD152D5DBC75F61D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_list.h.DA7AB9EFC8EF4001.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_map.h.B2F1089B00596A77.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_multimap.h.B4817D8DBD88C55B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_numeric.h.527896DFCBCCD972.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_pair.h.729142C99E64AE44.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_raw_storage_iter.h.52F26DF523CC3C4C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_relops.h.6163A32D00EC660C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_tempbuf.h.C00FD7382303C1FE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_tree.h.C65729A984CDDE51.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_uninitialized.h.C9F4C6EAF17E00BB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stl_vector.h.BCBF33A27C647160.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stream_iterator.h.EC056366D89171EE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/streambuf.F6FDBC0EDB5AACAB.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/streambuf.tcc.3FD889C2A4DC0AC2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/streambuf_iterator.h.263ADC499B5A1678.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/string.CDC9B6559AB040FA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/string.h.33283A0BB363EEFA.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/string_conversions.h.D1FE8BCD1538DFD1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/string_s.h.C56DFE9899B65DF7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/string_view.CE0E4EF1F788C0C7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/string_view.tcc.010FA1449D776409.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/stringfwd.h.6E2FC55FFFF11199.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/swprintf.inl.A9E72B120C2C1EF3.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/system_error.533DA6FBAE7C44F5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/time.h.C6A2B38F69FCB763.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/time_members.h.73668FD7A3785700.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/timeb.h.F58DA8A8888C0B72.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/timeb_s.h.4E4AB86B5CF18D2B.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/tuple.12873A7F047C85EC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/type_traits.09DF29005738E504.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/type_traits.h.E19DB54F699E0A55.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/typeinfo.17A4BDDCC47C1CAF.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/types.h.165E8E3C11EC3876.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ui_image_movement_widget.h.ABE043DA0E91AC11.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/ui_rpg.h.24DDD54950026A6F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/uniform_int_dist.h.3A3AB8FE8B79466D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/unique_ptr.h.F31022128EAC3375.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/unordered_map.80227D558A1CAFE2.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/unordered_map.h.3DEC13E391959470.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/uses_allocator.h.AA92A8CEE12F1158.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/uses_allocator_args.h.8234DD8286E975E1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/utility.C0F697E817E71774.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/utility.h.88284CA241F3BD79.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/vadefs.h.C390784D283D2D8C.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/vadefs.h.F8031701AF2D46FC.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/variant.C65D078AA6FAE965.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/vector.BCB7442F96BC0DA5.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/vector.tcc.BF38D1B0CB557FE7.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/version.61A10FF8EFAA7C21.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/wchar.h.48515F487674F28D.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/wchar_s.h.2838C470DBF7BBE1.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/wctype.h.61FEC1B562FDE295.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/widget.cpp.C7C11C59BCEA216F.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/.cache/clangd/index/widget.h.42846AF3D3F163FE.idx
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/.qtc_clangd/compile_commands.json
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/Makefile
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/Makefile.Debug
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/Makefile.Release
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/CHECK.exe
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/character.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/collisiondetector.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/image_movement_widget.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/main.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/mapwidget.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/moc_image_movement_widget.cpp
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/moc_image_movement_widget.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/moc_mapwidget.cpp
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/moc_mapwidget.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/moc_predefs.h
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/moc_rpg.cpp
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/moc_rpg.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/qrc_character.cpp
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/qrc_character.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/debug/rpg.o
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/ui_image_movement_widget.h
build/Desktop_Qt_6_8_2_MinGW_64_bit-Debug/ui_rpg.h
character.h
character.qrc
collisiondetector.cpp
collisiondetector.h
docs/ASSET_AUDIT.md
docs/DEVELOPMENT.md
docs/GAMEPLAY_DESIGN.md
docs/PROJECT_PLAN.md
gameclock.cpp
gameclock.h
gameconstants.h
itembar.cpp
itembar.h
itembar/itembar0.png
itembar/itembar1.png
itembar/itembar2.png
itembar/itembar3.png
itembar/itembar4.png
itembar/itembar5.png
mainwindow.cpp
mainwindow.h
mainwindow.ui
map.tiled-session
map6.tmj
map7.tiled-project
map7.tiled-session
map7.tmj
map8.tmj
mapdata.cpp
mapdata.h
maprenderer.cpp
maprenderer.h
mapwidget.cpp
mapwidget.h
playercontroller.cpp
playercontroller.h
rpg.cpp
rpg.h
sprout/Basic_Furniture.png
sprout/Basic_Grass_Biom_things.png
sprout/Basic_Plants.png
sprout/Basic_tools_and_meterials.png
sprout/Catpaw Mouse icon.png
sprout/Catpaw holding Mouse icon.png
sprout/Chest.png
sprout/Egg_item.png
sprout/Farming Plants.png
sprout/Fences.png
sprout/Free_Chicken_House.png
sprout/Grass.png
sprout/Grass_Hill_Tiles_Slopes v.2.png
sprout/Grass_tiles_v2.png
sprout/Mushrooms, Flowers, Stones.png
sprout/Paths.png
sprout/Simple_Milk_and_grass_item.png
sprout/Sprite sheet for Basic Pack.png
sprout/Sprout Lands - Sprites - Basic pack/Characters/Basic Charakter Actions.png
sprout/Sprout Lands - Sprites - Basic pack/Characters/Basic Charakter Spritesheet.png
sprout/Sprout Lands - Sprites - Basic pack/Characters/Egg_And_Nest.png
sprout/Sprout Lands - Sprites - Basic pack/Characters/Free Chicken Sprites.png
sprout/Sprout Lands - Sprites - Basic pack/Characters/Free Cow Sprites.png
sprout/Sprout Lands - Sprites - Basic pack/Characters/Tools.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Basic_Furniture.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Basic_Grass_Biom_things.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Basic_Plants.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Basic_tools_and_meterials.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Chest.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Egg_item.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Free_Chicken_House.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Paths.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Simple_Milk_and_grass_item.png
sprout/Sprout Lands - Sprites - Basic pack/Objects/Wood_Bridge.png
sprout/Sprout Lands - Sprites - Basic pack/Sprout Lands color pallet/Sprout Lands defautlt palette snip.PNG
sprout/Sprout Lands - Sprites - Basic pack/Sprout Lands color pallet/Sprout Lands defautlt palette.aseprite
sprout/Sprout Lands - Sprites - Basic pack/Sprout Lands color pallet/Sprout Lands defautlt palette.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Bitmask references 1.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Bitmask references 2.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Bitmask references gif.gif
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Doors.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Fences.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Grass.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Hills.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Tilled_Dirt.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Tilled_Dirt_Wide.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Tilled_Dirt_Wide_v2.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Tilled_Dirt_v2.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Water.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Wooden_House_Roof_Tilset.png
sprout/Sprout Lands - Sprites - Basic pack/Tilesets/Wooden_House_Walls_Tilset.png
sprout/Sprout Lands - Sprites - Basic pack/read_me.txt
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken/Chicken_Animations_AsepriteFile.aseprite
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken/Chicken_Animations_GifPreview.gif
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken/Free Chicken Sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken/chicken blue.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken/chicken brown.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken/chicken default.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken/chicken green.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken/chicken red.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Baby/Chicken_Baby.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Baby/Chicken_Baby_Animations_AspriteFile.aseprite
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Baby/Chicken_Baby_Animations_GifPreview.gif
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Baby/Chicken_Baby_Blue.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Baby/Chicken_Baby_Brown.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Baby/Chicken_Baby_Green.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Baby/Chicken_Baby_Red.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Egg/Chicken_Egg_Animations_AspriteFile.aseprite
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Egg/Chicken_Egg_Animations_GifPreview.gif
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Egg/Egg_Spritesheet.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Egg/Egg_Spritesheet_Brown.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Egg/Egg_Spritesheet_Green.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Egg/Egg_Spritesheet_blue.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Chicken_Egg/Egg_Spritesheet_red.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow/Brown cow animations.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow/Free Cow Sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow/Green cow animation sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow/Light cow animations.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow/Pink cow animation sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow/Purple cow animation sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow/all cow animations.aseprite
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow/all cow animations.gif
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow_Baby/all baby cow animations.aseprite
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow_Baby/all baby cow animations.gif
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow_Baby/baby brown cow animations sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow_Baby/baby green cow animations sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow_Baby/baby light cow animations sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow_Baby/baby pink cow animations sprites.png
sprout/Sprout Lands - Sprites - premium pack/Animals/Cow_Baby/baby purple cow animations sprites.png
sprout/Sprout Lands - Sprites - premium pack/Characters/Basic Charakter Actions.png
sprout/Sprout Lands - Sprites - premium pack/Characters/Charakter Spritesheet and animations.aseprite
sprout/Sprout Lands - Sprites - premium pack/Characters/Premium Charakter Spritesheet.png
sprout/Sprout Lands - Sprites - premium pack/Characters/Tools.png
sprout/Sprout Lands - Sprites - premium pack/Characters/water from water can frames and animations.aseprite
sprout/Sprout Lands - Sprites - premium pack/Characters/water from wateringcan frames.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Boats.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Farming Plants.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/egg-items.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/farming-Plants-items.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/fruit-n-berries-items.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/grass-n-ground-tile-items.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/item-shadow.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/items-spritesheet.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/milk-items.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/milk-n-grass-item-simple.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Items/tools-n-meterial-items.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Mushrooms, Flowers, Stones.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Piknik basket.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Piknik blanket.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/Fruit animations without tree/no_tree_appel_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/Fruit animations without tree/no_tree_orange_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/Fruit animations without tree/no_tree_peach_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/Fruit animations without tree/no_tree_pear_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree animations.aseprite
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree fall Preview animation.gif
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree preview animations.aseprite
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree preview animations.gif
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree_appel_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree_fall_animation_sprite_sheet.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree_orange_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree_peach_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree_pear_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Tree animations/tree_sprites.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Trees, stumps and bushes.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Water Objects.png
sprout/Sprout Lands - Sprites - premium pack/Objects/Water well.png
sprout/Sprout Lands - Sprites - premium pack/Objects/signs.png
sprout/Sprout Lands - Sprites - premium pack/Objects/signs_sides.png
sprout/Sprout Lands - Sprites - premium pack/Objects/work station.png
sprout/Sprout Lands - Sprites - premium pack/Sprout Lands color pallet/Sprout Lands defautlt palette snip.PNG
sprout/Sprout Lands - Sprites - premium pack/Sprout Lands color pallet/Sprout Lands defautlt palette.aseprite
sprout/Sprout Lands - Sprites - premium pack/Sprout Lands color pallet/Sprout Lands defautlt palette.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Animal Structures/Barn structures.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Animal Structures/Chikcen_Houses.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Animal Structures/Water tray.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Basic_Furniture.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Chest.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Fence gates animation sprites .png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Fences.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Mailbox Animation Frames.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Paths.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Stone_Path.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Wooden_Bridge.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Wooden_Bridge_v2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Wooden_House_Roof_Tilset.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/Wooden_House_Walls_Tilset.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/door animation sprites.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/Building parts/mailbox animation.aseprite
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Bitmask references 1.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Bitmask references 2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Bitmask references gif.gif
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Bush_Tiles.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Darker_Grass_Hill_Tiles_Slopes_v2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Darker_Grass_Hills_Tiles_v2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Darker_Grass_Tile_Layers.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Darker_Grass_Tile_Layers2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Darker_Grass_Tiles_v2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Darker_Soil_Ground_Hills_Tiles.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Darker_Soil_Ground_Tiles.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Grass_Hill_Tiles_Slopes v.2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Grass_Hill_Tiles_v2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Grass_Tile_Layers.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Grass_Tile_layers2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Grass_tiles_v2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Ground_Hill_Tiles_Slopes.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Soil_Ground_HiIls_Tiles.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Soil_Ground_Tiles.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Stone_Ground_Hills_Tiles.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/Stone_Ground_Tiles.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/TILE LAYER EXAMPLE.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/TILES PREVIEW v.2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/TILES PREVIEW.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Corner_NortWest.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Corner_NorthEast.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Corner_SouthEast.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Corner_SouthWest.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Edge_NorthEast.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Edge_NorthWest.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Edge_SouthEast.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Edge_SouthWest.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Flat_North.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Flat_South.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Flat_West.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Flowers1.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Flowers2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Grass1.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Grass2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Moss2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Moss3.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Moss4.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Sprouts1.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Sprouts2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Sprouts3.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_Mid_Sprouts4.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass tiles v2 simple cutout/Grass_tiles_v2_mMid_Moss1.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/New tiles/simpel versions/Grass_tiles_v2_simple.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Old tiles/Grass.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Old tiles/Hills.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Old tiles/Tilled_Dirt.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Old tiles/Tilled_Dirt_Wide.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Old tiles/Tilled_Dirt_Wide_v2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/Old tiles/Tilled_Dirt_v2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/water frames/Water animation.aseprite
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/water frames/Water.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/water frames/Water_1.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/water frames/Water_2.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/water frames/Water_3.png
sprout/Sprout Lands - Sprites - premium pack/Tilesets/ground tiles/water frames/Water_4.png
sprout/Sprout Lands - Sprites - premium pack/read_me.txt
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/Emotes/Teemo Basic emote animations sprite sheet.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/Premade dialog box  big.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/Premade dialog box medium.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/Premade dialog box small.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/dialog box big.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/dialog box character finished talking click to continue indicator - spritesheet .png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/dialog box medium.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/dialog box small.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Dialouge UI/dialog box.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Icons/All Icons.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Icons/special icons/Small Happines-Sadness icons.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Icons/special icons/Special Icons.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Icons/white icons.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Mouse sprites/Catpaw Mouse icon.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Mouse sprites/Catpaw holding Mouse icon.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Mouse sprites/Catpaw pointing Mouse icon.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Mouse sprites/Triangle Mouse icon 1.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Mouse sprites/Triangle Mouse icon 2.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Mouse sprites/Triangle Mouse icon 3.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Setting menu.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/Sprite sheet for Basic Pack.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/UI Big Play Button.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/UI Settings Buttons.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/buttons/Small Square Buttons.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/buttons/Square Buttons 19x26.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/buttons/Square Buttons 26x19.png
sprout/Sprout Lands - UI Pack - Basic pack/Sprite sheets/buttons/Square Buttons 26x26.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/Emoji_Spritesheet_Free.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/Inventory_Blocks_Spritesheet.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/Inventory_Herat_Spritesheet.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/Inventory_Light_Herat_Spritesheet.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/Inventory_Light_example_with_slots_2.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/Inventory_Spritesheet.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/inventory_Light_example.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/inventory_Light_example_2.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/inventory_Light_example_with_slots.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/inventory_example.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/inventory_example_2.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/inventory_example_with_slots.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/inventory_example_with_slots_2.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/weather/Weather_Icons_smal_freel.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/emoji style ui/weather/Weather_UI_Free.png
sprout/Sprout Lands - UI Pack - Basic pack/emojis-free/speech_bubble_grey.png
sprout/Sprout Lands - UI Pack - Basic pack/fonts/pixel-letters-7-8x14-preview.png
sprout/Sprout Lands - UI Pack - Basic pack/fonts/pixel-letters-7-8x14.png
sprout/Sprout Lands - UI Pack - Basic pack/fonts/pixelFont-7-8x14-sproutLands.ttf
sprout/Sprout Lands - UI Pack - Basic pack/read_me.txt
sprout/Tilled_Dirt_Wide_v2.png
sprout/Trees, stumps and bushes.png
sprout/Water Objects.png
sprout/Water.png
sprout/Wood_Bridge.png
sprout/Wooden_Bridge.png
sprout/clockbackground.png
sprout/cropnum.png
sprout/cropnum0.png
sprout/pixel-letters-7-8x14.png
sprout/pixelFont-7-8x14-sproutLands.ttf
tests/core_tests.cpp
tests/tests.pro
tooltype.h
tsx/Basic_Grass_Biom_things.tsx
tsx/Farming Plants.tsx
tsx/Fences.tsx
tsx/Grass.tsx
tsx/Grass_Hill_Tiles_Slopes v.2.tsx
tsx/Grass_tiles_v2.tsx
tsx/Mushrooms, Flowers, Stones.tsx
tsx/Tilled_Dirt_Wide_v2.tsx
tsx/Trees, stumps and bushes.tsx
tsx/Wood_Bridge.tsx
wav/Audacity_Installer_via_MuseHub.exe
wav/On_the_Farm.mp3
wav/On_the_Farm.wav
worldstate.cpp
worldstate.h
```

</details>
