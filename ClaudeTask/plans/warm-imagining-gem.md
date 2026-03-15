# 阵营行动点（AP）+ 技能优先级系统重构计划

## Context

当前 AP 绑定在每个棋子上，需要改为按阵营共享。同时引入技能优先级系统，确保 AP 分配不会总是被速度最快的敌人独占。

### 关键设计决策
- **阵营 AP**: 初始1，上限3，每轮回复1，双方各自独立
- **消耗时机**: 规划阶段分配时就扣除（确保不会超发）
- **技能后移动**: 使用技能后该单位本回合不能再移动
- **AP 耗尽**: 每个角色有 0 消耗基础技能，不影响行动
- **规划两步走**: 先按技能优先级全局分配 AP 技能，再按速度顺序移动展示

### 技能优先级机制
- 每个技能有 `SkillPriority`（当前值）、`BasePriority`（基础值）
- **释放后降低**: 释放后 SkillPriority -= PriorityDecreaseOnUse
- **每轮回复**: 每轮 SkillPriority += PriorityRecoveryPerRound，不超过上限
- **条件提升**: 特定游戏条件下本轮临时提升（预留 virtual 接口，后续实现）
- 0 消耗技能不参与优先级竞争，直接可用

---

## 实现步骤

### Step 1: SkillBase 新增优先级属性

**修改文件**: `Source/LFP2D/Skill/LFPSkillBase.h` 和 `.cpp`

**Header 新增属性**:
```cpp
// 技能优先级（当前运行时值）
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Priority")
float SkillPriority;

// 基础优先级（设计值，也是上限）
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Priority")
float BasePriority = 50.0f;

// 释放后优先级下降值
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Priority")
float PriorityDecreaseOnUse = 30.0f;

// 每轮优先级回复值
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill|Priority")
float PriorityRecoveryPerRound = 10.0f;
```

**新增方法**:
```cpp
// 释放后降低优先级
void OnSkillUsed();
// SkillPriority -= PriorityDecreaseOnUse, Clamp >= 0

// 每轮回复优先级
void RecoverPriority();
// SkillPriority += PriorityRecoveryPerRound, Clamp <= BasePriority

// 条件提升（预留 virtual 接口）
virtual float EvaluateConditionBonus() const;
// 默认返回 0，子类/蓝图可覆盖

// 获取本轮有效优先级（当前值 + 条件加成）
float GetEffectivePriority() const;
// return SkillPriority + EvaluateConditionBonus()
```

**构造函数**: `SkillPriority = BasePriority`

**OnTurnStart()**: 加入 `RecoverPriority()` 调用

---

### Step 2: TurnManager 新增阵营 AP 管理

**修改文件**: `Source/LFP2D/Turn/LFPTurnManager.h` 和 `.cpp`

**Header 新增**:
```cpp
// 配置
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Points")
int32 FactionMaxAP = 3;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Points")
int32 FactionInitialAP = 1;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Points")
int32 FactionAPRecovery = 1;

// 当前阵营 AP
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Action Points")
TMap<EUnitAffiliation, int32> FactionCurrentAP;

// AP 变化委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFactionAPChangedSignature, EUnitAffiliation, Faction, int32, NewAP);
UPROPERTY(BlueprintAssignable)
FOnFactionAPChangedSignature OnFactionAPChanged;

// 方法
int32 GetFactionAP(EUnitAffiliation Faction) const;
bool HasEnoughFactionAP(EUnitAffiliation Faction, int32 Amount) const;
void ConsumeFactionAP(EUnitAffiliation Faction, int32 Amount);
```

**CPP 逻辑**:
- `StartGame()`: `FactionCurrentAP[Player] = FactionCurrentAP[Enemy] = FactionInitialAP`
- `BeginNewRound()`: 每个阵营 `AP = Min(AP + Recovery, MaxAP)`

---

### Step 3: 重构规划阶段为两步流程

**修改文件**: `Source/LFP2D/Turn/LFPTurnManager.h` 和 `.cpp`

**新增辅助结构**（在 LFPBattleTypes.h 或 TurnManager.h 内部）:
```cpp
struct FSkillAllocationCandidate
{
    ALFPTacticsUnit* Unit;
    ULFPSkillBase* Skill;
    float EffectivePriority;
};
```

**修改 BeginEnemyPlanningPhase()**:
```
Step 1: AllocateEnemySkills()
  - 遍历所有敌方单位的所有技能
  - 过滤: IsAvailable() == true（冷却OK）
  - 过滤: ActionPointCost > 0 的技能才参与竞争（0消耗的不需要抢AP）
  - 计算每个技能的 GetEffectivePriority()
  - 按 EffectivePriority 降序排序
  - 依次分配: 如果阵营AP足够 → 标记该单位使用该技能，扣除阵营AP
  - 每个单位最多分配一个AP技能
  - 未分配到AP技能的单位 → 使用默认攻击（0消耗）

Step 2: ProcessNextEnemyPlan()（保持现有按速度顺序）
  - 每个敌人已经知道自己要用什么技能（Step1分配好了）
  - 按速度排序，依次: 选目标 → 移动到释放位置 → 显示图标
```

