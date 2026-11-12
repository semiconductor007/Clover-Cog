# Clover-Cog

A pixel-art farming game built with C++17, Qt 6 Widgets, Qt Multimedia and qmake. Explore the 72×40 farm, grow wheat, potatoes, carrots and beets, sell the harvest at the market and buy seeds and fertilizer. Each season lasts seven days. Time passes through midnight, and sleeping at home between 20:00 and 05:59 skips to 06:00. Sleep and season changes share a short fade to black.

| Action | Input |
| --- | --- |
| Move | W / A / S / D |
| Select a tool or item | Scroll or click a toolbar slot |
| Till, plant, fertilize, water or harvest | Left-click a tile with the appropriate item |
| Open the backpack or equip seeds and fertilizer | Press Tab, then select an item |
| Use the bed, shop or market | Press E at the corresponding location |
| Store or retrieve items | Stand directly south of the plaza chest and press E |
| Pause the farm | Press Esc in the farm to resume, read the guide or save and return to the title screen |
| Close a panel | Esc, or Tab in the backpack |

Open `CHECK.pro` in Qt Creator with a Qt 6 desktop kit and a shadow build directory. The Apple silicon build uses Qt 6.10.3. From a Qt-enabled shell with Python 3, run `python3 tests/run_all.py ../build-farm-tests` (on Windows with MinGW, use `py -3`). The Chinese guide covers the local Apple Clang option and the save format. [Source and asset layout](docs/DIRECTORY_STRUCTURE.md) · [Build and player guide](README.md) · [Integration and verification](docs/IMPLEMENTATION_PLAN.md)

The title screen and pause menu share an illustrated guide. The eight-slot toolbar includes the hoe for opening new plots on grass. Existing field soil accepts seeds directly. Stone paths lead to the fenced marketplace. Press E at the market, enter how many units of a crop to sell and check the price on its Sell button. You can also sell all the produce in your bag. Seeds, fertilizer and items in the chest are kept. The new save format stores the chest separately and can continue a valid v3 or v2 farm.

Code is licensed under the Mulan PSL v2. The artwork carries its authors’ licenses. [Asset credits and terms](docs/ASSET_LICENSES.md)
