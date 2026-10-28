# 当前接口约定

版本：v1.0。本文记录传统农场第一版已经使用的接口边界；尚未创建的自动化、物流和离线接口不在本版本中设计。

## GameClock

- `elapsedGameMinutes()`、`day()`、`hour()`、`minute()` 提供统一时间查询。
- `sleepToNextMorning()` 将时间推进到次日早晨。
- 主窗口统一调用日结算；同一日期不能重复处理。

## WorldState / MapWidget

- `cultivateTile()`、`plantSeed()`、`waterTile()` 执行土地操作。
- `advanceDay()` 根据浇水状态推进作物和空耕地恢复。
- `harvest()` 只返回成熟作物的类型和整格产量。
- MapWidget 负责地图输入和绘制，收获入包由外部 `harvestHandler` 决定。

## CropCatalog

每种作物提供稳定的 `CropType`、显示名、生长天数、产量、种子价格、出售价格和阶段图块。新增作物应扩展配置和测试，不在 UI 回调里复制数值。

## Inventory / EconomyService

- `Inventory` 负责容量、增加、扣除和恢复。
- `EconomyService::collectHarvest()` 先检查容量，再一次性入包。
- `EconomyService::sellAll()` 先检查收购点范围，再扣背包并增加金币。
- `EconomyService::buySeeds()` 先检查金币和数量，失败时状态不变。
- 工具、金币和种子不计入农产品背包容量。

## SaveManager

存档必须包含游戏时间、已处理日期、玩家位置、经济状态和世界快照。写入使用临时文件替换，并保留 `.bak`；加载失败时不能静默覆盖有效存档。

## UI 约束

UI 只调用上述服务并显示返回结果，不保存第二份业务状态。面板打开时应屏蔽地图输入；按钮、动画和绘制回调不能直接发放物品或金币。

## 未来扩展原则

新作物、扩地、分类出售和便利升级必须复用现有服务。自动化、运输和离线收益若未来立项，先单独冻结接口和存档字段，再实现业务。
