## Context
之前遗物系统暂停的核心原因是单位属性体系不完整。现在单位已完成“基础值/当前值”双轨重建，并且遗物的数据层已经开始扩到战斗规则：`Source/LFP2D/Shop/LFPRelicTypes.h` 已加入 Trigger / Condition / Effect / Duration / Synergy 的结构雏形。同时，持久拥有态已经在 `ULFPGameInstance` 中统一到 `OwnedRelicIDs`、`TryAddOwnedRelic()`、`GetOwnedRelicIDsArray()`，商店闭环也已经打通。

这一步的目标不再只是“商店能买、部署时加数值”，而是先做一个**最小可行的战斗内遗物运行时管理器**，让战斗遗物真正开始在战斗流程中按条件和时机生效。要求是尽量少改现有架构，不引入完整 buff/modifier 大系统，但要先把下面这类效果跑通：
- 血量低于 40% 时，攻击力提高 50%
- 血量低于 40% 时，速度 +2
- 同时拥有两类遗物时，每回合结束恢复 3 点生命

## Recommended Approach

### 1. 新增一个战斗内运行时管理器，生命周期挂在战斗场景
新增轻量 Actor：
- [Source/LFP2D/Turn/LFPBattleRelicRuntimeManager.h](Source/LFP2D/Turn/LFPBattleRelicRuntimeManager.h)
- [Source/LFP2D/Turn/LFPBattleRelicRuntimeManager.cpp](Source/LFP2D/Turn/LFPBattleRelicRuntimeManager.cpp)

职责只做战斗期事情：
- 从 `ULFPGameInstance` 读取已拥有遗物
- 从 `ULFPRelicDataAsset` 读取战斗规则和组合规则
- 扫描并绑定场上玩家单位事件
- 在少数关键时机重算临时遗物效果
- 执行回合结束这类即时效果
- 清理死亡单位的监听与运行时状态

不要把这部分继续塞进 `ULFPGameInstance`。`GameInstance` 继续只负责“永久拥有态”和定义查询。

### 2. 由 `ALFPTurnGameMode` 创建并初始化 runtime manager
修改：
- [Source/LFP2D/Core/LFPTurnGameMode.h](Source/LFP2D/Core/LFPTurnGameMode.h)
- [Source/LFP2D/Core/LFPTurnGameMode.cpp](Source/LFP2D/Core/LFPTurnGameMode.cpp)

做法：
- 在 `StartPlay()` 中像生成 `TurnManager` 一样生成 `BattleRelicRuntimeManager`
- 把 `TurnManager`、`GameInstance` 注入给它
- 由 `TurnGameMode` 持有该 manager 引用，作为战斗态子系统入口

这样 runtime relic 的生命周期和战斗场景一致，不污染跨关卡对象。

### 3. 第一版只支持最小触发集合，不做完整规则引擎
虽然 `LFPRelicTypes.h` 结构已经往通用方向扩了，但第一版运行时**只消费一个小子集**，避免为了“通用”把系统做大。

#### 这一版先支持的 Trigger
- `RTT_BattleStart`
- `RTT_RoundStart`
- `RTT_RoundEnd`
- `RTT_HealthChanged`
- `RTT_KillAfter` 先不真正执行效果，只预留后续挂点所需的上下文空间

更准确地说，第一版真正要落地的事件入口是：
- 部署结束后的首次初始化
- `ALFPTurnManager::BeginNewRound()`
- `ALFPTurnManager::EndCurrentRound()`
- `ALFPTacticsUnit::OnHealthChangedDelegate`
- `ALFPTacticsUnit::OnDeathDelegate`（用于清理，不作为规则触发）

#### 这一版暂不支持
- `RTT_MoveFinished`
- `RTT_UnitTurnStart`
- `RTT_UnitTurnEnd`
- `RTT_AttackAfter`
- `RTT_DamagedAfter`
- 真正可用的 `RTT_KillAfter`
- 复杂目标选择 / 概率 / 多段持续 / 标签增删

结论：第一版是“低血条件 + 回合结束即时效果”的 MVP，不是完整战斗遗物引擎。

