// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/AI/LFPEnemyBehaviorData.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillBase.h"
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
        // ��ʼ���ڰ�
        if (BehaviorTree && BlackboardComponent)
        {
            BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
        }

        // ��ȡ���������
        TArray<AActor*> GridManagers;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), GridManagers);
        if (GridManagers.Num() > 0)
        {
            GridManager = Cast<ALFPHexGridManager>(GridManagers[0]);
        }

        // �󶨻غ��¼�
        /*ControlledUnit->OnTurnStarted.AddDynamic(this, &ALFPAIController::StartUnitTurn);
        ControlledUnit->OnTurnEnded.AddDynamic(this, &ALFPAIController::EndUnitTurn);*/

        // ������Ϊ��
        RunBehaviorTree(BehaviorTree);
    }
}

void ALFPAIController::OnUnPossess()
{
    if (ControlledUnit)
    {
        /*ControlledUnit->OnTurnStarted.RemoveDynamic(this, &ALFPAIController::StartUnitTurn);
        ControlledUnit->OnTurnEnded.RemoveDynamic(this, &ALFPAIController::EndUnitTurn);*/
    }

    Super::OnUnPossess();
}

void ALFPAIController::StartUnitTurn()
{
    if (!ControlledUnit || !BlackboardComponent) return;

    // ���úڰ�ֵ
    BlackboardComponent->SetValueAsObject("TargetUnit", nullptr);
    BlackboardComponent->SetValueAsObject("TargetTile", nullptr);
    BlackboardComponent->SetValueAsBool("IsInAttackRange", false);
    BlackboardComponent->SetValueAsBool("CanAttack", false);
    BlackboardComponent->SetValueAsBool("ShouldEndTurn", false);

    // Ѱ�ҳ�ʼĿ��
    ALFPTacticsUnit* BestTarget = FindBestTarget();
    BlackboardComponent->SetValueAsObject("TargetUnit", BestTarget);
}

void ALFPAIController::EndUnitTurn()
{
    // ��Ϊ�����Զ����������߼�
}

ALFPTacticsUnit* ALFPAIController::FindBestTarget() const
{
    if (!ControlledUnit || !GridManager) return nullptr;

    // ��ȡ������ҵ�λ
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

    // ��ȡ���п��ƶ�λ��
    TArray<ALFPHexTile*> MovementRange = GridManager->GetTilesInRange(ControlledUnit->GetCurrentTile(), ControlledUnit->GetCurrentMovePoints());

    ALFPHexTile* BestTile = nullptr;
    float BestPositionValue = -MAX_FLT;

    for (ALFPHexTile* Tile : MovementRange)
    {
        // �������е�λ�ĸ���
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

    // ������вֵ = Ŀ�깥���� * (1 - Ŀ�굱ǰѪ��/���Ѫ��)
    float ThreatValue = Target->GetAttackPower() * (1.0f - (float)Target->GetCurrentHealth() / Target->GetMaxHealth());

    // �������� (Խ����вԽ��)
    int32 Distance = FLFPHexCoordinates::Distance(
        ControlledUnit->GetCurrentCoordinates(),
        Target->GetCurrentCoordinates()
    );
    float DistanceFactor = 1.0f / FMath::Max(Distance, 1);

    // Ӧ����Ϊ����
    if (BehaviorData)
    {
        if (BehaviorData->bPrioritizeWeakTargets)
        {
            // ���ӶԵ�Ѫ��Ŀ���Ȩ��
            float HealthRatio = (float)Target->GetCurrentHealth() / Target->GetMaxHealth();
            ThreatValue *= (2.0f - HealthRatio); // Ѫ��Խ�ͣ���вֵԽ��
        }

        // Ӧ�ù�������
        ThreatValue *= BehaviorData->Aggressiveness;
    }

    return ThreatValue * DistanceFactor;
}

float ALFPAIController::CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target) const
{
    if (!Tile || !Target) return 0.0f;

    float PositionValue = 0.0f;

    // 1. ����Ŀ��Խ��Խ��
    int32 DistanceToTarget = FLFPHexCoordinates::Distance(
        Tile->GetCoordinates(),
        Target->GetCurrentCoordinates()
    );
    PositionValue += 10.0f / FMath::Max(DistanceToTarget, 1);

    // 2. ����ڹ�����Χ�ڶ���ӷ�
    if (DistanceToTarget <= ControlledUnit->GetAttackRange())
    {
        PositionValue += 20.0f;
    }

    // 3. �����������˵�λ���Ŷ�Э����
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

    //// 4. ����Σ��λ�ã�����桢�����ȣ�
    //if (Tile->IsDangerous())
    //{
    //    PositionValue -= 30.0f;
    //}

    //// 5. �ߵ�����
    //if (Tile->IsHighGround())
    //{
    //    PositionValue += 15.0f;
    //}

    return PositionValue;
}

ALFPTacticsUnit* ALFPAIController::GetControlledUnit()
{
    return ControlledUnit;
}

void ALFPAIController::SetControlledUnit(ALFPTacticsUnit* NewUnit)
{
    ControlledUnit = NewUnit;
}
