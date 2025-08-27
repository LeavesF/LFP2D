// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "LFPAIController.generated.h"

class ALFPTacticsUnit;
class ALFPHexGridManager;
class ALFPHexTile;
class ULFPEnemyBehaviorData;
/**
 * 
 */
UCLASS()
class LFP2D_API ALFPAIController : public AAIController
{
	GENERATED_BODY()

public:
    ALFPAIController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    // 开始单位回合
    UFUNCTION()
    void StartUnitTurn();

    // 结束单位回合
    UFUNCTION()
    void EndUnitTurn();

    // 获取行为树
    UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

    // 获取网格管理器
    ALFPHexGridManager* GetGridManager() const { return GridManager; }

    // 寻找最佳目标
    UFUNCTION(BlueprintCallable, Category = "AI")
    ALFPTacticsUnit* FindBestTarget() const;

    // 寻找最佳移动位置
    UFUNCTION(BlueprintCallable, Category = "AI")
    ALFPHexTile* FindBestMovementTile(ALFPTacticsUnit* Target) const;

    // 计算威胁值
    UFUNCTION(BlueprintCallable, Category = "AI")
    float CalculateThreatValue(ALFPTacticsUnit* Target) const;

    // 计算位置价值
    UFUNCTION(BlueprintCallable, Category = "AI")
    float CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target) const;

protected:
    // 行为树资产
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* BehaviorTree;

    // 黑板组件
    UPROPERTY(BlueprintReadOnly, Category = "AI")
    UBlackboardComponent* BlackboardComponent;

public:
    ALFPTacticsUnit* GetControlledUnit();

    void SetControlledUnit(ALFPTacticsUnit* NewUnit);

protected:
    // 当前控制的单位
    UPROPERTY()
    ALFPTacticsUnit* ControlledUnit;

    // 网格管理器
    UPROPERTY()
    ALFPHexGridManager* GridManager;

    // AI 行为数据
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    ULFPEnemyBehaviorData* BehaviorData;
};
