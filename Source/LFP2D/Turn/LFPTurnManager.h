// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFPTurnManager.generated.h"

class ALFPTacticsUnit;

UCLASS()
class LFP2D_API ALFPTurnManager : public AActor
{
	GENERATED_BODY()
	
public:
    ALFPTurnManager();

    // 开始游戏回合系统
    void StartGame();

    // 开始新回合
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void BeginNewRound();

    // 结束当前回合
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void EndCurrentRound();

    // 获取当前行动单位
    UFUNCTION(BlueprintPure, Category = "Turn System")
    ALFPTacticsUnit* GetCurrentUnit() const { return CurrentUnit; }

    // 传递回合到下一个单位
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void PassTurn();

    // 当单位完成行动
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void OnUnitFinishedAction(ALFPTacticsUnit* Unit);

    // 注册单位到回合系统
    void RegisterUnit(ALFPTacticsUnit* Unit);

    // 从回合系统注销单位
    void UnregisterUnit(ALFPTacticsUnit* Unit);

protected:
    virtual void BeginPlay() override;

private:
    // 排序单位（速度优先）
    void SortUnitsBySpeed();

    // 开始单位回合
    void BeginUnitTurn(ALFPTacticsUnit* Unit);

    // 结束单位回合
    void EndUnitTurn(ALFPTacticsUnit* Unit);

    // 单位列表
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    TArray<ALFPTacticsUnit*> AllUnits;

    // 当前行动单位
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    ALFPTacticsUnit* CurrentUnit;

    // 当前回合数
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    int32 CurrentRound = 0;

    // 是否在回合中
    UPROPERTY(VisibleAnywhere, Category = "Turn System")
    bool bIsInRound = false;
};
