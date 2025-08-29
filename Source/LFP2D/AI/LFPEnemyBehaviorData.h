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
    // �������� (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    float Aggressiveness = 0.7f;

    // �������� (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    float Defensiveness = 0.3f;

    // �Ŷ�Э������ (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    float Teamwork = 0.5f;

    // �Ƿ����ȹ�����Ѫ��Ŀ��
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    bool bPrioritizeWeakTargets = true;

    // �Ƿ����Σ�յ���
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    bool bAvoidDangerousTerrain = true;

    // �Ƿ�Ѱ��ߵ�����
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI Behavior")
    bool bSeekHighGround = true;
};
