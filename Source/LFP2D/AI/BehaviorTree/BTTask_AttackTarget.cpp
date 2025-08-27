// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/BehaviorTree/BTTask_AttackTarget.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
    bNotifyTaskFinished = true;
    NodeName = "Attack Target";
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ALFPAIController* AIController = Cast<ALFPAIController>(OwnerComp.GetAIOwner());
    if (!AIController) return EBTNodeResult::Failed;

    ALFPTacticsUnit* ControlledUnit = Cast<ALFPTacticsUnit>(AIController->GetPawn());
    if (!ControlledUnit) return EBTNodeResult::Failed;

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    ALFPTacticsUnit* TargetUnit = Cast<ALFPTacticsUnit>(Blackboard->GetValueAsObject(GetSelectedBlackboardKey()));

    if (!TargetUnit) return EBTNodeResult::Failed;

    // 绑定攻击完成事件
    //ControlledUnit->OnAttackComplete.AddDynamic(this, &UBTTask_AttackTarget::OnAttackComplete);
    BTComponent = &OwnerComp;

    // 开始攻击
    ControlledUnit->AttackTarget(TargetUnit);

    return EBTNodeResult::InProgress;
}

void UBTTask_AttackTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // 清理
    /*if (ALFPTacticsUnit* ControlledUnit = Cast<ALFPTacticsUnit>(OwnerComp.GetAIOwner()->GetPawn()))
    {
        ControlledUnit->OnAttackComplete.RemoveDynamic(this, &UBTTask_AttackTarget::OnAttackComplete);
    }*/
}

void UBTTask_AttackTarget::OnAttackComplete()
{
    if (BTComponent)
    {
        FinishLatentTask(*BTComponent, EBTNodeResult::Succeeded);
    }
}