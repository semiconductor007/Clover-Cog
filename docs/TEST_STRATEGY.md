# 测试、验收与持续集成

基线 `8ddf705`，2026-10-22。本轮仅阅读源码/配置并验证 Markdown；未编译、未执行 Qt Test、未启动游戏。README 中 Qt 6.8.2 重构后检查和旧版 Qt 6.10.3 编译属于历史记录，不能替代本轮证据。未来 DEV-001 应填写实际提交/Kit/命令/结果。

## 1. 当前测试的实际覆盖

`tests/tests.pro` 使用 core/gui/testlib、C++17、testcase/console，目标 core_tests，直接编译 collisiondetector/gameclock/mapdata/worldstate。`tests/core_tests.cpp` 用 QTEST_APPLESS_MAIN；共 5 个功能测试函数，不把 QtTest 的 init/cleanup 项算新功能测试。

| 现有函数 | 断言范围 | 缺口 |
| --- | --- | --- |
| gameClockConvertsElapsedTime | 分钟/小时/天换算 | 暂停、恢复、负输入/溢出、大增量尚无 |
| collisionDetectorHandlesBounds | 空地/障碍、远负坐标 | 精确边界/单像素、非矩形层、角色世界边界 |
| mapLoaderValidatesAndInitializesLayers | 最小合法 JSON、缺省层初始化 | 名称虽含 validates，未断言坏 JSON/坏尺寸等拒绝分支 |
| mapLoaderPreservesDynamicLayerOrder | 动态层分类与顺序 | 实际图片渲染、资源加载、Tiled objectgroup |
| worldStateHandlesFarmingAndTrees | 耕地、小麦成熟/收获、土豆播种、树图块/碰撞同步、少量越界 | 两作物独立参数、容量、价格、重复输入、真实画面 |

因此当前通过记录也不能证明经济、存档或 GUI 功能通过；它们还不存在。包含 QPixmap 的加载器测试目前 tilesets 为空，真正资源测试要用 GUI 应用测试入口，不能直接扩大 APPLESS 用例加载图像。

## 2. 未来单元与集成矩阵

| 领域/主责 | 目标文件 | 必须覆盖 | 最晚阶段 |
| --- | --- | --- | --- |
| 时间/事务/配置 A | tests/core/tst_core.cpp | 暂停不推进、恢复无跳时、候选失败不变、重入排队、非法定义/循环前置/溢出 | M0/M1 |
| 农田 B | tests/farm/tst_farm.cpp | 耕地权限、两作物进度、GID无关成熟、产量快照、整格采收、范围排序、扩地幂等、再生 | M0–M2 |
| 移动 C | tests/player/tst_player.cpp | 世界/屏幕往返、负坐标、碰撞矩形边缘、边界、状态重载后有效引用 | M0 |
| 库存 D | tests/economy/tst_economy.cpp | 容量、加减/转移守恒、保留品、负数/溢出/不存在ID、resize不丢货 | M1 |
| 经济 D | 同上 | 距离/权限、报价重验、SellAll空包/重复、金币上限、前置/满级/五项效果和失败回滚 | M1 |
| 存档 A | tests/persistence/tst_persistence.cpp | 大整数、完整字段、版本迁移、坏主档/备份、未知ID、保存失败、恢复中断 | M1/M3 |
| 自动化 E | tests/logistics/tst_logistics.cpp | 吞吐、覆盖重叠、箱满、补种成功条件、移位不刷新冷却、暂停、唯一cargo | M3 |
| 物流 E | 同上 | 转移/保留、未到站不卖、重复到站、改线延迟、代售独立来源 | M3 |
| GUI C | tests/ui/tst_ui.cpp | 控件发一次命令、面板屏蔽、焦点、字体和资源加载；业务不用按钮测试替代 | M1/M2 |
| 跨域 E（相关成员审） | tests/integration/tst_integration.cpp | 下面的交易/采收/升级/存档/在线离线序列 | 各功能合入时 |

跨域最小场景：

1. 初始背包剩2，成熟格产3；手动和自动分别请求，失败后库存/地块/金币/补种均不变。
2. 范围内混合产量与成熟状态；逐格提交，后续小产量仍可收，成功格数与 UI 结果一致。
3. harvest→库存→到站 sell；数量减少等于已卖量、金币增加等于最终价；远程卖或重复请求无第二笔。
4. 买五项升级分别验证真正消费效果的 B/C/D，扩地非法时钱/等级都回滚。
5. 手动与机器同刻收同格、两个重叠机器收同格；仅一份产量入一个目标库存。
6. 箱→货舱→到站售卖；任意检查点，初始货物+收获量=各库存现量+已出售量（不含额外生成）。未到站金币不增。
7. 在途保存/重载，多次推进到站只卖一次；失败档不覆盖成功检查点。
8. 相同初态与总时长，在线不同 tick 分段与离线事件推进的作物、库存、货舱、金币/冷却一致；包括满箱、无运输、暂停、再生和保留品。

