// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/UI/Fighting/LFPTurnSpeedListWidget.h"
#include "Kismet/GameplayStatics.h"
//#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ALFPTacticsPlayerController::ALFPTacticsPlayerController()
{
    TargetUnit = nullptr;
    SelectedUnit = nullptr;
    SelectedTile = nullptr;
    bIsSelecting = false;
    bIsAttacking = false;
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

    // ��ʼ��UI
    if (TurnSpeedWidgetClass)
    {
        TurnSpeedListWidget = CreateWidget<ULFPTurnSpeedListWidget>(this, TurnSpeedWidgetClass);
        if (TurnSpeedListWidget)
        {
            TurnSpeedListWidget->AddToViewport();
            TurnSpeedListWidget->InitializeTurnOrder(); // ��ʼ����λ˳��
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
        EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnAttackStarted);

        // ��������
        EnhancedInputComponent->BindAction(ConfirmAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnConfirmAction);
        EnhancedInputComponent->BindAction(CancelAction, ETriggerEvent::Completed, this, &ALFPTacticsPlayerController::OnCancelAction);
        //EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &ALFPTacticsPlayerController::OnRotateCamera);
        EnhancedInputComponent->BindAction(DebugToggleAction, ETriggerEvent::Started, this, &ALFPTacticsPlayerController::OnToggleDebug);

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
        
        // ��ȡ���λ��
        float MouseX, MouseY;
        if (GetMousePosition(MouseX, MouseY))
        {
            FVector2D ScreenPosition(MouseX, MouseY);
            // ��ȡ����µ�������Tile
            ALFPHexTile* HoveredTile = GridManager->GetHexTileUnderCursor(ScreenPosition, this);
            // ���������ͣ�߼�
            if (HoveredTile != LastHoveredTile)
            {
                if (LastHoveredTile)
                {
                    //LastHoveredTile->OnMouseExit();
                }

                if (HoveredTile)
                {
                    SelectTile(HoveredTile);
                    //HoveredTile->OnMouseEnter();
                }

                LastHoveredTile = HoveredTile;
            }
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

void ALFPTacticsPlayerController::OnAttackStarted(const FInputActionValue& Value)
{
    if (SelectedUnit)
    {
        bIsAttacking = true;
        ShowUnitRange(EUnitRange::UR_Attack);
    }
}

void ALFPTacticsPlayerController::OnConfirmAction(const FInputActionValue& Value)
{
    if (bIsDragging)
    {
        bIsDragging = false;
        return;
    }
    if (SelectedUnit && SelectedTile)
    {
        if (bIsAttacking)
        {
            // ��ȡ���λ��
            float MouseX, MouseY;
            GetMousePosition(MouseX, MouseY);
            FVector2D SelectionEnd = FVector2D(MouseX, MouseY);

            FHitResult HitResult;
            GetHitResultAtScreenPosition(SelectionEnd, ECC_Visibility, false, HitResult);

            if (HitResult.bBlockingHit)
            {
                // ����ѡ��λ
                ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(HitResult.GetActor());
                if (Unit)
                {
                    AttackTarget(SelectedUnit, Unit);
                    return;
                }
            }
        }
        else
        {
            ConfirmMove();
        }
    }
    else if (SelectedUnit)
    {
        // ���û��ѡ��Ŀ����ӣ���ʾ���ƶ���Χ
        //ShowMovementRange(true);
    }
}

void ALFPTacticsPlayerController::OnCancelAction(const FInputActionValue& Value)
{
    if (bIsDragging)
    {
        bIsDragging = false;
        return;
    }
    bIsAttacking = false;
    ShowUnitRange(EUnitRange::UR_Move);
    // ȡ����ǰѡ��
    if (SelectedTile)
    {
        HidePathToDefault();
        //SelectedTile = nullptr;
    }
    else if (SelectedUnit)
    {
        // ȡ����λ�ĸ�����Χ
        //ShowMovementRange(false);
        //SelectedUnit->SetSelected(false);
        //SelectedUnit = nullptr;
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
    //bIsDragging = false;
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
        ShowUnitRange(EUnitRange::UR_Default);
    }

    // ѡ���µ�λ
    SelectedUnit = Unit;
    if (SelectedUnit)
    {
        SelectedUnit->SetSelected(true);
        ShowUnitRange(EUnitRange::UR_Move);
    }
}

void ALFPTacticsPlayerController::SelectTile(ALFPHexTile* Tile)
{
    if (!SelectedUnit || !GridManager) return;

    if (bIsAttacking)
    {
        SelectedTile = Tile;
    }
    else
    {
        // ����Ƿ��ڿ��ƶ���Χ��
        if (MovementRangeTiles.Contains(Tile))
        {
            SelectedTile = Tile;
            ShowPathToSelectedTile();
        }
        else
        {
            SelectedTile = Tile;
            // ������֮ǰ������ʾ��·��
            HidePathToRange();
        }
    }
}

void ALFPTacticsPlayerController::ConfirmMove()
{
    if (!SelectedUnit || !SelectedTile)
    {
        return;
    }
    if (SelectedTile->IsOccupied() || !SelectedTile->IsWalkable())
    {
        return;
    }
    //HidePathToDefault();
    // �ƶ�ǰȡ������
    ShowUnitRange(EUnitRange::UR_Default);

    // �ƶ���λ
    MoveUnit(SelectedUnit, SelectedTile);

    CurrentPath.Empty();

    ShowUnitRange(EUnitRange::UR_Move);
    /*SelectedUnit->SetSelected(false);
    SelectedUnit = nullptr;*/
}

void ALFPTacticsPlayerController::ShowUnitRange(EUnitRange UnitRange)
{
    if (!SelectedUnit || !GridManager) return;

    for (ALFPHexTile* Tile : CacheRangeTiles)
    {
        Tile->SetRangeSprite(EUnitRange::UR_Default);
    }
    CacheRangeTiles.Empty();
    MovementRangeTiles.Empty();

    switch (UnitRange)
    {
    case EUnitRange::UR_Default:
        break;
    case EUnitRange::UR_Move:
	{
        // ��ȡ���ƶ���Χ
		ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(SelectedUnit->GetCurrentCoordinates());
		if (UnitTile)
		{
			CacheRangeTiles= MovementRangeTiles = GridManager->GetTilesInRange(UnitTile, SelectedUnit->GetMovementRange());
			// ������ʾ��Щ����
			for (ALFPHexTile* Tile : CacheRangeTiles)
			{
				Tile->SetRangeSprite(EUnitRange::UR_Move);
			}
		}
        break;
	}
    case EUnitRange::UR_Attack:
		// ��ȡ�ɹ�����Χ
        CacheRangeTiles = SelectedUnit->GetAttackRangeTiles();
		// ������ʾ��Щ����
		for (ALFPHexTile* Tile : CacheRangeTiles)
		{
			Tile->SetRangeSprite(EUnitRange::UR_Attack);
		}
        break;
    default:
        break;
    }
}

void ALFPTacticsPlayerController::ShowPathToSelectedTile()
{
    if (!SelectedUnit || !SelectedTile || !GridManager) return;

    // ������֮ǰ������ʾ��·��
    HidePathToRange();
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

void ALFPTacticsPlayerController::HidePathToDefault()
{
    for (ALFPHexTile* Tile : CurrentPath)
    {
        Tile->SetMovementHighlight(false);
    }
    CurrentPath.Empty();
}

void ALFPTacticsPlayerController::HidePathToRange()
{
    for (ALFPHexTile* Tile : CurrentPath)
    {
        Tile->SetMovementHighlight(true);
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

void ALFPTacticsPlayerController::MoveUnit(ALFPTacticsUnit* Unit, ALFPHexTile* TargetTile)
{
    if (!Unit || !TargetTile || !Unit->CanAct()) return;

    // �����ƶ����ģ�����ÿ���ƶ�����1���ж��㣩
    const int32 MoveCost = 1;

    // ִ���ƶ�
    Unit->MoveToTile(TargetTile);

    // �����ж���
    Unit->ConsumeMovePoints(MoveCost);

    // ֪ͨ�غϹ�������λ����ж�
    /*if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->OnUnitFinishedAction(Unit);
    }*/
}

bool ALFPTacticsPlayerController::AttackTarget(ALFPTacticsUnit* Attacker, ALFPTacticsUnit* Target)
{
    if (!Attacker || !Target || !Attacker->CanAct())
    {
        return false;
    }

    // ���㹥������
    const int32 AttackCost = 1;

    bIsAttacking = false;

    // ִ�й����߼�
    bool bAttackSucceed = Attacker->AttackTarget(Target);

    // �����ж���
    //Attacker->ConsumeMovePoints(AttackCost);
    if (bAttackSucceed)
    {
        ShowUnitRange(EUnitRange::UR_Default);
    }
    else
    {
        ShowUnitRange(EUnitRange::UR_Move);
    }

    // ֪ͨ�غϹ�������λ����ж�
    if (bAttackSucceed)
    {
        if (ALFPTurnManager* TurnManager = GetTurnManager())
        {
            TurnManager->OnUnitFinishedAction(Attacker);
        }
    }
    return bAttackSucceed;
}

void ALFPTacticsPlayerController::SkipTurn(ALFPTacticsUnit* Unit)
{
    if (!Unit || !Unit->CanAct()) return;

    // ���Ϊ���ж�
    Unit->SetHasActed(true);

    // ��ѡ������1���ж�����Ϊ��������
    // Unit->ConsumeMovePoints(1);

    // ֪ͨ�غϹ�������λ����ж�
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->OnUnitFinishedAction(Unit);
    }
}

ALFPTurnManager* ALFPTacticsPlayerController::GetTurnManager() const
{
    TArray<AActor*> FoundManagers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTurnManager::StaticClass(), FoundManagers);

    if (FoundManagers.Num() > 0)
    {
        return Cast<ALFPTurnManager>(FoundManagers[0]);
    }
    return nullptr;
}