#include "LFP2D/Skill/SkillInstance/LFPSkill_PlagueSpread.h"
#include "LFP2D/Buff/LFPBuffComponent.h"
#include "LFP2D/Buff/LFPBuffDefinitionDataAsset.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "Kismet/GameplayStatics.h"

ULFPSkill_PlagueSpread::ULFPSkill_PlagueSpread()
{
    SkillName = FText::FromString(TEXT("瘟疫传播"));
    SkillDescription = FText::FromString(TEXT("目标3格内敌人获得该敌人的Debuff。"));
    ActionPointCost = 1;
    CooldownRounds = 0;
    TargetType = ESkillTargetType::SingleEnemy;
    ReleaseRangeType = ESkillRangeType::Coverage;
    MaxRange = 3;
    EffectRangeType = ESkillRangeType::Coverage;
    EffectMaxRange = SpreadRange;
    bRequireLineOfSight = false;
}

bool ULFPSkill_PlagueSpread::CanPlanFrom_Implementation(ALFPHexTile* CasterTile, ALFPHexTile* TargetTile)
{
    if (!Super::CanPlanFrom_Implementation(CasterTile, TargetTile))
    {
        return false;
    }

    ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
    return TargetUnit &&
        TargetUnit->IsAlive() &&
        IsHostileTarget(TargetUnit) &&
        HasSpreadableDebuffs(TargetUnit) &&
        HasSpreadTargetInRange(TargetUnit);
}

bool ULFPSkill_PlagueSpread::CanExecute_Implementation(ALFPHexTile* TargetTile)
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
    if (!OwnerTile || !CanReleaseFrom(OwnerTile, TargetTile))
    {
        return false;
    }

    return HasSpreadableDebuffs(TargetUnit) && HasSpreadTargetInRange(TargetUnit);
}

void ULFPSkill_PlagueSpread::Execute_Implementation(ALFPHexTile* TargetTile)
{
    if (!CanExecute(TargetTile))
    {
        return;
    }

    ALFPTacticsUnit* TargetUnit = GetUnitOnTile(TargetTile);
    if (!TargetUnit)
    {
        return;
    }

    SpreadDebuffsFromTarget(TargetUnit);
}

bool ULFPSkill_PlagueSpread::HasSpreadableDebuffs(ALFPTacticsUnit* SourceUnit) const
{
    const ULFPBuffComponent* SourceBuffComponent = SourceUnit ? SourceUnit->GetBuffComponent() : nullptr;
    if (!SourceBuffComponent)
    {
        return false;
    }

    for (const FLFPBuffInstance& BuffInstance : SourceBuffComponent->GetBuffInstances())
    {
        if (BuffInstance.IsActive() &&
            BuffInstance.Definition &&
            BuffInstance.Definition->Category == ELFPBuffCategory::Debuff)
        {
            return true;
        }
    }

    return false;
}

bool ULFPSkill_PlagueSpread::HasSpreadTargetInRange(ALFPTacticsUnit* SourceUnit) const
{
    if (!Owner || !SourceUnit || !SourceUnit->IsAlive())
    {
        return false;
    }

    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundUnits);

    const FLFPHexCoordinates SourceCoordinates = SourceUnit->GetCurrentCoordinates();
    for (AActor* Actor : FoundUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (!Unit || Unit == SourceUnit || !Unit->IsAlive() || !IsHostileTarget(Unit))
        {
            continue;
        }

        const int32 Distance = FLFPHexCoordinates::Distance(SourceCoordinates, Unit->GetCurrentCoordinates());
        if (Distance <= SpreadRange && Unit->GetBuffComponent())
        {
            return true;
        }
    }

    return false;
}

int32 ULFPSkill_PlagueSpread::SpreadDebuffsFromTarget(ALFPTacticsUnit* SourceUnit) const
{
    const ULFPBuffComponent* SourceBuffComponent = SourceUnit ? SourceUnit->GetBuffComponent() : nullptr;
    if (!Owner || !SourceUnit || !SourceUnit->IsAlive() || !SourceBuffComponent)
    {
        return 0;
    }

    TArray<AActor*> FoundUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundUnits);

    int32 AppliedCount = 0;
    const FLFPHexCoordinates SourceCoordinates = SourceUnit->GetCurrentCoordinates();
    for (AActor* Actor : FoundUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (!Unit || Unit == SourceUnit || !Unit->IsAlive() || !IsHostileTarget(Unit))
        {
            continue;
        }

        const int32 Distance = FLFPHexCoordinates::Distance(SourceCoordinates, Unit->GetCurrentCoordinates());
        if (Distance > SpreadRange)
        {
            continue;
        }

        AppliedCount += CopyDebuffsToTarget(SourceBuffComponent, Unit);
    }

    return AppliedCount;
}

int32 ULFPSkill_PlagueSpread::CopyDebuffsToTarget(const ULFPBuffComponent* SourceBuffComponent, ALFPTacticsUnit* RecipientUnit) const
{
    ULFPBuffComponent* RecipientBuffComponent = RecipientUnit ? RecipientUnit->GetBuffComponent() : nullptr;
    if (!SourceBuffComponent || !RecipientUnit || !RecipientUnit->IsAlive() || !RecipientBuffComponent)
    {
        return 0;
    }

    int32 AppliedCount = 0;
    for (const FLFPBuffInstance& BuffInstance : SourceBuffComponent->GetBuffInstances())
    {
        if (!BuffInstance.IsActive() ||
            !BuffInstance.Definition ||
            BuffInstance.Definition->Category != ELFPBuffCategory::Debuff)
        {
            continue;
        }

        const int32 DurationTurnsOverride = BuffInstance.HasTimedDuration()
            ? FMath::Max(BuffInstance.RemainingTurns, 1)
            : -1;
        ALFPTacticsUnit* SourceUnit = BuffInstance.SourceUnit ? BuffInstance.SourceUnit.Get() : Owner;
        RecipientBuffComponent->ApplyBuff(
            BuffInstance.Definition,
            SourceUnit,
            FMath::Max(BuffInstance.StackCount, 1),
            DurationTurnsOverride);
        AppliedCount++;
    }

    return AppliedCount;
}
