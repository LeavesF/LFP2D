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

    // 创建根组件
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    // 创建精灵组件
    SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
    SpriteComponent->SetupAttachment(RootComponent);
    SpriteComponent->SetRelativeLocation(FVector(0, 0, 0)); // 在格子上方

    // 默认值
    CurrentMovePoints = MaxMovePoints;
    CurrentPathIndex = -1;
    MoveProgress = 0.0f;

    // 创建时间线组件
    //MoveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
}

void ALFPTacticsUnit::BeginPlay()
{
    Super::BeginPlay();

    // 注册到回合管理器
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->RegisterUnit(this);
    }

    FLFPHexCoordinates SpawnPoint = FLFPHexCoordinates(StartCoordinates_Q, StartCoordinates_R);
    SetCurrentCoordinates(SpawnPoint);

    // 设置移动时间线
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

    // 更新位置
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

//// 获取可移动范围
//TArray<ALFPHexTile*> ALFPTacticsUnit::GetMovementRangeTiles()
//{
//    if (MovementRangeTiles.Num() > 0)
//        return MovementRangeTiles;
//
//    if (ALFPHexGridManager* GridManager = GetGridManager())
//    {
//        if (ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(CurrentCoordinates))
//        {
//            // 计算移动范围
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

    // 获取当前所在的格子
    ALFPHexTile* CurrentTile = GridManager->GetTileAtCoordinates(CurrentCoordinates);
    if (!CurrentTile) return;

    // 计算路径
    MovePath = GridManager->FindPath(CurrentTile, Target);
    if (MovePath.Num() == 0) return;

    // 设置移动状态
    CurrentTile->SetIsOccupied(false);
    TargetTile = Target;
    CurrentPathIndex = 0;
    MoveProgress = 0.0f;
    MovementRangeTiles.Empty();

    // 消耗行动点
    //ConsumeMovePoints(1);

    // 开始移动动画
    //if (MoveTimeline)
    //{
    //    MoveTimeline->PlayFromStart();
    //}
    //else
    //{
    //    // 如果没有时间线，直接完成移动
    //    FinishMove();
    //}
    FinishMove();
}

void ALFPTacticsUnit::UpdateMoveAnimation(float Value)
{
    if (CurrentPathIndex < 0 || CurrentPathIndex >= MovePath.Num() - 1) return;

    // 获取当前和下一个格子
    ALFPHexTile* CurrentTile = MovePath[CurrentPathIndex];
    ALFPHexTile* NextTile = MovePath[CurrentPathIndex + 1];

    if (!CurrentTile || !NextTile) return;

    // 计算位置
    FVector StartPos = CurrentTile->GetActorLocation() + FVector(0, 0, 50);
    FVector EndPos = NextTile->GetActorLocation() + FVector(0, 0, 50);

    // 应用曲线值
    FVector NewLocation = FMath::Lerp(StartPos, EndPos, Value);
    SetActorLocation(NewLocation);

    // 更新朝向
    FVector Direction = (EndPos - StartPos).GetSafeNormal();
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        SpriteComponent->SetWorldRotation(NewRotation);
    }

    // 检查是否移动到下一段
    if (Value >= 1.0f)
    {
        CurrentPathIndex++;
        MoveProgress = 0.0f;

        // 更新当前坐标
        if (CurrentPathIndex < MovePath.Num())
        {
            SetCurrentCoordinates(MovePath[CurrentPathIndex]->GetCoordinates());
        }

        // 如果还有路径，重新开始时间线
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
    // 更新到目标位置
    if (TargetTile)
    {
        SetCurrentCoordinates(TargetTile->GetCoordinates());
        //SetActorLocation(TargetTile->GetActorLocation() + FVector(0, 0, 50));
    }

    // 重置移动状态
    TargetTile = nullptr;
    MovePath.Empty();
    CurrentPathIndex = -1;
    MoveProgress = 0.0f;
}

void ALFPTacticsUnit::ResetForNewRound()
{
    CurrentMovePoints = MaxMovePoints;
    bHasActed = false;

    // 可选：添加视觉反馈
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
    // 视觉反馈：高亮单位
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

    // 如果没有行动点了，标记为已行动
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

    // 更新时间线
    /*if (MoveTimeline && MoveTimeline->IsPlaying())
    {
        MoveTimeline->TickComponent(DeltaTime, LEVELTICK_TimeOnly, nullptr);
    }*/
}