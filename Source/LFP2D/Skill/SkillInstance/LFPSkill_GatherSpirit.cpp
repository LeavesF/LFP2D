#include "LFP2D/Skill/SkillInstance/LFPSkill_GatherSpirit.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ULFPSkill_GatherSpirit::ULFPSkill_GatherSpirit()
{
    SkillName = FText::FromString(TEXT("聚灵"));
    SkillDescription = FText::FromString(TEXT("立即为当前阵营回复1点AP。"));
    ActionPointCost = 0;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::Self;
    ReleaseRangeType = ESkillRangeType::Origin;
    EffectRangeType = ESkillRangeType::Origin;
    MaxRange = 0;
    EffectMaxRange = 0;
    bRequireLineOfSight = false;
}

bool ULFPSkill_GatherSpirit::CanExecute_Implementation(ALFPHexTile* TargetTile)
{
    if (!Owner)
    {
        return false;
    }

    ALFPTurnManager* TurnManager = Owner->GetTurnManager();
    if (!TurnManager)
    {
        return false;
    }

    ALFPHexTile* SelfTile = Owner->GetCurrentTile();
    if (!SelfTile)
    {
        return false;
    }

    if (!Super::CanExecute_Implementation(SelfTile))
    {
        return false;
    }

    return TurnManager->GetFactionAP(Owner->GetAffiliation()) < TurnManager->GetFactionMaxAP();
}

void ULFPSkill_GatherSpirit::Execute_Implementation(ALFPHexTile* TargetTile)
{
    if (!CanExecute(TargetTile) || !Owner)
    {
        return;
    }

    if (ALFPTurnManager* TurnManager = Owner->GetTurnManager())
    {
        TurnManager->RestoreFactionAP(Owner->GetAffiliation(), RestoreAmount);
    }
}
