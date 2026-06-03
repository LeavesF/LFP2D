// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Core/LFPTurnGameMode.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/HexGrid/LFPMapData.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Turn/LFPBattleRelicRuntimeManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/UI/Fighting/LFPBattleHUDWidget.h"
#include "LFP2D/UI/Fighting/LFPBattleResultWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
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
	// 优先生成蓝图配置的 TurnManager，让 BP_TurnManager 上的资产引用参与运行。
	ClearPreplacedNonPlayerUnits();
	if (GridManager && !CachedBattleRequest.BattleMapName.IsEmpty())
	{
		SpawnEnemyUnitsFromCSV(CachedBattleRequest.BattleMapName);
	}

	TSubclassOf<ALFPTurnManager> EffectiveTurnManagerClass = TurnManagerClass;
	if (!EffectiveTurnManagerClass)
	{
		EffectiveTurnManagerClass = ALFPTurnManager::StaticClass();
	}
	TurnManager = GetWorld()->SpawnActor<ALFPTurnManager>(
		EffectiveTurnManagerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

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

void ALFPTurnGameMode::ClearPreplacedNonPlayerUnits()
{
	TArray<AActor*> FoundUnits;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundUnits);

	for (AActor* Actor : FoundUnits)
	{
		ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
		if (!Unit || Unit->GetAffiliation() == EUnitAffiliation::UA_Player)
		{
			continue;
		}

		if (ALFPHexTile* Tile = Unit->GetCurrentTile())
		{
			if (Tile->GetUnitOnTile() == Unit)
			{
				Tile->SetIsOccupied(false);
				Tile->SetUnitOnTile(nullptr);
			}
		}

		Unit->Destroy();
	}
}

void ALFPTurnGameMode::SpawnEnemyUnitsFromCSV(const FString& BattleMapName)
{
	if (!GridManager || BattleMapName.IsEmpty())
	{
		return;
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !GI->UnitRegistry)
	{
		UE_LOG(LogTemp, Warning, TEXT("Battle enemy spawn skipped: UnitRegistry is missing"));
		return;
	}

	const FString EnemyCSVPath = FPaths::ProjectSavedDir() / TEXT("Maps") / (BattleMapName + TEXT("_enemies.csv"));
	if (!FPaths::FileExists(EnemyCSVPath))
	{
		UE_LOG(LogTemp, Log, TEXT("Battle enemy spawn: enemy CSV not found %s"), *EnemyCSVPath);
		return;
	}

	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *EnemyCSVPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Battle enemy spawn: failed to read %s"), *EnemyCSVPath);
		return;
	}

	UDataTable* TempTable = NewObject<UDataTable>(GetTransientPackage());
	TempTable->RowStruct = FLFPEnemyMapUnitRow::StaticStruct();

	TArray<FString> Problems = TempTable->CreateTableFromCSVString(CSVContent);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogTemp, Warning, TEXT("Battle enemy CSV parse issue: %s"), *Problem);
	}

	TArray<FLFPEnemyMapUnitRow*> Rows;
	TempTable->GetAllRows<FLFPEnemyMapUnitRow>(TEXT("SpawnEnemyUnitsFromCSV"), Rows);

	int32 SpawnedCount = 0;
	for (const FLFPEnemyMapUnitRow* Row : Rows)
	{
		if (!Row || Row->UnitTypeID.IsNone())
		{
			continue;
		}

		const FLFPHexCoordinates Coord(Row->Q, Row->R);
		ALFPHexTile* Tile = GridManager->GetTileAtCoordinates(Coord);
		if (!Tile)
		{
			UE_LOG(LogTemp, Warning, TEXT("Battle enemy spawn: missing tile (%d,%d) for %s"),
				Row->Q, Row->R, *Row->UnitTypeID.ToString());
			continue;
		}

		TSubclassOf<ALFPTacticsUnit> UnitClass = GI->UnitRegistry->GetUnitClass(Row->UnitTypeID);
		if (!UnitClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Battle enemy spawn: invalid UnitTypeID %s"), *Row->UnitTypeID.ToString());
			continue;
		}

		const FTransform SpawnTransform(FRotator::ZeroRotator, Tile->GetActorLocation() + FVector(0.f, 0.f, 1.f));
		ALFPTacticsUnit* Unit = GetWorld()->SpawnActorDeferred<ALFPTacticsUnit>(
			UnitClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Unit)
		{
			continue;
		}

		Unit->Affiliation = EUnitAffiliation::UA_Enemy;
		Unit->UnitTypeID = Row->UnitTypeID;
		Unit->SetStartCoordinates(Coord.Q, Coord.R);
		Unit->FinishSpawning(SpawnTransform);
		++SpawnedCount;
	}

	UE_LOG(LogTemp, Log, TEXT("Battle enemy spawn: spawned %d enemy units from %s"), SpawnedCount, *EnemyCSVPath);
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

    // 通过 HUD 显示结算 UI
    ALFPTacticsPlayerController* TacticsPC = Cast<ALFPTacticsPlayerController>(GetWorld()->GetFirstPlayerController());
    if (TacticsPC && TacticsPC->GetBattleHUD())
    {
        ULFPBattleResultWidget* ResultWidget = TacticsPC->GetBattleHUD()->GetBattleResultWidget();
        if (ResultWidget)
        {
            ResultWidget->Setup(Result, GI->UnitRegistry);
            ResultWidget->OnConfirmPressed.AddDynamic(this, &ALFPTurnGameMode::OnBattleResultConfirmed);
            TacticsPC->GetBattleHUD()->ShowBattleResultWidget();

            // 切换到 UI 输入模式
            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(ResultWidget->TakeWidget());
            TacticsPC->SetInputMode(InputMode);
            TacticsPC->SetShowMouseCursor(true);
        }
        else
        {
            // 无 ResultWidget，直接返回（兼容测试场景）
            OnBattleResultConfirmed();
        }
    }
    else
    {
        // 无 HUD，直接返回（兼容测试场景）
        OnBattleResultConfirmed();
    }
}

void ALFPTurnGameMode::OnBattleResultConfirmed()
{
    ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
    if (!GI) return;

    // 写回战斗结果
    GI->SetBattleResult(CachedBattleResult);

    // 通过 HUD 隐藏结算 UI
    ALFPTacticsPlayerController* TacticsPC = Cast<ALFPTacticsPlayerController>(GetWorld()->GetFirstPlayerController());
    if (TacticsPC && TacticsPC->GetBattleHUD())
    {
        if (ULFPBattleResultWidget* ResultWidget = TacticsPC->GetBattleHUD()->GetBattleResultWidget())
        {
            ResultWidget->OnConfirmPressed.RemoveDynamic(this, &ALFPTurnGameMode::OnBattleResultConfirmed);
        }
        TacticsPC->GetBattleHUD()->HideBattleResultWidget();
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
