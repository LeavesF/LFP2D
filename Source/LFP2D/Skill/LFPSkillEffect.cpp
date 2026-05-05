#include "LFP2D/Skill/LFPSkillEffect.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Buff/LFPBuffDefinitionDataAsset.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

bool ULFPSkillEffect::CanApply_Implementation(const FLFPSkillEffectContext& Context) const
{
    ALFPTacticsUnit* EffectTarget = ResolveTargetUnit(Context);
    return EffectTarget && EffectTarget->IsAlive();
}

void ULFPSkillEffect::Apply_Implementation(const FLFPSkillEffectContext& Context) const
{
}

ALFPTacticsUnit* ULFPSkillEffect::ResolveTargetUnit(const FLFPSkillEffectContext& Context) const
{
    return Target == ELFPSkillEffectTarget::OwnerUnit
        ? Context.OwnerUnit.Get()
        : Context.TargetUnit.Get();
}

ULFPSkillEffect_DealOwnerSkillDamage::ULFPSkillEffect_DealOwnerSkillDamage()
{
    Target = ELFPSkillEffectTarget::TargetUnit;
}

bool ULFPSkillEffect_DealOwnerSkillDamage::CanApply_Implementation(const FLFPSkillEffectContext& Context) const
{
    return Context.Skill && Context.OwnerUnit && ResolveTargetUnit(Context) && ResolveTargetUnit(Context)->IsAlive();
}

void ULFPSkillEffect_DealOwnerSkillDamage::Apply_Implementation(const FLFPSkillEffectContext& Context) const
{
    ULFPSkillBase* Skill = Context.Skill.Get();
    ALFPTacticsUnit* EffectTarget = ResolveTargetUnit(Context);
    if (!Skill || !EffectTarget || !EffectTarget->IsAlive())
    {
        return;
    }

    Skill->DealOwnerSkillDamage(EffectTarget);
}

ULFPSkillEffect_ApplyBuff::ULFPSkillEffect_ApplyBuff()
{
    Target = ELFPSkillEffectTarget::TargetUnit;
}

bool ULFPSkillEffect_ApplyBuff::CanApply_Implementation(const FLFPSkillEffectContext& Context) const
{
    ALFPTacticsUnit* EffectTarget = ResolveTargetUnit(Context);
    return BuffDefinition && Context.OwnerUnit && EffectTarget && EffectTarget->IsAlive() && EffectTarget->GetBuffComponent();
}

void ULFPSkillEffect_ApplyBuff::Apply_Implementation(const FLFPSkillEffectContext& Context) const
{
    ALFPTacticsUnit* EffectTarget = ResolveTargetUnit(Context);
    if (!BuffDefinition || !Context.OwnerUnit || !EffectTarget || !EffectTarget->IsAlive())
    {
        return;
    }

    if (ULFPBuffComponent* BuffComponent = EffectTarget->GetBuffComponent())
    {
        BuffComponent->ApplyBuff(BuffDefinition, Context.OwnerUnit.Get(), StackCount, DurationTurnsOverride);
    }
}

ULFPSkillEffect_DamageByTargetBuffStack::ULFPSkillEffect_DamageByTargetBuffStack()
{
    Target = ELFPSkillEffectTarget::TargetUnit;
}

bool ULFPSkillEffect_DamageByTargetBuffStack::CanApply_Implementation(const FLFPSkillEffectContext& Context) const
{
    ALFPTacticsUnit* EffectTarget = ResolveTargetUnit(Context);
    return Context.OwnerUnit && EffectTarget && EffectTarget->IsAlive() && DamagePerStack > 0 && GetStackCount(Context) > 0;
}

void ULFPSkillEffect_DamageByTargetBuffStack::Apply_Implementation(const FLFPSkillEffectContext& Context) const
{
    ALFPTacticsUnit* EffectTarget = ResolveTargetUnit(Context);
    if (!Context.OwnerUnit || !EffectTarget || !EffectTarget->IsAlive() || DamagePerStack <= 0)
    {
        return;
    }

    const int32 StackCount = GetStackCount(Context);
    if (StackCount <= 0)
    {
        return;
    }

    EffectTarget->TakeTypedDamage(StackCount * DamagePerStack, ResolveDamageType(Context));
}

int32 ULFPSkillEffect_DamageByTargetBuffStack::GetStackCount(const FLFPSkillEffectContext& Context) const
{
    const ALFPTacticsUnit* EffectTarget = ResolveTargetUnit(Context);
    const ULFPBuffComponent* BuffComponent = EffectTarget ? EffectTarget->GetBuffComponent() : nullptr;
    if (!BuffComponent)
    {
        return 0;
    }

    if (RequiredBuffId.IsValid())
    {
        return BuffComponent->GetBuffStack(RequiredBuffId);
    }

    return bFallbackToLegacyBleedStacks ? BuffComponent->GetBleedStacks() : 0;
}

ELFPAttackType ULFPSkillEffect_DamageByTargetBuffStack::ResolveDamageType(const FLFPSkillEffectContext& Context) const
{
    if (DamageTypeSource == ELFPSkillDamageTypeSource::Override)
    {
        return OverrideDamageType;
    }

    return Context.OwnerUnit ? Context.OwnerUnit->GetAttackType() : ELFPAttackType::AT_Physical;
}
