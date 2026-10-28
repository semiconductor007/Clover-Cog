# Clover-Cog

A pixel-art traditional farming game built with **C++17, Qt 6 Widgets and qmake**. Time continues during the day; after 20:00 the player can sleep to advance one day, harvest the result and walk to a fixed buying point to sell it.

## Current status

The first daily farming loop is now on `main`:

- Wheat and potato with different growth times, yields and prices;
- Consumable seeds, tilling, planting, daily watering and sleep-based day changes;
- Limited backpack, coins, a fixed market and a seed shop;
- JSON save data, backup recovery and core rule tests.

The current focus is S2 polish: Chinese HUD, onboarding prompts, crop growth feedback and boundary regression. Automated planting, harvesting, transport and offline income are outside the current version.

## Core loop

```text
Buy seeds → Till and plant → Work during the day → Sleep after 20:00
    ↑                                      ↓
    └──────── Harvest → Walk to the market and sell ────────┘
```

## Controls

| Action | Input |
| --- | --- |
| Move | W / A / S / D |
| Select a tool or seed | Mouse wheel |
| Till, plant, water or harvest | Left mouse button |
| Sleep, sell or open the seed shop | Press E near the matching area |

## Build

Use a Qt 6 Desktop Kit, a C++17 compiler and Multimedia. Building tests additionally requires Qt Test (testlib):

```powershell
qmake CHECK.pro CONFIG+=release
mingw32-make -j2
```

Core tests:

```powershell
qmake tests/tests.pro CONFIG+=release
mingw32-make -j2
core_tests.exe -txt
```

## Documentation

- [Project plan](docs/PROJECT_PLAN.md)
- [Development guide](docs/DEVELOPMENT.md)
- [Implementation plan](docs/IMPLEMENTATION_PLAN.md)
- [Farming requirements](docs/FARMING_REQUIREMENTS.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Asset audit](docs/ASSET_AUDIT.md)

## License and assets

Code is licensed under the Mulan PSL v2. Pixel assets remain under the licenses provided in their source directories. New fonts and assets must record their source and license in the documentation.
