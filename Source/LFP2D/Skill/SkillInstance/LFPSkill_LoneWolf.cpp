#include "LFP2D/Skill/SkillInstance/LFPSkill_LoneWolf.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_LoneWolf::ULFPSkill_LoneWolf()
{
    SkillName = FText::FromString(TEXT("孤狼"));
    SkillDescription = FText::FromString(TEXT("2格内没有友军时，攻击力+10，防御力+5，速度+3"));
    ActionPointCost = 0;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::Self;
    ReleaseRangeType = ESkillRangeType::Origin;
    EffectRangeType = ESkillRangeType::Origin;
    MaxRange = 0;
}

bool ULFPSkill_LoneWolf::CanExecute_Implementation(ALFPHexTile* TargetTile)
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

void ULFPSkill_LoneWolf::Execute_Implementation(ALFPHexTile* TargetTile)
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

    if (PassiveBuffDefinition)
    {
        BuffComponent->ApplyBuff(PassiveBuffDefinition, Owner);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[LoneWolf] 未配置 PassiveBuffDefinition，已跳过孤狼 Buff。"));
    }
}