### 4. 用“重建当前属性”代替可逆 modifier 系统，避免永久叠属性
当前 `ALFPTacticsUnit` 只有：
- `ResetCurrentStatsToBase()`
- `AddCurrentAttack()`
- `AddCurrentMaxHealth()`
- `AddCurrentSpeed()`
- `AddCurrentPhysicalBlock()`

没有可逆 modifier handle，也没有来源追踪。因此第一版绝对不能在每次事件里直接 `AddCurrentAttack()` 叠加，否则低血条件会永久累加。

#### 采用的策略
对每个玩家单位，当相关事件发生时，由 runtime manager 执行：
1. `ResetCurrentStatsToBase(false)`，保留当前生命值，只把当前属性还原到基础值
2. 重新应用“常驻遗物基线效果”
3. 再根据当前条件应用“临时条件效果”

这样可以保证：
- 血量持续低于 40% 时不会重复叠 +50% 攻击
- 血量从低于 40% 回到高于 40% 时，会自动移除临时加成
- 不需要一开始就引入完整 buff / modifier 框架

#### 重要边界
第一版建议只让条件型运行时效果影响：
- 攻击力
- 速度

先不要做“条件型最大生命变化”，否则 `AddCurrentMaxHealth()` 会触发 `OnHealthChangedDelegate`，容易造成重入，需要额外防护。

### 5. 常驻遗物基线与运行时临时效果分开处理
现有 `ULFPGameInstance::ApplyOwnedRelicsToUnit()` 是部署时一次性应用当前拥有遗物效果。为了避免 runtime manager 和旧逻辑重复叠加，执行策略要明确：

#### 推荐做法
- 保留 `ULFPGameInstance` 的“拥有态 / 定义查询 / 商店购买”职责
- 在 runtime manager 内单独实现“战斗内属性重建”逻辑
- 重建时自行聚合：
  - 静态常驻数值效果（例如攻击 flat、生命 flat、速度 flat、防御 flat）
  - 条件型临时效果（例如低血攻击 +50%、低血速度 +2）

也就是说，runtime manager 不应在重建时直接反复调用 `ApplyOwnedRelicsToUnit()`，而应把它的数值逻辑收口/迁移到一个可重算路径里。

### 6. 组合效果作为 `SynergyRule` 的第一批消费者
第一版就支持你举的组合效果，但只支持最简单的形式：
- 拥有指定多个遗物
- 在 `RoundEnd` 触发
- 对玩家单位执行 `HealFlat`

建议使用 `ULFPRelicDataAsset::SynergyRules` 来表达这类规则，而不是把“需要两个遗物同时拥有”的逻辑硬塞回单个 relic definition。

这样示例可以映射为：
- 遗物 A：低血攻击 +50%
- 遗物 B：低血速度 +2
- 组合规则 AB：若同时拥有 A 和 B，则 `RoundEnd` 时对玩家单位 `HealFlat(3)`

### 7. 第一版建议支持的 Condition / Effect / Duration 子集
#### Condition
- `RCT_None`
- `RCT_HealthPercentBelow`
- `RCT_HasRelic`
- `RCT_HasAllRelics`

#### Effect
- `RBET_ModifyStatFlat`
- `RBET_ModifyStatPercent`
- `RBET_HealFlat`

#### Duration
- `RDT_Instant`
- `RDT_WhileConditionTrue`

其他枚举先允许留在数据结构里，但运行时 manager 对未支持项只打 warning 并忽略，不在这一版真正执行。

### 8. 第一版的关键接入点
#### TurnManager 侧
修改：
- [Source/LFP2D/Turn/LFPTurnManager.h](Source/LFP2D/Turn/LFPTurnManager.h)
- [Source/LFP2D/Turn/LFPTurnManager.cpp](Source/LFP2D/Turn/LFPTurnManager.cpp)

接入位置：
1. `EndDeploymentPhase()`
   - 玩家单位注册完成后，通知 runtime manager 扫描并绑定玩家单位
   - 做首次战斗遗物初始化
2. `BeginNewRound()`
   - 在排序前刷新玩家单位当前运行时遗物属性
   - 这样低血速度变化至少能影响下一轮排序
