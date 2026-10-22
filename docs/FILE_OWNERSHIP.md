# 文件所有权和修改权限

A–E 与五名成员一一对应，姓名/账号待填。主责可以在已认领任务分支直接修改专属实现；公共接口和共享文件必须按本表提前沟通。A 是默认合并/冲突协调人，不是其他模块的默认实现者。授权支援要在任务中写明文件和期限，主责和必审人不变。

## 所有权矩阵

| 文件/匹配范围（含未来路径） | 唯一主责/直接修改 | 需提前沟通/最终技术审查 | 合并 |
| --- | --- | --- | --- |
| src/core/*、gameclock.*、gameconstants.h | A | 涉及农田 B、库存经济 D、玩家 C、物流 E；至少一个直接消费方批准公共变更 | A；A 作者 PR 不能自审 |
| src/gameplay/gamecontroller.*、controller.pri、src/persistence/* | A | 存档由 E 复核跨域字段；业务调用对应 B/D/E/C | A |
| worldstate.*、src/gameplay/farm/*、data/crops.json | B | 契约变更 A + D/E；纯内部 A 或 E 审 | A |
| mainwindow.*（含 .ui）、src/ui/*、mapwidget.*、maprenderer.*、mapdata.* | C | 装配/状态迁移 A 必审；作物显示 B、交易面板 D、设施 E 按范围审 | A |
| rpg.*、playercontroller.*、collisiondetector.*、itembar.*、character.h、src/gameplay/player/* | C | 状态/输入 A，工具规则 B；纯视觉由其他成员核对截图 | A |
| tooltype.h | C（过渡期） | A/B/D；拆到 core 后该部分归 A，toolvisuals 仍 C | A |
| src/economy/*、data/upgrades.json、data/economy.json | D | B/E 消费规则，A 审资产一致性；价格调参附试玩记录 | A |
| src/logistics/*、data/facilities.json | E | B/D 复用规则，A 审时间/存档 | A |
| character.qrc、resources/ui.qrc、resources/fonts/*、data/render_mapping.json | C | A 校验构建，B/E 核对作物/设施映射 | A |
| resources/config.qrc | A | 数据文件主责 B/C/D/E 提供路径与 alias | A |
| 地图 TMJ/TSX、tsx/、Tiled 工程、全部现有图像/音频/素材包 | C | 地块/交易位置 B/D、路线 E；许可证文本保留来源 | A |
| CHECK.pro、tests/tests.pro、.github/*、.gitignore | A | 相关模块给变更清单，至少一名受影响成员审 | A |
| 各模块 .pri 与各域测试 .pro | 对应模块主责 | A 构建审查；包含新增路径需集成任务登记 | A |
| tests/core_tests.cpp、tests/legacy/*、tests/core/*、tests/persistence/*、公共 fixtures | A | 场景对应 B/C/D/E；拆迁前 A 单独操作 | A |
| tests/farm/*、fixtures/farm/* | B | A/E | A |
| tests/player/*、tests/ui/*、fixtures/ui/* | C | A | A |
| tests/economy/*、fixtures/economy/* | D | B/E | A |
| tests/logistics/*、tests/integration/*、fixtures/logistics/* | E | 被测模块 + A | A |
| AGENTS.md、README 两语言、DEVELOPMENT、ARCHITECTURE、TEAM_MODULES、DIRECTORY_STRUCTURE、INTERFACE_CONTRACTS、FILE_OWNERSHIP、IMPLEMENTATION_PLAN、GIT_WORKFLOW、TEST_STRATEGY | A | 文档涉及的模块必审；玩法变化按用户已确认范围处理 | A |
| PROJECT_PLAN.md、GAMEPLAY_DESIGN.md | A | B/D/E/C 依改动内容；不通过工程 PR 静默修改玩法 | A |
| ASSET_AUDIT.md | C | A 检查来源、状态表 | A |
| LICENSE、.trae/*、历史 build/*、CHECK.pro.user、*.tiled-session | A 保管 | 本轮不改删；后续清理 A 协调，C 核查资源/本机配置用途 | A |

没有列出的新文件在创建前由 A 指派所属 A–E，不新增第六模块。未来目录不存在时仍用原路径的主责；迁移表是目标所有权，不能据此同时占有两个旧文件。

## 三个高耦合文件窗口

| 窗口 | 认领文件 | 唯一编辑人 | 配合方式 / 退出条件 |
| --- | --- | --- | --- |
| W1 DEV-002 | mapwidget.*、playercontroller.*、rpg.*、collisiondetector.*、mainwindow.* | C | B 只提供规则测试；A 更新构建。一次输入/边界测试和 GUI 通过才释放 |
| W2 DEV-003 | worldstate.* 归 B；公共状态/控制器归 A；MapWidget/MainWindow/角色绑定归 C | 各自文件主责 | 按接口 PR→B 规则 PR→C 绑定集成 PR 排序；绑定 PR 合并前不能切换半套状态；旧路径暂停其他任务 |
| W3 分目录迁移 | 各模块自有旧文件，CHECK.pro、tests/tests.pro | 原主责、A | 每次只迁一组，冻结该组其他修改；新旧路径编译引用均核对，再释放锁 |

文件锁是 GitHub Issue/PR 中的任务认领记录：`任务、分支、基线 SHA、文件列表、接口版本、当前窗口、预计下一交接条件`。锁只针对文件和任务，不占用整个模块数周。完成合并/放弃任务要释放；禁止两个 Codex 在同一工作目录或同一分支同时写。

公共接口修改先文档 PR，再兼容实现 PR，再消费者迁移。CHECK.pro 由 A 汇总各 .pri 接入；主窗口由 C 消费 A 提供的 runtime 入口；QRC 由 C 按资源清单接入，A 复核；其他成员不能因本地编译方便直接改这些共享文件并覆盖他人结果。

这些是团队协作流程，不要求每次私有代码修改都请求用户确认。若当前任务已明确授权某次跨域修改，记录例外与受影响审查人即可；若改动超出任务范围，先完成本域工作并报告依赖。
