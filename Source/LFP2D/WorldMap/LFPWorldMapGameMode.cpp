#include "LFP2D/WorldMap/LFPWorldMapGameMode.h"
#include "LFP2D/WorldMap/LFPWorldMapManager.h"
#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "LFP2D/WorldMap/LFPWorldMapPlayerController.h"
#include "LFP2D/WorldMap/LFPWorldMapPlayerState.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/UI/WorldMap/LFPUnitReplacementWidget.h"

void ALFPWorldMapGameMode::StartPlay()
{
	Super::StartPlay();

	// 生成世界地图管理器
	if (!WorldMapManagerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldMapManagerClass 未配置"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	WorldMapManager = GetWorld()->SpawnActor<ALFPWorldMapManager>(
		WorldMapManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (!WorldMapManager)
	{
		UE_LOG(LogTemp, Error, TEXT("世界地图管理器生成失败"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("世界地图管理器已生成"));

	// 从 GameInstance 获取状态
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (GI && GI->GetWorldMapSnapshot().bIsValid)
	{
		// 从快照恢复
		const FLFPWorldMapSnapshot& Snapshot = GI->GetWorldMapSnapshot();
		UE_LOG(LogTemp, Log, TEXT("从快照恢复世界地图: %s, 节点 %d, 回合 %d"),
			*Snapshot.WorldMapName, Snapshot.CurrentNodeID, Snapshot.CurrentTurn);

		// 加载地图
		WorldMapManager->LoadWorldMap(Snapshot.WorldMapName);

		// 恢复玩家状态
		WorldMapManager->InitializePlayer(Snapshot.CurrentNodeID);
		if (ULFPWorldMapPlayerState* PS = WorldMapManager->GetPlayerState())
		{
			PS->CurrentTurn = Snapshot.CurrentTurn;
			PS->VisitedNodeIDs = Snapshot.VisitedNodeIDs;
			PS->RevealedNodeIDs = Snapshot.RevealedNodeIDs;
		}

		// 恢复已触发节点
		WorldMapManager->RestoreTriggeredNodes(Snapshot.TriggeredNodeIDs);

		// 更新迷雾
		WorldMapManager->UpdateFog();

		// 处理战斗结果
		if (GI->HasPendingBattleResult())
		{
			FLFPBattleResult Result = GI->ConsumeBattleResult();
			HandleBattleResult(Result);
		}
	}
	else if (!DefaultWorldMapName.IsEmpty())
	{
		// 首次进入：加载默认地图
		UE_LOG(LogTemp, Log, TEXT("首次进入世界地图: %s"), *DefaultWorldMapName);
		WorldMapManager->LoadWorldMap(DefaultWorldMapName);
		WorldMapManager->InitializePlayer(DefaultStartNodeID);
	}
	else
	{
		// 没有地图可加载（编辑器模式或测试）
		UE_LOG(LogTemp, Log, TEXT("无地图配置，空世界地图"));
	}
}

void ALFPWorldMapGameMode::HandleBattleResult(const FLFPBattleResult& Result)
{
	if (!WorldMapManager) return;

	UE_LOG(LogTemp, Log, TEXT("处理战斗结果: 节点 %d, 胜利=%s, 逃跑=%s"),
		Result.SourceNodeID,
		Result.bVictory ? TEXT("是") : TEXT("否"),
		Result.bEscaped ? TEXT("是") : TEXT("否"));

	if (Result.bVictory)
	{
		// 胜利：节点标记为已触发（如果尚未标记）
		ALFPWorldMapNode* Node = WorldMapManager->GetNode(Result.SourceNodeID);
		if (Node && !Node->bHasBeenTriggered)
		{
			Node->bHasBeenTriggered = true;
			Node->UpdateVisualState();
		}

		// 处理捕获的单位
		if (Result.CapturedUnits.Num() > 0)
		{
			ProcessCapturedUnits(Result.CapturedUnits);
		}
	}
	else if (Result.bEscaped)
	{
		// 逃跑：不标记为已触发，可再次进入
		UE_LOG(LogTemp, Log, TEXT("逃跑：节点 %d 未标记为已触发"), Result.SourceNodeID);
	}
	else
	{
		// 失败：根据游戏设计决定处理方式
		// TODO: 游戏结束 or 惩罚后继续
		UE_LOG(LogTemp, Warning, TEXT("战斗失败：节点 %d"), Result.SourceNodeID);
	}
}

void ALFPWorldMapGameMode::ProcessCapturedUnits(const TArray<FLFPUnitEntry>& CapturedUnits)
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI) return;

	for (const FLFPUnitEntry& Unit : CapturedUnits)
	{
		if (!GI->TryAddUnit(Unit))
		{
			// 队伍和备战营都满了，加入待处理队列
			PendingCapturedUnits.Add(Unit);
		}
	}

	// 有待处理的，显示替换 UI
	if (PendingCapturedUnits.Num() > 0)
	{
		ShowNextReplacementUI();
	}
}

void ALFPWorldMapGameMode::ShowNextReplacementUI()
{
	if (PendingCapturedUnits.Num() == 0) return;

	FLFPUnitEntry NextUnit = PendingCapturedUnits[0];
	PendingCapturedUnits.RemoveAt(0);

	if (!ReplacementWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReplacementWidgetClass 未配置，放弃单位 %s"), *NextUnit.TypeID.ToString());
		ShowNextReplacementUI();
		return;
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	if (!ReplacementWidget)
	{
		ReplacementWidget = CreateWidget<ULFPUnitReplacementWidget>(PC, ReplacementWidgetClass);
	}

	if (ReplacementWidget)
	{
		ReplacementWidget->Setup(NextUnit, GI->PartyUnits, GI->ReserveUnits, GI->UnitRegistry);
		ReplacementWidget->OnReplacementComplete.AddDynamic(this, &ALFPWorldMapGameMode::OnReplacementComplete);
		ReplacementWidget->AddToViewport(100);
	}
}

void ALFPWorldMapGameMode::OnReplacementComplete()
{
	if (ReplacementWidget)
	{
		ReplacementWidget->RemoveFromParent();
		ReplacementWidget->OnReplacementComplete.RemoveDynamic(this, &ALFPWorldMapGameMode::OnReplacementComplete);
	}

	// 处理下一个
	ShowNextReplacementUI();
}
