# Clover-Cog

A pixel-art farming game built with C++17, Qt 6 Widgets, Qt Multimedia and qmake. Explore the 72×40 farm, grow wheat, potatoes, carrots and beets, sell the harvest at the market and buy seeds and fertilizer. Each season lasts seven days. Press F within one tile of the bed and confirm to rest until the next 06:00. If you are still awake at 02:00, you pass out and wake beside your bed. Loading a save between 02:00 and 05:59 triggers the same recovery. Midnight settles each completed day once. Waking shows a Chinese summary with the morning date, farm results, and save status.

Summer evenings stay light longer than winter evenings. A short chime marks a successful wake-up save. If saving fails, the game pauses at the settled morning. You can retry the save or continue playing with the farm as it is.

| Action | Input |
| --- | --- |
| Move | W / A / S / D |
| Select a tool or item | Scroll through occupied toolbar slots or click a slot |
| Till, plant, fertilize, water or harvest | Left-click a tile with the appropriate item |
| Refill the watering can | Press F beside the well east of the front door or the well in the eastern field, with any tool selected |
| Open the backpack or equip seeds and fertilizer | Press E or Tab, then select a tool or item |
| Use the bed, shop or market | Press F within one tile. Confirm when resting at the bed |
| Store or retrieve items | Stand directly south of the plaza chest and press F |
| Pause the farm | Press Esc in the farm to resume, read the guide or save and return to the title screen |
| Close a panel | Esc, or E / Tab in the backpack |

Plant seeds during their growing season. Crops survive a change of season. In season, each watered day counts as one growth day. Out of season, an established plant needs two watered days for the same progress. Dry days pause growth. Fertilizer adds one extra growth day the next time the crop completes a full growth day. Each plant can be fertilized once. Hover over a seed to see its growing season and time to maturity.

The watering can starts full and holds enough water for 12 tiles. Watering one tile uses one charge. The shop sells two one-time upgrades. For 12 coins the can holds 24 charges and is refilled immediately. For 18 coins the axe fells a tree in two hits and clears the stump in one, down from four and two. Aim at the base of the trunk to collect wood.

Tilling, planting, watering, fertilizing and harvesting work only on the player's tile and its eight immediate neighbors, including diagonals. Remote clicks leave crops and inventory unchanged. Station interactions still require standing in their marked area and pressing F. Brief on-screen feedback reports the result of an action. The pause menu freezes time. Backpack, trading and storage panels allow time to continue.

Open `CHECK.pro` in Qt Creator with a Qt 6 desktop kit and a shadow build directory. The Apple silicon build uses Qt 6.10.3. From a Qt-enabled shell with Python 3, run `python3 tests/run_all.py ../build-farm-tests` (on Windows with MinGW, use `py -3`). The Chinese guide covers the local Apple Clang option and the save format. [Source and asset layout](docs/DIRECTORY_STRUCTURE.md) · [Build and player guide](README.md) · [Integration and verification](docs/IMPLEMENTATION_PLAN.md)

The title screen and pause menu share an illustrated guide. The seven-slot toolbar includes the hoe for opening new plots on grass. Existing field soil accepts seeds directly. Stone paths lead to the fenced marketplace. Press F at the market, enter how many units of a crop to sell and check the price on its Sell button. You can also sell all the produce in your bag. Seeds, fertilizer and items in the chest are kept. The new save format stores the chest separately and can continue a valid v3 or v2 farm.

Code is licensed under the Mulan PSL v2. The artwork carries its authors’ licenses. [Asset credits and terms](docs/ASSET_LICENSES.md)