**具体修改**:

`BeginEnemyPlanningPhase()` 改为:
```cpp
void ALFPTurnManager::BeginEnemyPlanningPhase()
{
    SetPhase(BP_EnemyPlanning);
    // ... 过滤敌方单位 ...

    // Step 1: 全局技能分配（按优先级）
    AllocateEnemySkills();

    // Step 2: 按速度顺序移动展示
    CurrentPlanningEnemyIndex = 0;
    ProcessNextEnemyPlan();
}
```

新增 `AllocateEnemySkills()`:
- 将分配结果存入 `TMap<ALFPTacticsUnit*, ULFPSkillBase*> AllocatedSkills`
- 供 ProcessNextEnemyPlan 中 AIController->CreateActionPlan() 使用

**修改 AIController::CreateActionPlan()**:
- 接受一个可选参数 `ULFPSkillBase* PreAllocatedSkill = nullptr`
- 如果传入了预分配的技能，直接使用；否则 fallback 到 SelectBestSkill()

**修改 AIController::SelectBestSkill()**:
- 不再自己检查 AP（AP 分配已在 Step1 完成）
- 只在没有预分配技能时被调用，选择 0 消耗的最优技能

---

### Step 4: TacticsUnit 的 AP 方法改为代理

**修改文件**: `Source/LFP2D/Unit/LFPTacticsUnit.h` 和 `.cpp`

**删除**: `MaxActionPoints`, `CurrentActionPoints` 属性

**修改**:
- `HasEnoughActionPoints(Required)` → 查询 `TurnManager->HasEnoughFactionAP(Affiliation, Required)`
- `ConsumeActionPoints(Amount)` → 调用 `TurnManager->ConsumeFactionAP(Affiliation, Amount)` + `CurrentMovePoints = 0`
- `ResetForNewRound()` → 删除 `CurrentActionPoints = MaxActionPoints`

---

### Step 5: 技能释放后更新优先级

**修改文件**: `Source/LFP2D/Skill/LFPSkillComponent.cpp`

在 `ExecuteSkill()` 成功后调用 `Skill->OnSkillUsed()` 降低优先级。

---

### Step 6: UI 显示阵营 AP

**修改文件**: `Source/LFP2D/UI/Fighting/LFPSkillSelectionWidget.h` 和 `.cpp`

- 取消注释 `ActionPointsText`
- 显示格式: `"AP: 2/3"`
- 绑定 `TurnManager->OnFactionAPChanged` 更新

---

## 实现顺序

1. Step 1: SkillBase 优先级属性
2. Step 2: TurnManager 阵营 AP
3. Step 4: TacticsUnit AP 代理
4. Step 3: 两步规划流程（依赖 Step 1 和 2）
5. Step 5: 释放后更新优先级
6. Step 6: UI

---

## 关键文件

| 文件 | 改动 |
|------|------|
| `Skill/LFPSkillBase.h/.cpp` | 新增优先级属性和方法 |
| `Turn/LFPTurnManager.h/.cpp` | 阵营AP管理 + AllocateEnemySkills两步规划 |
| `Turn/LFPBattleTypes.h` | 新增 FSkillAllocationCandidate |
| `Unit/LFPTacticsUnit.h/.cpp` | 删除单位AP，方法代理到TurnManager |
| `AI/LFPAIController.h/.cpp` | CreateActionPlan 接受预分配技能参数 |
| `Skill/LFPSkillComponent.cpp` | 释放后调用 OnSkillUsed |
| `UI/Fighting/LFPSkillSelectionWidget.h/.cpp` | 显示阵营AP |

**无需修改**: PlayerController（仍调用 Unit->ConsumeActionPoints）

---

## 规划阶段完整流程示例

假设：敌方阵营AP=1，3个敌方单位（A速度10, B速度8, C速度5），技能情况：
- A 有「火球术」(消耗1AP, 优先级20)
- B 有「治疗术」(消耗1AP, 优先级60)
- C 有「雷击」(消耗1AP, 优先级45)

**Step 1 - AllocateEnemySkills（按优先级）**:
1. B的治疗术(60) → AP够 → 分配，AP: 1→0
2. C的雷击(45) → AP不够 → 跳过，用基础攻击
3. A的火球术(20) → AP不够 → 跳过，用基础攻击

**Step 2 - 按速度移动展示**:
1. A(速度10): 用基础攻击 → 选目标 → 移动 → 显示图标
2. B(速度8): 用治疗术 → 选目标 → 移动 → 显示图标
3. C(速度5): 用基础攻击 → 选目标 → 移动 → 显示图标

**下一轮**: B的治疗术优先级降为30，C的雷击恢复+10变55，此时C更可能获得AP。
