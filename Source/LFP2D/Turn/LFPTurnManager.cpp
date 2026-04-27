// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/LFPSkillComponent.h"
#include "LFP2D/Core/LFPTurnGameMode.h"
#include "LFP2D/Turn/LFPBattleRelicRuntimeManager.h"
#include "Kismet/GameplayStatics.h"

FEnemyActionPlan ALFPTurnManager::EmptyPlan;

namespace
{
void SortUnitsBySpeedStable(TArray<ALFPTacticsUnit*>& Units)
{
    struct FSortableTurnUnit
    {
        ALFPTacticsUnit* Unit = nullptr;
        int32 OriginalIndex = INDEX_NONE;
    };

    TArray<FSortableTurnUnit> SortableUnits;
    SortableUnits.Reserve(Units.Num());

    for (int32 Index = 0; Index < Units.Num(); ++Index)
    {
        FSortableTurnUnit& Entry = SortableUnits.AddDefaulted_GetRef();
        Entry.Unit = Units[Index];
        Entry.OriginalIndex = Index;
    }

    SortableUnits.Sort([](const FSortableTurnUnit& A, const FSortableTurnUnit& B)
    {
        if (A.Unit == nullptr || B.Unit == nullptr)
        {
            if (A.Unit == B.Unit)
            {
                return A.OriginalIndex < B.OriginalIndex;
            }

            return A.Unit != nullptr;
        }

        if (A.Unit->GetSpeed() == B.Unit->GetSpeed())
        {
            return A.OriginalIndex < B.OriginalIndex;
        }

        return A.Unit->GetSpeed() > B.Unit->GetSpeed();
    });

    for (int32 Index = 0; Index < SortableUnits.Num(); ++Index)
    {
        Units[Index] = SortableUnits[Index].Unit;
    }
}

void UpdateBestBlockedCandidate(
    TMap<ALFPTacticsUnit*, FEnemySkillPlanCandidate>& BestBlockedCandidates,
    const FEnemySkillPlanCandidate& Candidate)
{
    if (!Candidate.EnemyUnit)
    {
        return;
    }

    FEnemySkillPlanCandidate* Existing = BestBlockedCandidates.Find(Candidate.EnemyUnit);
    if (!Existing ||
        Candidate.TotalScore > Existing->TotalScore ||
        (FMath::IsNearlyEqual(Candidate.TotalScore, Existing->TotalScore) &&
            Candidate.EffectivePriority > Existing->EffectivePriority) ||
        (FMath::IsNearlyEqual(Candidate.TotalScore, Existing->TotalScore) &&
            FMath::IsNearlyEqual(Candidate.EffectivePriority, Existing->EffectivePriority) &&
            Candidate.HatredValue > Existing->HatredValue))
    {
        BestBlockedCandidates.Add(Candidate.EnemyUnit, Candidate);
    }
}
}

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

	// 通知战斗遗物运行时管理器：部署已完成
	if (ALFPTurnGameMode* GM = Cast<ALFPTurnGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (ALFPBattleRelicRuntimeManager* RelicManager = GM->GetBattleRelicRuntimeManager())
		{
			RelicManager->OnDeploymentFinished();
		}
	}

	// 开始第一回合
    RefreshAllRuntimeUnitStates(false);
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

	// 回合开始前刷新玩家单位的战斗遗物属性（例如低血速度影响排序）
	if (ALFPTurnGameMode* GM = Cast<ALFPTurnGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (ALFPBattleRelicRuntimeManager* RelicManager = GM->GetBattleRelicRuntimeManager())
		{
			RelicManager->OnRoundStarted();
		}
	}

	// 按速度排序单位
	SortUnitsBySpeed();

    // 清空上一轮的敌人计划
    EnemyActionPlans.Empty();
    AllocatedPlans.Empty();

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

	// 触发回合结束即时遗物效果
	if (ALFPTurnGameMode* GM = Cast<ALFPTurnGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (ALFPBattleRelicRuntimeManager* RelicManager = GM->GetBattleRelicRuntimeManager())
		{
			RelicManager->OnRoundEnded(CurrentRound);
		}
	}

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
    SortUnitsBySpeedStable(TurnOrderUnits);
}

