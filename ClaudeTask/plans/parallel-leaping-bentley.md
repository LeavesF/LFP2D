# Context

当前项目的单位静态定义与运行时状态是分离的：`FLFPUnitRegistryEntry` 负责 TypeID → 蓝图类/名称/图标/Tier 等元数据，[LFPUnitRegistryDataAsset.h](d:/UE_Project/LFP2D/Source/LFP2D/Core/LFPUnitRegistryDataAsset.h)；`ALFPTacticsUnit` 负责战斗中的属性、血量、移动、速度等运行时状态，[LFPTacticsUnit.h](d:/UE_Project/LFP2D/Source/LFP2D/Unit/LFPTacticsUnit.h)。现在要补全单位构建体系：基础属性、高级属性、唯一种族、多个特殊标签，并且本轮只做到“数据结构 + 运行时初始化/读取”，不做详情 UI。

用户已确认两点：
1. 静态配置入口走“扩展注册表”，不新建独立 UnitDefinition DataAsset。
2. 本轮范围只做数据结构与运行时，不做详情面板显示。

推荐方案是：继续保持“注册表存静态模板、单位实例存运行时值”的现有分层，在注册表条目里补齐默认属性/种族/标签，在单位实例里补齐基础值与当前值双轨，并提供统一初始化入口。这样改动最小，也最贴近现有的 `PartyUnits + UnitRegistry -> SpawnActor(UnitClass)` 创建链路和技能系统已使用 `FGameplayTagContainer` 的现状。[LFPSkillBase.h](d:/UE_Project/LFP2D/Source/LFP2D/Skill/LFPSkillBase.h)

# Recommended approach

## 1. 先补统一的数据结构

### 1.1 在 `ALFPTacticsUnit` 头文件附近新增单位种族枚举
文件：`Source/LFP2D/Unit/LFPTacticsUnit.h`

新增一个唯一种族枚举，例如：
- `ELFPUnitRace::Human`
- `ELFPUnitRace::WoodSpirit`
- `ELFPUnitRace::Featherfolk`
- `ELFPUnitRace::Dragon`
- `ELFPUnitRace::None`（默认值，避免旧数据未配置时报错）

原因：项目当前对“唯一类别”普遍用 `UENUM`（如阵营、目标类型），种族也应沿用同样风格，而不是把唯一值也塞进 TagContainer。

### 1.2 定义基础属性结构与高级属性结构
优先放在 `Source/LFP2D/Unit/LFPTacticsUnit.h`，因为运行时单位会直接使用这些结构；注册表条目也可复用同一结构定义。

建议新增两个 `USTRUCT(BlueprintType)`：

- `FLFPUnitBaseStats`
  - `Attack`
  - `MaxHealth`
  - `MaxMovePoints`
  - `Speed`

- `FLFPUnitAdvancedStats`
  - `AttackCount`
  - `ActionCount`
  - `PhysicalBlock`
  - `SpellDefense`
  - `Weight`

说明：
- 基础属性里只保留你这次明确要求的“基础攻击力 / 最大血量 / 最大移动力 / 速度”的基础模板值。
- “当前攻击力 / 当前最大血量 / 当前移动力 / 当前速度 / 当前血量”属于运行时值，不放注册表静态结构里。
- 高级属性虽然默认值大多相同，但依然需要静态模板值，因此也放结构里，便于在注册表里配置并在实例初始化时复制一份。

### 1.3 在注册表条目中补静态定义字段
文件：`Source/LFP2D/Core/LFPUnitRegistryDataAsset.h`

给 `FLFPUnitRegistryEntry` 增加：
- `ELFPUnitRace Race`
- `FGameplayTagContainer SpecialTags`
- `FLFPUnitBaseStats BaseStats`
- `FLFPUnitAdvancedStats AdvancedStats`

职责：
- 这里保存“单位类型模板”。
- 由 `TypeID` 查到注册表条目后，给运行时单位实例灌默认值。
- 以后 UI、图鉴、队伍面板如果需要展示静态信息，也仍然优先从注册表取。

## 2. 在单位实例中补“基础值 + 当前值”双轨

文件：`Source/LFP2D/Unit/LFPTacticsUnit.h/.cpp`

### 2.1 新增单位实例字段
在 `ALFPTacticsUnit` 中新增并明确分层：

