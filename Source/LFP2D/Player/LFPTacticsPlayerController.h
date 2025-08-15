// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "InputActionValue.h"
//#include "LFP2D/HexGrid/LFPHexGridManager.h"
//#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFPTacticsPlayerController.generated.h"

class ALFPTacticsUnit;
class ALFPHexGridManager;

class UInputMappingContext;
class UInputAction;

class ALFPTurnManager;

class ULFPTurnSpeedListWidget;
/**
 * 
 */
UCLASS()
class LFP2D_API ALFPTacticsPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    ALFPTacticsPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

public:
    // Enhanced Input �󶨺���
    void OnSelectStarted(const FInputActionValue& Value);
    void OnSelectCompleted(const FInputActionValue& Value);
    void OnAttackStarted(const FInputActionValue& Value);
    void OnConfirmAction(const FInputActionValue& Value);
    void OnCancelAction(const FInputActionValue& Value);
    //void OnRotateCamera(const FInputActionValue& Value);
    void OnToggleDebug(const FInputActionValue& Value);
    void OnCameraPan(const FInputActionValue& Value);
    void OnCameraDragStarted(const FInputActionValue& Value);
    void OnCameraDragTriggered(const FInputActionValue& Value);
    void OnCameraDragCompleted(const FInputActionValue& Value);
    void OnCameraZoom(const FInputActionValue& Value);

    // ��Ϸ���̺���
    void SelectUnit(ALFPTacticsUnit* Unit);
    void SelectTile(ALFPHexTile* Tile);
    void ConfirmMove();
    void ShowMovementRange(bool bHighlight);
    void ShowAttackRange(bool bHighlight);
    void ShowPathToSelectedTile();
    void HidePathToDefault();
    void HidePathToRange();

    // ���Թ���
    void ToggleDebugDisplay();

protected:
    // ����ϵͳ�ʲ�
    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultInputMapping;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* SelectAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* AttackAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ConfirmAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CancelAction;

    //UPROPERTY(EditAnywhere, Category = "Input")
    //UInputAction* RotateCameraAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* DebugToggleAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CameraPanAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CameraDragAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* CameraZoomAction;


    // �������������
    UPROPERTY()
    ALFPHexGridManager* GridManager;

    // ��ǰѡ�еĵ�λ
    UPROPERTY()
    ALFPTacticsUnit* SelectedUnit;

    UPROPERTY()
    ALFPTacticsUnit* TargetUnit;

    // ��ǰѡ�еĸ��ӣ������ƶ�Ŀ�꣩
    UPROPERTY()
    ALFPHexTile* SelectedTile;

    UPROPERTY()
    ALFPHexTile* LastHoveredTile;

    // ��ǰ��ʾ�Ŀ��ƶ���Χ
    TArray<ALFPHexTile*> MovementRangeTiles;

    // ��ǰ��ʾ��·��
    TArray<ALFPHexTile*> CurrentPath;

    // ״̬��־
    bool bIsSelecting;
    bool bIsAttacking = false;
    FVector2D SelectionStart;

    // �������
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    float CameraRotationPitchAngle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    float CameraRotationYawAngle;
    FVector CameraOffset;

    // ��ק״̬
    bool bIsDragging;
    FVector2D DragStartPosition;

    // ���Ա�־
    bool bDebugEnabled;

    // ����ƶ��ٶ�
    UPROPERTY(EditAnywhere, Category = "Camera")
    float CameraPanSpeed = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float CameraDragSpeed = 10.0f;

    // �������
    UPROPERTY(EditAnywhere, Category = "Camera")
    float CameraZoomSpeed = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MinZoomDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MaxZoomDistance = 2000.0f;

    float CurrentZoom = 1000.0f;

public:
    ALFPTurnManager* GetTurnManager() const;

    // �غ��¼�
    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Events")
    void OnTurnStarted(ALFPTacticsUnit* Unit);

    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Events")
    void OnTurnEnded(ALFPTacticsUnit* Unit);

    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Events")
    void OnRoundStarted(int32 RoundNumber);

    UFUNCTION(BlueprintImplementableEvent, Category = "Turn Events")
    void OnRoundEnded(int32 RoundNumber);

    // ��λ�ж�
    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    void MoveUnit(ALFPTacticsUnit* Unit, ALFPHexTile* TargetTile);

    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    void AttackTarget(ALFPTacticsUnit* Attacker, ALFPTacticsUnit* Target);

    UFUNCTION(BlueprintCallable, Category = "Unit Actions")
    void SkipTurn(ALFPTacticsUnit* Unit);

// UI���
protected:
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> TurnSpeedWidgetClass;

    UPROPERTY()
    TObjectPtr<ULFPTurnSpeedListWidget> TurnSpeedListWidget;
};
