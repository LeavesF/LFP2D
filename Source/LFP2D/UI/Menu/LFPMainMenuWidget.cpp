#include "LFP2D/UI/Menu/LFPMainMenuWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/UI/Menu/LFPWorldMapListItem.h"
#include "LFP2D/UI/Menu/LFPSaveSlotListItem.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/CanvasPanel.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

void ULFPMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind main menu buttons
	if (Btn_StartGame) Btn_StartGame->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnStartGameClicked);
	if (Btn_Continue) Btn_Continue->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnContinueClicked);
	if (Btn_LoadGame) Btn_LoadGame->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnLoadGameClicked);
	if (Btn_Quit) Btn_Quit->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnQuitClicked);

	// Bind world map selection
	if (Btn_ConfirmWorldMap) Btn_ConfirmWorldMap->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnWorldMapSelected);
	if (Btn_BackFromWorldMap) Btn_BackFromWorldMap->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnBackFromWorldMap);

	// Bind load game back
	if (Btn_BackFromLoad) Btn_BackFromLoad->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnBackFromLoadGame);

	if (SaveSlotListView)
	{
		SaveSlotListView->OnItemClicked().AddUObject(this, &ULFPMainMenuWidget::OnSaveSlotItemClicked);
	}

	// Show main menu initially
	ShowMainMenu();
}

// ============== Main Menu ==============

void ULFPMainMenuWidget::ShowMainMenu()
{
	if (Panel_MainMenu) Panel_MainMenu->SetVisibility(ESlateVisibility::Visible);
	if (Panel_WorldMapSelection) Panel_WorldMapSelection->SetVisibility(ESlateVisibility::Hidden);
	if (Panel_LoadGame) Panel_LoadGame->SetVisibility(ESlateVisibility::Hidden);

	// Update Continue button enabled state
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (Btn_Continue && GI)
	{
		FLFPSaveSlotInfo Latest = GI->GetLatestSaveSlotInfo();
		Btn_Continue->SetIsEnabled(Latest.bIsValid);
	}
}

void ULFPMainMenuWidget::OnStartGameClicked()
{
	ShowWorldMapSelection();
}

void ULFPMainMenuWidget::OnContinueClicked()
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI) return;

	FLFPSaveSlotInfo Latest = GI->GetLatestSaveSlotInfo();
	if (!Latest.bIsValid) return;

	if (GI->LoadGame(Latest.SlotIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("Continue: Loading most recent save from slot %d"), Latest.SlotIndex);
		GI->TransitionToWorldMap(GI->DefaultWorldMapLevelName);
	}
}

void ULFPMainMenuWidget::OnLoadGameClicked()
{
	ShowLoadGame();
}

void ULFPMainMenuWidget::OnQuitClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		UKismetSystemLibrary::QuitGame(PC, nullptr, EQuitPreference::Quit, false);
	}
}

// ============== World Map Selection ==============

void ULFPMainMenuWidget::ShowWorldMapSelection()
{
	if (Panel_MainMenu) Panel_MainMenu->SetVisibility(ESlateVisibility::Hidden);
	if (Panel_WorldMapSelection) Panel_WorldMapSelection->SetVisibility(ESlateVisibility::Visible);
	if (Panel_LoadGame) Panel_LoadGame->SetVisibility(ESlateVisibility::Hidden);

	if (WorldMapListView)
	{
		WorldMapListView->ClearListItems();

		TArray<FString> MapNames = ULFPGameInstance::GetAvailableWorldMapNames();
		for (const FString& MapName : MapNames)
		{
			ULFPWorldMapListItem* Item = NewObject<ULFPWorldMapListItem>(this);
			Item->MapName = MapName;
			WorldMapListView->AddItem(Item);
		}

		const bool bHasMaps = MapNames.Num() > 0;
		if (Btn_ConfirmWorldMap)
		{
			Btn_ConfirmWorldMap->SetIsEnabled(bHasMaps);
		}

		if (bHasMaps && WorldMapListView->GetNumItems() > 0)
		{
			WorldMapListView->SetSelectedItem(WorldMapListView->GetItemAt(0));
		}
	}
}

void ULFPMainMenuWidget::OnWorldMapSelected()
{
	if (!WorldMapListView) return;

	ULFPWorldMapListItem* SelectedItem = Cast<ULFPWorldMapListItem>(WorldMapListView->GetSelectedItem());
	if (!SelectedItem || SelectedItem->MapName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnWorldMapSelected: No world map selected"));
		return;
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->StartNewWorldMapGame(SelectedItem->MapName, DefaultStartNodeID);

	const FString LevelName = WorldMapLevelName.IsEmpty() ? GI->DefaultWorldMapLevelName : WorldMapLevelName;

	UE_LOG(LogTemp, Log, TEXT("Starting new game on world map: %s, Level: %s"), *SelectedItem->MapName, *LevelName);
	GI->TransitionToWorldMap(LevelName);
}

void ULFPMainMenuWidget::OnBackFromWorldMap()
{
	ShowMainMenu();
}

// ============== Load Game ==============

void ULFPMainMenuWidget::ShowLoadGame()
{
	if (Panel_MainMenu) Panel_MainMenu->SetVisibility(ESlateVisibility::Hidden);
	if (Panel_WorldMapSelection) Panel_WorldMapSelection->SetVisibility(ESlateVisibility::Hidden);
	if (Panel_LoadGame) Panel_LoadGame->SetVisibility(ESlateVisibility::Visible);

	UpdateSaveSlotList();
}

void ULFPMainMenuWidget::UpdateSaveSlotList()
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI || !SaveSlotListView) return;

	SaveSlotListView->ClearListItems();

	for (int32 i = 1; i <= 10; i++)
	{
		ULFPSaveSlotListItem* Item = NewObject<ULFPSaveSlotListItem>(this);
		Item->SlotInfo = GI->GetSaveSlotInfo(i);
		if (!Item->SlotInfo.bIsValid)
		{
			Item->SlotInfo.SlotIndex = i;
		}
		SaveSlotListView->AddItem(Item);
	}
}

void ULFPMainMenuWidget::OnBackFromLoadGame()
{
	ShowMainMenu();
}

// ============== Save Slot Click Handlers ==============

void ULFPMainMenuWidget::OnSlotClicked(int32 SlotIndex)
{
	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI) return;

	// If save exists, load it; otherwise do nothing (could add save naming UI later)
	if (GI->DoesSaveExist(SlotIndex))
	{
		if (GI->LoadGame(SlotIndex))
		{
			UE_LOG(LogTemp, Log, TEXT("Load Game: Loaded from slot %d"), SlotIndex);
			GI->TransitionToWorldMap(GI->DefaultWorldMapLevelName);
		}
	}
}

void ULFPMainMenuWidget::OnSaveSlotItemClicked(UObject* Item)
{
	ULFPSaveSlotListItem* SlotItem = Cast<ULFPSaveSlotListItem>(Item);
	if (!SlotItem || !SlotItem->SlotInfo.bIsValid)
	{
		return;
	}

	OnSlotClicked(SlotItem->SlotInfo.SlotIndex);
}
