// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"

ALFPTacticsUnit::ALFPTacticsUnit()
{
    PrimaryActorTick.bCanEverTick = true;

    // ���������
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    // �����������
    SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
    SpriteComponent->SetupAttachment(RootComponent);
    SpriteComponent->SetRelativeLocation(FVector(0, 0, 0)); // �ڸ����Ϸ�

    // Ĭ��ֵ
    CurrentMovePoints = MaxMovePoints;
    CurrentPathIndex = -1;
    MoveProgress = 0.0f;

    // ����ʱ�������
    //MoveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
}

void ALFPTacticsUnit::BeginPlay()
{
    Super::BeginPlay();

    // ע�ᵽ�غϹ�����
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->RegisterUnit(this);
    }

    FLFPHexCoordinates SpawnPoint = FLFPHexCoordinates(StartCoordinates_Q, StartCoordinates_R);
    SetCurrentCoordinates(SpawnPoint);

    // �����ƶ�ʱ����
    if (MoveCurve)
    {
        FOnTimelineFloat TimelineCallback;
        TimelineCallback.BindUFunction(this, FName("UpdateMoveAnimation"));
        //MoveTimeline->AddInterpFloat(MoveCurve, TimelineCallback);

        FOnTimelineEvent TimelineFinishedCallback;
        TimelineFinishedCallback.BindUFunction(this, FName("FinishMove"));
        //MoveTimeline->SetTimelineFinishedFunc(TimelineFinishedCallback);
    }
}

void ALFPTacticsUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->UnregisterUnit(this);
    }
}

void ALFPTacticsUnit::SetCurrentCoordinates(const FLFPHexCoordinates& NewCoords)
{
    CurrentCoordinates = NewCoords;

    // ����λ��
    if (ALFPHexGridManager* GridManager = GetGridManager())
    {
        if (ALFPHexTile* Tile = GridManager->GetTileAtCoordinates(NewCoords))
        {
            SetActorLocation(Tile->GetActorLocation() + FVector(0, 0, 1));
            Tile->SetIsOccupied(true);
        }
    }
}

//void ALFPTacticsUnit::SnapToGrid()
//{
//    if (ALFPHexGridManager* GridManager = GetGridManager())
//    {
//        if (ALFPHexTile* Tile = GridManager->GetTileAtCoordinates(CurrentCoordinates))
//        {
//            SetActorLocation(Tile->GetActorLocation() + FVector(0, 0, 50));
//        }
//    }
//}

//// ��ȡ���ƶ���Χ
//TArray<ALFPHexTile*> ALFPTacticsUnit::GetMovementRangeTiles()
//{
//    if (MovementRangeTiles.Num() > 0)
//        return MovementRangeTiles;
//
//    if (ALFPHexGridManager* GridManager = GetGridManager())
//    {
//        if (ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(CurrentCoordinates))
//        {
//            // �����ƶ���Χ
//            MovementRangeTiles = GridManager->GetMovementRange(UnitTile, MovementRange);
//        }
//    }
//    return MovementRangeTiles;
//}

void ALFPTacticsUnit::MoveToTile(ALFPHexTile* Target)
{
    if (!Target || !HasEnoughMovePoints(1)) return;

    ALFPHexGridManager* GridManager = GetGridManager();
    if (!GridManager) return;

    // ��ȡ��ǰ���ڵĸ���
    ALFPHexTile* CurrentTile = GridManager->GetTileAtCoordinates(CurrentCoordinates);
    if (!CurrentTile) return;

    // ����·��
    MovePath = GridManager->FindPath(CurrentTile, Target);
    if (MovePath.Num() == 0) return;

    // �����ƶ�״̬
    CurrentTile->SetIsOccupied(false);
    TargetTile = Target;
    CurrentPathIndex = 0;
    MoveProgress = 0.0f;
    MovementRangeTiles.Empty();

    // �����ж���
    //ConsumeMovePoints(1);

    // ��ʼ�ƶ�����
    //if (MoveTimeline)
    //{
    //    MoveTimeline->PlayFromStart();
    //}
    //else
    //{
    //    // ���û��ʱ���ߣ�ֱ������ƶ�
    //    FinishMove();
    //}
    FinishMove();
}

