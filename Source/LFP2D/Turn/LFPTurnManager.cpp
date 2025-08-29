// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "Kismet/GameplayStatics.h"

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
            TurnOrderUnits.Add(Unit);
        }
    }

    // �����ӳٺ�ʼ��һ�غ�
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ALFPTurnManager::BeginNewRound, 0.5f, false);
}

void ALFPTurnManager::BeginNewRound()
{
    CurrentRound++;
    bIsInRound = true;

    // �������е�λ
    for (ALFPTacticsUnit* Unit : TurnOrderUnits)
    {
        if (Unit && Unit->IsValidLowLevel())
        {
            Unit->ResetForNewRound();
        }
    }

    // ���ٶ�����λ
    SortUnitsBySpeed();

    // ��ʼ��һ����λ�Ļغ�
    if (TurnOrderUnits.Num() > 0)
    {
        BeginUnitTurn(TurnOrderUnits[0]);
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
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ALFPTurnManager::BeginNewRound, 0.5f, false);
}

void ALFPTurnManager::SortUnitsBySpeed()
{
    // ���ٶȽ��������ٶȸߵ����ж���
    // Todo: �ٶ���ͬʱ���������AI
    TurnOrderUnits.Sort([](const ALFPTacticsUnit& A, const ALFPTacticsUnit& B) {
        return A.GetSpeed() > B.GetSpeed();
        });
}

void ALFPTurnManager::BeginUnitTurn(ALFPTacticsUnit* Unit)
{
    OnTurnChanged.Broadcast();

    CurrentUnit = Unit;

    // �����AI��λ����AI�������ӹ�
    if (Unit->IsEnemy())
    {
        if (ALFPAIController* AIController = Unit->GetAIController())
        {
            // AI������������غϿ�ʼ
            // ֪ͨ��λ�غϿ�ʼ
            if (Unit)
            {
                Unit->OnTurnStarted();
            }
            return;
        }
    }
    // ֪ͨ��λ�غϿ�ʼ
    if (Unit)
    {
        Unit->OnTurnStarted();
    }

    // ֪ͨ��ҿ�����
    if (ALFPTacticsPlayerController* PC = GetWorld()->GetFirstPlayerController<ALFPTacticsPlayerController>())
    {
        PC->OnTurnStarted(Unit);
        PC->SelectUnit(Unit);
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
    if (!CurrentUnit || TurnOrderUnits.IsEmpty())
    {
        return;
    }

    // ������ǰ��λ�غ�
    EndUnitTurn(CurrentUnit);

    // �ҵ���һ��δ�ж��ĵ�λ
    int32 CurrentIndex = TurnOrderUnits.Find(CurrentUnit);
    int32 NextIndex = (CurrentIndex + 1) % TurnOrderUnits.Num();

    // ������е�λ�Ƿ��ж���
    bool TurnOrderUnitsActed = true;
    for (ALFPTacticsUnit* Unit : TurnOrderUnits)
    {
        if (!Unit->HasActed())
        {
            TurnOrderUnitsActed = false;
            break;
        }
    }

    if (TurnOrderUnitsActed)
    {
        // ���е�λ�ж���ϣ������غ�
        EndCurrentRound();
    }
    else
    {
        // Ѱ����һ�����ж���λ
        for (int32 i = 0; i < TurnOrderUnits.Num(); i++)
        {
            int32 Index = (NextIndex + i) % TurnOrderUnits.Num();
            ALFPTacticsUnit* NextUnit = TurnOrderUnits[Index];

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
    if (Unit && !TurnOrderUnits.Contains(Unit))
    {
        TurnOrderUnits.Add(Unit);

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
        TurnOrderUnits.Remove(Unit);

        // �����ǰ��λ���Ƴ������ݻغ�
        if (Unit == CurrentUnit)
        {
            PassTurn();
        }
    }
}

