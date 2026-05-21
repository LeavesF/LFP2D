// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "LFPEnemyBehaviorData.generated.h"

/**
 * Enemy AI behavior tuning data.
 */
UCLASS(Blueprintable)
class LFP2D_API ULFPEnemyBehaviorData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// Target tag hatred modifiers. A matching target tag adds this value before weighting.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior|Hatred")
	TMap<FGameplayTag, float> TargetTagHatredModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior|Hatred")
	float TagHatredWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior|Hatred")
	float DistanceHatredWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior|Hatred")
	float ThreatHatredWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior|Hatred")
	float ExistingTargetLockHatredWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior|Hatred")
	float ExistingTargetLockHatredPenalty = 50.0f;

	// Aggressiveness (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
	float Aggressiveness = 0.7f;

	// Defensiveness (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
	float Defensiveness = 0.3f;

	// Teamwork tendency (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
	float Teamwork = 0.5f;

	// Whether to prioritize low-health targets.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
	bool bPrioritizeWeakTargets = true;

	// Whether to avoid dangerous terrain.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
	bool bAvoidDangerousTerrain = true;

	// Whether to seek high ground.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
	bool bSeekHighGround = true;
};
