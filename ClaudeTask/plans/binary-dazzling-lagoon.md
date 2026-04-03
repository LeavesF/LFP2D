# Context

当前已有一套完整的“遗物商店”链路：世界地图节点携带 `ShopID`，`ALFPWorldMapPlayerController` 进入节点后打开 `ULFPShopWidget`，再通过 `ULFPGameInstance` 的商店/金币接口完成购买。现在要参考这套模式实现“雇佣市场”，目标是让玩家在世界地图上进入一个独立节点类型的市场，用金币购买单位，并直接接入现有的队伍/备战营系统。

本次实现采用用户已确认的约束：
- 新建独立的 `ULFPHireMarketDataAsset`，不复用 `ShopDataAsset`
- 世界地图新增独立节点类型 `WNT_HireMarket`
- 只消耗金币
- 同种单位允许重复购买

## 推荐实现方案

### 1. 新建雇佣市场数据层

新增以下文件：
- `Source/LFP2D/Shop/LFPHireMarketTypes.h`
- `Source/LFP2D/Shop/LFPHireMarketDataAsset.h`
- `Source/LFP2D/Shop/LFPHireMarketDataAsset.cpp`

实现内容：
- 定义 `FLFPHireMarketUnitEntry`：`UnitTypeID` + `Price`
- 定义 `FLFPHireMarketDefinition`：`DisplayName` + `UnitList`
- 定义 `ULFPHireMarketDataAsset`：`TMap<FName, FLFPHireMarketDefinition> HireMarketMap`
- 提供 `FindHireMarketDefinition()`，实现方式直接复用现有 `ULFPShopDataAsset::FindShopDefinition()` 的查表模式

可复用参考：
- `Source/LFP2D/Shop/LFPRelicTypes.h`
- `Source/LFP2D/Shop/LFPShopDataAsset.h`

### 2. 在 GameInstance 中接入雇佣市场与购买单位逻辑

修改：
- `Source/LFP2D/Core/LFPGameInstance.h`
- `Source/LFP2D/Core/LFPGameInstance.cpp`

实现内容：
- 增加 `HireMarketDataAsset` 引用
- 增加 `FindHireMarketDefinition()`
- 增加“花金币购买单位”的接口，职责建议拆成：
  - 校验 `UnitTypeID` 是否存在于 `UnitRegistry`
  - 校验金币是否足够并扣除
  - 生成 `FLFPUnitEntry`（`TypeID` + 从注册表读取的 `Tier`）
  - 返回给 UI 层决定是直接 `TryAddUnit()`，还是走替换 UI

这里不建议把“队伍已满后的替换 UI”硬塞进 `GameInstance`，因为现有 `ULFPUnitReplacementWidget` 属于 UI 流程，放在雇佣市场 Widget 内编排更自然，也更符合当前项目的职责分层。

可复用参考：
- 商店查表：`LFPGameInstance::FindShopDefinition()`
- 金币逻辑：`LFPGameInstance::SpendGold()` / `CanAffordGold()`
- 队伍接纳：`LFPGameInstance::TryAddUnit()`
- 单位数据来源：`ULFPUnitRegistryDataAsset::FindEntry()` / `GetUnitTier()`

### 3. 世界地图节点与 CSV 数据扩展为雇佣市场

修改：
- `Source/LFP2D/WorldMap/LFPWorldMapData.h`
- `Source/LFP2D/WorldMap/LFPWorldMapNode.h`
- `Source/LFP2D/WorldMap/LFPWorldMapNode.cpp`

实现内容：
- 在 `ELFPWorldNodeType` 中新增 `WNT_HireMarket`
- 在 `FLFPWorldNodeRow` 中新增 `HireMarketID`
- 在 `ALFPWorldMapNode` 中新增 `HireMarketID`
- 在 `InitFromRowData()` / `ExportToRowData()` 中读写 `HireMarketID`

这样世界地图 CSV、运行时节点、编辑器三层会继续保持现在的同构设计。

可复用参考：
- 现有 `ShopID` 字段链路：`LFPWorldMapData.h` + `LFPWorldMapNode.h/.cpp`

### 4. 扩展世界地图编辑器，支持配置雇佣市场节点

修改：
- `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.h`
- `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.cpp`
- `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h`
- `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.cpp`

实现内容：
- 在编辑器笔刷中新增 `BrushHireMarketID`
- 放置节点 / 套用节点参数时，把 `BrushHireMarketID` 写入节点
- 在编辑器 Widget 中新增 `HireMarketIDInput`
- 节点类型下拉框新增 `WNT_HireMarket`
- 选中节点信息时显示并回填 `HireMarketID`

这样编辑体验会和现有 `ShopID` 配置方式一致。

可复用参考：
- `BrushShopID` 与 `OnShopIDChanged()` 的整条实现
- `UpdateNodeInfo()` 里对 `WNT_Shop` 的显示逻辑

### 5. 新建雇佣市场 UI，整体镜像遗物商店

新增以下文件：
- `Source/LFP2D/UI/WorldMap/LFPHireMarketWidget.h`
- `Source/LFP2D/UI/WorldMap/LFPHireMarketWidget.cpp`

实现内容：
- 新建 `ULFPHireMarketWidget`，整体结构尽量镜像 `ULFPShopWidget`
- `Setup(GI, HireMarketID, Definition)` 缓存数据并刷新 UI
- 左侧列表显示可购买单位（名字 + 价格）
- 右侧详情显示单位图标、名称、Tier、种族、基础属性/高级属性摘要
- 购买按钮只根据金币是否足够决定可用性，不做“已拥有”限制
- 点击购买后：
  1. 向 `GameInstance` 请求扣钱并生成 `FLFPUnitEntry`
  2. 先尝试 `TryAddUnit()`
  3. 若队伍与备战营都满，则复用 `ULFPUnitReplacementWidget` 让玩家选择替换或丢弃
  4. 购买完成后刷新金币与列表状态

