# Clover-Cog：所有 Codex 实例的工作约定

项目是五人协作的 Qt 传统农场种植游戏，一级责任域只有 A 核心/存档、B 农田、C 角色/地图/UI、D 经济/成长、E 内容/扩展。A–E 的实际成员账号待团队登记。用户本次任务的明确指令优先于本文件；工程方案不自动授权提交、推送或超范围开发。

## 开工必读

1. 本文件、[README](README.md)、[DEVELOPMENT](docs/DEVELOPMENT.md)。
2. [ARCHITECTURE](docs/ARCHITECTURE.md)、[TEAM_MODULES](docs/TEAM_MODULES.md)。
3. 本任务相关的 [INTERFACE_CONTRACTS](docs/INTERFACE_CONTRACTS.md)、[FILE_OWNERSHIP](docs/FILE_OWNERSHIP.md)、[IMPLEMENTATION_PLAN](docs/IMPLEMENTATION_PLAN.md)。
4. 迁移先读 [DIRECTORY_STRUCTURE](docs/DIRECTORY_STRUCTURE.md)，提交前读 [GIT_WORKFLOW](docs/GIT_WORKFLOW.md) 和 [TEST_STRATEGY](docs/TEST_STRATEGY.md)。

实际实现以源码/构建为准，需求以 README/DEVELOPMENT 优先，玩法细则再查 PROJECT_PLAN/GAMEPLAY_DESIGN，素材查 ASSET_AUDIT。计划目录和头文件草案不等于已经存在。发现矛盾须明确报告来源、实际代码和建议，不自行猜测覆盖已确认规则。

## 任务与文件边界

- 先检查工作区和基线提交，确认任务编号、所属模块、负责人、接口版本和迁移窗口。
- **动手修改前先给出本次文件清单**及必要的跨模块依赖。阅读和只读分析不需要等待确认。
- 只修改已认领任务的文件。跨域支援记录明确文件与责任人，按所有权安排审查；不能擅自实现其他成员功能或为编译方便覆盖公共头。
- 公共接口先提契约变更，由 A 与直接消费者确认；主窗口 C、顶层 qmake A、旧 QRC C，遵循各自修改流程。
- 不创建重复 GameState/Inventory/Clock/工具管理器，不因局部任务重构无关代码，不删除未理解的旧源码/资源。
- 每位成员独立分支/工作目录。禁止两个 Codex 同时编辑同一文件或共享未提交工作区。迁移窗口按文档认领和释放。
- 本轮工程规划只允许 Markdown 与本文件：不写业务、不移动/删除源码、不改构建/资源/数值、不装依赖、不提交推送。后续任务进入编码需有相应开发授权，不能仅因文档列了任务就自动执行。

## 技术与玩法约束

- 保留 C++17、Qt 6 Widgets、qmake、Multimedia、QPainter、Tiled、QRC、Qt Test；不自动迁移 CMake/其他框架。
- 保留现有 main()、地图、角色和像素风格，渐进迁移，每个阶段可编译验收；临时转发只委托新实现，不双写。
- UI 提交意图与展示快照，不能管理真实金币、库存、等级或结算。GameController 拥有活动状态和提交权，服务按契约只写自己的候选子域。
- 以“播种—浇水—睡眠—收获—出售—再购种子”为首要循环；保留有限背包和到站出售，自动化不属于当前版本目标。
- 日结算只能由统一时钟触发一次；不在动画回调或渲染帧中发放物品/金币。生产时间与绘制独立。
- 原型参数/字体候选/作物图集映射并非最终决定；未经试玩或明确选择，不把试验数值写成确认结论。

## 验证与提交

- 每次实现附与行为相匹配的必要测试；资产/交易/状态关联必须测失败不变、守恒和重复触发；纯装饰不写镜像测试。
- qmake 应用与相关 Qt Test 按任务运行；公共接口/构建变更检查所有消费者；UI 改动增加实际交互记录。
- 文档任务做相对链接、围栏、任务/接口一致性和差异检查；不得声称没执行的构建、GUI、音频或测试已通过。
- 提交前核对文件所有权、接口契约、git diff 和当前任务范围；只 stage 指定文件，不夹带 .pro.user、构建产物或个人存档。
- 报告改了什么、为何改、验证证据、未验证项和待决问题。上传/合并按本次授权与团队工作流，不以本文件作为上传许可。