使用 QTest 表驱动与明确的模拟时间输入，避免靠 sleep 等待成熟。测试 fixture 使用合法定义和一致初态，不自建“测试专用业务算法”。随机属性测试若采用需固定种子并输出复现样例，不引入新测试框架依赖。

## 3. GUI 人工验收表

| 阶段 | 操作 | 通过条件/记录 |
| --- | --- | --- |
| M0 | 启动、WASD/组合键、镜头边缘、树木/耕地、各种工具、播种成熟 | 记录真实支持的方向行为；图层/动画不退化，无越界；组合键期望需明确 |
| M1 | 近/远市场、E、背包满、交易/商店、五升级、关闭重开 | 到站才卖、数字一致、面板无穿透/焦点恢复、存档正确 |
| M2 | 1/2倍/高DPI与窗口调整、中文混排、范围预览、灌溉/再生 | 汉字无缺失截断、点击与预览匹配、范围工具少点击 |
| M3 | 箱满→取货、自动恢复、小车空/满载、途中退出、离线回归 | 物流状态可读、无丢货/重复钱、背包不自动出售 |
| M4 | 新玩家从开场到自动循环、背景音乐/关闭窗口、独立部署 | 引导可理解，声音/资源真实可用，发行包无需开发环境路径 |

记录模板：提交 SHA、Qt/编译器、分辨率/DPI、初态/步骤、预期/实际、截图或日志、通过/失败/未执行、缺陷任务。离屏启动只能检查基础启动，不能替代窗口可读性、键盘鼠标或实际声音。

## 4. 最小构建与 CI 方案（尚未创建 workflow）

保留 Windows/Qt 6/qmake，先单一 MinGW Kit，不加多平台矩阵、容器和新构建系统。建议 CI 固定 Qt 6.10.3 + 匹配 MinGW + Multimedia + Qt Test；最终 runner 和安装方式由 A 在 DEV-001 确认，不能把本机 D: 路径写进 CI。第三方安装 action 需固定可审查版本；本轮不安装依赖。

在已配置 Qt/MinGW PATH 的终端，当前工程示例（源码根开始；目录名尚未占用时执行）：

```powershell
New-Item -ItemType Directory -Path build_ci_app
Set-Location build_ci_app
qmake ../CHECK.pro CONFIG+=release
mingw32-make -j2
Set-Location ..
New-Item -ItemType Directory -Path build_ci_tests
Set-Location build_ci_tests
qmake ../tests/tests.pro CONFIG+=release
mingw32-make -j2
./release/core_tests.exe -txt
```

未来脚本必须逐命令检查 `$LASTEXITCODE`，失败立即终止；不能只检查最后命令。各域测试拆分后，tests/tests.pro 为 SUBDIRS 时 qmake/make 只保证构建，CI 必须显式运行每个测试 exe 并聚合退出码。旧 core_tests 保留到覆盖迁移完成；A 维护执行清单避免新测试“只编不跑”。

PR 最小必需检查：checkout→配置固定 Kit→影子构建应用→构建并运行所有已注册 Qt Test→QRC 文件/alias 与文档相对链接检查→保存失败日志/测试报告。GUI 用例可选 offscreen，但实际交互仍在阶段门人工执行。文档专属 PR 可只做 Markdown 检查；源码/配置/接口 PR 执行相应构建门禁。

当前 build/ 含旧追踪产物，CI 不复用它们。缓存不是必需品；先保证可重复干净构建，再考虑缓存。部署使用同 Kit 的 windeployqt 并在没有开发 PATH 的环境验收，Qt Multimedia 插件与字体/音频资源必须列入。

## 5. 本轮验证记录

- 已阅读全部根 C++ 头/实现、CHECK.pro、tests 工程/测试、character.qrc 和四篇已有设计文档、两种 README。
- 通过 Git 文件清单识别本机配置、历史构建产物和未追踪目录；未删除任何文件。
- 规划文件的链接、Markdown 代码围栏、任务覆盖、目录清单和改动范围在交付前静态核对；具体结果以本轮最终回复为准。
- 应用编译、Qt Test、GUI、字体、音频、部署和 CI：本轮均未执行/未启用。
