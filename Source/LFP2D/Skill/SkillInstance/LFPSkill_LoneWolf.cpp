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
    bIsPassiveSkill = true;
    bShowDisabledInSkillBar = true;
}

void ULFPSkill_LoneWolf::RegisterPassiveBuffs_Implementation(ALFPTacticsUnit* InOwner)
{
    if (!InOwner)
    {
        return;
    }

    ULFPBuffComponent* BuffComponent = InOwner->GetBuffComponent();
    if (!BuffComponent)
    {
        return;
    }

    FLFPPersistentBuffDefinition BuffDefinition;
    BuffDefinition.BuffType = ELFPBuffType::BT_StatModifier;
    BuffDefinition.ConditionType = ELFPBuffConditionType::BCT_NoFriendlyWithinRange;
    BuffDefinition.ConditionRange = FriendlyCheckRange;
    // 这个被动本身不处理逻辑，只是把条件与数值加成注册给 Buff 运行时。
    BuffDefinition.StatModifier.AttackDelta = AttackBonus;
    BuffDefinition.StatModifier.PhysicalBlockDelta = PhysicalBlockBonus;
    BuffDefinition.StatModifier.SpeedDelta = SpeedBonus;
    BuffDefinition.SourceSkillName = TEXT("LoneWolf");
    BuffDefinition.bVisibleInUI = true;
    BuffComponent->RegisterPersistentBuff(BuffDefinition);
}
