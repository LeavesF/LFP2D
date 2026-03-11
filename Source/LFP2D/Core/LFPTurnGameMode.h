// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPTurnGameMode.generated.h"

class ALFPTacticsUnit;
class ALFPHexGridManager;
class ALFPTurnManager;
class ULFPBattleResultWidget;

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

	// ============== 掉落追踪 ==============

	// 敌方单位被击杀时调用（累加掉落）
	void OnEnemyUnitKilled(ALFPTacticsUnit* Unit);

	// 获取网格管理器
	UFUNCTION(BlueprintPure, Category = "Battle")
	ALFPHexGridManager* GetGridManager() const { return GridManager; }

	// 获取回合管理器
	UFUNCTION(BlueprintPure, Category = "Battle")
	ALFPTurnManager* GetTurnManager() const { return TurnManager; }

protected:
	// 缓存的战斗请求
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	FLFPBattleRequest CachedBattleRequest;

	// 本场捕获的单位列表
	UPROPERTY()
	TArray<FLFPUnitEntry> CapturedUnits;

	// ============== 掉落累积 ==============

	// 累积的击杀掉落金币
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle|Loot")
	int32 AccumulatedDropGold = 0;

	// 累积的击杀掉落食物
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battle|Loot")
	int32 AccumulatedDropFood = 0;

	// ============== 结算 UI ==============

	// 结算 Widget 蓝图类
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULFPBattleResultWidget> BattleResultWidgetClass;

	// 结算 Widget 实例
	UPROPERTY()
	TObjectPtr<ULFPBattleResultWidget> BattleResultWidget;

	// 缓存的战斗结果（确认后写回 GameInstance）
	UPROPERTY()
	FLFPBattleResult CachedBattleResult;

	// 结算确认回调
	UFUNCTION()
	void OnBattleResultConfirmed();

	// ============== 管理器 ==============

	// 网格管理器类（蓝图中配置）
	UPROPERTY(EditDefaultsOnly, Category = "Battle")
	TSubclassOf<ALFPHexGridManager> GridManagerClass;

	// 网格管理器实例
	UPROPERTY()
	TObjectPtr<ALFPHexGridManager> GridManager;

	// 回合管理器实例
	UPROPERTY()
	TObjectPtr<ALFPTurnManager> TurnManager;
};
