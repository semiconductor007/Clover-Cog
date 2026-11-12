# 工程目录与维护方式

用 Qt Creator 打开根目录的 [CHECK.pro](../CHECK.pro)。它只定义应用入口和安装规则。游戏及两组整窗测试共用 [src/game.pri](../src/game.pri)，每个可执行程序只有一个入口。命令行构建和测试见 [README](../README.md#打开构建与运行)。

| 要改什么 | 去哪里 |
| --- | --- |
| 启动、主窗口、把输入接到游戏状态 | [src/app](../src/app) |
| 连续时钟、睡眠时间、共用常量 | [src/core](../src/core) |
| 作物品种、价格表、生长规则、农田和四季日历 | [src/farm](../src/farm) |
| 金币、买卖、背包、箱子存取规则 | [src/economy](../src/economy) |
| 存档格式、版本读取和备份恢复 | [src/storage](../src/storage) |
| 地图加载、绘制、地图输入与四季外观 | [src/map](../src/map) |
| 角色移动、碰撞、农具类型与操作 | [src/player](../src/player) |
| 首页、暂停、工具栏、各交易面板、画面过渡 | [src/ui](../src/ui) |
| 回归测试、按套件构建的运行器 | [tests](../tests) |
| 四季资源校验与预览 | [tools/season_preview](../tools/season_preview) |
| 早期角色头文件、Qt Designer 界面草稿（不参与构建） | [legacy/qt-prototype](../legacy/qt-prototype) |

**新增代码。** 把源码和对应的头文件放在同一个职责目录，在 `src/game.pri` 中登记。独立测试或预览工具用到新实现时，也更新对应的 `.pro`。同目录可直接包含文件名，跨目录从 `src` 起写，例如 `#include "economy/economyservice.h"`。界面读取库存并发起交易，结算仍由经济模块完成。

**编辑地图与美术。** 正式地图是 [map6.tmj](../map6.tmj)，可直接用 Tiled 打开。根目录的 Water.tsx、[tsx](../tsx) 和 [sprout](../sprout) 等目录保留原位置：Tiled 通过相对路径读取图片，四季资源也用这些路径标识图集。map7、map8 是旧地图。游戏通过 [character.qrc](../character.qrc) 和 [farm-sounds.qrc](../farm-sounds.qrc) 加载资源；新增游戏自有或已获授权的精选资源放在 [resources](../resources)，登记稳定的 Qt 路径和[来源](ASSET_LICENSES.md)。不要把整份购买包放进 QRC。

**本机文件。** `local-assets/` 用于原始购买包及解压内容，不参与构建，也不上传。构建目录、个人 `.pro.user` 和 Tiled 会话不提交；共享的 `.tiled-project` 保留。用新目录构建可以验证工程没有依赖旧缓存。个人农场存档仍由系统应用数据目录管理。

**其他资料。** [执行计划](IMPLEMENTATION_PLAN.md) 记录已集成的功能与待验项；[文件分工](FILE_OWNERSHIP.md) 对应各模块负责人；早期任务认领与玩法方案保留在 [开发文档](DEVELOPMENT.md)。
