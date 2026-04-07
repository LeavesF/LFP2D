#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/Shop/LFPRelicDataAsset.h"
#include "LFP2D/Shop/LFPShopDataAsset.h"
#include "LFP2D/Shop/LFPHireMarketDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

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
	if (bIsTransitioning) return;
	bIsTransitioning = true;

	FString LevelName = BattleLevelName.IsEmpty() ? DefaultBattleLevelName : BattleLevelName;
	UE_LOG(LogTemp, Log, TEXT("切换到战斗关卡: %s"), *LevelName);

	// 淡出到黑
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, TransitionFadeDuration, FLinearColor::Black, false, true);
		}

		// 延迟后加载关卡
		FTimerHandle TimerHandle;
		FString CapturedLevelName = LevelName;
		World->GetTimerManager().SetTimer(TimerHandle, [this, CapturedLevelName]()
		{
			bIsTransitioning = false;
			UGameplayStatics::OpenLevel(this, FName(*CapturedLevelName));
		}, TransitionFadeDuration, false);
	}
	else
	{
		bIsTransitioning = false;
		UGameplayStatics::OpenLevel(this, FName(*LevelName));
	}
}

void ULFPGameInstance::TransitionToWorldMap(const FString& WorldMapLevelName)
{
	if (bIsTransitioning) return;
	bIsTransitioning = true;

	FString LevelName = WorldMapLevelName.IsEmpty() ? DefaultWorldMapLevelName : WorldMapLevelName;
	UE_LOG(LogTemp, Log, TEXT("切换到世界地图关卡: %s"), *LevelName);

	// 淡出到黑
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, TransitionFadeDuration, FLinearColor::Black, false, true);
		}

		// 延迟后加载关卡
		FTimerHandle TimerHandle;
		FString CapturedLevelName = LevelName;
		World->GetTimerManager().SetTimer(TimerHandle, [this, CapturedLevelName]()
		{
			bIsTransitioning = false;
			UGameplayStatics::OpenLevel(this, FName(*CapturedLevelName));
		}, TransitionFadeDuration, false);
	}
	else
	{
		bIsTransitioning = false;
		UGameplayStatics::OpenLevel(this, FName(*LevelName));
	}
}

// ============== 资源系统 ==============

void ULFPGameInstance::AddGold(int32 Amount)
{
	Gold += Amount;
	UE_LOG(LogTemp, Log, TEXT("金币变动: %+d, 当前: %d"), Amount, Gold);
	OnResourceChanged.Broadcast(Gold, Food);
}

void ULFPGameInstance::AddFood(int32 Amount)
{
	Food += Amount;
	UE_LOG(LogTemp, Log, TEXT("食物变动: %+d, 当前: %d"), Amount, Food);
	OnResourceChanged.Broadcast(Gold, Food);
}

bool ULFPGameInstance::SpendGold(int32 Amount)
{
	if (Amount < 0)
	{
		return false;
	}

	if (!CanAffordGold(Amount))
	{
		UE_LOG(LogTemp, Warning, TEXT("金币不足: 需要 %d, 当前 %d"), Amount, Gold);
		return false;
	}

	Gold -= Amount;
	UE_LOG(LogTemp, Log, TEXT("金币变动: -%d, 当前: %d"), Amount, Gold);
	OnResourceChanged.Broadcast(Gold, Food);
	return true;
}

bool ULFPGameInstance::HasRelic(FName RelicID) const
{
	return RelicID != NAME_None && OwnedRelicIDs.Contains(RelicID);
}

bool ULFPGameInstance::TryAddOwnedRelic(FName RelicID)
{
	if (RelicID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryAddOwnedRelic: RelicID 无效"));
		return false;
	}

	FLFPRelicDefinition RelicDefinition;
	if (!FindRelicDefinition(RelicID, RelicDefinition))
	{
		UE_LOG(LogTemp, Warning, TEXT("TryAddOwnedRelic: 未找到遗物 %s"), *RelicID.ToString());
		return false;
	}

	if (HasRelic(RelicID))
	{
		UE_LOG(LogTemp, Warning, TEXT("TryAddOwnedRelic: 已拥有遗物 %s"), *RelicID.ToString());
		return false;
	}

	OwnedRelicIDs.Add(RelicID);
	UE_LOG(LogTemp, Log, TEXT("获得遗物成功: %s"), *RelicID.ToString());
	OnOwnedRelicsChanged.Broadcast();
	return true;
}

