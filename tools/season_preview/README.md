# 四季资源预览工具

A 的独立资源检查与渲染工具，读取正式 QRC/地图，导出四季效果。它不调用游戏主窗口、不加载/保存个人存档，不推进时间。运行时依赖 Qt Core/Gui，不需 Qt Widgets、Multimedia 或 Qt Svg；使用与项目相同的 Qt 6 Kit。

从仓库根目录起，在仓库外建立构建目录，避免个人缓存进入 Git。以下假设仓库目录名为 Clover-Cog。

Windows（原 Qt/MinGW 命令行，构建后进入 PowerShell运行）：

```powershell
mkdir ../build-season-preview
cd ../build-season-preview
qmake ../Clover-Cog/tools/season_preview/season_preview.pro CONFIG+=release
mingw32-make -j2
$env:QT_QPA_PLATFORM = "offscreen"
.\season_preview.exe --output .\rendered-seasons
```

macOS（已选择匹配 Qt 的终端）：

```sh
mkdir -p ../build-season-preview
cd ../build-season-preview
qmake ../Clover-Cog/tools/season_preview/season_preview.pro CONFIG+=release
make -j4
QT_QPA_PLATFORM=offscreen ./season_preview --output ./rendered-seasons
```

本机 Apple Clang 21 的 ARM 内建声明兼容选项仍通过本地 qmake 参数传入，不写进共享工程：需要时在 qmake 命令追加 `"QMAKE_CXXFLAGS+=-include arm_acle.h"`。中文标注默认 Microsoft YaHei，可用 `--font-file <本地字体文件>` 加载已有字体；未提供时使用系统字体回退。

退出码 0 表示本次资源校验通过；导出 spring/summer/autumn/winter-map.png 和 seasons-overview.png。检查包括源图哈希/尺寸、绑定、alpha/精确颜色映射、图块布局、装饰边界与位置。若源图或地图改动触发失败，先人工核对配色和坐标，再由 A 更新资源契约；不要为了通过而绕过检查。

图中四种成熟作物是只在内存创建的展示样本。详情和 C/E 接入步骤见 [SEASON_ASSETS](../../docs/SEASON_ASSETS.md)。macOS 本次已构建和运行；Windows 命令尚未实测。