3. `EndCurrentRound()`
   - 触发回合结束即时遗物效果，例如组合规则回血

#### Unit 侧
复用已有委托：
- [Source/LFP2D/Unit/LFPTacticsUnit.h](Source/LFP2D/Unit/LFPTacticsUnit.h)
- `OnHealthChangedDelegate`
- `OnDeathDelegate`

第一版尽量不改 `AttackTarget()` / `TakeDamage()` 事件模型，不把范围扩到攻击后/击杀后。

### 9. 运行时状态需要记录哪些最小信息
runtime manager 至少维护：
- 已拥有遗物 ID 列表缓存
- 已解析的 BattleRules
- 已解析的 SynergyRules
- 已绑定的玩家单位列表
- 每个单位是否正在重建属性（防止重入）
- 回合结束即时效果的“本轮已执行”标记，避免同一轮重复触发

第一版不要做完整 buff stack，只做：
- 单位级属性重建
- 轮次级即时效果去重

### 10. 对示例效果的具体落地方式
#### A：血量 < 40% 时攻击 +50%
- Trigger: `RTT_HealthChanged`
- Condition: `RCT_HealthPercentBelow(0.4)`
- Effect: `RBET_ModifyStatPercent + RST_Attack + 0.5`
- Duration: `RDT_WhileConditionTrue`

#### B：血量 < 40% 时速度 +2
- Trigger: `RTT_HealthChanged`
- Condition: `RCT_HealthPercentBelow(0.4)`
- Effect: `RBET_ModifyStatFlat + RST_Speed + 2`
- Duration: `RDT_WhileConditionTrue`

#### AB 组合：同时拥有两者时，每回合结束回血 3
- Synergy RequiredRelicIDs: `[A, B]`
- Trigger: `RTT_RoundEnd`
- Effect: `RBET_HealFlat(3)`
- Duration: `RDT_Instant`

### 11. 明确的第一版取舍
- 只作用于玩家单位，不做敌方遗物
- 速度变化不强制重排当前这一轮的已生成行动序列；保证下一轮排序正确即可
- 不做 `MoveFinished / AttackAfter / DamagedAfter / KillAfter` 的实际效果执行
- 不做 tag 增删
- 不做持续 N 回合 / 持续到单位回合结束 这类真正需要状态机的持续效果
- 不做战斗中途获得/失去遗物的热更新

## Critical Files To Modify

### 新增
- [Source/LFP2D/Turn/LFPBattleRelicRuntimeManager.h](Source/LFP2D/Turn/LFPBattleRelicRuntimeManager.h)
- [Source/LFP2D/Turn/LFPBattleRelicRuntimeManager.cpp](Source/LFP2D/Turn/LFPBattleRelicRuntimeManager.cpp)

### 核心战斗入口
- [Source/LFP2D/Core/LFPTurnGameMode.h](Source/LFP2D/Core/LFPTurnGameMode.h)
- [Source/LFP2D/Core/LFPTurnGameMode.cpp](Source/LFP2D/Core/LFPTurnGameMode.cpp)

### 回合流程挂点
- [Source/LFP2D/Turn/LFPTurnManager.h](Source/LFP2D/Turn/LFPTurnManager.h)
- [Source/LFP2D/Turn/LFPTurnManager.cpp](Source/LFP2D/Turn/LFPTurnManager.cpp)

### 遗物数据与拥有态
- [Source/LFP2D/Shop/LFPRelicTypes.h](Source/LFP2D/Shop/LFPRelicTypes.h)
- [Source/LFP2D/Shop/LFPRelicDataAsset.h](Source/LFP2D/Shop/LFPRelicDataAsset.h)
- [Source/LFP2D/Core/LFPGameInstance.h](Source/LFP2D/Core/LFPGameInstance.h)
- [Source/LFP2D/Core/LFPGameInstance.cpp](Source/LFP2D/Core/LFPGameInstance.cpp)

### 单位属性与事件复用
- [Source/LFP2D/Unit/LFPTacticsUnit.h](Source/LFP2D/Unit/LFPTacticsUnit.h)
- [Source/LFP2D/Unit/LFPTacticsUnit.cpp](Source/LFP2D/Unit/LFPTacticsUnit.cpp)

