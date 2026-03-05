#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFPWorldMapGameMode.generated.h"

class ALFPWorldMapManager;

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

	// 获取世界地图管理器
	UFUNCTION(BlueprintPure, Category = "World Map")
	ALFPWorldMapManager* GetWorldMapManager() const { return WorldMapManager; }
};
