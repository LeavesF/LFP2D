// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/LFPSkillComponent.h"
#include "LFP2D/Core/LFPTurnGameMode.h"
#include "Kismet/GameplayStatics.h"

FEnemyActionPlan ALFPTurnManager::EmptyPlan;

ALFPTurnManager::ALFPTurnManager()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentUnit = nullptr;
}

void ALFPTurnManager::BeginPlay()
{
    Super::BeginPlay();
}

void ALFPTurnManager::StartGame()
{
    // 收集场上所有单位（敌方预放置在关卡中）
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor))
        {
            // 仅注册非玩家单位（玩家单位在布置阶段结束后注册）
            if (Unit->GetAffiliation() != EUnitAffiliation::UA_Player)
            {
                TurnOrderUnits.Add(Unit);
            }
        }
    }

    // 初始化阵营 AP
    FactionCurrentAP.Add(EUnitAffiliation::UA_Player, FactionInitialAP);
    FactionCurrentAP.Add(EUnitAffiliation::UA_Enemy, FactionInitialAP);
    FactionCurrentAP.Add(EUnitAffiliation::UA_Neutral, FactionInitialAP);

    // 标记为布置阶段（不广播，等 PlayerController 绑定委托后主动调用）
    CurrentPhase = EBattlePhase::BP_Deployment;
}

void ALFPTurnManager::BeginDeploymentPhase()
{
    SetPhase(EBattlePhase::BP_Deployment);
    UE_LOG(LogTemp, Log, TEXT("布置阶段开始"));
}

void ALFPTurnManager::EndDeploymentPhase()
{
    UE_LOG(LogTemp, Log, TEXT("布置阶段结束，注册玩家单位"));

    // 收集布置阶段放置的玩家单位
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor))
        {
            if (Unit->GetAffiliation() == EUnitAffiliation::UA_Player && !TurnOrderUnits.Contains(Unit))
            {
                TurnOrderUnits.Add(Unit);
            }
        }
    }

    // 开始第一回合
    BeginNewRound();
}

void ALFPTurnManager::BeginNewRound()
{
    if (bBattleEnded) return;

    CurrentRound++;
    bIsInRound = true;

    // 重置所有单位
    for (ALFPTacticsUnit* Unit : TurnOrderUnits)
    {
        if (Unit && Unit->IsValidLowLevel())
        {
            Unit->ResetForNewRound();
        }
    }

    // 按速度排序单位
    SortUnitsBySpeed();

    // 清空上一轮的敌人计划
    EnemyActionPlans.Empty();
    AllocatedSkills.Empty();

    // 恢复阵营 AP
    for (auto& Pair : FactionCurrentAP)
    {
        Pair.Value = FMath::Min(Pair.Value + FactionAPRecovery, FactionMaxAP);
        OnFactionAPChanged.Broadcast(Pair.Key, Pair.Value);
    }

    // 进入敌人规划阶段
    BeginEnemyPlanningPhase();
}

void ALFPTurnManager::EndCurrentRound()
{
    if (bBattleEnded) return;

    bIsInRound = false;

    SetPhase(EBattlePhase::BP_RoundEnd);

    // 通知玩家本回合结束
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ALFPTacticsPlayerController* PC = Cast<ALFPTacticsPlayerController>(*It))
        {
            PC->OnRoundEnded(CurrentRound);
        }
    }

    // 添加延迟后开始新回合
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ALFPTurnManager::BeginNewRound, 0.5f, false);
}

void ALFPTurnManager::SortUnitsBySpeed()
{
    // 按速度降序排序（速度高的先行动）
    // Todo: 速度相同时随机排序（玩家优先于AI）
    TurnOrderUnits.Sort([](const ALFPTacticsUnit& A, const ALFPTacticsUnit& B) {
        return A.GetSpeed() > B.GetSpeed();
        });
}

// ==== 阶段管理 ====

void ALFPTurnManager::SetPhase(EBattlePhase NewPhase)
{
    CurrentPhase = NewPhase;
    UE_LOG(LogTemp, Log, TEXT("SetPhase:=%d"), NewPhase);
    OnPhaseChanged.Broadcast(NewPhase);
}

// ==== 敌人规划阶段 ====

