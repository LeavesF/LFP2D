#include "LFP2D/Animation/LFPUnitAnimInstance.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"

ALFPTacticsUnit* ULFPUnitAnimInstance::GetOwningUnit() const
{
    return Cast<ALFPTacticsUnit>(GetOwningActor()); // AnimBP 只读单位状态，不直接处理战斗逻辑。
}

ELFPUnitAnimState ULFPUnitAnimInstance::GetUnitAnimState() const
{
    if (const ALFPTacticsUnit* Unit = GetOwningUnit())
    {
        return Unit->GetCurrentAnimState();
    }

    return ELFPUnitAnimState::Idle;
}

FName ULFPUnitAnimInstance::GetCurrentAnimationKey() const
{
    if (const ALFPTacticsUnit* Unit = GetOwningUnit())
    {
        return Unit->GetCurrentAnimationKey();
    }

    return NAME_None;
}

bool ULFPUnitAnimInstance::IsActionAnimationLocked() const
{
    if (const ALFPTacticsUnit* Unit = GetOwningUnit())
    {
        return Unit->IsActionAnimationLocked();
    }

    return false;
}

bool ULFPUnitAnimInstance::HasPendingAction() const
{
    if (const ALFPTacticsUnit* Unit = GetOwningUnit())
    {
        return Unit->HasPendingSkillAction();
    }

    return false;
}

bool ULFPUnitAnimInstance::IsHitReactionActive() const
{
    if (const ALFPTacticsUnit* Unit = GetOwningUnit())
    {
        return Unit->IsHitReactionActive();
    }

    return false;
}
