#include "LFP2D/UI/WorldMapEditor/LFPWorldMapEditorWidget.h"
#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/CheckBox.h"

void ULFPWorldMapEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定工具按钮
	if (PlaceNodeButton) PlaceNodeButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnPlaceNodeClicked);
	if (RemoveNodeButton) RemoveNodeButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnRemoveNodeClicked);
	if (MoveNodeButton) MoveNodeButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnMoveNodeClicked);
	if (ConnectEdgeButton) ConnectEdgeButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnConnectEdgeClicked);
	if (RemoveEdgeButton) RemoveEdgeButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnRemoveEdgeClicked);
	if (SetParamsButton) SetParamsButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnSetParamsClicked);
	if (EditBattleMapButton) EditBattleMapButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnEditBattleMapClicked);

	// 绑定参数控件
	if (NodeTypeComboBox) NodeTypeComboBox->OnSelectionChanged.AddDynamic(this, &ULFPWorldMapEditorWidget::OnNodeTypeChanged);
	if (StarRatingInput) StarRatingInput->OnTextCommitted.AddDynamic(this, &ULFPWorldMapEditorWidget::OnStarRatingChanged);
	if (CanEscapeCheckBox) CanEscapeCheckBox->OnCheckStateChanged.AddDynamic(this, &ULFPWorldMapEditorWidget::OnCanEscapeChanged);
	if (BattleMapNameInput) BattleMapNameInput->OnTextCommitted.AddDynamic(this, &ULFPWorldMapEditorWidget::OnBattleMapNameChanged);
	if (EventIDInput) EventIDInput->OnTextCommitted.AddDynamic(this, &ULFPWorldMapEditorWidget::OnEventIDChanged);
	if (EdgeTurnCostInput) EdgeTurnCostInput->OnTextCommitted.AddDynamic(this, &ULFPWorldMapEditorWidget::OnEdgeTurnCostChanged);
	if (BaseGoldRewardInput) BaseGoldRewardInput->OnTextCommitted.AddDynamic(this, &ULFPWorldMapEditorWidget::OnBaseGoldRewardChanged);
	if (BaseFoodRewardInput) BaseFoodRewardInput->OnTextCommitted.AddDynamic(this, &ULFPWorldMapEditorWidget::OnBaseFoodRewardChanged);

	// 绑定保存/加载按钮
	if (SaveWorldMapButton) SaveWorldMapButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnSaveClicked);
	if (LoadWorldMapButton) LoadWorldMapButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnLoadClicked);
	if (NewWorldMapButton) NewWorldMapButton->OnClicked.AddDynamic(this, &ULFPWorldMapEditorWidget::OnNewMapClicked);

	// 填充节点类型下拉框
	if (NodeTypeComboBox)
	{
		NodeTypeComboBox->ClearOptions();
		NodeTypeComboBox->AddOption(TEXT("WNT_Battle"));
		NodeTypeComboBox->AddOption(TEXT("WNT_Event"));
		NodeTypeComboBox->AddOption(TEXT("WNT_Shop"));
		NodeTypeComboBox->AddOption(TEXT("WNT_Town"));
		NodeTypeComboBox->AddOption(TEXT("WNT_Boss"));
		NodeTypeComboBox->AddOption(TEXT("WNT_QuestNPC"));
		NodeTypeComboBox->AddOption(TEXT("WNT_SkillNode"));
		NodeTypeComboBox->SetSelectedOption(TEXT("WNT_Battle"));
	}

	// 默认值
	if (StarRatingInput) StarRatingInput->SetText(FText::FromString(TEXT("1")));
	if (CanEscapeCheckBox) CanEscapeCheckBox->SetIsChecked(true);
	if (EdgeTurnCostInput) EdgeTurnCostInput->SetText(FText::FromString(TEXT("1")));
	if (BaseGoldRewardInput) BaseGoldRewardInput->SetText(FText::FromString(TEXT("0")));
	if (BaseFoodRewardInput) BaseFoodRewardInput->SetText(FText::FromString(TEXT("0")));
}