#### 身份/标签
- `UnitRace`
- `SpecialTags`

#### 基础模板值（实例上保留一份，便于运行时比较与恢复）
- `BaseAttack`
- `BaseMaxHealth`
- `BaseMaxMovePoints`
- `BaseSpeed`
- `BaseAttackCount`
- `BaseActionCount`
- `BasePhysicalBlock`
- `BaseSpellDefense`
- `BaseWeight`

#### 当前运行时值
- `CurrentAttack`
- `CurrentMaxHealth`
- `CurrentHealth`
- `CurrentMaxMovePoints`
- `CurrentMovePoints`
- `CurrentSpeed`
- `CurrentAttackCount`
- `CurrentActionCount`
- `CurrentPhysicalBlock`
- `CurrentSpellDefense`
- `CurrentWeight`

说明：
- 现有 `MaxHealth / AttackPower / Speed / MaxMovePoints / CurrentMovePoints / CurrentHealth / Defense` 不建议长期继续混用。
- 本轮应把新体系立起来，并逐步把旧读取逻辑转接到新 getter 上，减少未来继续堆字段。

### 2.2 为兼容现有逻辑，保留旧接口名，但内部改读新字段
优先补 getter / setter，而不是让外部代码继续直接读裸字段。

建议新增或调整：
- `GetBaseAttack()` / `GetCurrentAttack()`
- `GetBaseMaxHealth()` / `GetCurrentMaxHealth()` / `GetCurrentHealth()`
- `GetBaseMaxMovePoints()` / `GetCurrentMaxMovePoints()` / `GetCurrentMovePoints()`
- `GetBaseSpeed()` / `GetCurrentSpeed()`
- `GetAttackCount()` / `GetActionCount()`
- `GetPhysicalBlock()` / `GetSpellDefense()` / `GetWeight()`
- `GetUnitRace()`
- `GetSpecialTags()`
- `HasSpecialTag(FGameplayTag Tag)`
- `HasAnySpecialTags(const FGameplayTagContainer& Tags)`

兼容策略：
- `GetAttackPower()` 先转为返回 `CurrentAttack`
- `GetSpeed()` 先转为返回 `CurrentSpeed`
- `GetMaxHealth()` 先转为返回 `CurrentMaxHealth`
- `GetMaxMovePoints()` 修正为返回 `CurrentMaxMovePoints`
- `GetMovementRange()` 现阶段直接返回 `CurrentMovePoints`，但应明确这是“当前可用移动力”，不是旧的 `MovementRange` 静态字段

同时建议清理方向：
- `MovementRange` 旧字段若已无真实用途，本轮可计划移除或废弃不用
- `Defense` 旧字段不再继续扩展；若现有伤害结算暂时仍要用，可先桥接到 `CurrentPhysicalBlock` 或临时保留兼容，待战斗公式统一时再收口

## 3. 增加统一初始化入口，避免玩家单位和敌方单位各写一套赋值

文件：`Source/LFP2D/Unit/LFPTacticsUnit.h/.cpp`

新增一个统一入口，例如：
- `bool InitializeFromRegistry(ULFPUnitRegistryDataAsset* Registry)`
或
- `bool ApplyRegistryEntry(const FLFPUnitRegistryEntry& Entry)`

推荐职责：
1. 依据 `UnitTypeID` 找注册表条目。
2. 写入 `UnitRace`、`SpecialTags`。
3. 写入全部基础值字段。
4. 将当前值初始化为基础值。
   - `CurrentAttack = BaseAttack`
   - `CurrentMaxHealth = BaseMaxHealth`
   - `CurrentHealth = CurrentMaxHealth`
   - `CurrentMaxMovePoints = BaseMaxMovePoints`
   - `CurrentMovePoints = CurrentMaxMovePoints`
   - `CurrentSpeed = BaseSpeed`
   - 其余高级属性同理

这样后续遗物、Buff、Debuff 都只在初始化后叠加到“当前值”上，基础值始终保留作为参照。

## 4. 改造单位创建链路，让注册表模板真正驱动实例属性

### 4.1 玩家部署生成
关键文件：`Source/LFP2D/Player/LFPTacticsPlayerController.cpp`
关键函数：`StartPlacingUnit()`

