// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_MoveToTile.generated.h"

/**
 * 
 */
UCLASS()
class LFP2D_API UBTTask_MoveToTile : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
    UBTTask_MoveToTile();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
    UFUNCTION()
    void OnMoveComplete();

    UPROPERTY()
    UBehaviorTreeComponent* BTComponent;
};
