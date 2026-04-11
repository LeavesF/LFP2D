#include "LFP2D/UI/WorldMap/LFPWorldMapSystemMenuWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/UI/Menu/LFPSaveSlotListItem.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"

void ULFPWorldMapSystemMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Button_Resume) Button_Resume->OnClicked.AddDynamic(this, &ULFPWorldMapSystemMenuWidget::OnResumeClicked);
	if (Button_SaveGame) Button_SaveGame->OnClicked.AddDynamic(this, &ULFPWorldMapSystemMenuWidget::OnOpenSaveClicked);
	if (Button_LoadGame) Button_LoadGame->OnClicked.AddDynamic(this, &ULFPWorldMapSystemMenuWidget::OnOpenLoadClicked);
	if (Button_ReturnToMainMenu) Button_ReturnToMainMenu->OnClicked.AddDynamic(this, &ULFPWorldMapSystemMenuWidget::OnReturnToMainMenuClicked);
	if (Button_QuitGame) Button_QuitGame->OnClicked.AddDynamic(this, &ULFPWorldMapSystemMenuWidget::OnQuitClicked);
	if (Button_Back) Button_Back->OnClicked.AddDynamic(this, &ULFPWorldMapSystemMenuWidget::OnBackClicked);
	if (Button_Close) Button_Close->OnClicked.AddDynamic(this, &ULFPWorldMapSystemMenuWidget::OnCloseClicked);
	if (SaveSlotListView)
	{
		SaveSlotListView->OnItemClicked().AddUObject(this, &ULFPWorldMapSystemMenuWidget::OnSaveSlotItemClicked);
	}
	if (LoadSlotListView)
	{
		LoadSlotListView->OnItemClicked().AddUObject(this, &ULFPWorldMapSystemMenuWidget::OnLoadSlotItemClicked);
	}

	ShowMainPanel();
}

void ULFPWorldMapSystemMenuWidget::Setup(ULFPGameInstance* InGameInstance)
{
	GameInstance = InGameInstance;
	ShowMainPanel();
}

void ULFPWorldMapSystemMenuWidget::ShowMainPanel()
{
	SetVisiblePanel(Panel_MainMenu);
	if (Text_Title)
	{
		Text_Title->SetText(FText::FromString(TEXT("系统菜单")));
	}
}

void ULFPWorldMapSystemMenuWidget::ShowSavePanel()
{
	RefreshSaveSlotList();
	SetVisiblePanel(Panel_SaveSlots);
	if (Text_Title)
	{
		Text_Title->SetText(FText::FromString(TEXT("保存游戏")));
	}
}

void ULFPWorldMapSystemMenuWidget::ShowLoadPanel()
{
	RefreshLoadSlotList();
	SetVisiblePanel(Panel_LoadSlots);
	if (Text_Title)
	{
		Text_Title->SetText(FText::FromString(TEXT("读取存档")));
	}
}

void ULFPWorldMapSystemMenuWidget::OnSaveSlotItemClicked(UObject* Item)
{
	ULFPSaveSlotListItem* SlotItem = Cast<ULFPSaveSlotListItem>(Item);
	if (!SlotItem)
	{
		return;
	}

	OnSaveSlotSelected.Broadcast(SlotItem->SlotInfo.SlotIndex);
}

void ULFPWorldMapSystemMenuWidget::OnLoadSlotItemClicked(UObject* Item)
{
	ULFPSaveSlotListItem* SlotItem = Cast<ULFPSaveSlotListItem>(Item);
	if (!SlotItem || !SlotItem->SlotInfo.bIsValid)
	{
		return;
	}

	OnLoadSlotSelected.Broadcast(SlotItem->SlotInfo.SlotIndex);
}

void ULFPWorldMapSystemMenuWidget::OnResumeClicked()
{
	OnResumeRequested.Broadcast();
}

void ULFPWorldMapSystemMenuWidget::OnOpenSaveClicked()
{
	ShowSavePanel();
}

void ULFPWorldMapSystemMenuWidget::OnOpenLoadClicked()
{
	ShowLoadPanel();
}

void ULFPWorldMapSystemMenuWidget::OnReturnToMainMenuClicked()
{
	OnReturnToMainMenuRequested.Broadcast();
}

void ULFPWorldMapSystemMenuWidget::OnQuitClicked()
{
	OnQuitRequested.Broadcast();
}

void ULFPWorldMapSystemMenuWidget::OnBackClicked()
{
	ShowMainPanel();
}

void ULFPWorldMapSystemMenuWidget::OnCloseClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
	RemoveFromParent();
	OnClosed.Broadcast();
}

void ULFPWorldMapSystemMenuWidget::RefreshSaveSlotList()
{
	if (!GameInstance || !SaveSlotListView)
	{
		return;
	}

	SaveSlotListView->ClearListItems();

	for (int32 SlotIndex = 1; SlotIndex <= 10; ++SlotIndex)
	{
		ULFPSaveSlotListItem* Item = NewObject<ULFPSaveSlotListItem>(this);
		Item->SlotInfo = GameInstance->GetSaveSlotInfo(SlotIndex);
		if (!Item->SlotInfo.bIsValid)
		{
			Item->SlotInfo.SlotIndex = SlotIndex;
		}
		SaveSlotListView->AddItem(Item);
	}
}

void ULFPWorldMapSystemMenuWidget::RefreshLoadSlotList()
{
	if (!GameInstance || !LoadSlotListView)
	{
		return;
	}

	LoadSlotListView->ClearListItems();

	for (int32 SlotIndex = 1; SlotIndex <= 10; ++SlotIndex)
	{
		ULFPSaveSlotListItem* Item = NewObject<ULFPSaveSlotListItem>(this);
		Item->SlotInfo = GameInstance->GetSaveSlotInfo(SlotIndex);
		if (!Item->SlotInfo.bIsValid)
		{
			Item->SlotInfo.SlotIndex = SlotIndex;
		}
		LoadSlotListView->AddItem(Item);
	}
}

void ULFPWorldMapSystemMenuWidget::SetVisiblePanel(UCanvasPanel* VisiblePanel)
{
	if (Panel_MainMenu) Panel_MainMenu->SetVisibility(Panel_MainMenu == VisiblePanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Panel_SaveSlots) Panel_SaveSlots->SetVisibility(Panel_SaveSlots == VisiblePanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Panel_LoadSlots) Panel_LoadSlots->SetVisibility(Panel_LoadSlots == VisiblePanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

