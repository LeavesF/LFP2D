// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include <Kismet/GameplayStatics.h>

ALFPTurnManager::ALFPTurnManager()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentUnit = nullptr;
}

void ALFPTurnManager::BeginPlay()
{
    Super::BeginPlay();
}

void ALFPTurnManager::StartGame()
{
    // 收集所有单位
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundActors);

    // 转换为具体的单位类型
    for (AActor* Actor : FoundActors)
    {
        if (ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor))
        {
            AllUnits.Add(Unit);
        }
    }

    // 开始第一回合
    BeginNewRound();
}

void ALFPTurnManager::BeginNewRound()
{
    CurrentRound++;
    bIsInRound = true;

    // 重置所有单位
    for (ALFPTacticsUnit* Unit : AllUnits)
    {
        if (Unit && Unit->IsValidLowLevel())
        {
            Unit->ResetForNewRound();
        }
    }

    // 按速度排序单位
    SortUnitsBySpeed();

    // 开始第一个单位的回合
    if (AllUnits.Num() > 0)
    {
        BeginUnitTurn(AllUnits[0]);
    }
}

void ALFPTurnManager::EndCurrentRound()
{
    bIsInRound = false;

    // 通知所有玩家回合结束
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ALFPTacticsPlayerController* PC = Cast<ALFPTacticsPlayerController>(*It))
        {
            PC->OnRoundEnded(CurrentRound);
        }
    }

    // 短暂延迟后开始新回合
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ALFPTurnManager::BeginNewRound, 2.0f, false);
}

void ALFPTurnManager::SortUnitsBySpeed()
{
    // 按速度降序排序（速度高的先行动）
    // Todo: 速度相同时，玩家先于AI
    AllUnits.Sort([](const ALFPTacticsUnit& A, const ALFPTacticsUnit& B) {
        return A.GetSpeed() > B.GetSpeed();
        });
}

void ALFPTurnManager::BeginUnitTurn(ALFPTacticsUnit* Unit)
{
    CurrentUnit = Unit;

    // 通知单位回合开始
    if (Unit)
    {
        Unit->OnTurnStarted();
    }

    // 通知玩家控制器
    if (ALFPTacticsPlayerController* PC = GetWorld()->GetFirstPlayerController<ALFPTacticsPlayerController>())
    {
        PC->OnTurnStarted(Unit);
    }
}

void ALFPTurnManager::EndUnitTurn(ALFPTacticsUnit* Unit)
{
    if (Unit)
    {
        Unit->OnTurnEnded();
    }

    // 通知玩家控制器
    if (ALFPTacticsPlayerController* PC = GetWorld()->GetFirstPlayerController<ALFPTacticsPlayerController>())
    {
        PC->OnTurnEnded(Unit);
    }
}

void ALFPTurnManager::PassTurn()
{
    if (!CurrentUnit) return;

    // 结束当前单位回合
    EndUnitTurn(CurrentUnit);

    // 找到下一个未行动的单位
    int32 CurrentIndex = AllUnits.Find(CurrentUnit);
    int32 NextIndex = (CurrentIndex + 1) % AllUnits.Num();

    // 检查所有单位是否都行动过
    bool AllUnitsActed = true;
    for (ALFPTacticsUnit* Unit : AllUnits)
    {
        if (!Unit->HasActed())
        {
            AllUnitsActed = false;
            break;
        }
    }

    if (AllUnitsActed)
    {
        // 所有单位行动完毕，结束回合
        EndCurrentRound();
    }
    else
    {
        // 寻找下一个可行动单位
        for (int32 i = 0; i < AllUnits.Num(); i++)
        {
            int32 Index = (NextIndex + i) % AllUnits.Num();
            ALFPTacticsUnit* NextUnit = AllUnits[Index];

            if (NextUnit && !NextUnit->HasActed())
            {
                BeginUnitTurn(NextUnit);
                return;
            }
        }

        // 没有找到可行动单位，结束回合
        EndCurrentRound();
    }
}

void ALFPTurnManager::OnUnitFinishedAction(ALFPTacticsUnit* Unit)
{
    if (Unit == CurrentUnit)
    {
        Unit->SetHasActed(true);

        // 自动传递回合
        PassTurn();
    }
}

void ALFPTurnManager::RegisterUnit(ALFPTacticsUnit* Unit)
{
    if (Unit && !AllUnits.Contains(Unit))
    {
        AllUnits.Add(Unit);

        // 如果游戏已经开始，重新排序
        if (bIsInRound)
        {
            SortUnitsBySpeed();
        }
    }
}

void ALFPTurnManager::UnregisterUnit(ALFPTacticsUnit* Unit)
{
    if (Unit)
    {
        AllUnits.Remove(Unit);

        // 如果当前单位被移除，传递回合
        if (Unit == CurrentUnit)
        {
            PassTurn();
        }
    }
}

