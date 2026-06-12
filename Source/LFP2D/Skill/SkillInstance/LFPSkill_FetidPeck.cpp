#include "LFP2D/Skill/SkillInstance/LFPSkill_FetidPeck.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Buff/LFPBuffDefinitionDataAsset.h"
#include "LFP2D/Buff/LFPBuffEffect.h"
#include "LFP2D/Buff/LFPBuffTags.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_FetidPeck::ULFPSkill_FetidPeck()
{
    SkillName = FText::FromString(TEXT("腐臭啄击"));
    SkillDescription = FText::FromString(TEXT("给目标单位添加5层中毒。中毒：每移动一格，受到中毒层数*1的伤害。"));
    ActionPointCost = 1;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::SingleEnemy;
    ReleaseRangeType = ESkillRangeType::Coverage;
    MaxRange = 1;
    EffectRangeType = ESkillRangeType::Origin;
}

bool ULFPSkill_FetidPeck::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
    if (!Super::CanExecute_Implementation(TargetTile) || !Owner || !TargetTile)
    {
        return false;
    }

    ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
    if (!TargetUnit || !TargetUnit->IsAlive() || !IsHostileTarget(TargetUnit))
    {
        return false;
    }

    ALFPHexTile* OwnerTile = Owner->GetCurrentTile();
    return OwnerTile && CanReleaseFrom(OwnerTile, TargetTile);
}

void ULFPSkill_FetidPeck::Execute_Implementation(ALFPHexTile* TargetTile)
{
    if (!CanExecute(TargetTile))
    {
        return;
    }

    if (HasConfiguredEffects())
    {
        ExecuteConfiguredEffects(TargetTile);
        return;
    }

    ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
    if (!TargetUnit || !TargetUnit->IsAlive())
    {
        return;
    }

    ULFPBuffDefinitionDataAsset* BuffDefinition = GetPoisonBuffDefinition();
    ULFPBuffComponent* BuffComponent = TargetUnit->GetBuffComponent();
    if (!BuffDefinition || !BuffComponent)
    {
        return;
    }

    BuffComponent->ApplyBuff(BuffDefinition, Owner, PoisonStacks, PoisonDurationTurns);
}

ULFPBuffDefinitionDataAsset* ULFPSkill_FetidPeck::GetPoisonBuffDefinition()
{
    if (PoisonBuffDefinition)
    {
        return PoisonBuffDefinition;
    }

    if (!RuntimePoisonBuffDefinition)
    {
        RuntimePoisonBuffDefinition = NewObject<ULFPBuffDefinitionDataAsset>(this, TEXT("RuntimePoisonBuffDefinition"));
        RuntimePoisonBuffDefinition->BuffId = LFPBuffTags::RequestBuffTag(LFPBuffTags::PoisonBuffIdName);
        RuntimePoisonBuffDefinition->DisplayName = FText::FromString(TEXT("中毒"));
        RuntimePoisonBuffDefinition->Description = FText::FromString(TEXT("每移动一格，受到中毒层数*1的伤害。"));
        RuntimePoisonBuffDefinition->Category = ELFPBuffCategory::Debuff;
        RuntimePoisonBuffDefinition->DurationPolicy.DurationType = ELFPBuffDurationType::TimedTurns;
        RuntimePoisonBuffDefinition->DurationPolicy.DurationTurns = PoisonDurationTurns;
        RuntimePoisonBuffDefinition->StackingPolicy.StackingMode = ELFPBuffStackingMode::AddStackAndRefreshDuration;
        RuntimePoisonBuffDefinition->StackingPolicy.MaxStacks = 99;

        ULFPBuffEffect_PeriodicDamage* MoveDamageEffect =
            NewObject<ULFPBuffEffect_PeriodicDamage>(RuntimePoisonBuffDefinition, TEXT("PoisonMoveDamageEffect"));
        MoveDamageEffect->TriggerEvent = ELFPBuffTriggerEvent::OnMovedOneTile;
        MoveDamageEffect->Damage = PoisonDamagePerMovedTile;
        MoveDamageEffect->bScaleByStack = true;
        RuntimePoisonBuffDefinition->Effects.Add(MoveDamageEffect);
    }

    return RuntimePoisonBuffDefinition;
}
