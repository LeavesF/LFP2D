#include "LFP2D/UI/Menu/LFPMainMenuWidget.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/VerticalBox.h"
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

	// Bind save slot buttons
	if (Btn_Slot_1) Btn_Slot_1->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot1Clicked);
	if (Btn_Slot_2) Btn_Slot_2->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot2Clicked);
	if (Btn_Slot_3) Btn_Slot_3->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot3Clicked);
	if (Btn_Slot_4) Btn_Slot_4->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot4Clicked);
	if (Btn_Slot_5) Btn_Slot_5->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot5Clicked);
	if (Btn_Slot_6) Btn_Slot_6->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot6Clicked);
	if (Btn_Slot_7) Btn_Slot_7->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot7Clicked);
	if (Btn_Slot_8) Btn_Slot_8->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot8Clicked);
	if (Btn_Slot_9) Btn_Slot_9->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot9Clicked);
	if (Btn_Slot_10) Btn_Slot_10->OnClicked.AddDynamic(this, &ULFPMainMenuWidget::OnSlot10Clicked);

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

	// Populate world map combo box
	if (WorldMapComboBox)
	{
		WorldMapComboBox->ClearOptions();
		for (const FString& DisplayName : WorldMapDisplayNames)
		{
			WorldMapComboBox->AddOption(DisplayName);
		}
		if (WorldMapDisplayNames.Num() > 0)
		{
			WorldMapComboBox->SetSelectedIndex(0);
		}
	}
}

void ULFPMainMenuWidget::OnWorldMapSelected()
{
	if (!WorldMapComboBox) return;

	int32 SelectedIndex = WorldMapComboBox->GetSelectedIndex();
	if (SelectedIndex < 0 || SelectedIndex >= WorldMapSaveNames.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnWorldMapSelected: Invalid selection index %d"), SelectedIndex);
		return;
	}

	FString WorldMapName = WorldMapSaveNames[SelectedIndex];
	FString LevelName;
	if (SelectedIndex < WorldMapLevelNames.Num())
	{
		LevelName = WorldMapLevelNames[SelectedIndex];
	}
	else
	{
		LevelName = WorldMapName;
	}

	ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->StartNewWorldMapGame(WorldMapName, DefaultStartNodeID);

	UE_LOG(LogTemp, Log, TEXT("Starting new game on world map: %s, Level: %s"), *WorldMapName, *LevelName);
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
	if (!GI) return;

	// Array of text blocks for slots 1-10
	TArray<UTextBlock*> TextBlocks = {
		Text_Slot_1, Text_Slot_2, Text_Slot_3, Text_Slot_4, Text_Slot_5,
		Text_Slot_6, Text_Slot_7, Text_Slot_8, Text_Slot_9, Text_Slot_10
	};

	TArray<UButton*> SlotButtons = {
		Btn_Slot_1, Btn_Slot_2, Btn_Slot_3, Btn_Slot_4, Btn_Slot_5,
		Btn_Slot_6, Btn_Slot_7, Btn_Slot_8, Btn_Slot_9, Btn_Slot_10
	};

	for (int32 i = 0; i < 10; i++)
	{
		FLFPSaveSlotInfo Info = GI->GetSaveSlotInfo(i + 1);

		// Update text
		if (TextBlocks.IsValidIndex(i) && TextBlocks[i])
		{
			if (Info.bIsValid)
			{
				FString TimeStr = Info.Timestamp.ToString();
				TextBlocks[i]->SetText(FText::FromString(FString::Printf(TEXT("Slot %d: %s\n%s | Turn %d\n%s"),
					Info.SlotIndex, *Info.SaveName, *Info.WorldMapName, Info.CurrentTurn, *TimeStr)));
			}
			else
			{
				TextBlocks[i]->SetText(FText::FromString(FString::Printf(TEXT("Slot %d: Empty"), i + 1)));
			}
		}

		// Update button enabled state (enable even if empty to allow overwriting)
		if (SlotButtons.IsValidIndex(i) && SlotButtons[i])
		{
			SlotButtons[i]->SetIsEnabled(true);
		}
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

void ULFPMainMenuWidget::OnSlot1Clicked() { OnSlotClicked(1); }
void ULFPMainMenuWidget::OnSlot2Clicked() { OnSlotClicked(2); }
void ULFPMainMenuWidget::OnSlot3Clicked() { OnSlotClicked(3); }
void ULFPMainMenuWidget::OnSlot4Clicked() { OnSlotClicked(4); }
void ULFPMainMenuWidget::OnSlot5Clicked() { OnSlotClicked(5); }
void ULFPMainMenuWidget::OnSlot6Clicked() { OnSlotClicked(6); }
void ULFPMainMenuWidget::OnSlot7Clicked() { OnSlotClicked(7); }
void ULFPMainMenuWidget::OnSlot8Clicked() { OnSlotClicked(8); }
void ULFPMainMenuWidget::OnSlot9Clicked() { OnSlotClicked(9); }
void ULFPMainMenuWidget::OnSlot10Clicked() { OnSlotClicked(10); }
