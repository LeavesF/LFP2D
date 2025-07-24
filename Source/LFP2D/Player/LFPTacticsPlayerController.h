// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFPTacticsPlayerController.generated.h"

class ALFPTacticsUnit;
class ALFPHexGridManager;

/**
 * 
 */
UCLASS()
class LFP2D_API ALFPTacticsPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    void OnTileClicked();
    void SelectUnit(ALFPTacticsUnit* Unit);
    void ConfirmMove();

private:
    UPROPERTY()
    ALFPTacticsUnit* SelectedUnit = nullptr;

    UPROPERTY()
    ALFPHexGridManager* GridManager;

    TArray<ALFPHexTile*> CurrentMoveRange;
};
