#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPMainMenuWidget.generated.h"

class UButton;
class UCanvasPanel;
class UListView;
class ULFPWorldMapEntryWidget;
class ULFPSaveSlotEntryWidget;

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
 *   - WorldMapListView, Btn_ConfirmWorldMap, Btn_BackFromWorldMap
 *   - SaveSlotListView, Btn_BackFromLoad
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

	void OnSlotClicked(int32 SlotIndex);
	void OnSaveSlotItemClicked(UObject* Item);

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
	TObjectPtr<UListView> WorldMapListView;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_ConfirmWorldMap;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_BackFromWorldMap;

	// Load game panel
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_LoadGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UListView> SaveSlotListView;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_BackFromLoad;

	// Main menu panel
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_MainMenu;

	UPROPERTY(EditAnywhere, Category = "World Map")
	TSubclassOf<ULFPWorldMapEntryWidget> WorldMapEntryWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Save System")
	TSubclassOf<ULFPSaveSlotEntryWidget> SaveSlotEntryWidgetClass;

	UPROPERTY(EditAnywhere, Category = "World Map")
	FString WorldMapLevelName;

	UPROPERTY(EditAnywhere, Category = "World Map")
	int32 DefaultStartNodeID = 0;
};
