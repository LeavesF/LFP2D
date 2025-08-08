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
    // �ռ����е�λ
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundActors);

    // ת��Ϊ����ĵ�λ����
    for (AActor* Actor : FoundActors)
    {
        if (ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor))
        {
            AllUnits.Add(Unit);
        }
    }

    // ��ʼ��һ�غ�
    BeginNewRound();
}

void ALFPTurnManager::BeginNewRound()
{
    CurrentRound++;
    bIsInRound = true;

    // �������е�λ
    for (ALFPTacticsUnit* Unit : AllUnits)
    {
        if (Unit && Unit->IsValidLowLevel())
        {
            Unit->ResetForNewRound();
        }
    }

    // ���ٶ�����λ
    SortUnitsBySpeed();

    // ��ʼ��һ����λ�Ļغ�
    if (AllUnits.Num() > 0)
    {
        BeginUnitTurn(AllUnits[0]);
    }
}

void ALFPTurnManager::EndCurrentRound()
{
    bIsInRound = false;

    // ֪ͨ������һغϽ���
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ALFPTacticsPlayerController* PC = Cast<ALFPTacticsPlayerController>(*It))
        {
            PC->OnRoundEnded(CurrentRound);
        }
    }

    // �����ӳٺ�ʼ�»غ�
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ALFPTurnManager::BeginNewRound, 2.0f, false);
}

void ALFPTurnManager::SortUnitsBySpeed()
{
    // ���ٶȽ��������ٶȸߵ����ж���
    // Todo: �ٶ���ͬʱ���������AI
    AllUnits.Sort([](const ALFPTacticsUnit& A, const ALFPTacticsUnit& B) {
        return A.GetSpeed() > B.GetSpeed();
        });
}

void ALFPTurnManager::BeginUnitTurn(ALFPTacticsUnit* Unit)
{
    CurrentUnit = Unit;

    // ֪ͨ��λ�غϿ�ʼ
    if (Unit)
    {
        Unit->OnTurnStarted();
    }

    // ֪ͨ��ҿ�����
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

    // ֪ͨ��ҿ�����
    if (ALFPTacticsPlayerController* PC = GetWorld()->GetFirstPlayerController<ALFPTacticsPlayerController>())
    {
        PC->OnTurnEnded(Unit);
    }
}

void ALFPTurnManager::PassTurn()
{
    if (!CurrentUnit) return;

    // ������ǰ��λ�غ�
    EndUnitTurn(CurrentUnit);

    // �ҵ���һ��δ�ж��ĵ�λ
    int32 CurrentIndex = AllUnits.Find(CurrentUnit);
    int32 NextIndex = (CurrentIndex + 1) % AllUnits.Num();

    // ������е�λ�Ƿ��ж���
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
        // ���е�λ�ж���ϣ������غ�
        EndCurrentRound();
    }
    else
    {
        // Ѱ����һ�����ж���λ
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

        // û���ҵ����ж���λ�������غ�
        EndCurrentRound();
    }
}

void ALFPTurnManager::OnUnitFinishedAction(ALFPTacticsUnit* Unit)
{
    if (Unit == CurrentUnit)
    {
        Unit->SetHasActed(true);

        // �Զ����ݻغ�
        PassTurn();
    }
}

void ALFPTurnManager::RegisterUnit(ALFPTacticsUnit* Unit)
{
    if (Unit && !AllUnits.Contains(Unit))
    {
        AllUnits.Add(Unit);

        // �����Ϸ�Ѿ���ʼ����������
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

        // �����ǰ��λ���Ƴ������ݻغ�
        if (Unit == CurrentUnit)
        {
            PassTurn();
        }
    }
}

