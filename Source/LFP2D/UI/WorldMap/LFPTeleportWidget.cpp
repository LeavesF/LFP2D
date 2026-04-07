#include "LFP2D/UI/WorldMap/LFPTeleportWidget.h"
#include "LFP2D/WorldMap/LFPWorldMapNode.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void ULFPTeleportWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &ULFPTeleportWidget::OnCloseClicked);
}

void ULFPTeleportWidget::Setup(const TArray<ALFPWorldMapNode*>& TargetNodes)
{
	// 清理旧按钮
	for (UButton* Btn : DestinationButtons)
	{
		if (Btn && Btn->GetParent())
		{
			Btn->RemoveFromParent();
		}
	}
	DestinationButtons.Empty();
	ButtonToNodeIDMap.Empty();

	if (!Box_DestinationList) return;

	Box_DestinationList->ClearChildren();

	for (ALFPWorldMapNode* Node : TargetNodes)
	{
		if (!Node) continue;

		// 创建按钮
		UButton* Btn = NewObject<UButton>(this);
		if (!Btn) continue;

		// 创建按钮内文字
		UTextBlock* Label = NewObject<UTextBlock>(Btn);
		if (Label)
		{
			Label->SetText(FText::FromString(FString::Printf(TEXT("城镇 %d"), Node->NodeID)));
		}

		Btn->AddChild(Label);
		Btn->OnClicked.AddDynamic(this, &ULFPTeleportWidget::OnDestinationClicked);

		Box_DestinationList->AddChild(Btn);
		DestinationButtons.Add(Btn);
		ButtonToNodeIDMap.Add(Btn, Node->NodeID);
	}

	if (TargetNodes.Num() == 0 && Text_Title)
	{
		Text_Title->SetText(FText::FromString(TEXT("没有可传送的目的地")));
	}
	else if (Text_Title)
	{
		Text_Title->SetText(FText::FromString(TEXT("选择传送目的地")));
	}
}

void ULFPTeleportWidget::OnDestinationClicked()
{
	// 查找被点击的按钮
	for (const auto& Pair : ButtonToNodeIDMap)
	{
		if (Pair.Key && Pair.Key->IsPressed())
		{
			OnTeleportTargetSelected.Broadcast(Pair.Value);
			return;
		}
	}
}

void ULFPTeleportWidget::OnCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
	OnClosed.Broadcast();
}
