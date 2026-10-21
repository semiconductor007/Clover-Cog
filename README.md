# Clover-Cog

基于 **C++17 和 Qt Widgets** 的像素风农场游戏原型，原 README 中称为 Sprout，源于 C++ 课程大作业。项目使用 Tiled 地图、精灵动画和鼠标交互，实现地图探索、开垦、播种与收获的基本流程。

> 本文根据当前源码整理。项目处于原型阶段，部分工具和作物功能尚未完整实现。

## 功能

- **角色与动画**：WASD 移动，支持组合方向键斜向移动，包含站立、行走、挥斧、锄地和浇水动画。
- **地图与镜头**：读取 Tiled `.tmj` 地图和 `.tsx` 图块集，使用 QPainter 分层绘制，镜头随角色移动。
- **碰撞检测**：根据碰撞图层检测角色脚部矩形与障碍图块的交叠。
- **耕作与种植**：在指定草地上开垦，在空闲耕地上播种。
- **生长与收获**：每约 10 秒推进一个生长阶段，约 30 秒成熟，点击成熟作物收获并增加小麦计数。
- **界面与音频**：六槽物品栏、像素字体、时间和天数显示、猫爪光标，以及背景音乐播放逻辑。

默认地图为 `map6.tmj`，包含 **49 × 30** 个 **16 × 16** 像素图块；初始地图显示区域为 **520 × 320** 像素。

## 环境要求

| 项目 | 说明 |
| --- | --- |
| 编程语言 | C++17 |
| 桌面框架 | Qt 6 Widgets |
| Qt 模块 | Core、Gui、Widgets、Multimedia |
| 构建系统 | qmake，配置文件为 CHECK.pro |
| 仓库记录的开发环境 | Qt 6.8.2 / MinGW 64-bit |
| 地图编辑工具 | Tiled，仅修改地图时需要 |

源码使用 QAudioOutput、QMediaPlayer::setSource() 和 QMouseEvent::position() 等 Qt 6 API，应使用 Qt 6 工具链。`CHECK.pro.user` 和仓库内 `build/` 属于原开发环境，不应直接作为本机配置复用。

## 获取与运行

```bash
git clone https://github.com/semiconductor007/Clover-Cog.git
cd Clover-Cog
```

### Qt Creator

1. 安装 Qt 6、对应的桌面编译器及 **Qt Multimedia** 模块。可优先采用仓库记录的 Qt 6.8.2 MinGW 64-bit 配置。
2. 打开 `CHECK.pro`，选择本机有效的 Desktop Kit。
3. 如提示恢复原作者配置，重新配置本机 Kit 和独立构建目录。
4. 构建并运行。程序入口位于 `mainwindow.cpp`，不需要额外的 main.cpp。

### Windows 命令行（MinGW）

在已配置对应 Qt Kit 环境的终端中执行，确保 qmake 和 mingw32-make 使用匹配的工具链：

```powershell
mkdir build_local
cd build_local
qmake ../CHECK.pro CONFIG+=release
mingw32-make -j4
./release/CHECK.exe
```

项目启用了 `resources_big`，图片、地图、字体和音频由 `character.qrc` 打包。请保留完整素材目录及文件名。

### 常见问题

- **Unknown module(s) in QT: multimedia**：为当前 Kit 安装 Qt Multimedia，并确认 qmake 指向正确的 Qt 版本。
- **出现原作者的磁盘路径**：重新选择本机 Kit 和构建目录，不沿用 CHECK.pro.user 中的旧配置。
- **缺少 DLL 或插件**：先从 Qt Creator 或 Qt 环境终端运行；独立分发时使用同一 Kit 的 windeployqt 部署依赖。
- **地图或图片加载失败**：检查 character.qrc、地图 tileset 引用及图片文件名，查看应用输出中的资源加载日志。

## 操作

| 操作 | 输入 |
| --- | --- |
| 向上 / 左 / 下 / 右移动 | W / A / S / D |
| 循环切换物品 | 鼠标滚轮 |
| 使用当前工具 / 点击地块 | 鼠标左键 |
| 收获成熟作物 | 左键点击成熟作物 |

物品栏顺序：**空手 → 斧子 → 锄头 → 水壶 → 小麦种子 → 土豆种子**。

建议体验流程：

