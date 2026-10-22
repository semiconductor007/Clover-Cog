# Clover-Cog

[中文说明](README.md)

A personal incremental farming game project built on a C++17 / Qt Widgets farm template. The planned progression starts with planting, harvesting into a limited backpack, and walking to a market to sell crops. Players then upgrade seeds, tools, backpack capacity and land, eventually automating harvesting, transport and sales.

> The current code is still the template prototype. Currency, limited inventory, market trading, upgrades, automation and persistence are planned features, not implemented gameplay.

## Current prototype

- WASD movement, character animations and tile collision detection.
- Tiled JSON map loading, layered drawing and a following camera.
- Basic tilling, planting, timed growth and wheat harvest counting.
- Six-slot tool selection, pixel UI, a custom cursor and background music logic.

Both seed selections currently use the same crop pipeline. Watering has an animation but no growth effect; tree interaction and map boundary handling need further work.

## Build

Use Qt 6 with Core, Gui, Widgets and **Multimedia**, plus a matching desktop compiler. Release compilation has succeeded with Qt 6.10.3 / MinGW 64-bit; full gameplay verification is still pending. The original repository records a Qt 6.8.2 MinGW Kit.

Open `CHECK.pro` in Qt Creator and configure a local Desktop Kit, or run the following in a matching Qt/MinGW environment:

```powershell
git clone https://github.com/semiconductor007/Clover-Cog.git
cd Clover-Cog
mkdir build_local
cd build_local
qmake ../CHECK.pro CONFIG+=release
mingw32-make -j4
./release/CHECK.exe
```

The application entry point is in `mainwindow.cpp`. Keep the resource directories intact; `character.qrc` embeds the assets. Select your own Kit and build directory instead of reusing the original `.pro.user` settings.

Current controls: **W/A/S/D** to move, the **mouse wheel** to select an item, and **left click** for map interactions. Market interaction and backpack controls described in the design documents are not yet available.

## Development documents

The detailed documents are currently in Chinese:

- [Development guide](docs/DEVELOPMENT.md): architecture, state, commands, persistence, tasks and acceptance criteria.
- [Project plan](docs/PROJECT_PLAN.md): scope, upgrade paths and milestones.
- [Gameplay design](docs/GAMEPLAY_DESIGN.md): crops, backpack limits, physical selling and automation rules.
- [Asset audit](docs/ASSET_AUDIT.md): existing assets, Chinese font candidates and missing artwork.

The first playable milestone targets two distinct crops, a limited backpack, physical market sales, currency, five basic upgrades and save/load. Later milestones add batch tools, irrigation, collection storage, transport and offline automation. Design numbers are provisional.

## License and assets

The repository includes [MulanPSL-2.0](LICENSE). Third-party art and fonts retain their own terms. Sprout Lands assets are by **Cup Nooble**; see the bundled `read_me.txt` files and the [asset audit](docs/ASSET_AUDIT.md) for source records. Candidate Chinese fonts have not yet been added.
