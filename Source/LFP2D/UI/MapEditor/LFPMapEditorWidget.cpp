#include "LFP2D/UI/MapEditor/LFPMapEditorWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"

void ULFPMapEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定工具按钮
	if (TerrainToolButton) TerrainToolButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnTerrainToolClicked);
	if (DecorationToolButton) DecorationToolButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnDecorationToolClicked);
	if (SpawnPointToolButton) SpawnPointToolButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnSpawnPointToolClicked);
	if (EventToolButton) EventToolButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnEventToolClicked);
	if (AddTileToolButton) AddTileToolButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnAddTileToolClicked);
	if (RemoveTileToolButton) RemoveTileToolButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnRemoveTileToolClicked);

	// 绑定参数下拉框
	if (TerrainTypeComboBox) TerrainTypeComboBox->OnSelectionChanged.AddDynamic(this, &ULFPMapEditorWidget::OnTerrainTypeChanged);
	if (DecorationComboBox) DecorationComboBox->OnSelectionChanged.AddDynamic(this, &ULFPMapEditorWidget::OnDecorationIDChanged);
	if (SpawnFactionComboBox) SpawnFactionComboBox->OnSelectionChanged.AddDynamic(this, &ULFPMapEditorWidget::OnSpawnFactionChanged);

	// 绑定保存/加载按钮
	if (SaveButton) SaveButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnSaveClicked);
	if (LoadButton) LoadButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnLoadClicked);
	if (NewMapButton) NewMapButton->OnClicked.AddDynamic(this, &ULFPMapEditorWidget::OnNewMapClicked);

	// 自动填充枚举下拉框
	auto PopulateEnumComboBox = [](UComboBoxString* ComboBox, const UEnum* EnumPtr, const FString& DefaultOption)
	{
		if (!ComboBox || !EnumPtr) return;
		ComboBox->ClearOptions();
		for (int32 i = 0; i < EnumPtr->NumEnums(); ++i)
		{
			const FString Name = EnumPtr->GetNameStringByIndex(i);
			if (Name.IsEmpty()) continue;
			ComboBox->AddOption(Name);
		}
		ComboBox->SetSelectedOption(DefaultOption);
	};

	PopulateEnumComboBox(TerrainTypeComboBox, StaticEnum<ELFPTerrainType>(), TEXT("TT_Grass"));
	PopulateEnumComboBox(SpawnFactionComboBox, StaticEnum<ELFPSpawnFaction>(), TEXT("SF_Player"));
}

void ULFPMapEditorWidget::InitializeEditor(ULFPMapEditorComponent* EditorComp)
{
	EditorComponent = EditorComp;

	if (EditorComponent)
	{
		EditorComponent->OnEditorToolChanged.AddDynamic(this, &ULFPMapEditorWidget::UpdateToolIndicator);
	}
}

// ============== 工具按钮回调 ==============

void ULFPMapEditorWidget::OnTerrainToolClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPMapEditorTool::MET_Terrain);
}

void ULFPMapEditorWidget::OnDecorationToolClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPMapEditorTool::MET_Decoration);
}

void ULFPMapEditorWidget::OnSpawnPointToolClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPMapEditorTool::MET_SpawnPoint);
}

void ULFPMapEditorWidget::OnEventToolClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPMapEditorTool::MET_Event);
}

void ULFPMapEditorWidget::OnAddTileToolClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPMapEditorTool::MET_AddTile);
}

void ULFPMapEditorWidget::OnRemoveTileToolClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPMapEditorTool::MET_RemoveTile);
}

// ============== 参数变化回调 ==============

void ULFPMapEditorWidget::OnTerrainTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!EditorComponent) return;

	const UEnum* EnumPtr = StaticEnum<ELFPTerrainType>();
	const int64 Value = EnumPtr->GetValueByNameString(SelectedItem);
	if (Value != INDEX_NONE)
	{
		EditorComponent->SetBrushTerrainType(static_cast<ELFPTerrainType>(Value));
	}
}

void ULFPMapEditorWidget::OnDecorationIDChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!EditorComponent) return;
	EditorComponent->SetBrushDecorationID(FName(*SelectedItem));
}

void ULFPMapEditorWidget::OnSpawnFactionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!EditorComponent) return;

	const UEnum* EnumPtr = StaticEnum<ELFPSpawnFaction>();
	const int64 Value = EnumPtr->GetValueByNameString(SelectedItem);
	if (Value != INDEX_NONE)
	{
		EditorComponent->SetBrushSpawnFaction(static_cast<ELFPSpawnFaction>(Value));
	}
}

// ============== 保存/加载 ==============

void ULFPMapEditorWidget::OnSaveClicked()
{
	if (!EditorComponent || !FileNameInput) return;

	FString FileName = FileNameInput->GetText().ToString();
	if (FileName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("地图编辑器: 请输入文件名"));
		return;
	}

	if (EditorComponent->SaveMap(FileName))
	{
		UE_LOG(LogTemp, Log, TEXT("地图编辑器: 保存成功 - %s"), *FileName);
	}
}

void ULFPMapEditorWidget::OnLoadClicked()
{
	if (!EditorComponent || !FileNameInput) return;

	FString FileName = FileNameInput->GetText().ToString();
	if (FileName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("地图编辑器: 请输入文件名"));
		return;
	}

	if (EditorComponent->LoadMap(FileName))
	{
		UE_LOG(LogTemp, Log, TEXT("地图编辑器: 加载成功 - %s"), *FileName);
	}
}

void ULFPMapEditorWidget::OnNewMapClicked()
{
	if (!EditorComponent) return;
	EditorComponent->NewMap(10, 10);
}

// ============== UI 更新 ==============

void ULFPMapEditorWidget::UpdateToolIndicator(ELFPMapEditorTool NewTool)
{
	if (!CurrentToolText) return;

	static const TMap<ELFPMapEditorTool, FString> ToolNames = {
		{ELFPMapEditorTool::MET_None, TEXT("无")},
		{ELFPMapEditorTool::MET_Terrain, TEXT("地形笔刷")},
		{ELFPMapEditorTool::MET_Decoration, TEXT("装饰笔刷")},
		{ELFPMapEditorTool::MET_SpawnPoint, TEXT("出生点")},
		{ELFPMapEditorTool::MET_Event, TEXT("事件标签")},
		{ELFPMapEditorTool::MET_AddTile, TEXT("添加格子")},
		{ELFPMapEditorTool::MET_RemoveTile, TEXT("移除格子")},
	};

	if (const FString* Name = ToolNames.Find(NewTool))
	{
		CurrentToolText->SetText(FText::FromString(*Name));
	}
}
