#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UComboBoxString;
class UVerticalBox;
class UCanvasPanel;

/**
 * Main Menu Widget
 * Provides: Start Game, Continue, Load Game, Quit
 * Start Game -> opens world map selection -> starts new game on chosen map
 * Continue -> loads the most recent save
 * Load Game -> shows save slot list for selection
 *
 * Expected UMG Blueprint bindings:
 *   - Btn_StartGame, Btn_Continue, Btn_LoadGame, Btn_Quit
 *   - Panel_MainMenu, Panel_WorldMapSelection, Panel_LoadGame
 *   - WorldMapComboBox, Btn_ConfirmWorldMap, Btn_BackFromWorldMap
 *   - Box_SaveSlotList, Btn_BackFromLoad
 *   - (Optional) Btn_Slot_1 through Btn_Slot_10 for save slot buttons
 *   - (Optional) Text_Slot_1 through Text_Slot_10 for save slot text
 */
UCLASS()
class LFP2D_API ULFPMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	// Button callbacks
	UFUNCTION() void OnStartGameClicked();
	UFUNCTION() void OnContinueClicked();
	UFUNCTION() void OnLoadGameClicked();
	UFUNCTION() void OnQuitClicked();

	// World map selection
	UFUNCTION() void OnWorldMapSelected();
	UFUNCTION() void OnBackFromWorldMap();

	// Load game selection
	UFUNCTION() void OnBackFromLoadGame();

	// Save slot button callbacks (one per slot)
	UFUNCTION() void OnSlot1Clicked();
	UFUNCTION() void OnSlot2Clicked();
	UFUNCTION() void OnSlot3Clicked();
	UFUNCTION() void OnSlot4Clicked();
	UFUNCTION() void OnSlot5Clicked();
	UFUNCTION() void OnSlot6Clicked();
	UFUNCTION() void OnSlot7Clicked();
	UFUNCTION() void OnSlot8Clicked();
	UFUNCTION() void OnSlot9Clicked();
	UFUNCTION() void OnSlot10Clicked();

	void OnSlotClicked(int32 SlotIndex);

	// UI state management
	void ShowMainMenu();
	void ShowWorldMapSelection();
	void ShowLoadGame();
	void UpdateSaveSlotList();

protected:
	// Main menu buttons
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_StartGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Continue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_LoadGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Quit;

	// World map selection panel
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_WorldMapSelection;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> WorldMapComboBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_ConfirmWorldMap;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_BackFromWorldMap;

	// Load game panel
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_LoadGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> Box_SaveSlotList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_BackFromLoad;

	// Main menu panel
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_MainMenu;

	// Save slot buttons and text (optional, defined in Blueprint)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_3;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_4;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_5;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_6;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_7;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_8;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_9;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Slot_10;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_3;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_4;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_5;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_6;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_7;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_8;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_9;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Slot_10;

	// World map display names and corresponding save/level names
	UPROPERTY(EditAnywhere, Category = "World Map")
	TArray<FString> WorldMapDisplayNames;

	UPROPERTY(EditAnywhere, Category = "World Map")
	TArray<FString> WorldMapLevelNames;

	UPROPERTY(EditAnywhere, Category = "World Map")
	TArray<FString> WorldMapSaveNames;

	UPROPERTY(EditAnywhere, Category = "World Map")
	int32 DefaultStartNodeID = 0;
};
