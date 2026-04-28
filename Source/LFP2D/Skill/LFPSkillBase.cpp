// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillRangeDataAsset.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "UObject/UnrealType.h"

namespace
{
const TArray<FLFPHexCoordinates>& GetSkillRayDirections()
{
	static const TArray<FLFPHexCoordinates> Directions = {
		FLFPHexCoordinates(1, 0),
		FLFPHexCoordinates(1, -1),
		FLFPHexCoordinates(0, -1),
		FLFPHexCoordinates(-1, 0),
		FLFPHexCoordinates(-1, 1),
		FLFPHexCoordinates(0, 1)
	};

	return Directions;
}

void AppendCoordsInDistanceRange(TArray<FLFPHexCoordinates>& OutCoords, int32 MaxDistance, int32 MinExclusiveDistance)
{
	if (MaxDistance <= 0 || MinExclusiveDistance >= MaxDistance)
	{
		return;
	}

	const int32 EffectiveMinDistance = FMath::Max(0, MinExclusiveDistance);

	for (int32 Q = -MaxDistance; Q <= MaxDistance; ++Q)
	{
		const int32 MinR = FMath::Max(-MaxDistance, -Q - MaxDistance);
		const int32 MaxR = FMath::Min(MaxDistance, -Q + MaxDistance);

		for (int32 R = MinR; R <= MaxR; ++R)
		{
			const FLFPHexCoordinates Coord(Q, R);
			const int32 Distance = FLFPHexCoordinates::Distance(FLFPHexCoordinates(), Coord);
			if (Distance >= EffectiveMinDistance && Distance <= MaxDistance)
			{
				OutCoords.Add(Coord);
			}
		}
	}
}
}


ULFPSkillBase::ULFPSkillBase()
{
	CurrentCooldown = 0;
	CooldownRounds = 0;
	ActionPointCost = 1;
	bIsDefaultAttack = false;
	SkillPriority = BasePriority;
}

void ULFPSkillBase::InitSkill(ALFPTacticsUnit* InOwner)
{
	// 用蓝图配置的 BasePriority 初始化运行时优先级
	// （构造函数执行时蓝图默认值尚未应用，所以需要在这里重新赋值）
	SkillPriority = BasePriority;
	if (InOwner)
	{
		Owner = InOwner;
	}

	UpdateSkillRange();
}

void ULFPSkillBase::Execute_Implementation(ALFPHexTile* TargetTile)
{
}

void ULFPSkillBase::RegisterPassiveBuffs_Implementation(ALFPTacticsUnit* InOwner)
{
}

ALFPTacticsUnit* ULFPSkillBase::GetUnitOnTile(ALFPHexTile* Tile) const
{
	if (!Tile)
	{
		return nullptr;
	}

	return Tile->GetUnitOnTile();
}

bool ULFPSkillBase::IsHostileTarget(ALFPTacticsUnit* Target) const
{
	if (!Owner || !Target)
	{
		return false;
	}

	return Owner->GetAffiliation() != Target->GetAffiliation();
}

bool ULFPSkillBase::IsValidReleaseTargetTile(ALFPHexTile* TargetTile) const
{
	if (!TargetTile)
	{
		return false;
	}

	ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
	switch (TargetType)
	{
	case ESkillTargetType::Self:
		return Owner && TargetUnit == Owner;

	case ESkillTargetType::SingleEnemy:
		return TargetUnit && TargetUnit->IsAlive() && IsHostileTarget(TargetUnit);

	case ESkillTargetType::SingleAlly:
		return TargetUnit && TargetUnit->IsAlive() && Owner && !IsHostileTarget(TargetUnit);

	case ESkillTargetType::SingleUnit:
		return TargetUnit && TargetUnit->IsAlive();

	case ESkillTargetType::AnyTile:
	case ESkillTargetType::MutiAlly:
	case ESkillTargetType::MutiEnemy:
	case ESkillTargetType::MutiUnit:
	case ESkillTargetType::AllAlly:
	case ESkillTargetType::AllEnemy:
	case ESkillTargetType::AllUnit:
	default:
		return true;
	}
}

int32 ULFPSkillBase::DealOwnerSkillDamage(ALFPTacticsUnit* Target) const
{
	if (!Owner || !Target)
	{
		return 0;
	}

	return Owner->ApplySkillDamage(Target, this);
}

int32 ULFPSkillBase::GetHitCount_Implementation(ALFPTacticsUnit* Target) const
{
	return 1;
}

