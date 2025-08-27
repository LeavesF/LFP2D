// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindMovementTile.generated.h"

/**
 * 
 */
UCLASS()
class LFP2D_API UBTTask_FindMovementTile : public UBTTask_BlackboardBase
{
	GENERATED_BODY()
	
public:
    UBTTask_FindMovementTile();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetUnitKey;
};
