// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Core/LFPTurnGameMode.h"
#include "LFP2D/Turn/LFPTurnManager.h"

void ALFPTurnGameMode::StartPlay()
{
    // 生成回合管理器
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;

    ALFPTurnManager* TurnManager = GetWorld()->SpawnActor<ALFPTurnManager>(
        ALFPTurnManager::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (TurnManager)
    {
        TurnManager->StartGame();
    }

    Super::StartPlay();
}
