// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/AI/LFPEnemyBehaviorData.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/LFPSkillComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Kismet/GameplayStatics.h"

ALFPAIController::ALFPAIController()
{
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
    ControlledUnit = nullptr;
    GridManager = nullptr;
}

void ALFPAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledUnit = Cast<ALFPTacticsUnit>(InPawn);
    if (ControlledUnit)
    {
        // 初始化黑板
        if (BehaviorTree && BlackboardComponent)
        {
            BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
        }

        // 获取网格管理器
        TArray<AActor*> GridManagers;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), GridManagers);
        if (GridManagers.Num() > 0)
        {
            GridManager = Cast<ALFPHexGridManager>(GridManagers[0]);
        }

        // 启动行为树
        RunBehaviorTree(BehaviorTree);
    }
}

void ALFPAIController::OnUnPossess()
{
    Super::OnUnPossess();
}

void ALFPAIController::StartUnitTurn()
{
    if (!ControlledUnit || !BlackboardComponent) return;

    // 设置黑板值
    BlackboardComponent->SetValueAsObject("TargetUnit", nullptr);
    BlackboardComponent->SetValueAsObject("TargetTile", nullptr);
    BlackboardComponent->SetValueAsBool("IsInAttackRange", false);
    BlackboardComponent->SetValueAsBool("CanAttack", false);
    BlackboardComponent->SetValueAsBool("ShouldEndTurn", false);

    // 寻找初始目标
    ALFPTacticsUnit* BestTarget = FindBestTarget();
    BlackboardComponent->SetValueAsObject("TargetUnit", BestTarget);
}

void ALFPAIController::EndUnitTurn()
{
    // 行为树会自动处理后续逻辑
}

ALFPTacticsUnit* ALFPAIController::FindBestTarget() const
{
    if (!ControlledUnit || !GridManager) return nullptr;

    // 获取所有玩家单位
    TArray<AActor*> PlayerUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), PlayerUnits);

    ALFPTacticsUnit* BestTarget = nullptr;
    float BestThreatValue = -MAX_FLT;

    for (AActor* Actor : PlayerUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (Unit && Unit->IsAlive() && Unit->IsAlly())
        {
            float ThreatValue = CalculateThreatValue(Unit);
            if (ThreatValue > BestThreatValue)
            {
                BestThreatValue = ThreatValue;
                BestTarget = Unit;
            }
        }
    }

    return BestTarget;
}

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
        if (Unit && Unit->IsAlive() && Unit->IsAlly())
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

ALFPHexTile* ALFPAIController::FindBestMovementTile(ALFPTacticsUnit* Target) const
{
    if (!ControlledUnit || !Target || !GridManager) return nullptr;

    // 获取所有可移动位置
    TArray<ALFPHexTile*> MovementRange = GridManager->GetTilesInRange(ControlledUnit->GetCurrentTile(), ControlledUnit->GetCurrentMovePoints());

    ALFPHexTile* BestTile = nullptr;
    float BestPositionValue = -MAX_FLT;

    for (ALFPHexTile* Tile : MovementRange)
    {
        // 跳过有单位的格子
        if (Tile->GetUnitOnTile()) continue;

        float PositionValue = CalculatePositionValue(Tile, Target);
        if (PositionValue > BestPositionValue)
        {
            BestPositionValue = PositionValue;
            BestTile = Tile;
        }
    }

    return BestTile;
}

float ALFPAIController::CalculateThreatValue(ALFPTacticsUnit* Target) const
{
    if (!ControlledUnit || !Target) return 0.0f;

    // 威胁值 = 目标攻击力 * (1 - 目标当前血量/最大血量)
    float ThreatValue = Target->GetAttackPower() * (1.0f - (float)Target->GetCurrentHealth() / Target->GetMaxHealth());

    // 距离因素（越近威胁越大）
    int32 Distance = FLFPHexCoordinates::Distance(
        ControlledUnit->GetCurrentCoordinates(),
        Target->GetCurrentCoordinates()
    );
    float DistanceFactor = 1.0f / FMath::Max(Distance, 1);

    // 应用行为数据
    if (BehaviorData)
    {
        if (BehaviorData->bPrioritizeWeakTargets)
        {
            // 增加对低血量目标的权重
            float HealthRatio = (float)Target->GetCurrentHealth() / Target->GetMaxHealth();
            ThreatValue *= (2.0f - HealthRatio); // 血量越低，威胁值越高
        }

        // 应用攻击性系数
        ThreatValue *= BehaviorData->Aggressiveness;
    }

    return ThreatValue * DistanceFactor;
}