当前链路已经有：
- `Entry = GI->PartyUnits[PartyIndex]`
- `UnitClass = GI->UnitRegistry->GetUnitClass(Entry.TypeID)`
- `SpawnActor<ALFPTacticsUnit>(UnitClass)`
- 设置 `Affiliation / UnitTypeID / UnitTier`
- `GI->ApplyOwnedRelicsToUnit(PlacingUnitPreview)`

建议调整顺序：
1. SpawnActor
2. 设置 `UnitTypeID / UnitTier / Affiliation`
3. 调用 `InitializeFromRegistry(GI->UnitRegistry)`
4. 再调用 `GI->ApplyOwnedRelicsToUnit(Unit)`

原因：遗物应该基于模板初始化后的基础/当前属性去修正，而不是在蓝图默认值和零散字段上叠加。

### 4.2 关卡预放置敌人
关键文件：`Source/LFP2D/Unit/LFPTacticsUnit.cpp`
关键函数：`BeginPlay()`

当前敌人多半是关卡里预放置的单位蓝图实例，因此它们不会走 `StartPlacingUnit()`。

建议在 `BeginPlay()` 前半段加入：
1. 若 `UnitTypeID != NAME_None`
2. 从 `ULFPGameInstance::UnitRegistry` 获取注册表
3. 调用 `InitializeFromRegistry(UnitRegistry)`
4. 再做 `CurrentHealth` 初始化、血条初始化、AI 初始化

注意：要防止重复初始化。可加一个：
- `bStatsInitialized`

这样无论是：
- 玩家部署生成
- 关卡预放置敌人
都能走同一套注册表模板灌值逻辑。

## 5. 把遗物修正从“旧字段”切到“当前值”体系

关键文件：`Source/LFP2D/Core/LFPGameInstance.cpp`

当前遗物类型已有：
- `RET_MaxHealthFlat`
- `RET_AttackFlat`
- `RET_DefenseFlat`
- `RET_SpeedFlat`

本轮建议：
- `RET_MaxHealthFlat` → 修改 `CurrentMaxHealth`，并同步处理 `CurrentHealth` 上限
- `RET_AttackFlat` → 修改 `CurrentAttack`
- `RET_SpeedFlat` → 修改 `CurrentSpeed`
- `RET_DefenseFlat` → 若暂未重构伤害公式，可先映射到兼容字段；若本轮同步收敛，则改到 `CurrentPhysicalBlock`

关键原则：
- 遗物不改基础值，只改当前值
- 这样能支持“显示基础值 vs 当前值”和后续临时增益效果

## 6. 让技能/AI/回合系统统一改读新 getter，而不是继续直取旧字段

### 6.1 回合系统
关键文件：`Source/LFP2D/Turn/LFPTurnManager.h/.cpp`

`SortUnitsBySpeed()` 应统一使用 `GetSpeed()`；而 `GetSpeed()` 内部改为返回 `CurrentSpeed`。

### 6.2 战斗/伤害逻辑
关键文件：`Source/LFP2D/Unit/LFPTacticsUnit.cpp`

- `ApplyDamageToTarget()` 使用 `GetCurrentAttack()` 或桥接后的 `GetAttackPower()`
- `TakeDamage()` 中原 `Defense` 的使用需要统一到新高级属性读取方式

### 6.3 AI
关键文件：`Source/LFP2D/AI/LFPAIController.h/.cpp`

当前凡是依赖攻击、速度、可行动次数、权重等判断的逻辑，都应优先改读 getter。这样后面加遗物/状态变化后，AI 读到的就是当前有效值。

本轮最低要求是先搜一遍：
- `AttackPower`
- `MaxHealth`
- `Speed`
- `Defense`
- `MaxMovePoints`
- `CurrentMovePoints`

把关键读取点接到 getter。

## 7. 文件改动优先顺序

### 第一优先级：数据模型核心
1. `Source/LFP2D/Unit/LFPTacticsUnit.h`
   - 新增种族枚举、基础/高级属性结构、实例字段、getter、初始化接口
2. `Source/LFP2D/Unit/LFPTacticsUnit.cpp`
   - 实现初始化、兼容 getter、BeginPlay 初始化链路、战斗读值切换
3. `Source/LFP2D/Core/LFPUnitRegistryDataAsset.h`
   - 给注册表条目加 Race / SpecialTags / BaseStats / AdvancedStats

