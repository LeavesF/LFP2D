#include "LFP2D/Skill/SkillInstance/LFPSkill_WoodRootEntangle.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Buff/LFPBuffDefinitionDataAsset.h"
#include "LFP2D/Buff/LFPBuffTags.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "Kismet/GameplayStatics.h"

ULFPSkill_WoodRootEntangle::ULFPSkill_WoodRootEntangle()
{
	SkillName = FText::FromString(TEXT("木根缠绕"));
	SkillDescription = FText::FromString(TEXT("缠绕自身两格内的所有敌方单位，使其下回合无法移动。"));
	ActionPointCost = 1;
	CooldownRounds = 0;
	TargetType = ESkillTargetType::Self;
	ReleaseRangeType = ESkillRangeType::Origin;
	EffectRangeType = ESkillRangeType::Coverage;
	MaxRange = 0;
	EffectMaxRange = EntangleRange;
	bRequireLineOfSight = false;
}

bool ULFPSkill_WoodRootEntangle::CanPlanFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile)
{
	if (!Super::CanPlanFrom_Implementation(CasterTile, TargetTile))
	{
		return false;
	}

	return HasHostileUnitInRange(CasterTile);
}

bool ULFPSkill_WoodRootEntangle::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
	if (!Owner)
	{
		return false;
	}

	ALFPHexTile* OriginTile = Owner->GetCurrentTile();
	if (!OriginTile || !Super::CanExecute_Implementation(OriginTile))
	{
		return false;
	}

	return HasHostileUnitInRange(OriginTile);
}

void ULFPSkill_WoodRootEntangle::Execute_Implementation(ALFPHexTile* TargetTile)
{
	if (!CanExecute(TargetTile) || !Owner)
	{
		return;
	}

	ApplyRootedToHostileUnitsInRange(Owner->GetCurrentTile());
}

void ULFPSkill_WoodRootEntangle::UpdateSkillRange_Implementation()
{
	EffectMaxRange = EntangleRange;
	Super::UpdateSkillRange_Implementation();
}

ULFPBuffDefinitionDataAsset* ULFPSkill_WoodRootEntangle::GetRootedBuffDefinition()
{
	if (RootedBuffDefinition)
	{
		return RootedBuffDefinition;
	}

	if (!RuntimeRootedBuffDefinition)
	{
		RuntimeRootedBuffDefinition = NewObject<ULFPBuffDefinitionDataAsset>(this, TEXT("RuntimeRootedBuffDefinition"));
		RuntimeRootedBuffDefinition->BuffId = LFPBuffTags::RequestBuffTag(LFPBuffTags::RootedBuffIdName);
		RuntimeRootedBuffDefinition->DisplayName = FText::FromString(TEXT("缠绕"));
		RuntimeRootedBuffDefinition->Description = FText::FromString(TEXT("下回合无法移动。"));
		RuntimeRootedBuffDefinition->Category = ELFPBuffCategory::Debuff;
		RuntimeRootedBuffDefinition->DurationPolicy.DurationType = ELFPBuffDurationType::TimedTurns;
		RuntimeRootedBuffDefinition->DurationPolicy.DurationTurns = RootedDurationTurns;
		RuntimeRootedBuffDefinition->StackingPolicy.StackingMode = ELFPBuffStackingMode::RefreshDuration;
		RuntimeRootedBuffDefinition->StackingPolicy.MaxStacks = 1;
	}

	return RuntimeRootedBuffDefinition;
}

bool ULFPSkill_WoodRootEntangle::HasHostileUnitInRange(ALFPHexTile* OriginTile) const
{
	if (!Owner || !OriginTile)
	{
		return false;
	}

	TArray<AActor*> FoundUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundUnits);

	const FLFPHexCoordinates OriginCoordinates = OriginTile->GetCoordinates();
	for (AActor* Actor : FoundUnits)
	{
		ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
		if (!Unit || !Unit->IsAlive() || !IsHostileTarget(Unit))
		{
			continue;
		}

		const int32 Distance = FLFPHexCoordinates::Distance(OriginCoordinates, Unit->GetCurrentCoordinates());
		if (Distance <= EntangleRange)
		{
			return true;
		}
	}

	return false;
}

int32 ULFPSkill_WoodRootEntangle::ApplyRootedToHostileUnitsInRange(ALFPHexTile* OriginTile)
{
	if (!Owner || !OriginTile)
	{
		return 0;
	}

	ULFPBuffDefinitionDataAsset* BuffDefinition = GetRootedBuffDefinition();
	if (!BuffDefinition)
	{
		return 0;
	}

	TArray<AActor*> FoundUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundUnits);

	int32 AppliedCount = 0;
	const FLFPHexCoordinates OriginCoordinates = OriginTile->GetCoordinates();
	for (AActor* Actor : FoundUnits)
	{
		ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
		if (!Unit || !Unit->IsAlive() || !IsHostileTarget(Unit))
		{
			continue;
		}

		const int32 Distance = FLFPHexCoordinates::Distance(OriginCoordinates, Unit->GetCurrentCoordinates());
		if (Distance > EntangleRange)
		{
			continue;
		}

		if (ULFPBuffComponent* BuffComponent = Unit->GetBuffComponent())
		{
			BuffComponent->ApplyBuff(BuffDefinition, Owner, 1, RootedDurationTurns);
			AppliedCount++;
		}
	}

	return AppliedCount;
}
