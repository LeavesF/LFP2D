// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Core/LFPTurnGameMode.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Turn/LFPBattleRelicRuntimeManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/UI/Fighting/LFPBattleResultWidget.h"
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

	// 生成战斗遗物运行时管理器
	BattleRelicRuntimeManager = GetWorld()->SpawnActor<ALFPBattleRelicRuntimeManager>(
		ALFPBattleRelicRuntimeManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (TurnManager)
	{
		TurnManager->StartGame();
	}

	if (BattleRelicRuntimeManager)
	{
		BattleRelicRuntimeManager->Initialize(Cast<ULFPGameInstance>(GetGameInstance()), TurnManager);
	}

	Super::StartPlay();
}

void ALFPTurnGameMode::EndBattle(bool bVictory, bool bEscaped)
{
    ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
    if (!GI) return;

    // 构建战斗结果
    FLFPBattleResult Result;
    Result.SourceNodeID = CachedBattleRequest.SourceNodeID;
    Result.bVictory = bVictory;
    Result.bEscaped = bEscaped;

    if (bVictory)
    {
        Result.CapturedUnits = CapturedUnits;
        // 总奖励 = 基础奖励 + 击杀掉落
        Result.GoldReward = CachedBattleRequest.BaseGoldReward + AccumulatedDropGold;
        Result.FoodReward = CachedBattleRequest.BaseFoodReward + AccumulatedDropFood;
    }
    // 逃跑或失败：不给奖励（默认 0）

    // 缓存结果，确认后写回
    CachedBattleResult = Result;

    // 显示结算 UI
    if (BattleResultWidgetClass)
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            BattleResultWidget = CreateWidget<ULFPBattleResultWidget>(PC, BattleResultWidgetClass);
            if (BattleResultWidget)
            {
                BattleResultWidget->Setup(Result, GI->UnitRegistry);
                BattleResultWidget->OnConfirmPressed.AddDynamic(this, &ALFPTurnGameMode::OnBattleResultConfirmed);
                BattleResultWidget->AddToViewport(100);

                // 切换到 UI 输入模式
                FInputModeUIOnly InputMode;
                InputMode.SetWidgetToFocus(BattleResultWidget->TakeWidget());
                PC->SetInputMode(InputMode);
                PC->SetShowMouseCursor(true);
            }
        }
    }
    else
    {
        // 无 Widget 类配置，直接返回（兼容测试场景）
        OnBattleResultConfirmed();
    }
}

void ALFPTurnGameMode::OnBattleResultConfirmed()
{
    ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
    if (!GI) return;

    // 写回战斗结果
    GI->SetBattleResult(CachedBattleResult);

    // 移除结算 UI
    if (BattleResultWidget)
    {
        BattleResultWidget->OnConfirmPressed.RemoveDynamic(this, &ALFPTurnGameMode::OnBattleResultConfirmed);
        BattleResultWidget->RemoveFromParent();
        BattleResultWidget = nullptr;
    }

    // 恢复输入模式
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        FInputModeGameAndUI InputMode;
        PC->SetInputMode(InputMode);
    }

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

void ALFPTurnGameMode::OnEnemyUnitKilled(ALFPTacticsUnit* Unit)
{
    if (!Unit) return;

    AccumulatedDropGold += Unit->DropGold;
    AccumulatedDropFood += Unit->DropFood;
    UE_LOG(LogTemp, Log, TEXT("敌方击杀掉落: 金币 +%d, 食物 +%d (累计: %d / %d)"),
        Unit->DropGold, Unit->DropFood, AccumulatedDropGold, AccumulatedDropFood);
}
