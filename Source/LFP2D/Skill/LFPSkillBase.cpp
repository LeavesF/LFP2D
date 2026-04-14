// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillReleaseRangeDataAsset.h"
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

	const int32 EffectiveMinDistance = FMath::Max(1, MinExclusiveDistance + 1);

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

int32 ULFPSkillBase::DealOwnerRepeatedDamage(ALFPTacticsUnit* Target, int32 HitCount, float DamageScalePerHit) const
{
	if (!Owner || !Target || HitCount <= 0)
	{
		return 0;
	}

	const int32 RawDamagePerHit = FMath::Max(0, FMath::RoundToInt(Owner->GetCurrentAttack() * DamageScalePerHit));
	return Owner->ApplyRepeatedHitDamage(Target, HitCount, RawDamagePerHit, Owner->GetAttackType());
}

bool ULFPSkillBase::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
	if (!Owner) return false;

	// 检查冷却
	//if (CurrentCooldown > 0) return false;

	// 检查行动力
	if (!Owner->HasEnoughActionPoints(ActionPointCost)) return false;

	return true;
}

bool ULFPSkillBase::IsAvailable() const
{
	if (!Owner) return false;

	// 检查冷却
	if (CurrentCooldown > 0) return false;

	// 检查行动点
	if (!Owner->HasEnoughActionPoints(ActionPointCost)) return false;

	return true;
}

bool ULFPSkillBase::CanReleaseFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile)
{
	if (!CasterTile || !TargetTile) return false;

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

	// TODO: 视线检查 (bRequireLineOfSight)

	return true;
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
}

FString ULFPSkillBase::GetReleaseRangeDescription() const
{
	switch (ReleaseRangeType)
	{
	case ESkillReleaseRangeType::Coverage:
		return FString::Printf(TEXT("Coverage: 1-%d"), MaxRange);

	case ESkillReleaseRangeType::Ring:
		return FString::Printf(TEXT("Ring: %d-%d"), MinRange + 1, MaxRange);

	case ESkillReleaseRangeType::Ray:
		return FString::Printf(TEXT("Ray: 1-%d"), RayRange);

	case ESkillReleaseRangeType::Custom:
	default:
		if (!CustomReleaseRangePresetName.IsNone())
		{
			if (CustomReleaseRangeData)
			{
				FLFPSkillReleaseRangePreset Preset;
				if (CustomReleaseRangeData->FindPreset(CustomReleaseRangePresetName, Preset) &&
					!Preset.DisplayName.IsEmpty())
				{
					return FString::Printf(TEXT("Custom: %s"), *Preset.DisplayName.ToString());
				}
			}

			return FString::Printf(TEXT("Custom: %s"), *CustomReleaseRangePresetName.ToString());
		}

		return TEXT("Custom");
	}
}

bool ULFPSkillBase::ApplyCustomReleaseRangePreset()
{
	if (ReleaseRangeType != ESkillReleaseRangeType::Custom)
	{
		return false;
	}

	return TryApplyCustomReleaseRangePreset();
}

TArray<FName> ULFPSkillBase::GetAvailableCustomReleaseRangePresetNames() const
{
	if (!CustomReleaseRangeData)
	{
		return TArray<FName>();
	}

	return CustomReleaseRangeData->GetPresetNames();
}

TArray<FString> ULFPSkillBase::GetAvailableCustomReleaseRangePresetOptions() const
{
	TArray<FString> Options;
	for (const FName PresetName : GetAvailableCustomReleaseRangePresetNames())
	{
		Options.Add(PresetName.ToString());
	}

	return Options;
}

TArray<FLFPHexCoordinates> ULFPSkillBase::GetReleaseRangeInGrid_Implementation()
{
	ReleaseRangeInGridCoords.Empty();
	if (!Owner)
	{
		return ReleaseRangeInGridCoords;
	}

	UpdateSkillRange();

	for (FLFPHexCoordinates Coord : ReleaseRangeCoords)
	{
		FLFPHexCoordinates CoordInGrid = FLFPHexCoordinates();
		FLFPHexCoordinates OwnerCoord = Owner->GetCurrentCoordinates();
		CoordInGrid.Q = OwnerCoord.Q + Coord.Q;
		CoordInGrid.R = OwnerCoord.R + Coord.R;
		CoordInGrid.S = OwnerCoord.S + Coord.S;
		ReleaseRangeInGridCoords.Add(CoordInGrid);
	}
	return ReleaseRangeInGridCoords;
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
	if (ReleaseRangeType == ESkillReleaseRangeType::Custom)
	{
		TryApplyCustomReleaseRangePreset();
		return;
	}

	ReleaseRangeCoords.Empty();

	switch (ReleaseRangeType)
	{
	case ESkillReleaseRangeType::Coverage:
		AppendCoordsInDistanceRange(ReleaseRangeCoords, MaxRange, 0);
		break;

	case ESkillReleaseRangeType::Ring:
		AppendCoordsInDistanceRange(ReleaseRangeCoords, MaxRange, FMath::Max(0, MinRange));
		break;

	case ESkillReleaseRangeType::Ray:
		if (RayRange <= 0)
		{
			return;
		}

		for (const FLFPHexCoordinates& Direction : GetSkillRayDirections())
		{
			for (int32 Distance = 1; Distance <= RayRange; ++Distance)
			{
				ReleaseRangeCoords.Add(FLFPHexCoordinates(Direction.Q * Distance, Direction.R * Distance));
			}
		}
		break;

	case ESkillReleaseRangeType::Custom:
	default:
		break;
	}
}

bool ULFPSkillBase::HasReleaseCoord(const FLFPHexCoordinates& RelativeCoord) const
{
	for (const FLFPHexCoordinates& Coord : ReleaseRangeCoords)
	{
		if (Coord.Q == RelativeCoord.Q && Coord.R == RelativeCoord.R && Coord.S == RelativeCoord.S)
		{
			return true;
		}
	}

	return false;
}

bool ULFPSkillBase::TryApplyCustomReleaseRangePreset()
{
	if (ReleaseRangeType != ESkillReleaseRangeType::Custom ||
		!CustomReleaseRangeData ||
		CustomReleaseRangePresetName.IsNone())
	{
		return false;
	}

	FLFPSkillReleaseRangePreset Preset;
	if (!CustomReleaseRangeData->FindPreset(CustomReleaseRangePresetName, Preset))
	{
		return false;
	}

	ReleaseRangeCoords = Preset.ReleaseRangeCoords;
	return true;
}

#if WITH_EDITOR
void ULFPSkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, ReleaseRangeType) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, CustomReleaseRangeData) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ULFPSkillBase, CustomReleaseRangePresetName))
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
