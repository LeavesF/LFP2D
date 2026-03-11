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

// ============== 资源系统 ==============

void ULFPGameInstance::AddGold(int32 Amount)
{
	Gold += Amount;
	UE_LOG(LogTemp, Log, TEXT("金币变动: %+d, 当前: %d"), Amount, Gold);
}

void ULFPGameInstance::AddFood(int32 Amount)
{
	Food += Amount;
	UE_LOG(LogTemp, Log, TEXT("食物变动: %+d, 当前: %d"), Amount, Food);
}

// ============== 编队系统 ==============

bool ULFPGameInstance::TryAddUnit(const FLFPUnitEntry& Unit)
{
	if (!Unit.IsValid()) return false;

	if (!IsPartyFull())
	{
		PartyUnits.Add(Unit);
		UE_LOG(LogTemp, Log, TEXT("单位 %s 加入出战队伍（%d/%d）"), *Unit.TypeID.ToString(), PartyUnits.Num(), MaxPartySize);
		return true;
	}
	if (!IsReserveFull())
	{
		ReserveUnits.Add(Unit);
		UE_LOG(LogTemp, Log, TEXT("单位 %s 加入备战营（%d/%d）"), *Unit.TypeID.ToString(), ReserveUnits.Num(), MaxReserveSize);
		return true;
	}
	return false;
}

void ULFPGameInstance::ReplacePartyUnit(int32 SlotIndex, const FLFPUnitEntry& NewUnit)
{
	if (PartyUnits.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("替换队伍槽 %d: %s → %s"), SlotIndex, *PartyUnits[SlotIndex].TypeID.ToString(), *NewUnit.TypeID.ToString());
		PartyUnits[SlotIndex] = NewUnit;
	}
}

void ULFPGameInstance::ReplaceReserveUnit(int32 SlotIndex, const FLFPUnitEntry& NewUnit)
{
	if (ReserveUnits.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("替换备战营槽 %d: %s → %s"), SlotIndex, *ReserveUnits[SlotIndex].TypeID.ToString(), *NewUnit.TypeID.ToString());
		ReserveUnits[SlotIndex] = NewUnit;
	}
}

void ULFPGameInstance::RemovePartyUnit(int32 SlotIndex)
{
	if (PartyUnits.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("移除队伍槽 %d: %s"), SlotIndex, *PartyUnits[SlotIndex].TypeID.ToString());
		PartyUnits.RemoveAt(SlotIndex);
	}
}

void ULFPGameInstance::RemoveReserveUnit(int32 SlotIndex)
{
	if (ReserveUnits.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("移除备战营槽 %d: %s"), SlotIndex, *ReserveUnits[SlotIndex].TypeID.ToString());
		ReserveUnits.RemoveAt(SlotIndex);
	}
}
