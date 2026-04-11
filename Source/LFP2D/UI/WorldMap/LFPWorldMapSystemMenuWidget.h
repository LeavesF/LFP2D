#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LFPWorldMapSystemMenuWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;
class UListView;
class UObject;
class ULFPGameInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldMapSystemMenuClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldMapSystemMenuResumeRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldMapSystemMenuReturnToMainMenuRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWorldMapSystemMenuQuitRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldMapSystemMenuSaveSlotSelected, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldMapSystemMenuLoadSlotSelected, int32, SlotIndex);

/**
 * 世界地图系统菜单 Widget
 * 提供继续/保存/读档/返回主菜单/退出游戏等功能
 */
UCLASS()
class LFP2D_API ULFPWorldMapSystemMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "System Menu")
	void Setup(ULFPGameInstance* InGameInstance);

	UFUNCTION(BlueprintCallable, Category = "System Menu")
	void ShowMainPanel();

	UFUNCTION(BlueprintCallable, Category = "System Menu")
	void ShowSavePanel();

	UFUNCTION(BlueprintCallable, Category = "System Menu")
	void ShowLoadPanel();

	UFUNCTION()
	void OnSaveSlotItemClicked(UObject* Item);

	UFUNCTION()
	void OnLoadSlotItemClicked(UObject* Item);

	UPROPERTY(BlueprintAssignable)
	FOnWorldMapSystemMenuClosed OnClosed;

	UPROPERTY(BlueprintAssignable)
	FOnWorldMapSystemMenuResumeRequested OnResumeRequested;

	UPROPERTY(BlueprintAssignable)
	FOnWorldMapSystemMenuSaveSlotSelected OnSaveSlotSelected;

	UPROPERTY(BlueprintAssignable)
	FOnWorldMapSystemMenuLoadSlotSelected OnLoadSlotSelected;

	UPROPERTY(BlueprintAssignable)
	FOnWorldMapSystemMenuReturnToMainMenuRequested OnReturnToMainMenuRequested;

	UPROPERTY(BlueprintAssignable)
	FOnWorldMapSystemMenuQuitRequested OnQuitRequested;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION() void OnResumeClicked();
	UFUNCTION() void OnOpenSaveClicked();
	UFUNCTION() void OnOpenLoadClicked();
	UFUNCTION() void OnReturnToMainMenuClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnBackClicked();
	UFUNCTION() void OnCloseClicked();

	void RefreshSaveSlotList();
	void RefreshLoadSlotList();
	void SetVisiblePanel(UCanvasPanel* VisiblePanel);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_MainMenu;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_SaveSlots;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Panel_LoadSlots;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Resume;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SaveGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_LoadGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ReturnToMainMenu;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_QuitGame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Back;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Close;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UListView> SaveSlotListView;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UListView> LoadSlotListView;

private:
	UPROPERTY()
	TObjectPtr<ULFPGameInstance> GameInstance;
};
