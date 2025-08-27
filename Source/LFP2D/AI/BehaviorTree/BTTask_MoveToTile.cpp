// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/AI/BehaviorTree/BTTask_MoveToTile.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_MoveToTile::UBTTask_MoveToTile()
{
    bNotifyTaskFinished = true;
    NodeName = "Move To Tile";
}

EBTNodeResult::Type UBTTask_MoveToTile::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ALFPAIController* AIController = Cast<ALFPAIController>(OwnerComp.GetAIOwner());
    if (!AIController) return EBTNodeResult::Failed;

    ALFPTacticsUnit* ControlledUnit = Cast<ALFPTacticsUnit>(AIController->GetPawn());
    if (!ControlledUnit) return EBTNodeResult::Failed;

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    ALFPHexTile* TargetTile = Cast<ALFPHexTile>(Blackboard->GetValueAsObject(GetSelectedBlackboardKey()));

    if (!TargetTile) return EBTNodeResult::Failed;

    // 绑定移动完成事件
    //ControlledUnit->OnMoveComplete.AddDynamic(this, &UBTTask_MoveToTile::OnMoveComplete);
    BTComponent = &OwnerComp;

    // 开始移动
    ControlledUnit->MoveToTile(TargetTile);

    return EBTNodeResult::InProgress;
}

void UBTTask_MoveToTile::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // 清理
    /*if (ALFPTacticsUnit* ControlledUnit = Cast<ALFPTacticsUnit>(OwnerComp.GetAIOwner()->GetPawn()))
    {
        ControlledUnit->OnMoveComplete.RemoveDynamic(this, &UBTTask_MoveToTile::OnMoveComplete);
    }*/
}

void UBTTask_MoveToTile::OnMoveComplete()
{
    if (BTComponent)
    {
        FinishLatentTask(*BTComponent, EBTNodeResult::Succeeded);
    }
}