float ULFPSkillBase::GetDamageScalePerHit_Implementation(ALFPTacticsUnit* Target) const
{
	return 1.0f;
}

ELFPAttackType ULFPSkillBase::GetDamageType_Implementation(ALFPTacticsUnit* Target) const
{
	return Owner ? Owner->GetAttackType() : ELFPAttackType::AT_Physical;
}

float ULFPSkillBase::GetCriticalChance_Implementation(ALFPTacticsUnit* Target) const
{
	return 0.0f;
}

float ULFPSkillBase::GetCriticalMultiplier_Implementation(ALFPTacticsUnit* Target) const
{
	return 2.0f;
}

bool ULFPSkillBase::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
	if (!Owner) return false;
	if (bIsPassiveSkill) return false;

	// 检查冷却
	//if (CurrentCooldown > 0) return false;

	// 检查行动力
	if (!Owner->IsEnemy() && !Owner->HasEnoughActionPoints(ActionPointCost)) return false;
	if (TargetTile && !IsValidReleaseTargetTile(TargetTile)) return false;

	return true;
}

bool ULFPSkillBase::IsAvailable() const
{
	if (!Owner) return false;
	if (bIsPassiveSkill) return false;

	// 检查冷却
	if (CurrentCooldown > 0) return false;

	// 检查行动点
	if (!Owner->HasEnoughActionPoints(ActionPointCost)) return false;

	return true;
}

bool ULFPSkillBase::CanReleaseFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile)
{
	if (!CasterTile || !TargetTile) return false;
	if (!IsValidReleaseTargetTile(TargetTile)) return false;

	// 计算施法格子到目标格子的距离
	UpdateSkillRange();
	const FLFPHexCoordinates CasterCoord = CasterTile->GetCoordinates();
	const FLFPHexCoordinates TargetCoord = TargetTile->GetCoordinates();
	const FLFPHexCoordinates RelativeCoord(TargetCoord.Q - CasterCoord.Q, TargetCoord.R - CasterCoord.R);
	const bool bHasReleaseCoord = HasReleaseCoord(RelativeCoord);
	//int32 Dist = FLFPHexCoordinates::Distance(
	//    CasterTile->GetCoordinates(),
	//    TargetTile->GetCoordinates()
	//);

	// 检查是否在技能释放范围内
	if (!bHasReleaseCoord) return false;

	// 视线检查
	if (bRequireLineOfSight && Owner)
	{
		if (ALFPHexGridManager* GridManager = Owner->GetGridManager())
		{
			if (!GridManager->IsLineOfSightClear(CasterTile, TargetTile))
			{
				return false;
			}
		}
	}

	return true;
}

bool ULFPSkillBase::CanPlanFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile)
{
	if (!Owner || bIsPassiveSkill)
	{
		return false;
	}

	// 规划阶段按“计划施法位 -> 计划目标位”做判断，
	// 不直接依赖 Owner 当前真实站位。
	return CanReleaseFromInternal(CasterTile, TargetTile, true);
}

FString ULFPSkillBase::GetCooldownStatus() const
{
	if (CurrentCooldown <= 0)
	{
		return FString::Printf(TEXT("Ready"));
	}
	else
	{
		return FString::Printf(TEXT("%d Round"), CurrentCooldown);
	}
}

TArray<ALFPHexTile*> ULFPSkillBase::GetTargetTiles(ALFPTacticsUnit* Caster) const
{
	TArray<ALFPHexTile*> TargetTiles;

	if (!Caster || !Caster->GetCurrentTile()) return TargetTiles;

	ALFPHexGridManager* GridManager = Caster->GetGridManager();
	if (!GridManager) return TargetTiles;

	// 根据技能范围和目标类型获取目标格子
	// 这里需要实现具体的逻辑
	// 例如：获取攻击范围内的所有格子，过滤掉不符合条件的格子等

	return TargetTiles;
}

void ULFPSkillBase::OnTurnStart()
{
	if (CurrentCooldown > 0)
	{
		CurrentCooldown--;
	}

	RecoverPriority();
}

// ==== 技能优先级 ====

void ULFPSkillBase::OnSkillUsed()
{
	SkillPriority = FMath::Max(0.0f, SkillPriority - PriorityDecreaseOnUse);
}

void ULFPSkillBase::RecoverPriority()
{
	SkillPriority = FMath::Min(BasePriority, SkillPriority + PriorityRecoveryPerRound);
}

float ULFPSkillBase::EvaluateConditionBonus_Implementation() const
{
	// 默认无条件加成，子类/蓝图可覆盖
	return 0.0f;
}

