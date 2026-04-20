#include "LFP2D/Animation/LFPPaperZDAnimNotify.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "PaperZDAnimInstance.h"

namespace
{
ALFPTacticsUnit* ResolveOwningUnit(UPaperZDAnimInstance* OwningInstance)
{
    return OwningInstance ? Cast<ALFPTacticsUnit>(OwningInstance->GetOwningActor()) : nullptr; // 所有核心 notify 都先解析回拥有者单位，再交给单位统一调度。
}
}

void ULFPPaperZDAnimNotify_CommitAction::OnReceiveNotify_Implementation(UPaperZDAnimInstance* OwningInstance) const
{
    if (ALFPTacticsUnit* Unit = ResolveOwningUnit(OwningInstance))
    {
        Unit->OnAnimCommitAction();
    }
}

FName ULFPPaperZDAnimNotify_CommitAction::GetDisplayName_Implementation() const
{
    return TEXT("CommitAction");
}

void ULFPPaperZDAnimNotify_ActionFinished::OnReceiveNotify_Implementation(UPaperZDAnimInstance* OwningInstance) const
{
    if (ALFPTacticsUnit* Unit = ResolveOwningUnit(OwningInstance))
    {
        Unit->OnAnimActionFinished();
    }
}

FName ULFPPaperZDAnimNotify_ActionFinished::GetDisplayName_Implementation() const
{
    return TEXT("ActionFinished");
}

void ULFPPaperZDAnimNotify_HitFinished::OnReceiveNotify_Implementation(UPaperZDAnimInstance* OwningInstance) const
{
    if (ALFPTacticsUnit* Unit = ResolveOwningUnit(OwningInstance))
    {
        Unit->OnAnimHitFinished();
    }
}

FName ULFPPaperZDAnimNotify_HitFinished::GetDisplayName_Implementation() const
{
    return TEXT("HitFinished");
}