建议不要把“已购买一次后变灰”加进来，因为用户已确认允许重复购买。

可复用参考：
- UI骨架：`Source/LFP2D/UI/WorldMap/LFPShopWidget.h/.cpp`
- 单位展示数据：`Source/LFP2D/Core/LFPUnitRegistryDataAsset.h`
- 满编替换：`Source/LFP2D/UI/WorldMap/LFPUnitReplacementWidget.h/.cpp`

### 6. 在世界地图玩家控制器中增加雇佣市场入口

修改：
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.h`
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.cpp`

实现内容：
- 增加 `HireMarketWidgetClass` / `HireMarketWidget`
- 增加 `OpenHireMarket()` / `OnHireMarketWidgetClosed()`
- 在 `EnterNode()` 的 switch 中新增 `WNT_HireMarket` 分支
- `OpenHireMarket()` 的结构直接镜像现有 `OpenShop()`：
  - 读 `HireMarketID`
  - `GI->FindHireMarketDefinition()`
  - 创建/显示 `ULFPHireMarketWidget`
  - 调 `Setup()`

这样运行时入口会完全贴合现有商店实现，后续维护成本最低。

可复用参考：
- `EnterNode()` 中的 `WNT_Shop`
- `OpenShop()` / `OnShopWidgetClosed()`

### 7. Town 建筑入口先保持最小实现

本轮建议先只实现“世界地图独立节点型雇佣市场”，暂不把 Town 建筑系统一起扩到 `HireMarket`。

原因：
- 用户当前明确目标是“参考遗物商店实现雇佣市场”，核心链路是独立商店节点
- 现有 Town 建筑枚举、TownWidget、编辑器勾选项会带来额外改动面
- 可以先把主链路跑通，再看是否需要把 Town 也接入

如果后续要接 Town，可按当前 `TBT_Shop` 的模式追加，不会和本轮方案冲突。

## 关键复用点

优先复用以下已有实现，不额外发明新模式：
- 商店数据资产查表：`Source/LFP2D/Shop/LFPShopDataAsset.h`
- 商店打开流程：`Source/LFP2D/WorldMap/LFPWorldMapPlayerController.cpp`
- 商店 UI 刷新模式：`Source/LFP2D/UI/WorldMap/LFPShopWidget.cpp`
- 金币系统：`Source/LFP2D/Core/LFPGameInstance.h/.cpp`
- 单位注册表：`Source/LFP2D/Core/LFPUnitRegistryDataAsset.h/.cpp`
- 单位入队：`Source/LFP2D/Core/LFPGameInstance::TryAddUnit()`
- 满编替换 UI：`Source/LFP2D/UI/WorldMap/LFPUnitReplacementWidget.h/.cpp`
- 世界地图节点 CSV 同步：`Source/LFP2D/WorldMap/LFPWorldMapNode.cpp`
- 世界地图编辑器笔刷同步：`Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.*` + `UI/WorldMapEditor/LFPWorldMapEditorWidget.*`

## 预计修改文件

新增：
- `Source/LFP2D/Shop/LFPHireMarketTypes.h`
- `Source/LFP2D/Shop/LFPHireMarketDataAsset.h`
- `Source/LFP2D/Shop/LFPHireMarketDataAsset.cpp`
- `Source/LFP2D/UI/WorldMap/LFPHireMarketWidget.h`
- `Source/LFP2D/UI/WorldMap/LFPHireMarketWidget.cpp`

修改：
- `Source/LFP2D/Core/LFPGameInstance.h`
- `Source/LFP2D/Core/LFPGameInstance.cpp`
- `Source/LFP2D/WorldMap/LFPWorldMapData.h`
- `Source/LFP2D/WorldMap/LFPWorldMapNode.h`
- `Source/LFP2D/WorldMap/LFPWorldMapNode.cpp`
- `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.h`
- `Source/LFP2D/WorldMap/LFPWorldMapEditorComponent.cpp`
- `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h`
- `Source/LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.cpp`
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.h`
- `Source/LFP2D/WorldMap/LFPWorldMapPlayerController.cpp`

## 验证方式

### 代码验证
- 编译 `LFP2D` 模块，确认新增类型、头文件引用、UCLASS/USTRUCT 反射都通过
- 检查世界地图节点 CSV 的导入/导出没有丢字段

### 运行验证
1. 创建 `HireMarketDataAsset`，配置至少一个 `HireMarketID`
2. 在世界地图编辑器中放置 `WNT_HireMarket` 节点并填写 `HireMarketID`
3. 保存并重新加载地图，确认节点类型和 `HireMarketID` 能正确回读
4. 进入雇佣市场节点，确认能打开 `HireMarketWidget`
5. 金币足够时购买单位：
   - 队伍未满时进入 `PartyUnits`
   - 队伍满且备战营未满时进入 `ReserveUnits`
6. 队伍和备战营都满时购买单位：
   - 弹出 `UnitReplacementWidget`
   - 测试替换队伍、替换备战营、丢弃新单位三条路径
7. 测试同一单位重复购买，确认不会被“已拥有”逻辑拦截
8. 金币不足时购买按钮应禁用或购买失败且不扣钱

### UE 编辑器手动配置
- 新建 `WBP_HireMarket`（父类 `ULFPHireMarketWidget`）
- 在 `BP_WorldMapPlayerController` 上配置 `HireMarketWidgetClass`
- 在 `GameInstance` 蓝图上配置 `HireMarketDataAsset`
- 为 `WNT_HireMarket` 配置节点视觉数据资产
