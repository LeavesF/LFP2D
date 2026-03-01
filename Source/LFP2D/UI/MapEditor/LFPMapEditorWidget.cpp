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

	// 填充地形类型下拉框
	if (TerrainTypeComboBox)
	{
		TerrainTypeComboBox->ClearOptions();
		TerrainTypeComboBox->AddOption(TEXT("TT_Grass"));
		TerrainTypeComboBox->AddOption(TEXT("TT_Sand"));
		TerrainTypeComboBox->AddOption(TEXT("TT_Dirt"));
		TerrainTypeComboBox->AddOption(TEXT("TT_Stone"));
		TerrainTypeComboBox->AddOption(TEXT("TT_Snow"));
		TerrainTypeComboBox->AddOption(TEXT("TT_Water"));
		TerrainTypeComboBox->AddOption(TEXT("TT_Lava"));
		TerrainTypeComboBox->AddOption(TEXT("TT_Magic"));
		TerrainTypeComboBox->AddOption(TEXT("TT_Bridge"));
		TerrainTypeComboBox->SetSelectedOption(TEXT("TT_Grass"));
	}

	// 填充出生点阵营下拉框
	if (SpawnFactionComboBox)
	{
		SpawnFactionComboBox->ClearOptions();
		SpawnFactionComboBox->AddOption(TEXT("SF_None"));
		SpawnFactionComboBox->AddOption(TEXT("SF_Player"));
		SpawnFactionComboBox->AddOption(TEXT("SF_Enemy"));
		SpawnFactionComboBox->AddOption(TEXT("SF_Neutral"));
		SpawnFactionComboBox->SetSelectedOption(TEXT("SF_Player"));
	}
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

	// 字符串映射到枚举
	static const TMap<FString, ELFPTerrainType> TerrainMap = {
		{TEXT("TT_Grass"), ELFPTerrainType::TT_Grass},
		{TEXT("TT_Sand"), ELFPTerrainType::TT_Sand},
		{TEXT("TT_Dirt"), ELFPTerrainType::TT_Dirt},
		{TEXT("TT_Stone"), ELFPTerrainType::TT_Stone},
		{TEXT("TT_Snow"), ELFPTerrainType::TT_Snow},
		{TEXT("TT_Water"), ELFPTerrainType::TT_Water},
		{TEXT("TT_Lava"), ELFPTerrainType::TT_Lava},
		{TEXT("TT_Magic"), ELFPTerrainType::TT_Magic},
		{TEXT("TT_Bridge"), ELFPTerrainType::TT_Bridge},
	};

	if (const ELFPTerrainType* Found = TerrainMap.Find(SelectedItem))
	{
		EditorComponent->SetBrushTerrainType(*Found);
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

	static const TMap<FString, ELFPSpawnFaction> FactionMap = {
		{TEXT("SF_None"), ELFPSpawnFaction::SF_None},
		{TEXT("SF_Player"), ELFPSpawnFaction::SF_Player},
		{TEXT("SF_Enemy"), ELFPSpawnFaction::SF_Enemy},
		{TEXT("SF_Neutral"), ELFPSpawnFaction::SF_Neutral},
	};

	if (const ELFPSpawnFaction* Found = FactionMap.Find(SelectedItem))
	{
		EditorComponent->SetBrushSpawnFaction(*Found);
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
