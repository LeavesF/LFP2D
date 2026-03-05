#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/WorldMap/LFPWorldMapEditorComponent.h"
#include "LFPWorldMapEditorWidget.generated.h"

class UButton;
class UTextBlock;
class UComboBoxString;
class UEditableTextBox;
class UCheckBox;

/**
 * 世界地图编辑器 UI Widget
 * 需要在 UMG 蓝图中创建对应的控件并绑定
 */
UCLASS()
class LFP2D_API ULFPWorldMapEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化编辑器 UI（绑定编辑器组件）
	UFUNCTION(BlueprintCallable, Category = "World Map Editor UI")
	void InitializeEditor(ULFPWorldMapEditorComponent* EditorComp);

protected:
	virtual void NativeConstruct() override;

private:
	// 工具按钮回调
	UFUNCTION() void OnPlaceNodeClicked();
	UFUNCTION() void OnRemoveNodeClicked();
	UFUNCTION() void OnMoveNodeClicked();
	UFUNCTION() void OnConnectEdgeClicked();
	UFUNCTION() void OnRemoveEdgeClicked();
	UFUNCTION() void OnSetParamsClicked();
	UFUNCTION() void OnEditBattleMapClicked();

	// 参数变化回调
	UFUNCTION() void OnNodeTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnStarRatingChanged(const FText& Text, ETextCommit::Type CommitType);
	UFUNCTION() void OnCanEscapeChanged(bool bIsChecked);
	UFUNCTION() void OnBattleMapNameChanged(const FText& Text, ETextCommit::Type CommitType);
	UFUNCTION() void OnEventIDChanged(const FText& Text, ETextCommit::Type CommitType);
	UFUNCTION() void OnEdgeTurnCostChanged(const FText& Text, ETextCommit::Type CommitType);

	// 保存/加载回调
	UFUNCTION() void OnSaveClicked();
	UFUNCTION() void OnLoadClicked();
	UFUNCTION() void OnNewMapClicked();

	// 工具切换时更新 UI
	UFUNCTION() void UpdateToolIndicator(ELFPWorldMapEditorTool NewTool);

	// 节点选中时更新 UI
	UFUNCTION() void UpdateNodeInfo(ALFPWorldMapNode* Node);

protected:
	// ==== BindWidget 控件 ====

	// 工具按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> PlaceNodeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> RemoveNodeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> MoveNodeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConnectEdgeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> RemoveEdgeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SetParamsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> EditBattleMapButton;

	// 参数面板
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> NodeTypeComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> StarRatingInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> CanEscapeCheckBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> BattleMapNameInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EventIDInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> EdgeTurnCostInput;

	// 文件操作
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> WorldMapFileNameInput;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveWorldMapButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoadWorldMapButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewWorldMapButton;

	// 状态指示
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentToolText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedNodeInfoText;

	// 编辑器组件引用
	UPROPERTY()
	TObjectPtr<ULFPWorldMapEditorComponent> EditorComponent;
};