void ALFPTurnManager::BeginEnemyPlanningPhase()
{
    SetPhase(EBattlePhase::BP_EnemyPlanning);

    // 过滤出存活的敌方单位，按速度排序
    PlanningOrderEnemies.Empty();
    for (ALFPTacticsUnit* Unit : TurnOrderUnits)
    {
        if (Unit && Unit->IsAlive() && Unit->IsEnemy())
        {
            PlanningOrderEnemies.Add(Unit);
        }
    }

    // 敌方单位已按速度排序（继承自 TurnOrderUnits 的排序）

    CurrentPlanningEnemyIndex = 0;

    if (PlanningOrderEnemies.Num() == 0)
    {
        // 没有敌方单位，直接进入行动阶段
        EndEnemyPlanningPhase();
        return;
    }

    // 隐藏玩家技能选择UI
    if (ALFPTacticsPlayerController* PC = GetWorld()->GetFirstPlayerController<ALFPTacticsPlayerController>())
    {
        PC->HideSkillSelection();
    }

    // Step 1: 全局技能分配（按优先级分配AP技能）
    AllocateEnemySkills();

    // Step 2: 按速度顺序移动展示
    ProcessNextEnemyPlan();
}

void ALFPTurnManager::ProcessNextEnemyPlan()
{
    if (CurrentPlanningEnemyIndex >= PlanningOrderEnemies.Num())
    {
        // 所有敌人规划完毕
        EndEnemyPlanningPhase();
        return;
    }

    ALFPTacticsUnit* Enemy = PlanningOrderEnemies[CurrentPlanningEnemyIndex];

    // 跳过死亡单位
    if (!Enemy || !Enemy->IsAlive())
    {
        CurrentPlanningEnemyIndex++;
        ProcessNextEnemyPlan();
        return;
    }

    ALFPAIController* AIController = Enemy->GetAIController();
    if (!AIController)
    {
        CurrentPlanningEnemyIndex++;
        ProcessNextEnemyPlan();
        return;
    }

    // 移动前等待，让玩家观察当前局势
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        this,
        &ALFPTurnManager::ExecuteCurrentEnemyPlan,
        EnemyPlanPreMoveDelay,
        false
    );
}

void ALFPTurnManager::ExecuteCurrentEnemyPlan()
{
    if (CurrentPlanningEnemyIndex >= PlanningOrderEnemies.Num())
    {
        EndEnemyPlanningPhase();
        return;
    }

    ALFPTacticsUnit* Enemy = PlanningOrderEnemies[CurrentPlanningEnemyIndex];
    if (!Enemy || !Enemy->IsAlive())
    {
        CurrentPlanningEnemyIndex++;
        ProcessNextEnemyPlan();
        return;
    }

    ALFPAIController* AIController = Enemy->GetAIController();
    if (!AIController)
    {
        CurrentPlanningEnemyIndex++;
        ProcessNextEnemyPlan();
        return;
    }

    // 创建行动计划，传入预分配的技能
    ULFPSkillBase* PreAllocatedSkill = nullptr;
    if (ULFPSkillBase** Found = AllocatedSkills.Find(Enemy))
    {
        PreAllocatedSkill = *Found;
    }
    FEnemyActionPlan Plan = AIController->CreateActionPlan(PreAllocatedSkill);

    // 存储计划
    EnemyActionPlans.Add(Plan);
    Enemy->SetActionPlan(Plan);

    // 推进到下一个敌人
    CurrentPlanningEnemyIndex++;

    // 如果敌人正在移动，等移动完成后再推进
    if (Enemy->bIsMoving)
    {
        // 记录当前移动中的敌人，回调中需要解绑
        PlanningMovingEnemy = Enemy;
        Enemy->OnMoveFinished.AddDynamic(this, &ALFPTurnManager::OnEnemyPlanMoveComplete);
    }
    else
    {
        // 没有移动，直接推进下一个敌人
        ProcessNextEnemyPlan();
    }
}

void ALFPTurnManager::OnEnemyPlanMoveComplete()
{
    // 解绑委托
    if (PlanningMovingEnemy)
    {
        PlanningMovingEnemy->OnMoveFinished.RemoveDynamic(this, &ALFPTurnManager::OnEnemyPlanMoveComplete);
        PlanningMovingEnemy = nullptr;
    }

    // 移动完成后推进下一个敌人规划
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        this,
        &ALFPTurnManager::ProcessNextEnemyPlan,
        EnemyPlanPostMoveDelay,
        false
    );
}

void ALFPTurnManager::EndEnemyPlanningPhase()
{
    BeginActionPhase();
}

// ==== 行动阶段 ====

void ALFPTurnManager::BeginActionPhase()
{
    SetPhase(EBattlePhase::BP_ActionPhase);

    // 开始第一个单位的回合
    if (TurnOrderUnits.Num() > 0)
    {
        BeginUnitTurn(TurnOrderUnits[0]);
    }
}

