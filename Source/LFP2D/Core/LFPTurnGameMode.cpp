// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Core/LFPTurnGameMode.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

void ALFPTurnGameMode::StartPlay()
{
    // 从 GameInstance 读取战斗请求
    if (ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance()))
    {
        CachedBattleRequest = GI->ConsumeBattleRequest();
        if (CachedBattleRequest.bIsValid)
        {
            UE_LOG(LogTemp, Log, TEXT("战斗模式: 由世界地图触发，节点 %d, 地图 %s, 星级 %d"),
                CachedBattleRequest.SourceNodeID,
                *CachedBattleRequest.BattleMapName,
                CachedBattleRequest.StarRating);
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;

    // 生成网格管理器
    if (GridManagerClass)
    {
        GridManager = GetWorld()->SpawnActor<ALFPHexGridManager>(
            GridManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }
    else
    {
        GridManager = GetWorld()->SpawnActor<ALFPHexGridManager>(
            ALFPHexGridManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    }

    // 如果有 BattleMapName，从 CSV 加载地图
    if (GridManager && !CachedBattleRequest.BattleMapName.IsEmpty())
    {
        FString CSVPath = FPaths::ProjectSavedDir() / TEXT("Maps") / CachedBattleRequest.BattleMapName + TEXT(".csv");
        if (GridManager->LoadMapFromCSV(CSVPath))
        {
            UE_LOG(LogTemp, Log, TEXT("战斗模式: 从 CSV 加载地图 %s"), *CachedBattleRequest.BattleMapName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("战斗模式: CSV 地图加载失败 %s"), *CSVPath);
        }
    }

    // 生成回合管理器
    TurnManager = GetWorld()->SpawnActor<ALFPTurnManager>(
        ALFPTurnManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

    if (TurnManager)
    {
        TurnManager->StartGame();
    }

    Super::StartPlay();
}

void ALFPTurnGameMode::EndBattle(bool bVictory, bool bEscaped)
{
    ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
    if (!GI) return;

    // 写回战斗结果
    FLFPBattleResult Result;
    Result.SourceNodeID = CachedBattleRequest.SourceNodeID;
    Result.bVictory = bVictory;
    Result.bEscaped = bEscaped;

    // 胜利时传递捕获的单位
    if (bVictory)
    {
        Result.CapturedUnits = CapturedUnits;
    }

    GI->SetBattleResult(Result);

    // 返回世界地图
    if (CachedBattleRequest.bIsValid)
    {
        GI->TransitionToWorldMap(TEXT(""));
    }
}

void ALFPTurnGameMode::RecordCapturedUnit(ALFPTacticsUnit* Unit)
{
    if (!Unit) return;

    FLFPUnitEntry Entry = Unit->ToUnitEntry();
    if (Entry.IsValid())
    {
        CapturedUnits.Add(Entry);
        UE_LOG(LogTemp, Log, TEXT("捕获单位: %s, 阶级 %d"), *Entry.TypeID.ToString(), Entry.Tier);
    }
}