void ULFPWorldMapEditorWidget::InitializeEditor(ULFPWorldMapEditorComponent* EditorComp)
{
	EditorComponent = EditorComp;

	if (EditorComponent)
	{
		EditorComponent->OnToolChanged.AddDynamic(this, &ULFPWorldMapEditorWidget::UpdateToolIndicator);
		EditorComponent->OnNodeSelected.AddDynamic(this, &ULFPWorldMapEditorWidget::UpdateNodeInfo);
	}
}

// ============== 工具按钮回调 ==============

void ULFPWorldMapEditorWidget::OnPlaceNodeClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPWorldMapEditorTool::WMET_PlaceNode);
}

void ULFPWorldMapEditorWidget::OnRemoveNodeClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPWorldMapEditorTool::WMET_RemoveNode);
}

void ULFPWorldMapEditorWidget::OnMoveNodeClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPWorldMapEditorTool::WMET_MoveNode);
}

void ULFPWorldMapEditorWidget::OnConnectEdgeClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPWorldMapEditorTool::WMET_ConnectEdge);
}

void ULFPWorldMapEditorWidget::OnRemoveEdgeClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPWorldMapEditorTool::WMET_RemoveEdge);
}

void ULFPWorldMapEditorWidget::OnSetParamsClicked()
{
	if (EditorComponent) EditorComponent->SetCurrentTool(ELFPWorldMapEditorTool::WMET_SetNodeParams);
}

void ULFPWorldMapEditorWidget::OnEditBattleMapClicked()
{
	if (EditorComponent) EditorComponent->EnterBattleMapEditor();
}

// ============== 参数变化回调 ==============

void ULFPWorldMapEditorWidget::OnNodeTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!EditorComponent) return;

	static const TMap<FString, ELFPWorldNodeType> NodeTypeMap = {
		{TEXT("WNT_Battle"), ELFPWorldNodeType::WNT_Battle},
		{TEXT("WNT_Event"), ELFPWorldNodeType::WNT_Event},
		{TEXT("WNT_Shop"), ELFPWorldNodeType::WNT_Shop},
		{TEXT("WNT_Town"), ELFPWorldNodeType::WNT_Town},
		{TEXT("WNT_Boss"), ELFPWorldNodeType::WNT_Boss},
		{TEXT("WNT_QuestNPC"), ELFPWorldNodeType::WNT_QuestNPC},
		{TEXT("WNT_SkillNode"), ELFPWorldNodeType::WNT_SkillNode},
	};

	if (const ELFPWorldNodeType* Found = NodeTypeMap.Find(SelectedItem))
	{
		EditorComponent->SetBrushNodeType(*Found);
	}
}

void ULFPWorldMapEditorWidget::OnStarRatingChanged(const FText& Text, ETextCommit::Type CommitType)
{
	if (!EditorComponent) return;
	int32 Rating = FCString::Atoi(*Text.ToString());
	EditorComponent->SetBrushStarRating(Rating);
}

void ULFPWorldMapEditorWidget::OnCanEscapeChanged(bool bIsChecked)
{
	if (EditorComponent) EditorComponent->SetBrushCanEscape(bIsChecked);
}

void ULFPWorldMapEditorWidget::OnBattleMapNameChanged(const FText& Text, ETextCommit::Type CommitType)
{
	if (EditorComponent) EditorComponent->SetBrushBattleMapName(Text.ToString());
}

void ULFPWorldMapEditorWidget::OnEventIDChanged(const FText& Text, ETextCommit::Type CommitType)
{
	if (EditorComponent) EditorComponent->SetBrushEventID(Text.ToString());
}

void ULFPWorldMapEditorWidget::OnEdgeTurnCostChanged(const FText& Text, ETextCommit::Type CommitType)
{
	if (!EditorComponent) return;
	int32 Cost = FCString::Atoi(*Text.ToString());
	EditorComponent->SetBrushEdgeTurnCost(Cost);
}

void ULFPWorldMapEditorWidget::OnBaseGoldRewardChanged(const FText& Text, ETextCommit::Type CommitType)
{
	if (!EditorComponent) return;
	int32 Gold = FCString::Atoi(*Text.ToString());
	EditorComponent->SetBrushBaseGoldReward(Gold);
}

