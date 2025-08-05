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

    // ��������ģʽ
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    bShowMouseCursor = true;

    // ��ȡ���������
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        GridManager = Cast<ALFPHexGridManager>(FoundActors[0]);
    }

    // ����Enhanced Input
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

    // ����Enhanced Input���
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // ѡ�����
        EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnSelectStarted);
        EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnSelectCompleted);

        // ��������
        EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnConfirmAction);
        EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCancelAction);
        //EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnRotateCamera);
        EnhancedInputComponent->BindAction(DebugToggleAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnToggleDebug);

        // �������
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

    // �������λ��
    if (GridManager)
    {
        FVector CameraLocation = GridManager->GetActorLocation() + CameraOffset;
        FRotator CameraRotation(CameraRotationPitchAngle, CameraRotationYawAngle, 0);
        SetControlRotation(CameraRotation);

        // �������λ��ƫ��
        FVector DirectionOffset = CameraRotation.Vector() * CurrentZoom;
        DirectionOffset.Z = CurrentZoom * 0.5f; // �߶������ɱ���

        // �������λ��
        APawn* ControlledPawn = GetPawn();
        if (ControlledPawn)
        {
            FVector TargetLocation = CameraLocation + DirectionOffset;
            ControlledPawn->SetActorLocation(FMath::VInterpTo(
                ControlledPawn->GetActorLocation(),
                TargetLocation,
                DeltaTime,
                5.0f // ��ֵ�ٶ�
            ));
        }
        
    }
}

// ѡ��ʼ
void ALFPTacticsPlayerController::OnSelectStarted(const FInputActionValue& Value)
{
    bIsSelecting = true;

    // ��ȡ���λ��
    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionStart = FVector2D(MouseX, MouseY);
    }
}

