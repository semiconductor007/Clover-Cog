# 五人模块职责

工程提案 v0.2；责任人暂用成员 A–E，实际姓名/账号待登记。一级开发责任模块恰好五个。各阶段任务编号和依赖以 [执行计划](IMPLEMENTATION_PLAN.md) 为准，签名以 [接口契约](INTERFACE_CONTRACTS.md) 为准。

## A：游戏核心与存档系统

- 目标：唯一活动状态、稳定命令入口、可持续集成和可靠恢复。
- 范围：GameState、公用数据/错误码、GameClock、GameController、GameRuntime、DefinitionCatalog 加载基础设施、SaveManager、qmake、CI 与发布集成。
- 禁止：在控制器里重写采收、容量、价格、运输规则；接管 B–E 内部实现；因“通用性”引入框架。
- 接口：IC-01 状态/事务、IC-02 命令、IC-06 时钟、IC-07 配置、IC-08 保存恢复。
- 依赖：core 类型无 UI；GameRuntime 组合 B/D/E 和 C 的纯移动规则；SaveManager 只处理完整数据和校验，不做第二套离线结算。
- 现有类：GameClock、GameConstants；原 MainWindow 组装/计数部分需 C 配合移交。
- 新类：GameState、GameController、GameRuntime、DefinitionCatalog、SaveManager。
- 专属：src/core、src/persistence、src/gameplay/gamecontroller.*；build 配置、tests/core、tests/persistence；详细逐文件见目录表。
- 共享修改窗口：mainwindow.cpp 内组装由 C 主责，A 提交组装需求；CHECK.pro、tests/tests.pro、公共类型由 A 直接修改，消费方审查。
- 交付/测试：契约头、候选事务、时间/暂停、配置报错、存档备份/恢复、构建日志；A 对集成门禁负责，模块测试仍由各成员负责。
- 阶段：M0 接口/时钟/状态；M1 存档/集成；M2 兼容性；M3 离线协调与存档版本；M4 部署。

## B：农田与作物系统

- 目标：不依赖 QWidget/GID 的确定性农田规则。
- 范围：WorldState 的语义农田/树木状态、FarmSimulation、作物配置内容、耕作/播种/生长/成熟/再生/收获、土壤、扩地合法性和范围目标规则。
- 禁止：直接增背包数字或金币、自己维护库存副本、调度自动机器、在 renderer 判定成熟。
- 接口：IC-03；收获调用 D 的 IC-04；读取 A 的时间/定义/快照，向 E 暴露同一采收接口。
- 现有类：WorldState；旧 MapWidget 中尚存的业务由 C 在窗口内移交。
- 新类：FarmSimulation；CropDefinition/PlotState 在 A 公共头中定义，由 B 审查；B 不自行建重复类型。
- 专属：src/gameplay/farm、data/crops.json、tests/farm。
- 共享：公共农田类型由 A 主责、B 必审；MapData 的语义导入由 C 负责，B 提供校验需求。
- 交付/测试：独立作物状态、可控时间生长、整格不丢货、稳定范围顺序、升级后进度与产量快照、再生测试。
- 阶段：M0 WorldState 语义化；M1 两作物/安全采收/首次扩地；M2 四种行为/灌溉/范围；M3 支援 E 复用规则；M4 调参回归。

## C：角色、地图呈现与 UI 系统

- 目标：保留当前像素观感和角色基础效果，让业务状态可读可操作。
- 范围：MainWindow、MapWidget、MapRenderer、MapLoader、ImageMovementWidget、PlayerController 输入适配、碰撞/纯移动规则、ItemBar、HUD/背包/商店/交易面板、音频、资源、字体和地图语义导入。
- 禁止：持有可写真实库存/金币/等级；按钮重复结算；用动画完成回调发奖励；客户端自行传入授权位置或自动化权限。
- 接口：IC-02 命令与快照；IC-09 地图定义/只读碰撞/移动结果。
- 现有类：mainwindow/mapwidget/mapdata/maprenderer/rpg/playercontroller/collisiondetector/itembar；ClockBgWidget、ToolAnimation。
- 新类：MoveRules、ViewTransform、GamePanels（先合并背包/交易/商店面板文件）、AssetCatalog。可按实际复杂度再拆面板，避免空类堆积。
- 专属：src/ui、src/gameplay/player、resources（仅未来新资源）、现有图集/地图/QRC、tests/ui、tests/player。
- 共享：MainWindow 组装主责 C，A 必审；character.qrc 主责 C，A 检查打包；tooltype.h 拆分期间 C 与 A/B/D 协调。
- 交付/测试：一次输入一次命令、坐标与碰撞、面板屏蔽、资源加载清单、中文截图/GUI 验收。真实位置由 A 提交，C 保留纯移动算法所有权。
- 阶段：M0 输入/视图和拆分；M1 最小可用 HUD/交易/商店；M2 字体和批量反馈；M3 设施/货舱表现；M4 引导/部署试玩。