float ALFPAIController::CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target) const
{
    if (!Tile || !Target) return 0.0f;

    float PositionValue = 0.0f;

    // 1. 离目标越近越好
    int32 DistanceToTarget = FLFPHexCoordinates::Distance(
        Tile->GetCoordinates(),
        Target->GetCurrentCoordinates()
    );
    PositionValue += 10.0f / FMath::Max(DistanceToTarget, 1);

    // 2. 如果在攻击范围内额外加分
    if (DistanceToTarget <= ControlledUnit->GetAttackRange())
    {
        PositionValue += 20.0f;
    }

    // 3. 靠近其他敌方单位（团队协作加分）
    TArray<AActor*> EnemyUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), EnemyUnits);

    for (AActor* Actor : EnemyUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (Unit && Unit != ControlledUnit && Unit->IsEnemy() && Unit->IsAlive())
        {
            int32 DistanceToAlly = FLFPHexCoordinates::Distance(
                Tile->GetCoordinates(),
                Unit->GetCurrentCoordinates()
            );

            if (DistanceToAlly <= 2)
            {
                PositionValue += 5.0f / FMath::Max(DistanceToAlly, 1);
            }
        }
    }

    return PositionValue;
}

// ==== 规划阶段方法 ====

FEnemyActionPlan ALFPAIController::CreateActionPlan()
{
    FEnemyActionPlan Plan;
    Plan.EnemyUnit = ControlledUnit;

    if (!ControlledUnit || !GridManager)
    {
        return Plan;
    }

    // 1. 选择技能
    ULFPSkillBase* ChosenSkill = SelectBestSkill();
    if (!ChosenSkill)
    {
        return Plan;
    }
    Plan.PlannedSkill = ChosenSkill;

    // 2. 根据技能的仇恨值选择目标
    ALFPTacticsUnit* Target = FindBestSkillTarget(ChosenSkill);
    if (!Target)
    {
        return Plan;
    }
    Plan.TargetUnit = Target;

    // 3. 目标格子 = 目标单位当前所在格子
    ALFPHexTile* TargetTile = Target->GetCurrentTile();
    if (!TargetTile)
    {
        return Plan;
    }
    Plan.TargetTile = TargetTile;

    // 4. 寻找最佳施法站位
    ALFPHexTile* CasterPos = FindBestCasterPosition(ChosenSkill, TargetTile);
    if (!CasterPos)
    {
        // 找不到合适的施法位置，尝试原地
        CasterPos = ControlledUnit->GetCurrentTile();
    }
    Plan.CasterPositionTile = CasterPos;

    // 5. 移动到施法位置
    if (CasterPos && CasterPos != ControlledUnit->GetCurrentTile())
    {
        ControlledUnit->MoveToTile(CasterPos);
    }

    // 6. 计算技能生效范围（EffectRangeCoords 相对于目标格子偏移）
    FLFPHexCoordinates TargetCoords = TargetTile->GetCoordinates();
    for (const FLFPHexCoordinates& Offset : ChosenSkill->EffectRangeCoords)
    {
        FLFPHexCoordinates AbsCoord(
            TargetCoords.Q + Offset.Q,
            TargetCoords.R + Offset.R
        );
        if (ALFPHexTile* EffectTile = GridManager->GetTileAtCoordinates(AbsCoord))
        {
            Plan.EffectAreaTiles.Add(EffectTile);
        }
    }

    // 如果没有定义 EffectRangeCoords，至少包含目标格子本身
    if (Plan.EffectAreaTiles.Num() == 0)
    {
        Plan.EffectAreaTiles.Add(TargetTile);
    }

    Plan.bIsValid = true;
    return Plan;
}

