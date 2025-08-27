// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LFPEnemyBehaviorData.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class LFP2D_API ULFPEnemyBehaviorData : public UDataAsset
{
	GENERATED_BODY()
	
public:
    // 攻击倾向 (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    float Aggressiveness = 0.7f;

    // 防御倾向 (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    float Defensiveness = 0.3f;

    // 团队协作倾向 (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    float Teamwork = 0.5f;

    // 是否优先攻击低血量目标
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    bool bPrioritizeWeakTargets = true;

    // 是否避免危险地形
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    bool bAvoidDangerousTerrain = true;

    // 是否寻求高地优势
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    bool bSeekHighGround = true;
};