// ѡ�����
void ALFPTacticsPlayerController::OnSelectCompleted(const FInputActionValue& Value)
{
    if (!bIsSelecting) return;
    bIsSelecting = false;

    // ��ȡ���λ��
    float MouseX, MouseY;
    GetMousePosition(MouseX, MouseY);
    FVector2D SelectionEnd = FVector2D(MouseX, MouseY);

    // �򵥵ĵ����������ק��
    if (FVector2D::Distance(SelectionStart, SelectionEnd) < 10.0f)
    {
        FHitResult HitResult;
        GetHitResultAtScreenPosition(SelectionEnd, ECC_Visibility, false, HitResult);

        if (HitResult.bBlockingHit)
        {
            // ����ѡ��λ
            ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
            if (Unit)
            {
                SelectUnit(Unit);
                return;
            }

            // ����ѡ�����
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
        // ���û��ѡ��Ŀ����ӣ���ʾ���ƶ���Χ
        ShowMovementRange();
    }
}

void ALFPTacticsPlayerController::OnCancelAction(const FInputActionValue& Value)
{
    // ȡ����ǰѡ��
    if (SelectedTile)
    {
        HidePath();
        SelectedTile = nullptr;
    }
    else if (SelectedUnit)
    {
        // ȡ����λ�ĸ�����Χ
        SelectedUnit->HighlightMovementRange(false);
        SelectedUnit->SetSelected(false);
        SelectedUnit = nullptr;
    }
}

//void ALFPTacticsPlayerController::OnRotateCamera(const FInputActionValue& Value)
//{
//    // ��ȡ��ֵ
//    const FVector2D RotationValue = Value.Get<FVector2D>();
//    if (FMath::Abs(RotationValue.X) > 0.1f)
//    {
//        CameraRotationYawAngle += RotationValue.X * 2.0f; // ��ת�ٶ�
//    }
//}

void ALFPTacticsPlayerController::OnToggleDebug(const FInputActionValue& Value)
{
    ToggleDebugDisplay();
}

void ALFPTacticsPlayerController::OnCameraPan(const FInputActionValue& Value)
{
    // ��ȡ��ֵ (WASD/�����)
    const FVector2D PanValue = Value.Get<FVector2D>();

    // �����ƶ����� (���ڵ�ǰ�����ת)
    FRotator CameraRotation(CameraRotationPitchAngle, CameraRotationYawAngle, 0);
    FVector Forward = CameraRotation.RotateVector(FVector::ForwardVector);
    FVector Right = CameraRotation.RotateVector(FVector::RightVector);

    // �����ƶ�ƫ��
    FVector PanDirection = (Forward * PanValue.Y) + (Right * PanValue.X);
    PanDirection.Z = 0;
    PanDirection.Normalize();

    // Ӧ���ƶ�
    CameraOffset += PanDirection * CameraPanSpeed * GetWorld()->GetDeltaSeconds();
}

void ALFPTacticsPlayerController::OnCameraDragStarted(const FInputActionValue& Value)
{
    // ��¼��ק��ʼλ��
    bIsDragging = true;
    GetMousePosition(DragStartPosition.X, DragStartPosition.Y);
}

void ALFPTacticsPlayerController::OnCameraDragTriggered(const FInputActionValue& Value)
{
    if (!bIsDragging) return;

    // ��ȡ��ǰ���λ��
    FVector2D CurrentMousePosition;
    GetMousePosition(CurrentMousePosition.X, CurrentMousePosition.Y);

    // ��������ƶ�ƫ��
    FVector2D MouseDelta = CurrentMousePosition - DragStartPosition;

    // ��תY�� (��ĻY����Ϊ����������Y����Ϊ��)
    MouseDelta.Y = -MouseDelta.Y;

    // �����ƶ����� (���ڵ�ǰ�����ת)
    FRotator CameraRotation(0, CameraRotationYawAngle, 0);
    FVector Right = CameraRotation.RotateVector(FVector::RightVector);
    FVector Forward = CameraRotation.RotateVector(FVector::ForwardVector);

    // �����ƶ�ƫ��
    FVector PanDirection = (Right * -MouseDelta.X) + (Forward * -MouseDelta.Y);
    //PanDirection.Normalize();

    // Ӧ���ƶ�
    CameraOffset += PanDirection * CameraDragSpeed;

    // ������ʼλ��Ϊ��ǰλ��
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
    // ȡ��֮ǰѡ�еĵ�λ
    if (SelectedUnit && SelectedUnit != Unit)
    {
        SelectedUnit->SetSelected(false);
        SelectedUnit->HighlightMovementRange(false); // ȡ��������Χ
        MovementRangeTiles.Empty();
    }

    // ѡ���µ�λ
    SelectedUnit = Unit;
    if (SelectedUnit)
    {
        SelectedUnit->SetSelected(true);
        SelectedUnit->HighlightMovementRange(true); // �������ƶ���Χ
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

    // ����Ƿ��ڿ��ƶ���Χ��
    if (MovementRangeTiles.Contains(Tile))
    {
        SelectedTile = Tile;
        ShowPathToSelectedTile();
    }
}

void ALFPTacticsPlayerController::ConfirmMove()
{
    if (!SelectedUnit || !SelectedTile) return;

    // �ƶ�ǰȡ������
    SelectedUnit->HighlightMovementRange(false);
    HidePath();

    // �ƶ���λ
    SelectedUnit->MoveToTile(SelectedTile);

    // ���ѡ��
    SelectedTile = nullptr;
    SelectedUnit->SetSelected(false);
    SelectedUnit = nullptr;
}

void ALFPTacticsPlayerController::ShowMovementRange()
{
    if (!SelectedUnit || !GridManager) return;

    // ��ȡ���ƶ���Χ
    ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
    if (UnitTile)
    {
        MovementRangeTiles = GridManager->GetMovementRange(UnitTile, SelectedUnit->GetMovementRange());

        // ������ʾ��Щ����
        for (ALFPHexTile* Tile : MovementRangeTiles)
        {
            Tile->Highlight(true);
        }
    }
}

void ALFPTacticsPlayerController::HideMovementRange()
{
    // ȡ������
    for (ALFPHexTile* Tile : MovementRangeTiles)
    {
        Tile->Highlight(false);
    }
    MovementRangeTiles.Empty();
}

void ALFPTacticsPlayerController::ShowPathToSelectedTile()
{
    if (!SelectedUnit || !SelectedTile || !GridManager) return;

    // ������֮ǰ������ʾ��·��
    HidePath();

    // ����·��
    ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
    if (UnitTile)
    {
        CurrentPath = GridManager->FindPath(UnitTile, SelectedTile);

        // ������ʾ·���������ò�ͬ��ɫ��
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