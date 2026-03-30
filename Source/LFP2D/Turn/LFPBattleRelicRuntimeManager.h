#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFPBattleRelicRuntimeManager.generated.h"

class ULFPGameInstance;
class ALFPTurnManager;
class ALFPTacticsUnit;
class ULFPRelicDataAsset;
struct FLFPRelicBattleRule;
struct FLFPRelicSynergyRule;

// 单位运行时状态
USTRUCT()
struct FLFPUnitRelicRuntimeState
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ALFPTacticsUnit> Unit;

	// 是否正在重建属性（防重入）
	bool bIsRebuilding = false;

	// 上次已知血量比例区间（用于判断是否跨越阈值）
	float LastHealthPercent = 1.0f;
};

/**
 * 战斗遗物运行时管理器
 * 负责战斗期间遗物效果的条件判断、属性重建、即时效果执行
 */
UCLASS()
class LFP2D_API ALFPBattleRelicRuntimeManager : public AActor
{
	GENERATED_BODY()

public:
	ALFPBattleRelicRuntimeManager();

	// 初始化（由 TurnGameMode 调用）
	void Initialize(ULFPGameInstance* InGameInstance, ALFPTurnManager* InTurnManager);

	// 部署结束后扫描并绑定玩家单位
	void OnDeploymentFinished();

	// 回合开始前刷新所有玩家单位属性
	void OnRoundStarted();

	// 回合结束时执行即时效果
	void OnRoundEnded(int32 CurrentRound);

protected:
	// 扫描并绑定玩家单位
	void ScanAndBindPlayerUnits();

	// 绑定单个单位事件
	void BindUnitEvents(ALFPTacticsUnit* Unit);

	// 解绑单个单位事件
	void UnbindUnitEvents(ALFPTacticsUnit* Unit);

	// 重建单位遗物属性
	void RebuildUnitRelicStats(ALFPTacticsUnit* Unit);

	// 血量变化回调
	UFUNCTION()
	void OnUnitHealthChanged(ALFPTacticsUnit* Unit, int32 CurrentHealth, int32 MaxHealth);

	// 单位死亡回调
	UFUNCTION()
	void OnUnitDeath(ALFPTacticsUnit* Unit);

	// 检查条件是否满足
	bool EvaluateConditions(const TArray<struct FLFPRelicCondition>& Conditions, ALFPTacticsUnit* Unit) const;

	// 应用效果
	void ApplyEffects(const TArray<struct FLFPRelicBattleEffect>& Effects, ALFPTacticsUnit* Unit);

protected:
	UPROPERTY()
	TObjectPtr<ULFPGameInstance> CachedGameInstance;

	UPROPERTY()
	TObjectPtr<ALFPTurnManager> CachedTurnManager;

	UPROPERTY()
	TObjectPtr<ULFPRelicDataAsset> CachedRelicDataAsset;

	// 已拥有遗物 ID 缓存
	UPROPERTY()
	TArray<FName> OwnedRelicIDs;

	// 已绑定的玩家单位运行时状态
	UPROPERTY()
	TArray<FLFPUnitRelicRuntimeState> BoundUnits;

	// 上次处理回合结束效果的回合数
	int32 LastProcessedRoundEndRound = -1;
};