void ALFPTurnManager::BeginUnitTurn(ALFPTacticsUnit* Unit)
{
    if (bBattleEnded) return;

    OnTurnChanged.Broadcast();

    CurrentUnit = Unit;

    ALFPTacticsPlayerController* PC = GetWorld()->GetFirstPlayerController<ALFPTacticsPlayerController>();

    // 敌方单位在行动阶段：执行预定计划
    if (Unit->IsEnemy() && CurrentPhase == EBattlePhase::BP_ActionPhase)
    {
        if (PC)
        {
            PC->HideSkillSelection();
        }

        Unit->OnTurnStarted();

        ExecuteEnemyPlan(Unit);
        return;
    }

    // 玩家单位：保持原有逻辑
    if (Unit)
    {
        Unit->OnTurnStarted();
    }

    if (PC)
    {
        PC->OnTurnStarted(Unit);
        PC->SelectUnit(Unit);
        PC->HandleSkillSelection();
    }
}

void ALFPTurnManager::ExecuteEnemyPlan(ALFPTacticsUnit* Unit)
{
    const FEnemyActionPlan& Plan = GetPlanForEnemy(Unit);

    if (Plan.bIsValid && Plan.PlannedSkill)
    {
        // 在原定目标格子释放技能（即使目标已死亡）
        // AP 已在规划阶段 AllocateEnemySkills 中扣除
        Unit->ExecuteSkill(Plan.PlannedSkill, Plan.TargetTile);
    }

    // 清除头顶技能图标
    Unit->ClearActionPlan();

    // 短延时后结束回合（让玩家看到技能效果）
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, Unit]()
    {
        Unit->SetHasActed(true);
        PassTurn();
    }, 0.5f, false);
}

void ALFPTurnManager::EndUnitTurn(ALFPTacticsUnit* Unit)
{
    if (Unit)
    {
        Unit->OnTurnEnded();
    }

    // 通知玩家控制器
    if (ALFPTacticsPlayerController* PC = GetWorld()->GetFirstPlayerController<ALFPTacticsPlayerController>())
    {
        PC->OnTurnEnded(Unit);
    }
}

void ALFPTurnManager::PassTurn()
{
    if (bBattleEnded) return;

    if (TurnOrderUnits.IsEmpty())
    {
        return;
    }

    // 结束当前单位回合
    if (CurrentUnit)
    {
        EndUnitTurn(CurrentUnit);
    }

    // 找到下一个未行动的单位
    int32 CurrentIndex = CurrentUnit ? TurnOrderUnits.Find(CurrentUnit) : -1;
    int32 StartIndex = (CurrentIndex + 1) % TurnOrderUnits.Num();

    // 检查所有单位是否行动完毕
    bool TurnOrderUnitsActed = true;
    for (ALFPTacticsUnit* Unit : TurnOrderUnits)
    {
        if (Unit && Unit->IsAlive() && !Unit->HasActed())
        {
            TurnOrderUnitsActed = false;
            break;
        }
    }

    if (TurnOrderUnitsActed)
    {
        // 所有单位行动完毕，结束回合
        EndCurrentRound();
    }
    else
    {
        // 寻找下一个未行动单位
        for (int32 i = 0; i < TurnOrderUnits.Num(); i++)
        {
            int32 Index = (StartIndex + i) % TurnOrderUnits.Num();
            ALFPTacticsUnit* NextUnit = TurnOrderUnits[Index];

            if (NextUnit && NextUnit->IsAlive() && !NextUnit->HasActed())
            {
                BeginUnitTurn(NextUnit);
                return;
            }
        }

        // 没有找到未行动单位，结束回合
        EndCurrentRound();
    }
}

void ALFPTurnManager::OnUnitFinishedAction(ALFPTacticsUnit* Unit)
{
    if (Unit == CurrentUnit)
    {
        Unit->SetHasActed(true);

        // 自动传递回合
        PassTurn();
    }
}

void ALFPTurnManager::RegisterUnit(ALFPTacticsUnit* Unit)
{
    if (Unit && !TurnOrderUnits.Contains(Unit))
    {
        TurnOrderUnits.Add(Unit);

        // 如果游戏已经开始，重新排序
        if (bIsInRound)
        {
            SortUnitsBySpeed();
        }
    }
}

void ALFPTurnManager::UnregisterUnit(ALFPTacticsUnit* Unit)
{
    if (Unit)
    {
        bool bWasCurrentUnit = (Unit == CurrentUnit);
        TurnOrderUnits.Remove(Unit);

        // 先检查战斗结束条件
        CheckBattleEnd();

        // 如果战斗未结束且当前单位被移除，传递回合
        if (!bBattleEnded && bWasCurrentUnit)
        {
            PassTurn();
        }
    }
}

