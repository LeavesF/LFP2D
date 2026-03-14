#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "LFPWorldMapPlayerController.generated.h"

class ALFPWorldMapManager;
class ALFPWorldMapNode;
class ULFPWorldMapEditorComponent;
class ULFPWorldMapEditorWidget;
class UInputMappingContext;
class UInputAction;

/**
 * 世界地图玩家控制器
 * 负责世界地图上的玩家交互、节点选择、移动等
 */
UCLASS()
class LFP2D_API ALFPWorldMapPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALFPWorldMapPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ============== 输入处理 ==============

	void OnConfirmAction(const FInputActionValue& Value);
	void OnCancelAction(const FInputActionValue& Value);
	void OnToggleEditorAction(const FInputActionValue& Value);

	// 相机操作
	void OnCameraPan(const FInputActionValue& Value);
	void OnCameraDragStarted(const FInputActionValue& Value);
	void OnCameraDragTriggered(const FInputActionValue& Value);
	void OnCameraDragCompleted(const FInputActionValue& Value);
	void OnCameraZoom(const FInputActionValue& Value);

	// ============== 节点交互 ==============

	// 选中节点
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void SelectNode(ALFPWorldMapNode* Node);

	// 移动到相邻节点
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void MoveToNode(ALFPWorldMapNode* Node);

	// 显示可达节点高亮
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void ShowReachableNodes();

	// 清除可达节点高亮
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void ClearReachableHighlight();

	// 进入节点（触发战斗/事件等）
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void EnterNode(ALFPWorldMapNode* Node);

	// 进入战斗（保存快照 + 设置请求 + 切换关卡）
	UFUNCTION(BlueprintCallable, Category = "World Map")
	void EnterBattle(ALFPWorldMapNode* BattleNode);

	// 获取世界地图管理器
	UFUNCTION(BlueprintPure, Category = "World Map")
	ALFPWorldMapManager* GetWorldMapManager() const;

protected:
	// ============== 输入系统 ==============

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMapping;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ConfirmAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CancelAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ToggleEditorAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CameraPanAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CameraDragAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CameraZoomAction;

	// ============== 世界地图编辑器 ==============

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Map Editor")
	TObjectPtr<ULFPWorldMapEditorComponent> WorldMapEditorComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<ULFPWorldMapEditorWidget> WorldMapEditorWidgetClass;

	UPROPERTY()
	TObjectPtr<ULFPWorldMapEditorWidget> WorldMapEditorWidget;

	// ============== 相机 ==============

	// 相机俯视角度（Pitch，负值向下看）
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPitchAngle = -60.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPanSpeed = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraDragSpeed = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraZoomSpeed = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MinZoomDistance = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float MaxZoomDistance = 2000.0f;

	float CurrentZoom = 1000.0f;
	FVector CameraOffset;

	// 场景切换后首帧跳过相机平滑插值
	bool bSnapCameraNextFrame = true;

	// 拖拽状态
	bool bIsDragging = false;
	FVector2D DragStartPosition;
	float DragTime = 0.f;

	UPROPERTY(EditAnywhere, Category = "Input")
	float DragThresholdTime = 0.5f;

	// ============== 状态 ==============

	// 当前选中的节点
	UPROPERTY()
	TObjectPtr<ALFPWorldMapNode> SelectedNode;

	// 玩家移动的目标节点（移动完成后自动进入）
	UPROPERTY()
	TObjectPtr<ALFPWorldMapNode> PendingEnterNode;

	// 可达节点缓存（高亮用）
	TArray<ALFPWorldMapNode*> ReachableNodes;

	// 棋子移动完成回调
	UFUNCTION()
	void OnPawnMoveCompleted();

	// 世界地图管理器缓存
	UPROPERTY()
	mutable TObjectPtr<ALFPWorldMapManager> CachedWorldMapManager;
};
