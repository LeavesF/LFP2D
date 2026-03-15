# AI 技能仇恨值目标选择系统

## 背景
LFP2D 中技能由玩家操作和敌人 AI 共享。当前 AI 通过 `CalculateThreatValue` 选择普通攻击目标。新需求：每个技能拥有独立的仇恨值计算公式，AI 释放技能时通过该公式遍历所有玩家单位，选出仇恨值最高的作为攻击目标。

---

## 修改的文件

| 文件 | 类型 |
|------|------|
| `Source/LFP2D/Skill/LFPSkillBase.h` | 修改 |
| `Source/LFP2D/Skill/LFPSkillBase.cpp` | 修改 |
| `Source/LFP2D/AI/LFPAIController.h` | 修改 |
| `Source/LFP2D/AI/LFPAIController.cpp` | 修改 |
| `Source/LFP2D/AI/BehaviorTree/BTTask_FindSkillTarget.h` | 新建 |
| `Source/LFP2D/AI/BehaviorTree/BTTask_FindSkillTarget.cpp` | 新建 |

---

## 实现步骤

### Step 1 — `ULFPSkillBase` 添加仇恨值接口

在 `LFPSkillBase.h` 的 `public` 区新增：
```cpp
// AI 目标选择：计算对目标的仇恨值（值越高，越优先攻击）
// Caster: 使用技能的敌方单位（自身属性/位置）
// Target: 候选的玩家单位（目标属性/位置）
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|AI")
float CalculateHatredValue(ALFPTacticsUnit* Caster, ALFPTacticsUnit* Target) const;
```

在 `LFPSkillBase.cpp` 添加默认实现（通用公式，Blueprint 子类可覆盖）：
```cpp
float ULFPSkillBase::CalculateHatredValue_Implementation(ALFPTacticsUnit* Caster, ALFPTacticsUnit* Target) const
{
    if (!Caster || !Target) return 0.0f;

    // 基础仇恨 = 目标攻击力（威胁越大越优先打）
    float Hatred = (float)Target->GetAttackPower();

    // 距离系数（越近仇恨越高）
    int32 Dist = FLFPHexCoordinates::Distance(
        Caster->GetCurrentCoordinates(),
        Target->GetCurrentCoordinates()
    );
    float DistanceFactor = 1.0f / FMath::Max(Dist, 1);

    return Hatred * DistanceFactor;
}
```

### Step 2 — `ALFPAIController` 添加技能目标查找方法

在 `LFPAIController.h` 添加声明（参考已有 `FindBestTarget`）：
```cpp
// 根据技能的仇恨值公式，从所有玩家单位中找到最优攻击目标
UFUNCTION(BlueprintCallable, Category = "AI")
ALFPTacticsUnit* FindBestSkillTarget(ULFPSkillBase* Skill) const;
```
需要在头文件 include `LFPSkillBase.h`（或前向声明 + cpp 中 include）。

在 `LFPAIController.cpp` 实现（复用 `FindBestTarget` 的遍历逻辑）：
```cpp
ALFPTacticsUnit* ALFPAIController::FindBestSkillTarget(ULFPSkillBase* Skill) const
{
    if (!ControlledUnit || !Skill) return nullptr;

    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), AllUnits);

    ALFPTacticsUnit* BestTarget = nullptr;
    float BestHatred = -MAX_FLT;

    for (AActor* Actor : AllUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (Unit && Unit->IsAlive() && Unit->IsAlly())  // 只评估玩家单位
        {
            float Hatred = Skill->CalculateHatredValue(ControlledUnit, Unit);
            if (Hatred > BestHatred)
            {
                BestHatred = Hatred;
                BestTarget = Unit;
            }
        }
    }

    return BestTarget;
}
```

### Step 3 — 新建 BT Task：`BTTask_FindSkillTarget`

仿照 `BTTask_FindTarget` 的结构新建。

**BTTask_FindSkillTarget.h**
```cpp
UCLASS()
class LFP2D_API UBTTask_FindSkillTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FindSkillTarget();
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // 黑板键：输入 — 要使用的技能（Object 类型，指向 ULFPSkillBase）
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector SkillKey;

    // 黑板键：输出 — 最优目标单位（Object 类型，指向 ALFPTacticsUnit）
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetUnitKey;
};
```

**BTTask_FindSkillTarget.cpp**
```cpp
EBTNodeResult::Type UBTTask_FindSkillTarget::ExecuteTask(...)
{
    ALFPAIController* AIController = Cast<ALFPAIController>(OwnerComp.GetAIOwner());
    if (!AIController) return EBTNodeResult::Failed;

    UObject* SkillObj = OwnerComp.GetBlackboardComponent()->GetValueAsObject(SkillKey.SelectedKeyName);
    ULFPSkillBase* Skill = Cast<ULFPSkillBase>(SkillObj);
    if (!Skill) return EBTNodeResult::Failed;

    ALFPTacticsUnit* BestTarget = AIController->FindBestSkillTarget(Skill);
    if (!BestTarget) return EBTNodeResult::Failed;

    OwnerComp.GetBlackboardComponent()->SetValueAsObject(TargetUnitKey.SelectedKeyName, BestTarget);
    return EBTNodeResult::Succeeded;
}
```

---

## 设计要点

| 要点 | 说明 |
|------|------|
| `BlueprintNativeEvent` | C++ 提供默认公式；每个 Blueprint 技能子类（如 `BP_Skill_TestArrow`）可在 Blueprint 中用 `CalculateHatredValue` 事件覆盖，实现专属逻辑 |
| 参数设计 | `Caster`（自身）和 `Target`（目标）均为 `ALFPTacticsUnit*`，可在 Blueprint 中访问全部属性（攻防血速）和位置（`GetCurrentCoordinates`） |
| 距离工具 | 复用已有 `FLFPHexCoordinates::Distance()` 静态函数 |
| BT 集成 | `BTTask_FindSkillTarget` 通过两个黑板键解耦技能输入与目标输出，行为树设计者可灵活组合 |

---

## 验证方法

1. 编译通过（无 C++ 报错）
2. 在 Blueprint 技能子类中能看到 `Calculate Hatred Value` 事件节点，可覆盖
3. 在 Blueprint 中直接调用 `AIController->FindBestSkillTarget(Skill)` 可返回正确目标
4. 在行为树编辑器中能找到 `BTTask_FindSkillTarget` 节点，两个黑板键可绑定
5. 运行游戏：敌人使用技能时，根据技能的仇恨公式选择对应的玩家单位为目标
