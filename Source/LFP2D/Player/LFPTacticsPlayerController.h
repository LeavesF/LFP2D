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

    // Enhanced Input �󶨺���
    void OnSelectTriggered();
    void OnSelectStarted(const FInputActionValue& Value);
    void OnSelectCompleted(const FInputActionValue& Value);
    void OnConfirmAction(const FInputActionValue& Value);
    void OnCancelAction(const FInputActionValue& Value);
    void OnRotateCamera(const FInputActionValue& Value);
    void OnToggleDebug(const FInputActionValue& Value);

    // ��Ϸ���̺���
    void SelectUnit(ALFPTacticsUnit* Unit);
    void SelectTile(ALFPHexTile* Tile);
    void ConfirmMove();
    void ShowMovementRange();
    void HideMovementRange();
    void ShowPathToSelectedTile();
    void HidePath();

    // ���Թ���
    void ToggleDebugDisplay();

protected:
    // ����ϵͳ�ʲ�
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultInputMapping;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* SelectAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* ConfirmAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* CancelAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* RotateCameraAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* DebugToggleAction;

    // �������������
    UPROPERTY()
    ALFPHexGridManager* GridManager;

    // ��ǰѡ�еĵ�λ
    UPROPERTY()
    ALFPTacticsUnit* SelectedUnit;

    // ��ǰѡ�еĸ��ӣ������ƶ�Ŀ�꣩
    UPROPERTY()
    ALFPHexTile* SelectedTile;

    // ��ǰ��ʾ�Ŀ��ƶ���Χ
    TArray<ALFPHexTile*> MovementRangeTiles;

    // ��ǰ��ʾ��·��
    TArray<ALFPHexTile*> CurrentPath;

    // ״̬��־
    bool bIsSelecting;
    FVector2D SelectionStart;

    // �������
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    float CameraRotationPitchAngle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
    float CameraRotationYawAngle;

    // ���Ա�־
    bool bDebugEnabled;
};
