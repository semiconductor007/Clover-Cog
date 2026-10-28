# 文件所有权和修改权限

A–E 与五名成员一一对应，姓名和账号待登记。主责可以修改所属实现；公共接口、构建文件、存档格式和资源清单必须提前沟通。

| 文件范围 | 主责 | 需要审查 |
| --- | --- | --- |
| `gameclock.*`、`gameconstants.h`、`savemanager.*` | A 核心与存档 | B 农田、D 经济 |
| `worldstate.*`、`cropdefinition.*`、农田规则 | B 农田与作物 | A 核心、C 表现 |
| `mainwindow.*`、`mapwidget.*`、`maprenderer.*`、`mapdata.*` | C 地图与 UI | A 装配、B 状态、D 交易 |
| `rpg.*`、`playercontroller.*`、`collisiondetector.*`、`itembar.*` | C 角色与工具 | A 输入、B 工具规则 |
| `inventory.*`、`economyservice.*`、`seedshopdialog.*` | D 经济与库存 | A 存档、B 收获 |
| `tests/*`、试玩记录、资源来源记录 | E 测试与内容 | 对应功能主责、A 集成 |
| `CHECK.pro`、`tests/tests.pro`、QRC 和公共配置 | A 集成；C 负责资源内容 | 所有受影响模块 |
| `README*`、`docs/*` | A 文档维护 | 玩法涉及的模块负责人 |

## 修改边界

- UI 不得持有第二份金币、种子、背包或农田状态。
- B 不直接修改金币，D 不直接推进作物成长。
- 日期只在夜间成功睡眠后推进，公共时钟接口由 A 维护。
- 新作物、扩地和便利升级必须复用现有服务，不创建独立的重复库存或经济算法。
- 本版本不设置自动化、运输、物流或离线收益的文件所有权。

## 协作窗口

修改公共头、存档字段、`CHECK.pro`、测试工程或资源清单时，在提交说明中列出影响文件和验证命令。不同成员不得在同一工作目录同时编辑同一文件。