void ULFPWorldMapEditorWidget::OnBaseFoodRewardChanged(const FText& Text, ETextCommit::Type CommitType)
{
	if (!EditorComponent) return;
	int32 Food = FCString::Atoi(*Text.ToString());
	EditorComponent->SetBrushBaseFoodReward(Food);
}

// ============== 保存/加载 ==============

void ULFPWorldMapEditorWidget::OnSaveClicked()
{
	if (!EditorComponent || !WorldMapFileNameInput) return;

	FString MapName = WorldMapFileNameInput->GetText().ToString();
	if (MapName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("世界地图编辑器: 请输入文件名"));
		return;
	}

	if (EditorComponent->SaveWorldMap(MapName))
	{
		UE_LOG(LogTemp, Log, TEXT("世界地图编辑器: 保存成功 - %s"), *MapName);
	}
}

void ULFPWorldMapEditorWidget::OnLoadClicked()
{
	if (!EditorComponent || !WorldMapFileNameInput) return;

	FString MapName = WorldMapFileNameInput->GetText().ToString();
	if (MapName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("世界地图编辑器: 请输入文件名"));
		return;
	}

	if (EditorComponent->LoadWorldMap(MapName))
	{
		UE_LOG(LogTemp, Log, TEXT("世界地图编辑器: 加载成功 - %s"), *MapName);
	}
}

void ULFPWorldMapEditorWidget::OnNewMapClicked()
{
	if (EditorComponent) EditorComponent->NewWorldMap();
}

// ============== UI 更新 ==============

void ULFPWorldMapEditorWidget::UpdateToolIndicator(ELFPWorldMapEditorTool NewTool)
{
	if (!CurrentToolText) return;

	static const TMap<ELFPWorldMapEditorTool, FString> ToolNames = {
		{ELFPWorldMapEditorTool::WMET_None, TEXT("无")},
		{ELFPWorldMapEditorTool::WMET_PlaceNode, TEXT("放置节点")},
		{ELFPWorldMapEditorTool::WMET_RemoveNode, TEXT("删除节点")},
		{ELFPWorldMapEditorTool::WMET_MoveNode, TEXT("移动节点")},
		{ELFPWorldMapEditorTool::WMET_ConnectEdge, TEXT("连接边")},
		{ELFPWorldMapEditorTool::WMET_RemoveEdge, TEXT("删除边")},
		{ELFPWorldMapEditorTool::WMET_SetNodeParams, TEXT("设置参数")},
		{ELFPWorldMapEditorTool::WMET_EditBattleMap, TEXT("编辑战斗地图")},
	};

	if (const FString* Name = ToolNames.Find(NewTool))
	{
		CurrentToolText->SetText(FText::FromString(*Name));
	}
}

void ULFPWorldMapEditorWidget::UpdateNodeInfo(ALFPWorldMapNode* Node)
{
	if (!SelectedNodeInfoText) return;

	if (!Node)
	{
		SelectedNodeInfoText->SetText(FText::FromString(TEXT("未选中")));
		return;
	}

	// 节点类型名
	FString TypeStr = UEnum::GetValueAsString(Node->NodeType);
	TypeStr = TypeStr.RightChop(TypeStr.Find(TEXT("::")) + 2);

	FString Info = FString::Printf(TEXT("ID: %d | 类型: %s"), Node->NodeID, *TypeStr);

	if (Node->NodeType == ELFPWorldNodeType::WNT_Battle)
	{
		Info += FString::Printf(TEXT(" | 星级: %d | 地图: %s | 金币: %d | 食物: %d"),
			Node->StarRating, *Node->BattleMapName, Node->BaseGoldReward, Node->BaseFoodReward);
	}
	else if (Node->NodeType == ELFPWorldNodeType::WNT_Event)
	{
		Info += FString::Printf(TEXT(" | 事件: %s"), *Node->EventID);
	}

	SelectedNodeInfoText->SetText(FText::FromString(Info));
}
