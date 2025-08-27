// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/BehaviorTree/BTTask_FindTarget.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindTarget::UBTTask_FindTarget()
{
    NodeName = "Find Target";
    bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_FindTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ALFPAIController* AIController = Cast<ALFPAIController>(OwnerComp.GetAIOwner());
    if (!AIController) return EBTNodeResult::Failed;

    ALFPTacticsUnit* BestTarget = AIController->FindBestTarget();
    if (BestTarget)
    {
        OwnerComp.GetBlackboardComponent()->SetValueAsObject(GetSelectedBlackboardKey(), BestTarget);
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}