void ALFPTacticsUnit::UpdateMoveAnimation(float Value)
{
    if (CurrentPathIndex < 0 || CurrentPathIndex >= MovePath.Num() - 1) return;

    // ��ȡ��ǰ����һ������
    ALFPHexTile* CurrentTile = MovePath[CurrentPathIndex];
    ALFPHexTile* NextTile = MovePath[CurrentPathIndex + 1];

    if (!CurrentTile || !NextTile) return;

    // ����λ��
    FVector StartPos = CurrentTile->GetActorLocation() + FVector(0, 0, 50);
    FVector EndPos = NextTile->GetActorLocation() + FVector(0, 0, 50);

    // Ӧ������ֵ
    FVector NewLocation = FMath::Lerp(StartPos, EndPos, Value);
    SetActorLocation(NewLocation);

    // ���³���
    FVector Direction = (EndPos - StartPos).GetSafeNormal();
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        SpriteComponent->SetWorldRotation(NewRotation);
    }

    // ����Ƿ��ƶ�����һ��
    if (Value >= 1.0f)
    {
        CurrentPathIndex++;
        MoveProgress = 0.0f;

        // ���µ�ǰ����
        if (CurrentPathIndex < MovePath.Num())
        {
            SetCurrentCoordinates(MovePath[CurrentPathIndex]->GetCoordinates());
        }

        // �������·�������¿�ʼʱ����
        if (CurrentPathIndex < MovePath.Num() - 1)
        {
            //MoveTimeline->PlayFromStart();
        }
        else
        {
            FinishMove();
        }
    }
}

void ALFPTacticsUnit::FinishMove()
{
    // ���µ�Ŀ��λ��
    if (TargetTile)
    {
        SetCurrentCoordinates(TargetTile->GetCoordinates());
        //SetActorLocation(TargetTile->GetActorLocation() + FVector(0, 0, 50));
    }

    // �����ƶ�״̬
    TargetTile = nullptr;
    MovePath.Empty();
    CurrentPathIndex = -1;
    MoveProgress = 0.0f;
}

void ALFPTacticsUnit::ResetForNewRound()
{
    CurrentMovePoints = MaxMovePoints;
    bHasActed = false;

    // ��ѡ������Ӿ�����
    if (SpriteComponent)
    {
        SpriteComponent->SetSpriteColor(FLinearColor::White);
    }
}

void ALFPTacticsUnit::OnTurnStarted()
{
    bOnTurn = true;
}

void ALFPTacticsUnit::OnTurnEnded()
{
    bOnTurn = false;
}

void ALFPTacticsUnit::OnMouseEnter()
{
}

void ALFPTacticsUnit::SetSelected(bool bSelected)
{
    bIsSelected = bSelected;
    // �Ӿ�������������λ
    if (bSelected)
    {
        SpriteComponent->SetSpriteColor(FLinearColor::Yellow);
    }
    else
    {
        SpriteComponent->SetSpriteColor(FLinearColor::White);
    }
}

//void ALFPTacticsUnit::HighlightMovementRange(bool bHighlight)
//{
//    TArray<ALFPHexTile*> Tiles = GetMovementRangeTiles();
//
//    for (ALFPHexTile* Tile : Tiles)
//    {
//        if (bHighlight)
//        {
//            Tile->SetMovementHighlight(true);
//        }
//        else
//        {
//            Tile->SetMovementHighlight(false);
//        }
//    }
//
//    if (!bHighlight)
//    {
//        MovementRangeTiles.Empty();
//    }
//}

void ALFPTacticsUnit::ConsumeMovePoints(int32 Amount)
{
    CurrentMovePoints = FMath::Max(0, CurrentMovePoints - Amount);

    // ���û���ж����ˣ����Ϊ���ж�
    if (CurrentMovePoints <= 0)
    {
        //bHasActed = true;
    }
}

bool ALFPTacticsUnit::HasEnoughMovePoints(int32 Required) const
{
    return CurrentMovePoints >= Required;
}

ALFPHexGridManager* ALFPTacticsUnit::GetGridManager() const
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        return Cast<ALFPHexGridManager>(FoundActors[0]);
    }
    return nullptr;
}

ALFPTurnManager* ALFPTacticsUnit::GetTurnManager() const
{
    TArray<AActor*> FoundManagers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTurnManager::StaticClass(), FoundManagers);

    if (FoundManagers.Num() > 0)
    {
        return Cast<ALFPTurnManager>(FoundManagers[0]);
    }
    return nullptr;
}

void ALFPTacticsUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // ����ʱ����
    /*if (MoveTimeline && MoveTimeline->IsPlaying())
    {
        MoveTimeline->TickComponent(DeltaTime, LEVELTICK_TimeOnly, nullptr);
    }*/
}