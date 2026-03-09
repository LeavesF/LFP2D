#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPWorldMapGameMode.generated.h"

class ALFPWorldMapManager;
class ULFPUnitReplacementWidget;

/**
 * 世界地图游戏模式
 * 负责世界地图场景的初始化和管理
 */
UCLASS()
class LFP2D_API ALFPWorldMapGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void StartPlay() override;

	// 处理战斗结果
	void HandleBattleResult(const FLFPBattleResult& Result);

	// 处理捕获的单位（加入编队/后备/显示替换 UI）
	void ProcessCapturedUnits(const TArray<FLFPUnitEntry>& CapturedUnits);

	// 显示下一个替换 UI
	void ShowNextReplacementUI();

	// 替换 UI 完成回调
	UFUNCTION()
	void OnReplacementComplete();

public:
	// 世界地图管理器引用
	UPROPERTY()
	TObjectPtr<ALFPWorldMapManager> WorldMapManager;

	// 世界地图管理器类（蓝图中配置）
	UPROPERTY(EditDefaultsOnly, Category = "World Map")
	TSubclassOf<ALFPWorldMapManager> WorldMapManagerClass;

	// 默认世界地图名（首次加载用）
	UPROPERTY(EditDefaultsOnly, Category = "World Map")
	FString DefaultWorldMapName;

	// 默认起始节点 ID
	UPROPERTY(EditDefaultsOnly, Category = "World Map")
	int32 DefaultStartNodeID = 0;

	// 替换 UI Widget 类（蓝图中配置）
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ULFPUnitReplacementWidget> ReplacementWidgetClass;

	// 获取世界地图管理器
	UFUNCTION(BlueprintPure, Category = "World Map")
	ALFPWorldMapManager* GetWorldMapManager() const { return WorldMapManager; }

private:
	// 待处理的捕获单位队列
	UPROPERTY()
	TArray<FLFPUnitEntry> PendingCapturedUnits;

	// 替换 UI 实例
	UPROPERTY()
	TObjectPtr<ULFPUnitReplacementWidget> ReplacementWidget;
};
