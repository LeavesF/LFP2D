// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/BehaviorTree/BTTask_FindMovementTile.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindMovementTile::UBTTask_FindMovementTile()
{
    NodeName = "Find Movement Tile";
}

EBTNodeResult::Type UBTTask_FindMovementTile::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ALFPAIController* AIController = Cast<ALFPAIController>(OwnerComp.GetAIOwner());
    if (!AIController) return EBTNodeResult::Failed;

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    ALFPTacticsUnit* TargetUnit = Cast<ALFPTacticsUnit>(Blackboard->GetValueAsObject(TargetUnitKey.SelectedKeyName));

    if (!TargetUnit) return EBTNodeResult::Failed;

    ALFPHexTile* BestTile = AIController->FindBestMovementTile(TargetUnit);
    if (BestTile)
    {
        Blackboard->SetValueAsObject(GetSelectedBlackboardKey(), BestTile);
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}