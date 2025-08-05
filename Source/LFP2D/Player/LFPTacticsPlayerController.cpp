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
    CameraRotationPitchAngle = 60.0f;
    CameraRotationYawAngle = 0.0f;
    bDebugEnabled = false;
    CameraOffset = FVector::ZeroVector;
    bIsDragging = false;
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
        if (DefaultInputMapping)
        {
            Subsystem->AddMappingContext(DefaultInputMapping, 0);
        }
    }
}

void ALFPTacticsPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // 设置Enhanced Input组件
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // 选择操作
        EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnSelectStarted);
        EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnSelectCompleted);

        // 其他操作
        EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnConfirmAction);
        EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCancelAction);
        //EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnRotateCamera);
        EnhancedInputComponent->BindAction(DebugToggleAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnToggleDebug);

        // 相机控制
        EnhancedInputComponent->BindAction(CameraPanAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCameraPan);
        EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnCameraDragStarted);
        EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCameraDragTriggered);
        EnhancedInputComponent->BindAction(CameraDragAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnCameraDragCompleted);
        EnhancedInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCameraZoom);
    }
}

void ALFPTacticsPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新相机位置
    if (GridManager)
    {
        FVector CameraLocation = GridManager->GetActorLocation() + CameraOffset;
        FRotator CameraRotation(CameraRotationPitchAngle, CameraRotationYawAngle, 0);
        SetControlRotation(CameraRotation);

        // 计算相机位置偏移
        FVector DirectionOffset = CameraRotation.Vector() * CurrentZoom;
        DirectionOffset.Z = CurrentZoom * 0.5f; // 高度与距离成比例

        // 设置相机位置
        APawn* ControlledPawn = GetPawn();
        if (ControlledPawn)
        {
            FVector TargetLocation = CameraLocation + DirectionOffset;
            ControlledPawn->SetActorLocation(FMath::VInterpTo(
                ControlledPawn->GetActorLocation(),
                TargetLocation,
                DeltaTime,
                5.0f // 插值速度
            ));
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
        // 取消单位的高亮范围
        SelectedUnit->HighlightMovementRange(false);
        SelectedUnit->SetSelected(false);
        SelectedUnit = nullptr;
    }
}

//void ALFPTacticsPlayerController::OnRotateCamera(const FInputActionValue& Value)
//{
//    // 获取轴值
//    const FVector2D RotationValue = Value.Get<FVector2D>();
//    if (FMath::Abs(RotationValue.X) > 0.1f)
//    {
//        CameraRotationYawAngle += RotationValue.X * 2.0f; // 旋转速度
//    }
//}

void ALFPTacticsPlayerController::OnToggleDebug(const FInputActionValue& Value)
{
    ToggleDebugDisplay();
}

void ALFPTacticsPlayerController::OnCameraPan(const FInputActionValue& Value)
{
    // 获取轴值 (WASD/方向键)
    const FVector2D PanValue = Value.Get<FVector2D>();

    // 计算移动方向 (基于当前相机旋转)
    FRotator CameraRotation(CameraRotationPitchAngle, CameraRotationYawAngle, 0);
    FVector Forward = CameraRotation.RotateVector(FVector::ForwardVector);
    FVector Right = CameraRotation.RotateVector(FVector::RightVector);

    // 计算移动偏移
    FVector PanDirection = (Forward * PanValue.Y) + (Right * PanValue.X);
    PanDirection.Z = 0;
    PanDirection.Normalize();

    // 应用移动
    CameraOffset += PanDirection * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
}

void ALFPTacticsPlayerController::OnCameraDragStarted(const FInputActionValue& Value)
{
    // 记录拖拽起始位置
    bIsDragging = true;
    GetMousePosition(DragStartPosition.X, DragStartPosition.Y);
}

void ALFPTacticsPlayerController::OnCameraDragTriggered(const FInputActionValue& Value)
{
    if (!bIsDragging) return;

    // 获取当前鼠标位置
    FVector2D CurrentMousePosition;
    GetMousePosition(CurrentMousePosition.X, CurrentMousePosition.Y);

    // 计算鼠标移动偏移
    FVector2D MouseDelta = CurrentMousePosition - DragStartPosition;

    // 反转Y轴 (屏幕Y向下为正，但世界Y向上为正)
    MouseDelta.Y = -MouseDelta.Y;

    // 计算移动方向 (基于当前相机旋转)
    FRotator CameraRotation(0, CameraRotationYawAngle, 0);
    FVector Right = CameraRotation.RotateVector(FVector::RightVector);
    FVector Forward = CameraRotation.RotateVector(FVector::ForwardVector);

    // 计算移动偏移
    FVector PanDirection = (Right * -MouseDelta.X) + (Forward * -MouseDelta.Y);
    //PanDirection.Normalize();

    // 应用移动
    CameraOffset += PanDirection * CameraDragSpeed;

    // 更新起始位置为当前位置
    DragStartPosition = CurrentMousePosition;
}

void ALFPTacticsPlayerController::OnCameraDragCompleted(const FInputActionValue& Value)
{
    bIsDragging = false;
}

void ALFPTacticsPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
    const float ZoomValue = Value.Get<float>();
    CurrentZoom = FMath::Clamp(CurrentZoom - ZoomValue * CameraZoomSpeed, MinZoomDistance, MaxZoomDistance);
}

void ALFPTacticsPlayerController::SelectUnit(ALFPTacticsUnit* Unit)
{
    // 取消之前选中的单位
    if (SelectedUnit && SelectedUnit != Unit)
    {
        SelectedUnit->SetSelected(false);
        SelectedUnit->HighlightMovementRange(false); // 取消高亮范围
        MovementRangeTiles.Empty();
    }

    // 选择新单位
    SelectedUnit = Unit;
    if (SelectedUnit)
    {
        SelectedUnit->SetSelected(true);
        SelectedUnit->HighlightMovementRange(true); // 高亮可移动范围
        ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
        if (UnitTile)
        {
            MovementRangeTiles = GridManager->GetMovementRange(UnitTile, SelectedUnit->GetMovementRange());
        }
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
    if (!SelectedUnit || !SelectedTile) return;

    // 移动前取消高亮
    SelectedUnit->HighlightMovementRange(false);
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