float ULFPSkillBase::GetEffectivePriority() const
{
	return SkillPriority + EvaluateConditionBonus();
}

void ULFPSkillBase::UpdateSkillRange_Implementation()
{
	RebuildReleaseRangeCoords();
	RebuildEffectRangeCoords();
}

FString ULFPSkillBase::GetReleaseRangeDescription() const
{
	return GetRangeDescription(
		ReleaseRangeType,
		MinRange,
		MaxRange,
		RayRange,
		CustomReleaseRangeData,
		CustomReleaseRangePresetName);
}

FString ULFPSkillBase::GetEffectRangeDescription() const
{
	return GetRangeDescription(
		EffectRangeType,
		EffectMinRange,
		EffectMaxRange,
		EffectRayRange,
		CustomEffectRangeData,
		CustomEffectRangePresetName);
}

bool ULFPSkillBase::ApplyCustomReleaseRangePreset()
{
	return TryApplyCustomReleaseRangePreset();
}

bool ULFPSkillBase::ApplyCustomEffectRangePreset()
{
	return TryApplyCustomEffectRangePreset();
}

TArray<FName> ULFPSkillBase::GetAvailableCustomReleaseRangePresetNames() const
{
	return GetAvailableCustomRangePresetNames(CustomReleaseRangeData);
}

TArray<FName> ULFPSkillBase::GetAvailableCustomEffectRangePresetNames() const
{
	return GetAvailableCustomRangePresetNames(CustomEffectRangeData);
}

TArray<FString> ULFPSkillBase::GetAvailableCustomReleaseRangePresetOptions() const
{
	return GetAvailableCustomRangePresetOptions(CustomReleaseRangeData);
}

TArray<FString> ULFPSkillBase::GetAvailableCustomEffectRangePresetOptions() const
{
	return GetAvailableCustomRangePresetOptions(CustomEffectRangeData);
}

TArray<FLFPHexCoordinates> ULFPSkillBase::GetReleaseRangeInGrid_Implementation()
{
	ReleaseRangeInGridCoords.Empty();
	if (!Owner)
	{
		return ReleaseRangeInGridCoords;
	}

	UpdateSkillRange();

	// Ray 类型 + 阻挡截断：直接在网格上沿 6 方向步进，遇阻挡即停
	if (ReleaseRangeType == ESkillRangeType::Ray && bStopOnBlocker)
	{
		if (ALFPHexGridManager* GM = Owner->GetGridManager())
		{
			const FLFPHexCoordinates OwnerCoord = Owner->GetCurrentCoordinates();
			for (const FLFPHexCoordinates& Dir : ALFPHexGridManager::HexDirections)
			{
				TArray<ALFPHexTile*> RayTiles = GM->WalkRayUntilBlocked(OwnerCoord, Dir, RayRange);
				for (const ALFPHexTile* Tile : RayTiles)
				{
					ReleaseRangeInGridCoords.Add(Tile->GetCoordinates());
				}
			}
			return ReleaseRangeInGridCoords;
		}
		// GridManager 不可用时回退到标准转换
	}

	// 标准转换：相对坐标 + 施法者位置
	for (const FLFPHexCoordinates& Coord : ReleaseRangeCoords)
	{
		const FLFPHexCoordinates OwnerCoord = Owner->GetCurrentCoordinates();
		ReleaseRangeInGridCoords.Add(FLFPHexCoordinates(
			OwnerCoord.Q + Coord.Q,
			OwnerCoord.R + Coord.R));
	}
	return ReleaseRangeInGridCoords;
}

TArray<FLFPHexCoordinates> ULFPSkillBase::GetEffectRangeInGrid_Implementation()
{
    EffectRangeInGridCoords.Empty();
    if (!Owner)
    {
        return EffectRangeInGridCoords;
    }

    RebuildEffectRangeCoords();

    for (FLFPHexCoordinates Coord : EffectRangeCoords)
    {
        FLFPHexCoordinates CoordInGrid = FLFPHexCoordinates();
        FLFPHexCoordinates OwnerCoord = Owner->GetCurrentCoordinates();
        CoordInGrid.Q = OwnerCoord.Q + Coord.Q;
        CoordInGrid.R = OwnerCoord.R + Coord.R;
        CoordInGrid.S = OwnerCoord.S + Coord.S;
        EffectRangeInGridCoords.Add(CoordInGrid);
    }
    return EffectRangeInGridCoords;
}

