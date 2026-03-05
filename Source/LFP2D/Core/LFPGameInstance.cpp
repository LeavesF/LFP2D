#include "LFP2D/Core/LFPGameInstance.h"
#include "Kismet/GameplayStatics.h"

void ULFPGameInstance::SetBattleRequest(const FLFPBattleRequest& Request)
{
	PendingBattleRequest = Request;
	PendingBattleRequest.bIsValid = true;
	UE_LOG(LogTemp, Log, TEXT("战斗请求: 节点 %d, 地图 %s, 星级 %d"),
		Request.SourceNodeID, *Request.BattleMapName, Request.StarRating);
}

FLFPBattleRequest ULFPGameInstance::ConsumeBattleRequest()
{
	FLFPBattleRequest Result = PendingBattleRequest;
	PendingBattleRequest = FLFPBattleRequest();
	return Result;
}

void ULFPGameInstance::SetBattleResult(const FLFPBattleResult& Result)
{
	PendingBattleResult = Result;
	PendingBattleResult.bIsValid = true;
	UE_LOG(LogTemp, Log, TEXT("战斗结果: 节点 %d, 胜利=%s, 逃跑=%s"),
		Result.SourceNodeID,
		Result.bVictory ? TEXT("是") : TEXT("否"),
		Result.bEscaped ? TEXT("是") : TEXT("否"));
}

FLFPBattleResult ULFPGameInstance::ConsumeBattleResult()
{
	FLFPBattleResult Result = PendingBattleResult;
	PendingBattleResult = FLFPBattleResult();
	return Result;
}

void ULFPGameInstance::SaveWorldMapSnapshot(const FLFPWorldMapSnapshot& Snapshot)
{
	WorldMapSnapshot = Snapshot;
	WorldMapSnapshot.bIsValid = true;
	UE_LOG(LogTemp, Log, TEXT("保存世界地图快照: 节点 %d, 回合 %d, 已访问 %d 个节点"),
		Snapshot.CurrentNodeID, Snapshot.CurrentTurn, Snapshot.VisitedNodeIDs.Num());
}

void ULFPGameInstance::TransitionToBattle(const FString& BattleLevelName)
{
	FString LevelName = BattleLevelName.IsEmpty() ? DefaultBattleLevelName : BattleLevelName;
	UE_LOG(LogTemp, Log, TEXT("切换到战斗关卡: %s"), *LevelName);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

void ULFPGameInstance::TransitionToWorldMap(const FString& WorldMapLevelName)
{
	FString LevelName = WorldMapLevelName.IsEmpty() ? DefaultWorldMapLevelName : WorldMapLevelName;
	UE_LOG(LogTemp, Log, TEXT("切换到世界地图关卡: %s"), *LevelName);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}
