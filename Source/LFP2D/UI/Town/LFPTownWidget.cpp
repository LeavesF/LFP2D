#include "LFP2D/UI/Town/LFPTownWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void ULFPTownWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Shop) Button_Shop->OnClicked.AddDynamic(this, &ULFPTownWidget::OnShopClicked);
	if (Button_EvolutionTower) Button_EvolutionTower->OnClicked.AddDynamic(this, &ULFPTownWidget::OnEvolutionTowerClicked);
	if (Button_Teleport) Button_Teleport->OnClicked.AddDynamic(this, &ULFPTownWidget::OnTeleportClicked);
	if (Button_QuestNPC) Button_QuestNPC->OnClicked.AddDynamic(this, &ULFPTownWidget::OnQuestNPCClicked);
	if (Button_SkillNode) Button_SkillNode->OnClicked.AddDynamic(this, &ULFPTownWidget::OnSkillNodeClicked);
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &ULFPTownWidget::OnCloseClicked);
}

void ULFPTownWidget::Setup(const TArray<ELFPTownBuildingType>& AvailableBuildings)
{
	// 先全部隐藏
	HideAllBuildings();

	// 根据城镇配置显示对应建筑
	for (ELFPTownBuildingType Type : AvailableBuildings)
	{
		switch (Type)
		{
		case ELFPTownBuildingType::TBT_Shop:
			if (Button_Shop) Button_Shop->SetVisibility(ESlateVisibility::Visible);
			break;
		case ELFPTownBuildingType::TBT_EvolutionTower:
			if (Button_EvolutionTower) Button_EvolutionTower->SetVisibility(ESlateVisibility::Visible);
			break;
		case ELFPTownBuildingType::TBT_Teleport:
			if (Button_Teleport) Button_Teleport->SetVisibility(ESlateVisibility::Visible);
			break;
		case ELFPTownBuildingType::TBT_QuestNPC:
			if (Button_QuestNPC) Button_QuestNPC->SetVisibility(ESlateVisibility::Visible);
			break;
		case ELFPTownBuildingType::TBT_SkillNode:
			if (Button_SkillNode) Button_SkillNode->SetVisibility(ESlateVisibility::Visible);
			break;
		default:
			break;
		}
	}
}

void ULFPTownWidget::HideAllBuildings()
{
	if (Button_Shop) Button_Shop->SetVisibility(ESlateVisibility::Collapsed);
	if (Button_EvolutionTower) Button_EvolutionTower->SetVisibility(ESlateVisibility::Collapsed);
	if (Button_Teleport) Button_Teleport->SetVisibility(ESlateVisibility::Collapsed);
	if (Button_QuestNPC) Button_QuestNPC->SetVisibility(ESlateVisibility::Collapsed);
	if (Button_SkillNode) Button_SkillNode->SetVisibility(ESlateVisibility::Collapsed);
}

void ULFPTownWidget::OnShopClicked()
{
	OnBuildingRequested.Broadcast(ELFPTownBuildingType::TBT_Shop);
}

void ULFPTownWidget::OnEvolutionTowerClicked()
{
	OnBuildingRequested.Broadcast(ELFPTownBuildingType::TBT_EvolutionTower);
}

void ULFPTownWidget::OnTeleportClicked()
{
	OnBuildingRequested.Broadcast(ELFPTownBuildingType::TBT_Teleport);
}

void ULFPTownWidget::OnQuestNPCClicked()
{
	OnBuildingRequested.Broadcast(ELFPTownBuildingType::TBT_QuestNPC);
}

void ULFPTownWidget::OnSkillNodeClicked()
{
	OnBuildingRequested.Broadcast(ELFPTownBuildingType::TBT_SkillNode);
}

void ULFPTownWidget::OnCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
	OnClosed.Broadcast();
}
