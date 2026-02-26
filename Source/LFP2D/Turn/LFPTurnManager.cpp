// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Skill/LFPSkillBase.h"
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
    // 收集所有单位
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundActors);

    // 转换为战术单位数组
    for (AActor* Actor : FoundActors)
    {
        if (ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor))
        {
            TurnOrderUnits.Add(Unit);
        }
    }

    // 添加延迟后开始第一回合
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ALFPTurnManager::BeginNewRound, 0.5f, false);
}

void ALFPTurnManager::BeginNewRound()
{
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

    // 进入敌人规划阶段
    BeginEnemyPlanningPhase();
}

void ALFPTurnManager::EndCurrentRound()
{
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

    // 创建行动计划（包括移动到施法位置）
    FEnemyActionPlan Plan = AIController->CreateActionPlan();

    // 存储计划
    EnemyActionPlans.Add(Plan);
    Enemy->SetActionPlan(Plan);

    // 推进到下一个敌人，加延时让玩家观察
    CurrentPlanningEnemyIndex++;
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        this,
        &ALFPTurnManager::ProcessNextEnemyPlan,
        0.5f,
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
        Unit->ExecuteSkill(Plan.PlannedSkill, Plan.TargetTile);
        Unit->ConsumeActionPoints(Plan.PlannedSkill->ActionPointCost);
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
    if (!CurrentUnit || TurnOrderUnits.IsEmpty())
    {
        return;
    }

    // 结束当前单位回合
    EndUnitTurn(CurrentUnit);

    // 找到下一个未行动的单位
    int32 CurrentIndex = TurnOrderUnits.Find(CurrentUnit);
    int32 NextIndex = (CurrentIndex + 1) % TurnOrderUnits.Num();

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
            int32 Index = (NextIndex + i) % TurnOrderUnits.Num();
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
        TurnOrderUnits.Remove(Unit);

        // 如果当前单位被移除，传递回合
        if (Unit == CurrentUnit)
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