const FEnemyActionPlan& ALFPTurnManager::GetPlanForEnemy(ALFPTacticsUnit* Enemy) const
{
    for (const FEnemyActionPlan& Plan : EnemyActionPlans)
    {
        if (Plan.EnemyUnit == Enemy)
        {
            return Plan;
        }
    }
    return EmptyPlan;
}

// ==== 阵营行动点 ====

int32 ALFPTurnManager::GetFactionAP(EUnitAffiliation Faction) const
{
    const int32* AP = FactionCurrentAP.Find(Faction);
    return AP ? *AP : 0;
}

bool ALFPTurnManager::HasEnoughFactionAP(EUnitAffiliation Faction, int32 Amount) const
{
    return GetFactionAP(Faction) >= Amount;
}

void ALFPTurnManager::ConsumeFactionAP(EUnitAffiliation Faction, int32 Amount)
{
    if (int32* AP = FactionCurrentAP.Find(Faction))
    {
        *AP = FMath::Max(0, *AP - Amount);
        OnFactionAPChanged.Broadcast(Faction, *AP);
    }
}

// ==== 全局技能分配 ====

void ALFPTurnManager::AllocateEnemySkills()
{
    AllocatedSkills.Empty();

    // 收集所有敌方单位的所有消耗AP的技能候选
    struct FSkillCandidate
    {
        ALFPTacticsUnit* Unit;
        ULFPSkillBase* Skill;
        float EffectivePriority;
    };
    TArray<FSkillCandidate> Candidates;

    for (ALFPTacticsUnit* Enemy : PlanningOrderEnemies)
    {
        if (!Enemy || !Enemy->IsAlive()) continue;

        TArray<ULFPSkillBase*> Skills = Enemy->GetAvailableSkills();
        for (ULFPSkillBase* Skill : Skills)
        {
            if (!Skill) continue;
            if (Skill->ActionPointCost <= 0) continue; // 0消耗技能不参与竞争
            if (Skill->CurrentCooldown > 0) continue;  // 冷却中跳过

            FSkillCandidate Candidate;
            Candidate.Unit = Enemy;
            Candidate.Skill = Skill;
            Candidate.EffectivePriority = Skill->GetEffectivePriority();
            Candidates.Add(Candidate);
        }
    }

    // 按有效优先级降序排序
    Candidates.Sort([](const FSkillCandidate& A, const FSkillCandidate& B) {
        return A.EffectivePriority > B.EffectivePriority;
    });

    // 依次分配：如果阵营AP足够且该单位尚未分配
    for (const FSkillCandidate& Candidate : Candidates)
    {
        // 该单位已分配过技能，跳过
        if (AllocatedSkills.Contains(Candidate.Unit)) continue;

        // 检查阵营AP是否足够
        if (HasEnoughFactionAP(EUnitAffiliation::UA_Enemy, Candidate.Skill->ActionPointCost))
        {
            AllocatedSkills.Add(Candidate.Unit, Candidate.Skill);
            ConsumeFactionAP(EUnitAffiliation::UA_Enemy, Candidate.Skill->ActionPointCost);
        }
    }

    // 未分配到AP技能的敌人会在 CreateActionPlan 中 fallback 到默认攻击
}

void ALFPTurnManager::CheckBattleEnd()
{
    if (bBattleEnded) return;
    if (CurrentPhase == EBattlePhase::BP_Deployment) return;

    bool bHasPlayerUnit = false;
    bool bHasEnemyUnit = false;

    for (ALFPTacticsUnit* Unit : TurnOrderUnits)
    {
        if (!Unit || !Unit->IsAlive()) continue;

        if (Unit->GetAffiliation() == EUnitAffiliation::UA_Player)
            bHasPlayerUnit = true;
        else if (Unit->GetAffiliation() == EUnitAffiliation::UA_Enemy)
            bHasEnemyUnit = true;
    }

    if (!bHasEnemyUnit)
    {
        // 胜利：所有敌方单位消灭
        bBattleEnded = true;
        UE_LOG(LogTemp, Log, TEXT("战斗结束: 胜利！"));
        if (ALFPTurnGameMode* GM = Cast<ALFPTurnGameMode>(GetWorld()->GetAuthGameMode()))
        {
            GM->EndBattle(true);
        }
    }
    else if (!bHasPlayerUnit)
    {
        // 失败：所有玩家单位消灭
        bBattleEnded = true;
        UE_LOG(LogTemp, Log, TEXT("战斗结束: 败北"));
        if (ALFPTurnGameMode* GM = Cast<ALFPTurnGameMode>(GetWorld()->GetAuthGameMode()))
        {
            GM->EndBattle(false);
        }
    }
}
