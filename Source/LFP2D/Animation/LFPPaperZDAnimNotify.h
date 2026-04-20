#pragma once

#include "CoreMinimal.h"
#include "Notifies/PaperZDAnimNotify.h"
#include "LFPPaperZDAnimNotify.generated.h"

UCLASS()
class LFP2D_API ULFPPaperZDAnimNotify_CommitAction : public UPaperZDAnimNotify
{
    GENERATED_BODY()

public:
    virtual void OnReceiveNotify_Implementation(UPaperZDAnimInstance* OwningInstance = nullptr) const override;
    virtual FName GetDisplayName_Implementation() const override;
};

UCLASS()
class LFP2D_API ULFPPaperZDAnimNotify_ActionFinished : public UPaperZDAnimNotify
{
    GENERATED_BODY()

public:
    virtual void OnReceiveNotify_Implementation(UPaperZDAnimInstance* OwningInstance = nullptr) const override;
    virtual FName GetDisplayName_Implementation() const override;
};

UCLASS()
class LFP2D_API ULFPPaperZDAnimNotify_HitFinished : public UPaperZDAnimNotify
{
    GENERATED_BODY()

public:
    virtual void OnReceiveNotify_Implementation(UPaperZDAnimInstance* OwningInstance = nullptr) const override;
    virtual FName GetDisplayName_Implementation() const override;
};