float ULFPSkillBase::CalculateHatredValue_Implementation(ALFPTacticsUnit* Caster, ALFPTacticsUnit* Target) const
{
	if (!Caster || !Target) return 0.0f;

	// 基础仇恨 = 目标攻击力（威胁越大越优先打）
	float Hatred = (float)Target->GetCurrentAttack();

	// 距离系数（越近仇恨越高）
	int32 Dist = FLFPHexCoordinates::Distance(
		Caster->GetCurrentCoordinates(),
		Target->GetCurrentCoordinates()
	);
	float DistanceFactor = 1.0f / FMath::Max(Dist, 1);

	return Hatred * DistanceFactor;
}

void ULFPSkillBase::RebuildReleaseRangeCoords()
{
	RebuildRangeCoords(
		ReleaseRangeCoords,
		ReleaseRangeType,
		MinRange,
		MaxRange,
		RayRange,
		CustomReleaseRangeData,
		CustomReleaseRangePresetName);
}

void ULFPSkillBase::RebuildEffectRangeCoords()
{
	RebuildRangeCoords(
		EffectRangeCoords,
		EffectRangeType,
		EffectMinRange,
		EffectMaxRange,
		EffectRayRange,
		CustomEffectRangeData,
		CustomEffectRangePresetName);
}

bool ULFPSkillBase::HasReleaseCoord(const FLFPHexCoordinates& RelativeCoord) const
{
	for (const FLFPHexCoordinates& Coord : ReleaseRangeCoords)
	{
		if (Coord == RelativeCoord)
		{
			return true;
		}
	}

	return false;
}

bool ULFPSkillBase::CanReleaseFromInternal(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile, bool bAllowPlannedSelfTarget) const
{
	if (!CasterTile || !TargetTile)
	{
		return false;
	}

	const bool bIsPlannedSelfTarget =
		bAllowPlannedSelfTarget &&
		TargetType == ESkillTargetType::Self &&
		TargetTile == CasterTile;

	if (!bIsPlannedSelfTarget && !IsValidReleaseTargetTile(TargetTile))
	{
		return false;
	}

	const_cast<ULFPSkillBase*>(this)->UpdateSkillRange();
	const FLFPHexCoordinates CasterCoord = CasterTile->GetCoordinates();
	const FLFPHexCoordinates TargetCoord = TargetTile->GetCoordinates();
	const FLFPHexCoordinates RelativeCoord(TargetCoord.Q - CasterCoord.Q, TargetCoord.R - CasterCoord.R);
	if (!HasReleaseCoord(RelativeCoord))
	{
		return false;
	}

	// 视线检查
	if (bRequireLineOfSight && Owner)
	{
		if (ALFPHexGridManager* GridManager = Owner->GetGridManager())
		{
			if (!GridManager->IsLineOfSightClear(CasterTile, TargetTile))
			{
				return false;
			}
		}
	}

	return true;
}

bool ULFPSkillBase::TryApplyCustomReleaseRangePreset()
{
	return TryApplyCustomRangePreset(
		ReleaseRangeCoords,
		ReleaseRangeType,
		CustomReleaseRangeData,
		CustomReleaseRangePresetName);
}

bool ULFPSkillBase::TryApplyCustomEffectRangePreset()
{
	return TryApplyCustomRangePreset(
		EffectRangeCoords,
		EffectRangeType,
		CustomEffectRangeData,
		CustomEffectRangePresetName);
}

void ULFPSkillBase::RebuildRangeCoords(
	TArray<FLFPHexCoordinates>& RangeCoords,
	ESkillRangeType RangeType,
	int32 RangeMin,
	int32 RangeMax,
	int32 RangeRay,
	ULFPSkillRangeDataAsset* CustomRangeData,
	FName CustomRangePresetName)
{
	if (RangeType == ESkillRangeType::Custom)
	{
		TryApplyCustomRangePreset(RangeCoords, RangeType, CustomRangeData, CustomRangePresetName);
		return;
	}

	RangeCoords.Empty();

	switch (RangeType)
	{
	case ESkillRangeType::Origin:
		RangeCoords.Add(FLFPHexCoordinates());
		break;

	case ESkillRangeType::Coverage:
		AppendCoordsInDistanceRange(RangeCoords, RangeMax, 0);
		break;

	case ESkillRangeType::Ring:
		AppendCoordsInDistanceRange(RangeCoords, RangeMax, FMath::Max(0, RangeMin));
		break;

	case ESkillRangeType::Ray:
		if (RangeRay <= 0)
		{
			return;
		}

		for (const FLFPHexCoordinates& Direction : GetSkillRayDirections())
		{
			for (int32 Distance = 1; Distance <= RangeRay; ++Distance)
			{
				RangeCoords.Add(FLFPHexCoordinates(Direction.Q * Distance, Direction.R * Distance));
			}
		}
		break;

	case ESkillRangeType::Custom:
	default:
		break;
	}
}

