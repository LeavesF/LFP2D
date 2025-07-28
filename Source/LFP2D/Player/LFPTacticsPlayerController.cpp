// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "Kismet/GameplayStatics.h"
//#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ALFPTacticsPlayerController::ALFPTacticsPlayerController()
{
    SelectedUnit = nullptr;
    SelectedTile = nullptr;
    bIsSelecting = false;
    CameraRotationAngle = 0.0f;
    bDebugEnabled = false;
}

void ALFPTacticsPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 设置输入模式
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    bShowMouseCursor = true;

    // 获取网格管理器
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        GridManager = Cast<ALFPHexGridManager>(FoundActors[0]);
    }

    // 设置Enhanced Input
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultInputMapping.IsValid())
        {
            Subsystem->AddMappingContext(DefaultInputMapping.LoadSynchronous(), 0);
        }
    }
}

void ALFPTacticsPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // 设置Enhanced Input组件
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // 选择操作（有开始/完成事件）
        EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnSelectStarted);
        EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnSelectCompleted);

        // 其他操作
        EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnConfirmAction);
        EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCancelAction);
        EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnRotateCamera);
        EnhancedInputComponent->BindAction(DebugToggleAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnToggleDebug);
    }
}

void ALFPTacticsPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新相机位置
    if (GridManager)
    {
        FVector CameraLocation = GridManager->GetActorLocation();
        FRotator CameraRotation(0, CameraRotationAngle, 0);
        SetControlRotation(CameraRotation);

        // 计算相机位置偏移
        FVector Offset = CameraRotation.Vector() * 1000.0f;
        Offset.Z = 500.0f; // 高度

        // 设置相机位置
        APawn* ControlledPawn = GetPawn();
        if (ControlledPawn)
        {
            ControlledPawn->SetActorLocation(CameraLocation + Offset);
        }
    }
}

// 选择开始
void ALFPTacticsPlayerController::OnSelectStarted(const FInputActionValue& Value)
{
    bIsSelecting = true;

    // 获取鼠标位置
    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionStart = FVector2D(MouseX, MouseY);
    }
}

// 选择完成
void ALFPTacticsPlayerController::OnSelectCompleted(const FInputActionValue& Value)
{
    if (!bIsSelecting) return;
    bIsSelecting = false;

    // 获取鼠标位置
    float MouseX, MouseY;
    GetMousePosition(MouseX, MouseY);
    FVector2D SelectionEnd = FVector2D(MouseX, MouseY);

    // 简单的点击（不是拖拽）
    if (FVector2D::Distance(SelectionStart, SelectionEnd) < 10.0f)
    {
        FHitResult HitResult;
        GetHitResultAtScreenPosition(SelectionEnd, ECC_Visibility, false, HitResult);

        if (HitResult.bBlockingHit)
        {
            // 尝试选择单位
            ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
            if (Unit)
            {
                SelectUnit(Unit);
                return;
            }

            // 尝试选择格子
            ALFPHexTile* Tile = Cast<ALFPHexTile>(HitResult.GetActor());
            if (Tile)
            {
                SelectTile(Tile);
                return;
            }
        }
    }
}

void ALFPTacticsPlayerController::OnConfirmAction(const FInputActionValue& Value)
{
    if (SelectedUnit && SelectedTile)
    {
        ConfirmMove();
    }
    else if (SelectedUnit)
    {
        // 如果没有选择目标格子，显示可移动范围
        ShowMovementRange();
    }
}

void ALFPTacticsPlayerController::OnCancelAction(const FInputActionValue& Value)
{
    // 取消当前选择
    if (SelectedTile)
    {
        HidePath();
        SelectedTile = nullptr;
    }
    else if (SelectedUnit)
    {
        HideMovementRange();
        SelectedUnit->SetSelected(false);
        SelectedUnit = nullptr;
    }
}

void ALFPTacticsPlayerController::OnRotateCamera(const FInputActionValue& Value)
{
    // 获取轴值
    const FVector2D RotationValue = Value.Get<FVector2D>();
    if (FMath::Abs(RotationValue.X) > 0.1f)
    {
        CameraRotationAngle += RotationValue.X * 2.0f; // 旋转速度
    }
}

void ALFPTacticsPlayerController::OnToggleDebug(const FInputActionValue& Value)
{
    ToggleDebugDisplay();
}

void ALFPTacticsPlayerController::SelectUnit(ALFPTacticsUnit* Unit)
{
    // 如果已经有选中的单位，取消选择
    if (SelectedUnit && SelectedUnit != Unit)
    {
        SelectedUnit->SetSelected(false);
        HideMovementRange();
    }

    // 选择新单位
    SelectedUnit = Unit;
    if (SelectedUnit)
    {
        SelectedUnit->SetSelected(true);

        // 显示可移动范围
        ShowMovementRange();
    }
}

void ALFPTacticsPlayerController::SelectTile(ALFPHexTile* Tile)
{
    if (!SelectedUnit || !GridManager) return;

    // 检查是否在可移动范围内
    if (MovementRangeTiles.Contains(Tile))
    {
        SelectedTile = Tile;
        ShowPathToSelectedTile();
    }
}

void ALFPTacticsPlayerController::ConfirmMove()
{
    if (!SelectedUnit || !SelectedTile || !GridManager) return;

    // 隐藏范围与路径
    HideMovementRange();
    HidePath();

    // 移动单位
    SelectedUnit->MoveToTile(SelectedTile);

    // 清除选择
    SelectedTile = nullptr;
    SelectedUnit->SetSelected(false);
    SelectedUnit = nullptr;
}

void ALFPTacticsPlayerController::ShowMovementRange()
{
    if (!SelectedUnit || !GridManager) return;

    // 获取可移动范围
    ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
    if (UnitTile)
    {
        MovementRangeTiles = GridManager->GetMovementRange(UnitTile, SelectedUnit->GetMovementRange());

        // 高亮显示这些格子
        for (ALFPHexTile* Tile : MovementRangeTiles)
        {
            Tile->Highlight(true);
        }
    }
}

void ALFPTacticsPlayerController::HideMovementRange()
{
    // 取消高亮
    for (ALFPHexTile* Tile : MovementRangeTiles)
    {
        Tile->Highlight(false);
    }
    MovementRangeTiles.Empty();
}

void ALFPTacticsPlayerController::ShowPathToSelectedTile()
{
    if (!SelectedUnit || !SelectedTile || !GridManager) return;

    // 先隐藏之前可能显示的路径
    HidePath();

    // 计算路径
    ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
    if (UnitTile)
    {
        CurrentPath = GridManager->FindPath(UnitTile, SelectedTile);

        // 高亮显示路径（例如用不同颜色）
        for (ALFPHexTile* Tile : CurrentPath)
        {
            Tile->SetPathHighlight(true);
        }
    }
}

void ALFPTacticsPlayerController::HidePath()
{
    for (ALFPHexTile* Tile : CurrentPath)
    {
        Tile->SetPathHighlight(false);
    }
    CurrentPath.Empty();
}

void ALFPTacticsPlayerController::ToggleDebugDisplay()
{
    bDebugEnabled = !bDebugEnabled;
    if (GridManager)
    {
        GridManager->SetDebugEnabled(bDebugEnabled);
    }
}