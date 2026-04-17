#pragma once

#include "CoreMinimal.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkill_ChargedStrike.generated.h"

UCLASS(Blueprintable)
class LFP2D_API ULFPSkill_ChargedStrike : public ULFPSkillBase
{
    GENERATED_BODY()

public:
    ULFPSkill_ChargedStrike();

    virtual bool CanExecute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void Execute_Implementation(ALFPHexTile* TargetTile = nullptr) override;
    virtual void OnTurnStart() override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    float DamageScale = 3.0f;

    UPROPERTY(Transient)
    bool bHasPendingStrike = false;

    UPROPERTY(Transient)
    TObjectPtr<ALFPHexTile> PendingTargetTile = nullptr;
};