// ==== 阶段管理 ====

void ALFPTurnManager::ReorderRemainingUnitsBySpeed()
{
    if (TurnOrderUnits.IsEmpty())
    {
        return;
    }

    const int32 CurrentIndex = CurrentUnit ? TurnOrderUnits.Find(CurrentUnit) : INDEX_NONE;
    const int32 StartIndex = CurrentIndex == INDEX_NONE ? 0 : CurrentIndex + 1;

    TArray<int32> ReorderSlots;
    TArray<ALFPTacticsUnit*> RemainingUnits;
    for (int32 Index = StartIndex; Index < TurnOrderUnits.Num(); ++Index)
    {
        if (ALFPTacticsUnit* Unit = TurnOrderUnits[Index])
        {
            // 当前单位、已行动单位和死亡单位都保持原位，只重排本轮剩余可行动单位。
            if (!Unit->IsAlive() || Unit == CurrentUnit || Unit->HasActed())
            {
                continue;
            }

            ReorderSlots.Add(Index);
            RemainingUnits.Add(Unit);
        }
    }

    if (RemainingUnits.Num() <= 1)
    {
        return;
    }

    SortUnitsBySpeedStable(RemainingUnits);

    for (int32 Index = 0; Index < RemainingUnits.Num(); ++Index)
    {
        TurnOrderUnits[ReorderSlots[Index]] = RemainingUnits[Index];
    }
}

void ALFPTurnManager::RefreshAllRuntimeUnitStates(bool bAllowReorder)
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundActors);

    TArray<ALFPTacticsUnit*> Units;
    for (AActor* Actor : FoundActors)
    {
        if (ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor))
        {
            Units.Add(Unit);
        }
    }

    for (ALFPTacticsUnit* Unit : Units)
    {
        if (Unit && Unit->GetBuffComponent())
        {
            // 先统一刷新条件型 Buff，再重建属性，避免用旧状态算数值。
            Unit->GetBuffComponent()->EvaluateBuffs();
        }
    }

    for (ALFPTacticsUnit* Unit : Units)
    {
        if (Unit && Unit->IsAlive())
        {
            Unit->RebuildCurrentStatsFromRuntimeSources();
        }
    }

    if (bAllowReorder)
    {
        if (bIsInRound && CurrentPhase == EBattlePhase::BP_ActionPhase)
        {
            ReorderRemainingUnitsBySpeed();
        }
        else
        {
            SortUnitsBySpeed();
        }
    }

    OnTurnChanged.Broadcast();
}

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
    AllocateEnemyPlans();

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
    FEnemyActionPlan Plan;
    Plan.EnemyUnit = Enemy;
    if (const FEnemyActionPlan* Found = AllocatedPlans.Find(Enemy))
    {
        Plan = *Found;
    }

    // 存储计划
    EnemyActionPlans.Add(Plan);
    Enemy->SetActionPlan(Plan);

    const bool bStartedMove =
        Plan.bIsValid &&
        Plan.CasterPositionTile &&
        Plan.CasterPositionTile != Enemy->GetCurrentTile() &&
        Enemy->MoveToTile(Plan.CasterPositionTile);

    // 推进到下一个敌人
    CurrentPlanningEnemyIndex++;

    // 如果敌人正在移动，等移动完成后再推进
    if (bStartedMove && Enemy->bIsMoving)
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
    if (!Unit || !Unit->IsAlive())
    {
        PassTurn();
        return;
    }

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
        if (!Unit->IsAlive())
        {
            PassTurn();
            return;
        }

        ExecuteEnemyPlan(Unit);
        return;
    }

    // 玩家单位：保持原有逻辑
    Unit->OnTurnStarted();
    if (!Unit->IsAlive())
    {
        PassTurn();
        return;
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
        ALFPHexTile* ExecutionTargetTile = Plan.TargetTile;
        const FString SkillLogName =
            Plan.PlannedSkill->SkillName.IsEmpty() ? Plan.PlannedSkill->GetName() : Plan.PlannedSkill->SkillName.ToString();

        if (Plan.PlannedSkill->bTrackTargetUnitForAIExecution)
        {
            ExecutionTargetTile = nullptr;

            if (Plan.TargetUnit && Plan.TargetUnit->IsAlive())
            {
                ALFPHexTile* CurrentTargetTile = Plan.TargetUnit->GetCurrentTile();
                ALFPHexTile* CasterTile = Unit ? Unit->GetCurrentTile() : nullptr;

                if (CasterTile && CurrentTargetTile && Plan.PlannedSkill->CanReleaseFrom(CasterTile, CurrentTargetTile))
                {
                    ExecutionTargetTile = CurrentTargetTile;
                }
            }
        }

        if (ExecutionTargetTile)
        {
            UE_LOG(LogTemp, Log, TEXT("敌方计划执行: 单位[%s] 技能[%s]"), *Unit->GetName(), *SkillLogName);

            const bool bExecuted = Unit->ExecuteSkill(Plan.PlannedSkill, ExecutionTargetTile);
            UE_LOG(
                LogTemp,
                Log,
                TEXT("敌方计划执行结果: 单位[%s] 技能[%s] %s"),
                *Unit->GetName(),
                *SkillLogName,
                bExecuted ? TEXT("已调用释放") : TEXT("释放失败"));
        }
        else
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("敌方计划执行跳过: 单位[%s] 技能[%s] 当前没有合法目标格"),
                *Unit->GetName(),
                *SkillLogName);
        }
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

        RefreshAllRuntimeUnitStates(bIsInRound);
    }
}

