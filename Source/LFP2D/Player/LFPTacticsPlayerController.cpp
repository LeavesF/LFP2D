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
        if (DefaultInputMapping.IsValid())
        {
            Subsystem->AddMappingContext(DefaultInputMapping.LoadSynchronous(), 0);
        }
    }
}

void ALFPTacticsPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // ����Enhanced Input���
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        // ѡ��������п�ʼ/����¼���
        EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnSelectStarted);
        EnhancedInputComponent->BindAction(SelectAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnSelectCompleted);

        // ��������
        EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnConfirmAction);
        EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnCancelAction);
        EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnRotateCamera);
        EnhancedInputComponent->BindAction(DebugToggleAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnToggleDebug);
    }
}

void ALFPTacticsPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // �������λ��
    if (GridManager)
    {
        FVector CameraLocation = GridManager->GetActorLocation();
        FRotator CameraRotation(0, CameraRotationAngle, 0);
        SetControlRotation(CameraRotation);

        // �������λ��ƫ��
        FVector Offset = CameraRotation.Vector() * 1000.0f;
        Offset.Z = 500.0f; // �߶�

        // �������λ��
        APawn* ControlledPawn = GetPawn();
        if (ControlledPawn)
        {
            ControlledPawn->SetActorLocation(CameraLocation + Offset);
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
        HideMovementRange();
        SelectedUnit->SetSelected(false);
        SelectedUnit = nullptr;
    }
}

void ALFPTacticsPlayerController::OnRotateCamera(const FInputActionValue& Value)
{
    // ��ȡ��ֵ
    const FVector2D RotationValue = Value.Get<FVector2D>();
    if (FMath::Abs(RotationValue.X) > 0.1f)
    {
        CameraRotationAngle += RotationValue.X * 2.0f; // ��ת�ٶ�
    }
}

void ALFPTacticsPlayerController::OnToggleDebug(const FInputActionValue& Value)
{
    ToggleDebugDisplay();
}

void ALFPTacticsPlayerController::SelectUnit(ALFPTacticsUnit* Unit)
{
    // ����Ѿ���ѡ�еĵ�λ��ȡ��ѡ��
    if (SelectedUnit && SelectedUnit != Unit)
    {
        SelectedUnit->SetSelected(false);
        HideMovementRange();
    }

    // ѡ���µ�λ
    SelectedUnit = Unit;
    if (SelectedUnit)
    {
        SelectedUnit->SetSelected(true);

        // ��ʾ���ƶ���Χ
        ShowMovementRange();
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
    if (!SelectedUnit || !SelectedTile || !GridManager) return;

    // ���ط�Χ��·��
    HideMovementRange();
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