// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/BehaviorTree/BTService_UpdateConditions.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdateConditions::UBTService_UpdateConditions()
{
    NodeName = "Update Conditions";
    bNotifyTick = true;
}

void UBTService_UpdateConditions::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    ALFPAIController* AIController = Cast<ALFPAIController>(OwnerComp.GetAIOwner());
    if (!AIController) return;

    ALFPTacticsUnit* ControlledUnit = Cast<ALFPTacticsUnit>(AIController->GetPawn());
    if (!ControlledUnit) return;

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    ALFPTacticsUnit* TargetUnit = Cast<ALFPTacticsUnit>(Blackboard->GetValueAsObject(TargetUnitKey.SelectedKeyName));

    // 更新攻击范围状态
    bool bInAttackRange = TargetUnit && ControlledUnit->IsTargetInAttackRange(TargetUnit);
    Blackboard->SetValueAsBool("IsInAttackRange", bInAttackRange);

    // 更新能否攻击状态
    bool bCanAttack = TargetUnit && bInAttackRange && ControlledUnit->CanAct();
    Blackboard->SetValueAsBool("CanAttack", bCanAttack);

    // 更新是否应该结束回合
    bool bShouldEndTurn = !TargetUnit || (ControlledUnit->HasActed() && !bCanAttack);
    Blackboard->SetValueAsBool("ShouldEndTurn", bShouldEndTurn);
}