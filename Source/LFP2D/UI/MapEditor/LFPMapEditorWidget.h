#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFP2D/HexGrid/LFPMapEditorComponent.h"
#include "LFPMapEditorWidget.generated.h"

class UButton;
class UTextBlock;
class UComboBoxString;
class UEditableTextBox;

/**
 * 地图编辑器 UI Widget
 * 需要在 UMG 蓝图中创建对应的控件并绑定
 */
UCLASS()
class LFP2D_API ULFPMapEditorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 初始化编辑器 UI（绑定编辑器组件）
	UFUNCTION(BlueprintCallable, Category = "Map Editor UI")
	void InitializeEditor(ULFPMapEditorComponent* EditorComp);

protected:
	virtual void NativeConstruct() override;

private:
	// 工具按钮回调
	UFUNCTION() void OnTerrainToolClicked();
	UFUNCTION() void OnDecorationToolClicked();
	UFUNCTION() void OnSpawnPointToolClicked();
	UFUNCTION() void OnEventToolClicked();
	UFUNCTION() void OnAddTileToolClicked();
	UFUNCTION() void OnRemoveTileToolClicked();

	// 参数变化回调
	UFUNCTION() void OnTerrainTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnDecorationIDChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnSpawnFactionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	// 保存/加载回调
	UFUNCTION() void OnSaveClicked();
	UFUNCTION() void OnLoadClicked();
	UFUNCTION() void OnNewMapClicked();

	// 工具切换时更新 UI
	UFUNCTION() void UpdateToolIndicator(ELFPMapEditorTool NewTool);

protected:
	// ==== BindWidget 控件 ====

	// 工具按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> TerrainToolButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> DecorationToolButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SpawnPointToolButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> EventToolButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> AddTileToolButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> RemoveTileToolButton;

	// 地形类型下拉框
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> TerrainTypeComboBox;

	// 装饰下拉框
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> DecorationComboBox;

	// 出生点阵营下拉框
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> SpawnFactionComboBox;

	// 出生点索引输入
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> SpawnIndexInput;

	// 当前工具指示
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentToolText;

	// 文件名输入
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> FileNameInput;

	// 保存/加载/新建按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoadButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewMapButton;

	// 编辑器组件引用
	UPROPERTY()
	TObjectPtr<ULFPMapEditorComponent> EditorComponent;
};
