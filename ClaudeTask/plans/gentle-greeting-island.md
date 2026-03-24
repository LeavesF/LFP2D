# Context
用户要做的是**遗物商店**，不是卖单位的商店；卖单位的版本后续会单独命名为“雇佣市场”。当前代码里已经有商店入口预留，但还没有真正的商店逻辑，也**完全没有遗物系统**。

已确认的需求：
- 第一期开的是**遗物商店**
- 商店配置采用 **ShopID + DataAsset**
- 购买遗物后要**立即生效**
- 第一批效果优先做**战斗层被动效果**
- 目标是先做一个最小可用闭环：**世界地图进入商店 → 购买遗物 → 持久化拥有状态 → 下一场战斗自动生效**

# Recommended Approach
## 1. 先补齐“遗物数据 + 商店数据 + 持久化拥有状态”三层
建议新增一套独立的 Relic/Shop 数据定义，不把遗物硬塞进 `Town` 或 `WorldMapNode`。

推荐最小结构：
- `RelicID -> 遗物定义`
  - 名称、图标、描述
  - 效果列表（第一期只支持战斗被动数值型）
- `ShopID -> 商店配置`
  - 商店名
  - 售卖列表（RelicID + Price）
- `OwnedRelicIDs`
  - 玩家当前已拥有遗物，保存在 `ULFPGameInstance`

第一期遗物效果建议只支持**稳定的被动数值型**，不要一上来做触发式/事件式效果。最小效果枚举建议：
- MaxHealthFlat
- AttackFlat
- DefenseFlat
- SpeedFlat

这样能直接复用现有单位属性字段，不需要先造新的战斗事件系统。

## 2. 节点层只负责挂 ShopID，不直接存商品内容
在世界地图节点数据里新增 `ShopID` 字段：
- `FLFPWorldNodeRow`
- `ALFPWorldMapNode`
- CSV 导入/导出
- 世界地图编辑器参数面板

这样：
- Town 节点可以通过 `TownBuildingList` 提供“商店入口”，再通过节点自身 `ShopID` 决定卖什么
- `WNT_Shop` 独立商店节点也能复用同一套 `ShopWidget`
- 后续“雇佣市场”也可以继续沿用 `ShopID` 分流，而不是推翻入口结构

## 3. 商店入口继续复用现有 Town / WorldMapPlayerController 流程
复用现有模式，不新建额外 Manager：
- `ALFPWorldMapPlayerController::EnterNode()` 处理 `WNT_Shop`
- `ALFPWorldMapPlayerController::OnTownBuildingRequested()` 处理 `TBT_Shop`
- 新增 `OpenShop(...)` / `OnShopWidgetClosed()`

行为对齐现有升华塔：
- 从 Town 打开商店时：先隐藏 `TownWidget`
- 商店关闭后：若是从 Town 进入，则恢复 `TownWidget`
- 从 `WNT_Shop` 打开时：关闭后仅返回世界地图

## 4. ShopWidget 复用现有动态列表 UI 模式
新增 `ULFPShopWidget`，结构参考：
- 入口/关闭逻辑参考 `UI/WorldMap/LFPUnitMergeWidget.cpp`
- 动态商品列表参考 `UI/Fighting/LFPSkillSelectionWidget.cpp`
- 单项按钮/卡片参考 `Skill/LFPSkillButtonWidget.cpp`

第一期 UI 保持最小：
- 顶部：商店名、当前金币
- 中间：遗物列表
- 右侧或下方：选中遗物详情（图标/名称/描述/效果/价格）
- 底部：购买按钮、关闭按钮

商品项状态：
- 已拥有：禁用并显示已拥有
- 金币不足：禁用或提示不足
- 可购买：允许购买

## 5. 购买逻辑集中放在 GameInstance，保证事务完整
在 `ULFPGameInstance` 中新增最小商店/遗物接口：
- `HasRelic(RelicID)`
- `CanAffordGold(Cost)`
- `SpendGold(Cost)`
- `TryPurchaseRelic(RelicID, Cost)`
- `GetOwnedRelics()` 或直接暴露只读集合

`TryPurchaseRelic` 里统一处理：
1. 遗物是否存在
2. 是否已拥有
3. 金币是否足够
4. 扣金币
5. 写入 `OwnedRelicIDs`

不要把扣钱放在 Widget 里，避免 UI 层和状态层分散导致“扣了钱但没拿到遗物”。

## 6. 遗物效果先在“我方单位生成时”统一应用
第一期既然是**战斗被动遗物**，最稳的接线点不是技能系统，而是**玩家布置单位生成 Actor 时**。

推荐做法：
- 在 `ULFPGameInstance` 或一个轻量 Relic 工具类里，新增“将已拥有遗物效果应用到单位”的函数
- 在 `ALFPTacticsPlayerController::StartPlacingUnit()` 生成预览单位后立刻调用

原因：
- 这里已经拿到了真正的 `ALFPTacticsUnit` 实例
- 现有属性字段都在 `ALFPTacticsUnit` 上（`MaxHealth`、`CurrentHealth`、`AttackPower`、`Defense`、`Speed`）
- 改动集中，不需要先改 AI/技能/AP 系统

实现约束：
- 只对玩家单位应用
- 每次生成单位 Actor 时按“当前已拥有遗物”重新计算一次
- 若修改 `MaxHealth`，同步修正 `CurrentHealth`，确保开场满血

