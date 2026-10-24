# Clover-Cog

[中文说明](README.md)

An incremental farming game project developed by a five-person team built on a C++17 / Qt Widgets farm template. The planned progression starts with planting, harvesting into a limited backpack, and walking to a market to sell crops. Players then upgrade seeds, tools, backpack capacity and land, eventually automating harvesting, transport and sales.

> The current prototype includes two crops, consumable seeds, a seed shop, limited inventory, market sales, currency, daily watering, bed-side sleeping and local persistence. Upgrades and automation remain planned features.

## Current prototype

- WASD movement, character animations and tile collision detection.
- Tiled JSON map loading, layered drawing and a following camera.
- Tilling, planting, daily watering, day-based crop growth and harvesting.
- A 12-slot inventory, a fixed market area and currency settlement.
- Initial seed stock, a seed shop, atomic JSON saves and backup recovery.
- Six-slot tool selection, pixel UI, a custom cursor and background music logic.

Wheat needs three watered days and potato needs five. Stand inside the purple bed marker and press `E` to sleep until 06:00 on the next day. Unwatered crops pause for that day, watered soil dries overnight, and empty tilled soil has a 20% daily chance to return to grass.

## Build

Use Qt 6 with Core, Gui, Widgets and **Multimedia**, plus a matching desktop compiler. Release compilation has succeeded with Qt 6.10.3 / MinGW 64-bit; full gameplay verification is still pending. The repository also records a post-refactor Qt 6.8.2 build, core tests and offscreen startup check. These are historical records; this planning round did not rerun builds, tests or GUI checks. See the [test strategy](docs/TEST_STRATEGY.md) for actual test coverage and pending validation.

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

Current controls: **W/A/S/D** to move, the **mouse wheel** to select an item, **left click** to use the selected tool or harvest, and **E** inside the marked market, seed-shop or bed area to interact.

## Development documents

The detailed documents are currently in Chinese. Start team work with [AGENTS.md](AGENTS.md) and the [engineering architecture](docs/ARCHITECTURE.md), which links module ownership, interfaces, migration and the implementation plan. These are plans; no business implementation or source relocation was performed in this documentation round.

Existing design documents:

- [Development guide](docs/DEVELOPMENT.md): architecture, state, commands, persistence, tasks and acceptance criteria.
- [Project plan](docs/PROJECT_PLAN.md): scope, upgrade paths and milestones.
- [Gameplay design](docs/GAMEPLAY_DESIGN.md): crops, backpack limits, physical selling and automation rules.
- [Asset audit](docs/ASSET_AUDIT.md): existing assets, Chinese font candidates and missing artwork.

The first playable milestone targets two distinct crops, a limited backpack, physical market sales, currency, five basic upgrades and save/load. Later milestones add batch tools, irrigation, collection storage, transport and offline automation. Design numbers are provisional.

## License and assets

The repository includes [MulanPSL-2.0](LICENSE). Third-party art and fonts retain their own terms. Sprout Lands assets are by **Cup Nooble**; see the bundled `read_me.txt` files and the [asset audit](docs/ASSET_AUDIT.md) for source records. Candidate Chinese fonts have not yet been added.
