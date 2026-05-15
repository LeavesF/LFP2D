#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPCardDataAsset.generated.h"

UCLASS(BlueprintType)
class LFP2D_API ULFPCardDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FName CardID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	TSubclassOf<ULFPSkillBase> SkillClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card", meta = (ClampMin = "0"))
	int32 ActionPointCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ELFPCardCategory CardCategory = ELFPCardCategory::FullyGeneric;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FGameplayTag RequiredTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	ELFPCardPile DestinationAfterPlay = ELFPCardPile::DiscardPile;

	UFUNCTION(BlueprintPure, Category = "Card")
	bool IsValidCardData() const { return SkillClass != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Card")
	FLFPCardDefinition BuildCardDefinition() const;
};
