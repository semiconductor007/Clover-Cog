# 当前接口约定

2026-11-06 修订：本轮增加四季与可操作背包。B 负责作物/四季规则，D 负责背包/商店，E 完整负责时间/昼夜绘制/睡眠，C 不再承担昼夜。B 规则已于 2026-11-08 实现，C/D/E 的新增界面与调度仍待实现；状态及首版规则以 [DEVELOPMENT.md](DEVELOPMENT.md) 第 0 节为准。

版本：v1.2 B 实现交接（消费者审查待确认）。本文区分已有接口与本轮待实现要求；尚未创建的自动化、物流和离线接口不在本版本中设计。

## GameClock（E 主责）

- `elapsedGameMinutes()`、`day()`、`hour()`、`minute()` 提供统一时间查询。
- `canSleep()` 的目标窗口为 20:00–次日 06:00，覆盖午夜后的凌晨。
- `sleepToNextMorning()` 在白天拒绝操作，夜间跳到最近的 06:00。晚间跳到次日，凌晨跳到当天。
- 时间昼夜连续流逝，午夜自然进入次日。
- 自然跨日和睡眠跳时均由主窗口统一调用日结算，按已跨过日期顺序处理，同一日期不能重复处理。
- 场景夜色读取统一时间，绘制不能触发日结算。在家中床边按 E 才能发起睡眠。

## WorldState / MapWidget

- `cultivateTile()`、`plantSeed()`、`waterTile()` 执行土地操作。
- `advanceDay()` 根据浇水状态推进作物和空耕地恢复。
- `harvest()` 只返回成熟作物的类型和整格产量。
- MapWidget 负责地图输入和绘制，收获入包由外部 `harvestHandler` 决定。

## 四季查询（B 已实现，E 接入待做）

- `SeasonCalendar::dateForDay(qint64)` / `dateForMinutes(qint64)` 返回 `SeasonDate { year, season, dayOfSeason }`，只读换算、不计时。
- 首轮 `DaysPerSeason=7`，日 0 为第一年春季第 1 天，日 28 为第二年春季第 1 天；季长仍待试玩确认。
- `CropCatalog::canPlant(type, season)` / `canPlantOnDay(type, day)` 与 WorldState 共同校验新播种；非适季失败不扣种，已有作物跨季继续成长。
- E 必须按顺序逐日调用 `advanceDay(completedDay)`，同日重复调用无效；该入口不自动补结算遗漏日期。时钟、睡眠、HUD 和环境仍属 E 待开发。

## 施肥（B 地块效果已实现，C/D 接入待做）

`WorldState::fertilize(tile)` 返回 `FertilizeResult {success, failure}`。未成熟作物每株最多一次，下一次有浇水日结算额外增长 1 天，干旱保留奖励；无效目标/空地/已施肥/成熟均失败不改地块。D 检查库存、同步调用后仅在成功时扣 1，C 负责施用输入；库存事务尚未实现。`isFertilized()`、`hasPendingFertilizerBonus()` 可供展示，日结算统计返回 `fertilizerBonusesUsed`。

## CropCatalog

已包含 Wheat=1、Potato=2、Carrot=3、Beet=4；原枚举值保留。目录提供稳定 id、displayName/localizedName、生长天数、产量、种子价格、出售价格、stageGids、plantingSeasons、seedIcon/harvestIcon。`cropTypes()` 枚举全部品种，`fromId()` 解析存档 ID，`stageGid()` 按有效生长日取图。配置和矩形清单见 [CROP_CATALOG](CROP_CATALOG.md)，新增价格待 D 复核，UI 不复制数值。

## Inventory / EconomyService

- `Inventory` 负责容量、增加、扣除和恢复。
- `EconomyService::collectHarvest()` 先检查容量，再一次性入包。
- `EconomyService::sellAll()` 先检查收购点范围，再扣背包并增加金币。
- `EconomyService::buySeeds()` 先检查金币和数量，失败时状态不变。
- 工具、金币、种子和肥料不计入农产品背包容量。
- D 背包面板读取同一库存，分类展示农产品/种子/肥料，选中只发选择意图，由 C 同步当前工具并在有效目标施用。面板不扣数量，不提供随地出售。
- 面板打开屏蔽地图输入，时间继续，跨日后刷新真实状态；关闭恢复焦点。

## SaveManager

当前 v2 在 SavedCropState/JSON 增加 fertilizerApplied、fertilizerBonusPending 布尔字段；只接受新版本，旧档保留、不迁移。非法标志或快照恢复失败时不改变活动状态。D 后续汇总存档，包含累计游戏时间、已处理日期、玩家位置、容量及农产品/种子/肥料数量、世界和施肥状态。季节从日期推导，必要时记录季节配置版本，不保存第二份可漂移的当前季节计数。写入使用临时文件替换，并保留 `.bak`；加载失败时不能静默覆盖有效存档。

## UI 约束

UI 只调用上述服务并显示返回结果，不保存第二份业务状态。面板打开时应屏蔽地图输入；按钮、动画和绘制回调不能直接发放物品或金币。

## 未来扩展原则

新作物、扩图、肥料和分类出售必须复用现有服务。肥料成功作用于有效目标后才扣除数量，失败保持道具与地块状态不变，由 D/B 对齐操作契约和保存字段。自动化、运输和离线收益若未来立项，先单独冻结接口和存档字段，再实现业务。

## 季节环境资源 v1（A 已交付，C/E 待接入）

`:/seasons/manifest.json` 提供与 SeasonCalendar 一致的四季 ID、七张原始纹理绑定和精确 RGB 映射；`:/seasons/decorations.json` 提供四种矩形像素图案及十二个可达草地点。只处理白名单环境图，保留 alpha/尺寸/旧 GID；原始纹理按季节生成缓存，地图、作物和存档不重置。装饰在地图后、角色/HUD 前绘制，有锄地/作物则跳过，无碰撞和业务效果。完整字段与渲染参考见 [SEASON_ASSETS](SEASON_ASSETS.md)，消费者审查和游戏实际联调待进行。