TArray<FName> ULFPGameInstance::GetOwnedRelicIDsArray() const
{
	return OwnedRelicIDs.Array();
}

bool ULFPGameInstance::FindRelicDefinition(FName RelicID, FLFPRelicDefinition& OutDefinition) const
{
	return RelicDataAsset ? RelicDataAsset->FindRelicDefinition(RelicID, OutDefinition) : false;
}

bool ULFPGameInstance::FindShopDefinition(FName ShopID, FLFPShopDefinition& OutDefinition) const
{
	return ShopDataAsset ? ShopDataAsset->FindShopDefinition(ShopID, OutDefinition) : false;
}

bool ULFPGameInstance::TryPurchaseRelic(FName RelicID, int32 Price)
{
	if (RelicID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPurchaseRelic: RelicID 无效"));
		return false;
	}

	if (!SpendGold(Price))
	{
		return false;
	}

	if (!TryAddOwnedRelic(RelicID))
	{
		AddGold(Price);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("购买遗物成功: %s"), *RelicID.ToString());
	return true;
}

void ULFPGameInstance::ApplyOwnedRelicsToUnit(ALFPTacticsUnit* Unit) const
{
	if (!Unit || !RelicDataAsset)
	{
		return;
	}

	for (const FName& RelicID : OwnedRelicIDs)
	{
		FLFPRelicDefinition Definition;
		if (!FindRelicDefinition(RelicID, Definition))
		{
			continue;
		}

		for (const FLFPRelicEffectEntry& Effect : Definition.Effects)
		{
			switch (Effect.EffectType)
			{
			case ELFPRelicEffectType::RET_MaxHealthFlat:
				Unit->AddCurrentMaxHealth(Effect.Value);
				break;
			case ELFPRelicEffectType::RET_AttackFlat:
				Unit->AddCurrentAttack(Effect.Value);
				break;
			case ELFPRelicEffectType::RET_DefenseFlat:
				Unit->AddCurrentPhysicalBlock(Effect.Value);
				break;
			case ELFPRelicEffectType::RET_SpeedFlat:
				Unit->AddCurrentSpeed(Effect.Value);
				break;
			default:
				break;
			}
		}
	}
}

// ============== 雇佣市场 ==============

bool ULFPGameInstance::FindHireMarketDefinition(FName HireMarketID, FLFPHireMarketDefinition& OutDefinition) const
{
	return HireMarketDataAsset ? HireMarketDataAsset->FindHireMarketDefinition(HireMarketID, OutDefinition) : false;
}

bool ULFPGameInstance::HasPurchasedHireMarketUnit(FName HireMarketID, FName UnitTypeID) const
{
	if (HireMarketID == NAME_None || UnitTypeID == NAME_None)
	{
		return false;
	}

	const TSet<FName>* PurchasedUnits = PurchasedHireMarketUnitMap.Find(HireMarketID);
	return PurchasedUnits && PurchasedUnits->Contains(UnitTypeID);
}

bool ULFPGameInstance::TryPurchaseHireMarketUnit(FName HireMarketID, FName UnitTypeID, int32 Price, FLFPUnitEntry& OutUnit)
{
	if (HireMarketID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPurchaseHireMarketUnit: HireMarketID 无效"));
		return false;
	}

	if (HasPurchasedHireMarketUnit(HireMarketID, UnitTypeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("TryPurchaseHireMarketUnit: 单位 %s 已在市场 %s 购买过"), *UnitTypeID.ToString(), *HireMarketID.ToString());
		return false;
	}

	if (!TrySpendForUnit(UnitTypeID, Price, OutUnit))
	{
		return false;
	}

	PurchasedHireMarketUnitMap.FindOrAdd(HireMarketID).Add(UnitTypeID);
	UE_LOG(LogTemp, Log, TEXT("雇佣市场购买成功: 市场 %s, 单位 %s"), *HireMarketID.ToString(), *UnitTypeID.ToString());
	return true;
}