void ALFPTurnManager::UnregisterUnit(ALFPTacticsUnit* Unit)
{
    if (Unit)
    {
        bool bWasCurrentUnit = (Unit == CurrentUnit);
        TurnOrderUnits.Remove(Unit);
        RefreshAllRuntimeUnitStates(!bBattleEnded);

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

void ALFPTurnManager::RestoreFactionAP(EUnitAffiliation Faction, int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    if (int32* AP = FactionCurrentAP.Find(Faction))
    {
        *AP = FMath::Min(*AP + Amount, FactionMaxAP);
        OnFactionAPChanged.Broadcast(Faction, *AP);
    }
}

// ==== 全局技能分配 ====

void ALFPTurnManager::AllocateEnemyPlans()
{
    AllocatedPlans.Empty();

    // 收集所有敌方单位的所有消耗AP的技能候选
    // 先把“每个敌人每个技能”的最佳完整候选收集到同一个全局池里。
    TArray<FEnemySkillPlanCandidate> Candidates;
    TSet<ALFPHexTile*> ReservedDestinationTiles;
    TMap<ALFPTacticsUnit*, FEnemySkillPlanCandidate> BestBlockedCandidates;

    for (int32 EnemyIndex = 0; EnemyIndex < PlanningOrderEnemies.Num(); ++EnemyIndex)
    {
        ALFPTacticsUnit* Enemy = PlanningOrderEnemies[EnemyIndex];
        if (!Enemy || !Enemy->IsAlive())
        {
            continue;
        }

        ALFPAIController* AIController = Enemy->GetAIController();
        if (!AIController)
        {
            continue;
        }

        const TArray<ULFPSkillBase*> Skills = Enemy->GetAvailableSkills();
        for (ULFPSkillBase* Skill : Skills)
        {
            if (!Skill) continue;
            if (Skill->IsPassiveSkill()) continue;
            //if (Skill->ActionPointCost <= 0) continue; // 0消耗技能不参与竞争
            if (Skill->CurrentCooldown > 0) continue;  // 冷却中跳过

            FEnemySkillPlanCandidate Candidate;
            if (AIController->BuildBestSkillPlanCandidate(Skill, EnemyIndex, Candidate))
            {
                Candidates.Add(Candidate);
            }
        }
    }

    // 按有效优先级降序排序
    // 全局统一归一化打分，再按稳定规则排序，保证相同局面下结果一致。
    NormalizeCandidateScores(Candidates);

    Candidates.StableSort([](const FEnemySkillPlanCandidate& A, const FEnemySkillPlanCandidate& B)
    {
        if (!FMath::IsNearlyEqual(A.TotalScore, B.TotalScore))
        {
            return A.TotalScore > B.TotalScore;
        }

        if (!FMath::IsNearlyEqual(A.EffectivePriority, B.EffectivePriority))
        {
            return A.EffectivePriority > B.EffectivePriority;
        }

        if (!FMath::IsNearlyEqual(A.HatredValue, B.HatredValue))
        {
            return A.HatredValue > B.HatredValue;
        }

        if (A.APCost != B.APCost)
        {
            return A.APCost > B.APCost;
        }

        return A.PlanningOrderIndex < B.PlanningOrderIndex;
    });

    // 依次分配：如果阵营AP足够且该单位尚未分配
    // 依次扫描候选池：
    // 1. 每个敌人本轮最多只拿一个计划
    // 2. 遇到“完整成立但 AP 不够”的正耗 AP 候选后，后续正耗 AP 候选整体截断
    // 3. 后续 0 AP 候选仍继续参与分配
    int32 RemainingEnemyAP = GetFactionAP(EUnitAffiliation::UA_Enemy);
    bool bStopPositiveCostAllocation = false;

    for (const FEnemySkillPlanCandidate& Candidate : Candidates)
    {
        // 该单位已分配过技能，跳过
        if (!Candidate.bIsValid || !Candidate.EnemyUnit)
        {
            continue;
        }

        if (AllocatedPlans.Contains(Candidate.EnemyUnit))
        {
            continue;
        }

        if (Candidate.CasterPositionTile && ReservedDestinationTiles.Contains(Candidate.CasterPositionTile))
        {
            continue;
        }

        // 检查阵营AP是否足够
        if (Candidate.APCost > 0)
        {
            if (bStopPositiveCostAllocation)
            {
                FEnemySkillPlanCandidate BlockedCandidate = Candidate;
                BlockedCandidate.bBlockedByInsufficientAP = true;
                UpdateBestBlockedCandidate(BestBlockedCandidates, BlockedCandidate);
                continue;
            }

            if (RemainingEnemyAP < Candidate.APCost)
            {
                FEnemySkillPlanCandidate BlockedCandidate = Candidate;
                BlockedCandidate.bBlockedByInsufficientAP = true;
                UpdateBestBlockedCandidate(BestBlockedCandidates, BlockedCandidate);
                bStopPositiveCostAllocation = true;
                continue;
            }
        }

        AllocatedPlans.Add(Candidate.EnemyUnit, Candidate.ToActionPlan());
        if (Candidate.CasterPositionTile)
        {
            ReservedDestinationTiles.Add(Candidate.CasterPositionTile);
        }

        if (Candidate.APCost > 0)
        {
            RemainingEnemyAP -= Candidate.APCost;
            ConsumeFactionAP(EUnitAffiliation::UA_Enemy, Candidate.APCost);
        }
    }

    for (ALFPTacticsUnit* Enemy : PlanningOrderEnemies)
    {
        if (!Enemy || !Enemy->IsAlive() || AllocatedPlans.Contains(Enemy))
        {
            continue;
        }

        // 没分到技能计划的敌人仍然会得到一个纯移动铺路计划。
        FEnemyActionPlan Plan;
        Plan.EnemyUnit = Enemy;

        ALFPAIController* AIController = Enemy->GetAIController();
        const FEnemySkillPlanCandidate* PreferredBlockedCandidate = BestBlockedCandidates.Find(Enemy);
        if (AIController)
        {
            Plan = AIController->CreateMovementOnlyPlan(PreferredBlockedCandidate);
            Plan.EnemyUnit = Enemy;
        }

        if (!Plan.CasterPositionTile)
        {
            Plan.CasterPositionTile = Enemy->GetCurrentTile();
        }

        if (Plan.CasterPositionTile &&
            Plan.CasterPositionTile != Enemy->GetCurrentTile() &&
            ReservedDestinationTiles.Contains(Plan.CasterPositionTile))
        {
            Plan.CasterPositionTile = Enemy->GetCurrentTile();
        }

        Plan.bIsValid = Plan.CasterPositionTile != nullptr;

        if (Plan.bIsValid && Plan.CasterPositionTile)
        {
            ReservedDestinationTiles.Add(Plan.CasterPositionTile);
        }

        AllocatedPlans.Add(Enemy, Plan);
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
