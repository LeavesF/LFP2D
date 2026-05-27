#include "LFP2D/Skill/SkillInstance/LFPSkill_SelfApplyBuff.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_SelfApplyBuff::ULFPSkill_SelfApplyBuff()
{
	TargetType = ESkillTargetType::Self;
	ReleaseRangeType = ESkillRangeType::Origin;
	EffectRangeType = ESkillRangeType::Origin;
	MaxRange = 0;
	CooldownRounds = 0;
	ActionPointCost = 0;
}

bool ULFPSkill_SelfApplyBuff::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
	if (!Owner)
	{
		return false;
	}

	ALFPHexTile* SelfTile = Owner->GetCurrentTile();
	if (!SelfTile)
	{
		return false;
	}

	return Super::CanExecute_Implementation(SelfTile);
}

void ULFPSkill_SelfApplyBuff::Execute_Implementation(ALFPHexTile* TargetTile)
{
	if (!CanExecute(TargetTile) || !Owner)
	{
		return;
	}

	ULFPBuffComponent* BuffComponent = Owner->GetBuffComponent();
	if (!BuffComponent)
	{
		return;
	}

	if (BuffDefinition)
	{
		BuffComponent->ApplyBuff(BuffDefinition, Owner);
		Owner->RebuildCurrentStatsFromRuntimeSources();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelfApplyBuff] %s: BuffDefinition 未配置。"), *SkillName.ToString());
	}
}
