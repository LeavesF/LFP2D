// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LFPTurnGameMode.generated.h"

/**
 * 
 */
UCLASS()
class LFP2D_API ALFPTurnGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void StartPlay() override;
};