bool ULFPGameInstance::TrySpendForUnit(FName UnitTypeID, int32 Price, FLFPUnitEntry& OutUnit)
{
	if (UnitTypeID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrySpendForUnit: UnitTypeID 无效"));
		return false;
	}

	if (!UnitRegistry)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrySpendForUnit: 注册表未配置"));
		return false;
	}

	FLFPUnitRegistryEntry RegistryEntry;
	if (!UnitRegistry->FindEntry(UnitTypeID, RegistryEntry))
	{
		UE_LOG(LogTemp, Warning, TEXT("TrySpendForUnit: 注册表中未找到单位 %s"), *UnitTypeID.ToString());
		return false;
	}

	if (!SpendGold(Price))
	{
		return false;
	}

	OutUnit.TypeID = UnitTypeID;
	OutUnit.Tier = RegistryEntry.Tier;
	UE_LOG(LogTemp, Log, TEXT("雇佣单位扣款成功: %s (T%d), 花费 %d 金币"), *UnitTypeID.ToString(), OutUnit.Tier, Price);
	return true;
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

// ============== 单位升阶 ==============

TArray<FLFPUnitEntry> ULFPGameInstance::GetMergeablePairs() const
{
	// 统计每个 TypeID 的出现次数
	TMap<FName, int32> CountMap;

	for (const FLFPUnitEntry& Unit : PartyUnits)
	{
		if (Unit.IsValid())
		{
			CountMap.FindOrAdd(Unit.TypeID, 0)++;
		}
	}
	for (const FLFPUnitEntry& Unit : ReserveUnits)
	{
		if (Unit.IsValid())
		{
			CountMap.FindOrAdd(Unit.TypeID, 0)++;
		}
	}

	// 筛选出现 >= 2 次且注册表中有进化目标的 TypeID
	TArray<FLFPUnitEntry> Result;
	for (const auto& Pair : CountMap)
	{
		if (Pair.Value >= 2 && UnitRegistry && UnitRegistry->CanEvolve(Pair.Key))
		{
			FLFPUnitEntry Entry;
			Entry.TypeID = Pair.Key;
			Entry.Tier = UnitRegistry->GetUnitTier(Pair.Key);
			Result.Add(Entry);
		}
	}

	return Result;
}

bool ULFPGameInstance::MergeUnits(bool bSourceAIsParty, int32 SourceAIndex,
	bool bSourceBIsParty, int32 SourceBIndex, FName TargetTypeID)
{
	TArray<FLFPUnitEntry>& ArrayA = bSourceAIsParty ? PartyUnits : ReserveUnits;
	TArray<FLFPUnitEntry>& ArrayB = bSourceBIsParty ? PartyUnits : ReserveUnits;

	if (!ArrayA.IsValidIndex(SourceAIndex) || !ArrayB.IsValidIndex(SourceBIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeUnits: 无效的槽位索引"));
		return false;
	}

	const FLFPUnitEntry& UnitA = ArrayA[SourceAIndex];
	const FLFPUnitEntry& UnitB = ArrayB[SourceBIndex];

	// 验证两个源单位 TypeID 相同
	if (UnitA.TypeID != UnitB.TypeID)
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeUnits: 单位类型不匹配 (%s vs %s)"),
			*UnitA.TypeID.ToString(), *UnitB.TypeID.ToString());
		return false;
	}

	// 验证 TargetTypeID 在进化目标列表中
	if (!UnitRegistry)
	{
		UE_LOG(LogTemp, Error, TEXT("MergeUnits: 注册表未配置"));
		return false;
	}

	TArray<FName> Targets = UnitRegistry->GetEvolutionTargets(UnitA.TypeID);
	if (!Targets.Contains(TargetTypeID))
	{
		UE_LOG(LogTemp, Warning, TEXT("MergeUnits: %s 不是 %s 的有效进化目标"),
			*TargetTypeID.ToString(), *UnitA.TypeID.ToString());
		return false;
	}

	// 创建进化后的新单位
	FLFPUnitEntry NewUnit;
	NewUnit.TypeID = TargetTypeID;
	NewUnit.Tier = UnitRegistry->GetUnitTier(TargetTypeID);

	// 移除两个源单位（同一数组时先移除大索引避免偏移）
	if (bSourceAIsParty == bSourceBIsParty)
	{
		int32 First = FMath::Max(SourceAIndex, SourceBIndex);
		int32 Second = FMath::Min(SourceAIndex, SourceBIndex);
		ArrayA.RemoveAt(First);
		ArrayA.RemoveAt(Second);
	}
	else
	{
		ArrayA.RemoveAt(SourceAIndex);
		ArrayB.RemoveAt(SourceBIndex);
	}

	TryAddUnit(NewUnit);

	UE_LOG(LogTemp, Log, TEXT("进化成功: %s × 2 → %s (T%d)"),
		*UnitA.TypeID.ToString(), *TargetTypeID.ToString(), NewUnit.Tier);

	return true;
}