### 第二优先级：实例创建与模板灌值
4. `Source/LFP2D/Player/LFPTacticsPlayerController.cpp`
   - 在 `StartPlacingUnit()` 中接入 `InitializeFromRegistry`
5. `Source/LFP2D/Core/LFPGameInstance.cpp`
   - `ApplyOwnedRelicsToUnit()` 改为修正当前值体系

### 第三优先级：运行时读取统一
6. `Source/LFP2D/Turn/LFPTurnManager.cpp`
   - 速度排序与可能的其他读取点统一走 getter
7. `Source/LFP2D/AI/LFPAIController.cpp`
   - 攻击/速度/防御等读取改走 getter
8. 其他直接读取旧字段的文件
   - 根据搜索结果做最小修正

## 8. 实施步骤

1. 在 `LFPTacticsUnit.h` 定义 `ELFPUnitRace`、`FLFPUnitBaseStats`、`FLFPUnitAdvancedStats`。
2. 在 `LFPUnitRegistryDataAsset.h` 扩展 `FLFPUnitRegistryEntry`，增加 Race / SpecialTags / 两套静态属性结构。
3. 在 `ALFPTacticsUnit` 中增加 Race、Tags、基础值字段、当前值字段，以及统一 getter。
4. 在 `LFPTacticsUnit.cpp` 实现 `InitializeFromRegistry` / `ApplyRegistryEntry`，把“当前值 = 基础值”的初始化集中到一个函数。
5. 在 `BeginPlay()` 中为关卡预放置单位接入注册表初始化，并加防重入标记。
6. 在 `StartPlacingUnit()` 中为玩家部署单位接入注册表初始化，并确保在遗物应用之前完成。
7. 调整 `ApplyOwnedRelicsToUnit()`，改为只修改当前值体系。
8. 将回合排序、伤害计算、AI 评估等关键读取点改为走 getter，避免继续依赖旧字段。
9. 最后清理明显错误/混用接口，尤其是：
   - `GetMaxMovePoints()` 返回值错误
   - `GetMovementRange()` 与 `MovementRange` 命名混用

# Reuse from existing code

- 单位类型到静态定义的主入口：`ULFPUnitRegistryDataAsset::FindEntry` / `GetUnitClass()`
  - `Source/LFP2D/Core/LFPUnitRegistryDataAsset.h`
- 玩家部署创建链路：`ALFPTacticsPlayerController::StartPlacingUnit()`
  - `Source/LFP2D/Player/LFPTacticsPlayerController.cpp`
- 运行时单位初始化主入口：`ALFPTacticsUnit::BeginPlay()`
  - `Source/LFP2D/Unit/LFPTacticsUnit.cpp`
- 现有技能标签实现：`ULFPSkillBase::SkillTags`
  - `Source/LFP2D/Skill/LFPSkillBase.h`
- 遗物修正入口：`ULFPGameInstance::ApplyOwnedRelicsToUnit()`
  - `Source/LFP2D/Core/LFPGameInstance.cpp`
- 速度排序读取点：`ALFPTurnManager::SortUnitsBySpeed()`
  - `Source/LFP2D/Turn/LFPTurnManager.cpp`

# Verification

1. 编译项目，确保新增结构、枚举、getter 与包含关系通过。
2. 在 `UnitRegistry` 里给至少 2 个单位配置：
   - 不同基础属性
   - 不同高级属性
   - 不同 Race
   - 1~2 个特殊标签
3. 进入部署阶段，放置玩家单位，确认生成后实例已拿到注册表属性，且遗物加成能叠加到当前值。
4. 在战斗地图里放置一个预放置敌人，只配置 `UnitTypeID`，确认 `BeginPlay()` 后也能从注册表灌值成功。
5. 验证回合顺序是否按 `CurrentSpeed` 生效。
6. 验证攻击/受伤是否使用新 getter 读到的当前值。
7. 如果保留了旧兼容接口，重点检查：
   - `GetAttackPower()`
   - `GetSpeed()`
   - `GetMaxHealth()`
   - `GetMaxMovePoints()`
   返回是否已与新体系一致。
8. 运行一次基础战斗流程：部署 → 敌方规划 → 行动阶段，确认没有因旧字段残留导致空值或排序/伤害异常。