ULFPSkillBase* ALFPAIController::SelectBestSkill()
{
    if (!ControlledUnit) return nullptr;

    TArray<ULFPSkillBase*> Skills = ControlledUnit->GetAvailableSkills();
    ULFPSkillBase* BestSkill = nullptr;
    float BestScore = -MAX_FLT;

    for (ULFPSkillBase* Skill : Skills)
    {
        if (!Skill) continue;
        if (!Skill->CanExecute()) continue;

        // 找该技能的最佳目标
        ALFPTacticsUnit* PotentialTarget = FindBestSkillTarget(Skill);
        if (!PotentialTarget) continue;

        // 检查是否存在可达的施法位置
        ALFPHexTile* PotentialTargetTile = PotentialTarget->GetCurrentTile();
        if (!PotentialTargetTile) continue;

        ALFPHexTile* CasterPos = FindBestCasterPosition(Skill, PotentialTargetTile);
        if (!CasterPos) continue;

        // 评分：仇恨值作为基础分
        float Score = Skill->CalculateHatredValue(ControlledUnit, PotentialTarget);

        // 非默认攻击技能额外加分（鼓励使用特殊技能）
        if (!Skill->bIsDefaultAttack)
        {
            Score *= 1.5f;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestSkill = Skill;
        }
    }

    // Fallback 到默认攻击
    if (!BestSkill)
    {
        BestSkill = ControlledUnit->GetDefaultAttackSkill();
    }

    return BestSkill;
}

ALFPHexTile* ALFPAIController::FindBestCasterPosition(ULFPSkillBase* Skill, ALFPHexTile* TargetTile)
{
    if (!ControlledUnit || !Skill || !TargetTile || !GridManager) return nullptr;

    // 获取移动范围内的所有格子
    ALFPHexTile* CurrentTile = ControlledUnit->GetCurrentTile();
    TArray<ALFPHexTile*> MovementRange = GridManager->GetTilesInRange(
        CurrentTile,
        ControlledUnit->GetCurrentMovePoints()
    );

    // 也包含当前格子（可能不需要移动）
    MovementRange.AddUnique(CurrentTile);

    ALFPHexTile* BestTile = nullptr;
    float BestValue = -MAX_FLT;

    FLFPHexCoordinates TargetCoords = TargetTile->GetCoordinates();

    for (ALFPHexTile* Tile : MovementRange)
    {
        if (!Tile) continue;

        // 跳过被其他单位占据的格子（但允许自己当前所在格）
        ALFPTacticsUnit* Occupant = Tile->GetUnitOnTile();
        if (Occupant && Occupant != ControlledUnit) continue;

        // 检查从该格子到目标格子的距离是否在技能释放范围内
        int32 DistToTarget = FLFPHexCoordinates::Distance(
            Tile->GetCoordinates(),
            TargetCoords
        );

        if (DistToTarget < Skill->Range.MinRange || DistToTarget > Skill->Range.MaxRange)
        {
            continue;
        }

        // 评估位置价值
        float Value = 0.0f;

        // 在技能范围内加分
        Value += 30.0f;

        // 离目标越近越好（在范围内的前提下）
        Value += 10.0f / FMath::Max(DistToTarget, 1);

        // 离当前位置越近越好（减少移动消耗）
        int32 MoveDistance = FLFPHexCoordinates::Distance(
            CurrentTile->GetCoordinates(),
            Tile->GetCoordinates()
        );
        Value -= MoveDistance * 2.0f;

        if (Value > BestValue)
        {
            BestValue = Value;
            BestTile = Tile;
        }
    }

    return BestTile;
}

ALFPTacticsUnit* ALFPAIController::GetControlledUnit()
{
    return ControlledUnit;
}

void ALFPAIController::SetControlledUnit(ALFPTacticsUnit* NewUnit)
{
    ControlledUnit = NewUnit;
}