## D：经济与成长系统

- 目标：数量守恒、容量安全、交易原子、可配置成长。
- 范围：InventoryService、EconomyService、UpgradeDefinition/效果求值、背包/箱体/货舱通用规则、预留数量、出售/购买/解锁、经济配置校验。
- 禁止：画面板、直接改 B 地块或 E 运输阶段、根据 UI 位置信任交易、不经统一状态保留余额。
- 接口：IC-04 库存，IC-05 经济/升级；向 B/E 提供服务，向 C 提供只读报价/升级卡数据。
- 现有类：无完整服务；MainWindow 的计数在 C/A 迁移窗口接入库存。
- 新类：InventoryService、EconomyService；UpgradeDefinition/EffectiveEffects 是公共值类型，不再加一套独立状态管理器。
- 专属：src/economy、data/upgrades.json、data/economy.json、tests/economy。
- 共享：库存/经济/升级公用头由 A 主责、D 必审；升级关联农田/角色/设施时相应成员必审。
- 交付/测试：满包/负数/溢出/预留/事务回滚、到站出售、五强化配置及效果、库存转移守恒、重复输入安全。
- 阶段：M0 契约测试样例；M1 背包/出售/升级；M2 扩展升级和容量校验；M3 共用转移/代售结算；M4 实际到账与价格调参。

## E：自动化与物流系统

- 目标：在同一时间和规则上连接生产、存储、运输、售卖。
- 范围：AutomationSystem、TransportSystem、补种调度、设施覆盖/启停/吞吐、采收箱身份、代售箱批次、固定路线装运/到站结算、离线业务事件。
- 禁止：独立作物/经济/库存实现；远程卖个人背包；按渲染帧发收入；M0–M2 提前实现整套自动化。
- 接口：IC-10；调用 IC-03/04/05，依赖 A 的时钟/候选事务/保存字段。
- 现有类：无；不得把 Clock 或 WorldState 整体划给 E。
- 新类：AutomationSystem、TransportSystem；设施/路线/货舱状态由公共类型承载。
- 专属：src/logistics、data/facilities.json、tests/logistics、tests/integration、测试场景文档。
- 共享：物流公共类型由 A 主责、E 必审；自动化图片由 C 主责，E 定义表现状态；tests/tests.pro 的注册交 A。
- 交付/测试：满箱停工恢复、货物唯一归属、吞吐/稳定顺序、重复到站、在途恢复、在线离线同结果。
- 阶段：M0 审阅时间/事务契约和边界场景；M1 以真实 B/D API 写收获-出售集成测试，检查存档 fixture；M2 设计物流状态表/保存样例与瓶颈验收；M3 正式实现；M4 自动链路回归。

## 分工评估与负载管理

| 冲突热点 | 决策 |
| --- | --- |
| A 成为公共头/构建瓶颈 | 小接口 PR 先合；模块 .pri 各成员提交，A 只审顶层注册；E 承担跨模块测试而非 A 写所有测试 |
| C 原型文件最多且 UI 工作重 | M0 冻结装饰改造，M1 用现有素材与最小面板，M2 才做完整中文/窗口适配；经 C 授权可让空闲成员支援一个独占文件，审查仍归 C |
| B 与 E 补种/灌溉重叠 | B 执行植物规则，E 安排机器事件/提供覆盖输入；B 不包含 E 头，E 可调用 B |
| D 与 E 箱体重叠 | D 管库存容量数量，E 管设施身份/时机/路线；E 只保存 inventoryId，无第二份货物列表 |
| A 与 C 玩家位置 | C 的纯 MoveRules 计算合法位移；A 命令串行提交 PlayerState，角色控件只读显示 |
| 扩地跨 B/D/C | D 购得区域解锁，B 校验并应用明确地块集，C 可视化；A 同一候选事务编排 |

无需每人同时改主窗口。接口冻结前 B 可整理作物 fixture、C 可做坐标分析、D 可列容量交易用例、E 可写跨域验收场景；冻结后才按正式签名并行编码。不能用各自的临时金币/库存类来制造“并行”。
