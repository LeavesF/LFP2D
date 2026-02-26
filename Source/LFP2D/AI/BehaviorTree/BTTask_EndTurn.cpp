// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/BehaviorTree/BTTask_EndTurn.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_EndTurn::UBTTask_EndTurn()
{
    NodeName = "End Turn";
}

EBTNodeResult::Type UBTTask_EndTurn::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ALFPAIController* AIController = Cast<ALFPAIController>(OwnerComp.GetAIOwner());
    if (!AIController) return EBTNodeResult::Failed;

    // 通知回合管理器结束回合
    if (ALFPTurnManager* TurnManager = AIController->GetControlledUnit()->GetTurnManager())
    {
        TurnManager->PassTurn();
    }

    return EBTNodeResult::Succeeded;
}
