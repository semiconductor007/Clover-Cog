# 当前目录与传统农场结构

## 当前实际代码

```text
Clover-Cog/
├── CHECK.pro                 # Qt/qmake 应用工程
├── mainwindow.*              # 主窗口、HUD、时间和交互组装
├── mapdata.*                 # Tiled 地图加载
├── maprenderer.*             # 地图与作物绘制
├── mapwidget.*               # 地图输入和农田操作
├── rpg.* / playercontroller.*# 角色、移动和工具动作
├── gameclock.*               # 游戏时间和睡眠推进
├── gameconstants.h           # 默认时间与玩法参数
├── worldstate.*              # 土地、浇水、作物和树木状态
├── cropdefinition.*          # 作物配置
├── inventory.*               # 农产品背包
├── economyservice.*          # 金币、出售和种子购买
├── savemanager.*             # JSON 存档和备份
├── seedshopdialog.*          # 种子商店窗口
├── itembar.* / tooltype.h    # 工具栏和工具类型
├── tests/                    # Qt Test 核心规则测试
├── docs/                     # 玩法、开发和素材文档
└── sprout/、tsx/、wav/       # 地图、像素图和音频资源
```

## 传统农场目标结构

后续可以按责任域渐进整理，但不为未立项的自动化或物流系统预留目录：

```text
src/
├── core/          # 时钟、公共类型、运行时和存档
├── farm/          # 土地、作物、浇水、睡眠日结算
├── economy/       # 背包、金币、出售、种子商店
├── player/        # 移动、碰撞和工具规则
└── ui/            # 地图、HUD、工具栏、背包和商店
data/
└── crops.json、economy.json、render_mapping.json
tests/
├── core_tests.cpp
├── farm/
├── economy/
├── player/
└── ui/
```

## 迁移原则

- 先保持根目录文件和 QRC 路径可运行，再按模块逐步移动。
- 一次只迁移一个责任域，更新 `CHECK.pro` 和测试工程后再删除旧引用。
- 作物规则、金币、库存和存档只能各有一份真实状态。
- 不创建 `automation`、`logistics`、`transport` 或 `offline` 目录。
- 资源目录暂不搬家；新增字体、作物图和 UI 图标按素材盘点登记。

文件负责人见 [FILE_OWNERSHIP.md](FILE_OWNERSHIP.md)。