## Existing Code To Reuse
- `ULFPGameInstance::TryAddOwnedRelic()` / `GetOwnedRelicIDsArray()` / `FindRelicDefinition()` / `HasRelic()` in [Source/LFP2D/Core/LFPGameInstance.h](Source/LFP2D/Core/LFPGameInstance.h)
- `ALFPTurnGameMode::StartPlay()` in [Source/LFP2D/Core/LFPTurnGameMode.cpp](Source/LFP2D/Core/LFPTurnGameMode.cpp#L13-L67)
- `ALFPTurnManager::EndDeploymentPhase()` in [Source/LFP2D/Turn/LFPTurnManager.cpp](Source/LFP2D/Turn/LFPTurnManager.cpp#L59-L80)
- `ALFPTurnManager::BeginNewRound()` in [Source/LFP2D/Turn/LFPTurnManager.cpp](Source/LFP2D/Turn/LFPTurnManager.cpp#L82-L114)
- `ALFPTurnManager::EndCurrentRound()` in [Source/LFP2D/Turn/LFPTurnManager.cpp](Source/LFP2D/Turn/LFPTurnManager.cpp#L116-L136)
- `ALFPTacticsUnit::OnHealthChangedDelegate` / `OnDeathDelegate` in [Source/LFP2D/Unit/LFPTacticsUnit.h](Source/LFP2D/Unit/LFPTacticsUnit.h)
- `ALFPTacticsUnit::TakeDamage()` / `Heal()` / `HandleDeath()` in [Source/LFP2D/Unit/LFPTacticsUnit.cpp](Source/LFP2D/Unit/LFPTacticsUnit.cpp#L562-L687)
- `ALFPTacticsUnit::ResetCurrentStatsToBase()` and `AddCurrentAttack()` / `AddCurrentSpeed()` / `AddCurrentPhysicalBlock()` / `AddCurrentMaxHealth()` in [Source/LFP2D/Unit/LFPTacticsUnit.h](Source/LFP2D/Unit/LFPTacticsUnit.h)

## Verification
1. **初始化验证**
   - 进入战斗并完成部署。
   - 确认 runtime manager 已创建，并成功扫描玩家单位。
   - 确认没有敌方单位被错误绑定。

2. **低血攻击加成验证**
   - 给玩家拥有遗物 A。
   - 让单位血量降到 40% 以下，确认攻击提升为基线攻击的 150%。
   - 在持续低血状态下再次受伤，确认不会重复叠加。
   - 把血量治疗回 40% 以上，确认攻击恢复。

3. **低血速度加成验证**
   - 给玩家拥有遗物 B。
   - 让单位血量降到 40% 以下，确认当前速度 +2。
   - 下一轮开始前确认排序刷新能读取新的速度值。
   - 恢复血量后确认速度恢复。

4. **组合规则回血验证**
   - 同时拥有遗物 A 和 B。
   - 回合结束时确认所有存活玩家单位回复 3 点生命。
   - 同一轮内确认不会重复触发两次。
   - 下一轮结束时应再次正常触发。

5. **死亡清理验证**
   - 让一个玩家单位死亡。
   - 确认后续血量变化/回合结束逻辑不会再访问该单位。

6. **回归验证**
   - 现有商店购买、拥有遗物、部署前静态数值遗物链路不回退。
   - 不影响当前 `TryPurchaseRelic()` / `TryAddOwnedRelic()` 行为。

7. **构建验证**
   - 运行 UE C++ 构建命令，确保新 manager 和回合挂点修改无编译错误。

## Notes / Recommended Decisions
- 这一步是 **runtime battle relic MVP**，不是完整战斗遗物引擎。
- 第一版先解决“条件型属性加成 + 回合结束即时效果 + 组合规则”三个核心能力。
- 真正的 `AttackAfter / DamagedAfter / KillAfter / MoveFinished / UnitTurnStart / UnitTurnEnd` 可等 MVP 跑通后再补成统一事件模型。
- 如果后续效果种类快速增加，再考虑把“属性重建”升级为真正的 modifier 系统。