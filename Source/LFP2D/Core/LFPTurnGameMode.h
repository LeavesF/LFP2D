// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPTurnGameMode.generated.h"

class ALFPTacticsUnit;

/**
 * 战斗游戏模式
 */
UCLASS()
class LFP2D_API ALFPTurnGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;

	// 结束战斗，写回结果并返回世界地图
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void EndBattle(bool bVictory, bool bEscaped = false);

	// 获取当前战斗请求
	UFUNCTION(BlueprintPure, Category = "Battle")
	const FLFPBattleRequest& GetBattleRequest() const { return CachedBattleRequest; }

	// 是否由世界地图触发的战斗（否则为独立测试）
	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsWorldMapBattle() const { return CachedBattleRequest.bIsValid; }

	// ============== 捕获追踪 ==============

	// 记录战斗中被捕获的单位（背叛转化后调用）
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void RecordCapturedUnit(ALFPTacticsUnit* Unit);

	// 获取捕获列表
	UFUNCTION(BlueprintPure, Category = "Battle")
	const TArray<FLFPUnitEntry>& GetCapturedUnits() const { return CapturedUnits; }

protected:
	// 缓存的战斗请求
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	FLFPBattleRequest CachedBattleRequest;

	// 本场捕获的单位列表
	UPROPERTY()
	TArray<FLFPUnitEntry> CapturedUnits;
};
