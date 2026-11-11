# Clover-Cog · 四季农场

一款用 C++17、Qt 6 Widgets 和 qmake 开发的像素农场小游戏。在 72×40 的地图上种地，拿着收获去集市出售，再到商店买下一批种子和肥料。家、小路、桥和农田从开局就能探索。

## 从第一批种子开始

开局有 6 颗小麦种子和 3 颗土豆种子。选锄头开地，换种子播种，再用水壶浇水。小麦有 3 个浇过水的生长日就能收获。把 6 份小麦带到收购点，确认出售可得 **30 金币**。商店里 2 金币能买一颗胡萝卜种子，1 金币能买一份基础肥料。给未成熟作物施肥后，下一个浇过水的生长日会额外长一天，每株可施一次。

小麦四季皆可播种，土豆适合春夏，胡萝卜适合春秋，甜菜适合秋冬。已经播下的作物可以跨季生长。农产品背包可装 12 份，种子和肥料另计。

每季 7 天。夜间可以继续活动，午夜会自动跨日。20:00 至次日 05:59 可以到家中床边按 E，直接睡到最近的 06:00。睡眠和换季都有约 0.9 秒的渐黑过渡，季末入睡时只播放一次。

## 操作

| 操作 | 输入 |
| --- | --- |
| 移动 | W / A / S / D |
| 选择农具、种子或肥料 | 滚轮、触控板滚动，或点击底部工具格 |
| 锄地、播种、施肥、浇水 | 选中相应物品，左键点击地块 |
| 收获 | 选空手，左键点击成熟作物 |
| 打开背包、选用物品 | Tab 打开，点击种子或肥料即可装备 |
| 睡眠、商店、收购 | 走到床铺或对应摊位，按 E 互动 |
| 关闭面板 | Esc，背包也可用 Tab 关闭 |

游戏在系统的应用数据目录中自动保存为 `farm-v3.json`，旁边的 `.bak` 用于恢复。能读取的 v2 农场会接续到新档。遇到 v1 时另开新局，旧文件保留在原位置。

## 打开、构建与运行

在 Qt Creator 打开 `CHECK.pro`，选择 Qt 6 Desktop Kit 和仓库外的构建目录，构建后运行。Mac 集成版使用 Qt 6.10.3、C++17 编译器、Widgets、Multimedia，测试另用 Qt Test。Windows 队友可以继续用原 Qt 6 Kit。Qt Creator 的 Kit、个人项目设置和构建缓存保存在本机。

在已配置好 qmake 和编译器的终端，从仓库根目录运行九组自动化测试。每组各用一个独立构建目录。

```sh
# macOS
python3 tests/run_all.py ../build-farm-tests
```

```powershell
# Windows 的 Qt / MinGW 命令行
py -3 tests/run_all.py ../build-farm-tests
```

若 Apple Clang 21 搭配 Qt 6.10.3 提示缺少 ARM intrinsic 声明，在本机 Kit 的 qmake 参数加上 `QMAKE_CXXFLAGS+=-include arm_acle.h`。测试命令可追加 `--qmake-arg 'QMAKE_CXXFLAGS+=-include arm_acle.h'`。

## 开发资料

- 本轮已实现的玩法、收益、过渡时序、Mac 实测和 Windows 待验项见[执行计划](docs/IMPLEMENTATION_PLAN.md)。
- 四种作物的成长阶段、种子及收获物图标见[作物映射](docs/CROP_CATALOG.md)，地图配色和装饰见[季节资源](docs/SEASON_ASSETS.md)。
- 团队的原始任务范围见[开发文档](docs/DEVELOPMENT.md)和[任务认领与产品玩法 Word 文档](docs/Clover-Cog任务认领与产品玩法_四季背包版.docx)。分支协作约定见[Git 工作流](docs/GIT_WORKFLOW.md)。

代码采用木兰宽松许可证第 2 版，像素素材遵循各自作者的许可。[素材来源与署名](docs/ASSET_LICENSES.md)
