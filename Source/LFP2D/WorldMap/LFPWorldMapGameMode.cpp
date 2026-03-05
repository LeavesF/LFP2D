#include "LFP2D/WorldMap/LFPWorldMapGameMode.h"
#include "LFP2D/WorldMap/LFPWorldMapManager.h"
#include "LFP2D/WorldMap/LFPWorldMapPlayerController.h"

void ALFPWorldMapGameMode::StartPlay()
{
	Super::StartPlay();

	// 生成世界地图管理器
	if (WorldMapManagerClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		WorldMapManager = GetWorld()->SpawnActor<ALFPWorldMapManager>(
			WorldMapManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (WorldMapManager)
		{
			UE_LOG(LogTemp, Log, TEXT("世界地图管理器已生成"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldMapManagerClass 未配置"));
	}
}