这意味着“立即生效”的语义是：**购买后从下一场战斗开始立即生效**。对世界地图当下不需要额外表现层。

## 7. 命名上把“商店”和“雇佣市场”分开
这次只实现“遗物商店”，文件和数据命名也应显式区分：
- `Shop` = 遗物商店
- `MercenaryMarket` / `HireMarket` = 以后卖单位

避免后面再把现在的类大面积重命名。

# Critical Files To Modify
## 现有文件
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.h`
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.cpp`
- `Source/LFP2D/WorldMap/LFPWorldMapData.h`
- `Source/LFP2D/WorldMap/LFPWorldMapNode.h`
- `Source/LFP2D/WorldMap/LFPWorldMapNode.cpp`
- `Source/LFP2D/WorldMap/LFPWorldMapManager.cpp`  （CSV 头与导出）
- `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.h`
- `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.cpp`
- `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h`
- `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.cpp`
- `Source/LFP2D/Core/LFPGameInstance.h`
- `Source/LFP2D/Core/LFPGameInstance.cpp`
- `Source/LFP2D/Player/LFPTacticsPlayerController.cpp`

## 建议新增文件
- `Source/LFP2D/Shop/LFPRelicTypes.h`  （遗物/商店结构与枚举）
- `Source/LFP2D/Shop/LFPRelicDataAsset.h/.cpp`  （RelicID -> 定义）
- `Source/LFP2D/Shop/LFPShopDataAsset.h/.cpp`  （ShopID -> 商店配置）
- `Source/LFP2D/UI/WorldMap/LFPShopWidget.h/.cpp`
- 可选：`Source/LFP2D/UI/WorldMap/LFPShopItemWidget.h/.cpp`

# Existing Code To Reuse
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.cpp`
  - `EnterNode()`：已有 `WNT_Shop` 入口预留
  - `OpenTown()`：Town 打开流程模板
  - `OpenEvolutionTower()`：二级功能面板模板
  - `OnTownBuildingRequested()`：已有 `TBT_Shop` 分发点
- `Source/LFP2D/UI/Town/LFPTownWidget.cpp`
  - `Setup()`：根据建筑列表显示入口
  - `OnShopClicked()`：已有商店按钮广播
- `Source/LFP2D/UI/WorldMap/LFPUnitMergeWidget.cpp`
  - `Setup()` / `RefreshUnitIcons()` / `UpdatePreview()`：最接近商店的动态列表+详情+关闭返回模式
- `Source/LFP2D/Core/LFPGameInstance.cpp`
  - `AddGold()` / `GetGold()`：现有金币资源入口
- `Source/LFP2D/Player/LFPTacticsPlayerController.cpp`
  - `StartPlacingUnit()`：玩家单位生成后的最佳遗物效果挂点
- `Source/LFP2D/Unit/LFPTacticsUnit.h`
  - 现有可直接修改的战斗属性：`MaxHealth`、`CurrentHealth`、`AttackPower`、`Defense`、`Speed`

# Implementation Order
1. 新增遗物/商店数据结构与 DataAsset
2. 给 `GameInstance` 增加拥有遗物与购买接口
3. 给世界地图节点增加 `ShopID`，打通 CSV 与编辑器
4. 新增 `ShopWidget`，先完成只读展示与金币显示
5. 在 `WorldMapPlayerController` 接入 Town 商店入口和 `WNT_Shop`
6. 接入购买逻辑与“已拥有/金币不足”状态刷新
7. 在 `LFPTacticsPlayerController::StartPlacingUnit()` 接入遗物效果应用
8. 用 2~3 个简单数值遗物做端到端验证

# Risks / Notes
- 当前代码库里**没有任何遗物系统**，所以第一期不要混入“背包道具”“主动使用道具”“掉落遗物”“触发式效果”。
- `ShopID` 加入 CSV 后，要同步修改保存头、导入结构、编辑器输入框，否则地图编辑流程会断。
- 若未来要做“雇佣市场”，不要复用 `OwnedRelicIDs` 或当前商店商品结构去硬兼容单位购买；只复用入口框架和 ShopID 思路。
- 如果第一批遗物包含太复杂的效果（例如回合触发、技能改写、AP 操作），会明显扩大范围；建议首批只放数值型遗物。

# Verification
1. 编译项目，至少通过 Editor/Development 构建。
2. 在世界地图编辑器里给一个 Town 节点和一个 `WNT_Shop` 节点分别配置 `ShopID`。
3. 为该 `ShopID` 配置 2~3 个遗物，价格不同，效果分别设置为 `MaxHealthFlat` / `AttackFlat` / `DefenseFlat`。
4. 进入 Town 点击商店：
   - 能打开商店
   - 关闭后能返回 Town
5. 进入 `WNT_Shop` 节点：
   - 能直接打开同一个商店 UI
   - 关闭后不弹 Town
6. 购买测试：
   - 金币足够时购买成功，金币减少，遗物变为已拥有
   - 金币不足时无法购买
   - 已拥有遗物不能重复购买
7. 战斗验证：
   - 购买遗物后进入下一场战斗
   - 玩家单位生成时属性已被修改
   - `MaxHealth` 类遗物会同步影响 `CurrentHealth`
8. 回到世界地图再进下一场战斗，遗物效果仍然存在，说明持久化链路正确。