bool ULFPSkillBase::TryApplyCustomRangePreset(
	TArray<FLFPHexCoordinates>& RangeCoords,
	ESkillRangeType RangeType,
	ULFPSkillRangeDataAsset* CustomRangeData,
	FName CustomRangePresetName)
{
	if (RangeType != ESkillRangeType::Custom ||
		!CustomRangeData ||
		CustomRangePresetName.IsNone())
	{
		return false;
	}

	FLFPSkillRangePreset Preset;
	if (!CustomRangeData->FindPreset(CustomRangePresetName, Preset))
	{
		return false;
	}

	RangeCoords = Preset.RangeCoords;
	return true;
}

FString ULFPSkillBase::GetRangeDescription(
	ESkillRangeType RangeType,
	int32 RangeMin,
	int32 RangeMax,
	int32 RangeRay,
	ULFPSkillRangeDataAsset* CustomRangeData,
	FName CustomRangePresetName) const
{
	switch (RangeType)
	{
	case ESkillRangeType::Origin:
		return TEXT("Origin");

	case ESkillRangeType::Coverage:
		return FString::Printf(TEXT("Coverage: 1-%d"), RangeMax);

	case ESkillRangeType::Ring:
		return FString::Printf(TEXT("Ring: %d-%d"), RangeMin + 1, RangeMax);

	case ESkillRangeType::Ray:
		return FString::Printf(TEXT("Ray: 1-%d"), RangeRay);

	case ESkillRangeType::Custom:
	default:
		if (!CustomRangePresetName.IsNone())
		{
			if (CustomRangeData)
			{
				FLFPSkillRangePreset Preset;
				if (CustomRangeData->FindPreset(CustomRangePresetName, Preset) &&
					!Preset.DisplayName.IsEmpty())
				{
					return FString::Printf(TEXT("Custom: %s"), *Preset.DisplayName.ToString());
				}
			}

			return FString::Printf(TEXT("Custom: %s"), *CustomRangePresetName.ToString());
		}

		return TEXT("Custom");
	}
}

TArray<FName> ULFPSkillBase::GetAvailableCustomRangePresetNames(ULFPSkillRangeDataAsset* CustomRangeData) const
{
	if (!CustomRangeData)
	{
		return TArray<FName>();
	}

	return CustomRangeData->GetPresetNames();
}

TArray<FString> ULFPSkillBase::GetAvailableCustomRangePresetOptions(ULFPSkillRangeDataAsset* CustomRangeData) const
{
	TArray<FString> Options;
	for (const FName PresetName : GetAvailableCustomRangePresetNames(CustomRangeData))
	{
		Options.Add(PresetName.ToString());
	}

	return Options;
}

#if WITH_EDITOR
void ULFPSkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, ReleaseRangeType) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, MinRange) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, MaxRange) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, RayRange) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, CustomReleaseRangeData) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, CustomReleaseRangePresetName) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, EffectRangeType) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, EffectMinRange) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, EffectMaxRange) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, EffectRayRange) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, CustomEffectRangeData) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, CustomEffectRangePresetName))
	{
		UpdateSkillRange();
	}
}
#endif

//void ULFPSkillBase::ShowReleaseRange(bool bShow)
//{
//    if (!ReleaseRangeTiles.IsEmpty())
//    {
//        for (ALFPHexTile* Tile : ReleaseRangeTiles)
//        {
//            if (bShow)
//            {
//                Tile->SetRangeSprite(EUnitRange::UR_Attack);
//            }
//            else
//            {
//                Tile->SetRangeSprite(EUnitRange::UR_Default);
//            }
//        }
//    }
//}
//
//void ULFPSkillBase::ShowEffectRange(bool bShow)
//{
//    if (!ReleaseRangeTiles.IsEmpty())
//    {
//        for (ALFPHexTile* Tile : ReleaseRangeTiles)
//        {
//            if (bShow)
//            {
//                Tile->SetRangeSprite(EUnitRange::UR_SkillEffect);
//            }
//            else
//            {
//                Tile->SetRangeSprite(EUnitRange::UR_Default);
//            }
//        }
//    }
//}