1. 用 WASD 移动，观察镜头跟随和障碍碰撞。
2. 切换锄头，点击可耕作草地以生成耕地。
3. 切换小麦种子，点击空闲耕地播种。
4. 等待约 30 秒，点击成熟作物，观察小麦计数变化。

作物生长使用现实经过时间，每秒检查一次，与界面时钟独立。界面每现实秒推进一个显示分钟，每现实分钟推进一个显示小时；DAY 则按现实 24 小时计算，尚未统一为同一游戏日历。

## 目录与架构

```text
Clover-Cog/
├── CHECK.pro                 # qmake 配置
├── mainwindow.cpp/.h         # main()、主窗口、时钟、音频及信号连接
├── rpg.cpp/.h                # 角色输入、移动、动画和工具操作
├── mapwidget.cpp/.h          # 地图、视口、耕地、作物生长和收获
├── collisiondetector.cpp/.h  # 图块碰撞检测
├── itembar.cpp/.h            # 六槽物品栏和滚轮切换
├── tooltype.h                # 工具枚举、属性及动画数据结构
├── character.qrc             # Qt 资源清单
├── map6.tmj                  # 默认地图
├── map7.tmj / map8.tmj       # 其他地图，当前入口未加载
├── tsx/                      # 图块集描述
├── sprout/                   # 地图、角色、字体及 UI 素材
├── itembar/                  # 物品栏图片
├── wav/                      # 背景音乐
└── LICENSE                   # 木兰宽松许可证，第 2 版
```

MainWindow 组装地图、角色和物品栏，通过 Qt 信号与槽连接交互：物品栏切换更新角色工具，地图点击触发工具动作或播种，收获信号更新小麦数量。MapWidget 管理地图及作物状态；ImageMovementWidget 移动前调用 CollisionDetector 检查目标位置，再更新地图视口。

## 地图与资源约定

- 默认地图入口为 MainWindow 中的 `loadMap(":/map/map6.tmj")`。
- 特殊图层：名称包含 collision 的图层用于碰撞，cultivatable 用于可耕作地面，cultivated 用于耕地，crop 用于作物，tree 用于树木信息。
- 图层尺寸需与地图尺寸一致；现有交互假定相关特殊图层存在。
- 图块集属性通过 `:/tilesets/tsx/<文件名>` 查找，图片通过 `:/tilesets/sprout/<同名 png>` 查找。修改地图引用后也需同步资源清单。
- 当前逻辑硬编码草地 GID 13、新耕地 GID 135 和作物 GID 240–243；替换图块集或调整 firstgid 时需要同步检查代码。

## 当前限制

- **土豆尚未独立实现**：两种种子均调用同一个 plantSeed()，使用相同生长图块，成熟收获统一按小麦处理。
- **浇水只有动画**：尚无土壤湿度状态，作物生长不依赖浇水。
- **砍树逻辑未闭环**：首次加载时树木实例初始化早于图块属性加载，可能无法建立可交互树木；扣血后也未同步清除绘制图层和碰撞数据。
- **状态不持久化**：未实现存档、读档、交易、背包库存和种子消耗。
- **边界与布局待完善**：部分点击和图层访问缺少完整边界保护；地图及覆盖层尺寸固定，窗口缩放适配有限。
- **地图加载失败容错不足**：后续绘制依赖有效图块尺寸和图层数据，修改地图后应检查资源与结构。

后续可优先统一时间系统、区分作物类型、补齐坐标和边界校验，再完善砍树、浇水与存档。

## 验证说明

已静态核对构建配置、入口、按键、资源清单和默认地图结构，character.qrc 引用的本地文件均存在。

本机补装 Qt Multimedia，并显式包含 QElapsedTimer 头文件后，已在 Qt 6.10.3 / MinGW 64-bit 下完成 Release 编译并生成 CHECK.exe；尚未完成游戏运行及交互验证。构建仍有未使用参数及中文路径下 moc_predefs.h 解析警告。Qt 6.8.2 是仓库记录的原开发环境。

## 许可证与素材

仓库包含 [木兰宽松许可证，第 2 版（MulanPSL-2.0）](LICENSE)。

素材目录包含 Sprout Lands、Farm RPG FREE 16x16 等第三方资源。使用或分发素材前，应另行核对素材包内的 read_me.txt 和原作者授权条件，尤其是标有 premium 的资源；仓库代码许可证不能代替第三方素材授权。
