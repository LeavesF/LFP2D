#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/Turn/LFPBattleTypes.h"
#include "LFP2D/HexGrid/LFPMapData.h"
#include "GameplayTagContainer.h"
#include "LFPMapEditorComponent.generated.h"

class ALFPHexGridManager;
class ALFPHexTile;

// 编辑器工具模式
UENUM(BlueprintType)
enum class ELFPMapEditorTool : uint8
{
	MET_None        UMETA(DisplayName = "无"),
	MET_Terrain     UMETA(DisplayName = "地形笔刷"),
	MET_Decoration  UMETA(DisplayName = "装饰笔刷"),
	MET_SpawnPoint  UMETA(DisplayName = "出生点"),
	MET_Event       UMETA(DisplayName = "事件标签"),
	MET_AddTile     UMETA(DisplayName = "添加格子"),
	MET_RemoveTile  UMETA(DisplayName = "移除格子")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEditorToolChanged, ELFPMapEditorTool, NewTool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEditorModeChanged, bool, bEditorActive);

/**
 * 地图编辑器组件：管理编辑器状态、工具切换、笔刷参数
 * 挂载在 PlayerController 上
 */
UCLASS(Blueprintable, ClassGroup=(MapEditor), meta=(BlueprintSpawnableComponent))
class LFP2D_API ULFPMapEditorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULFPMapEditorComponent();

	// 切换编辑器模式
	UFUNCTION(BlueprintCallable, Category = "Map Editor")
	void ToggleEditorMode();

	UFUNCTION(BlueprintPure, Category = "Map Editor")
	bool IsEditorActive() const { return bEditorActive; }

	// 设置当前工具
	UFUNCTION(BlueprintCallable, Category = "Map Editor")
	void SetCurrentTool(ELFPMapEditorTool Tool);

	UFUNCTION(BlueprintPure, Category = "Map Editor")
	ELFPMapEditorTool GetCurrentTool() const { return CurrentTool; }

	// 笔刷参数设置
	UFUNCTION(BlueprintCallable, Category = "Map Editor|Brush")
	void SetBrushTerrainType(ELFPTerrainType InType) { BrushTerrainType = InType; }

	UFUNCTION(BlueprintCallable, Category = "Map Editor|Brush")
	void SetBrushDecorationID(FName InID) { BrushDecorationID = InID; }

	UFUNCTION(BlueprintCallable, Category = "Map Editor|Brush")
	void SetBrushSpawnFaction(ELFPSpawnFaction InFaction) { BrushSpawnFaction = InFaction; }

	UFUNCTION(BlueprintCallable, Category = "Map Editor|Brush")
	void SetBrushSpawnIndex(int32 InIndex) { BrushSpawnIndex = InIndex; }

	UFUNCTION(BlueprintCallable, Category = "Map Editor|Brush")
	void SetBrushEventTag(FGameplayTag InTag) { BrushEventTag = InTag; }

	// 对已有格子应用当前工具
	UFUNCTION(BlueprintCallable, Category = "Map Editor")
	void ApplyToolToTile(ALFPHexTile* Tile);

	// 对空位坐标应用工具（添加格子用）
	UFUNCTION(BlueprintCallable, Category = "Map Editor")
	void ApplyToolToCoord(int32 Q, int32 R);

	// 保存/加载
	UFUNCTION(BlueprintCallable, Category = "Map Editor")
	bool SaveMap(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "Map Editor")
	bool LoadMap(const FString& FileName);

	// 获取保存目录下所有地图文件列表
	UFUNCTION(BlueprintCallable, Category = "Map Editor")
	TArray<FString> GetSavedMapList();

	// 创建新的空白地图
	UFUNCTION(BlueprintCallable, Category = "Map Editor")
	void NewMap(int32 Width = 10, int32 Height = 10);

	// 委托
	UPROPERTY(BlueprintAssignable, Category = "Map Editor")
	FOnEditorToolChanged OnEditorToolChanged;

	UPROPERTY(BlueprintAssignable, Category = "Map Editor")
	FOnEditorModeChanged OnEditorModeChanged;

protected:
	virtual void BeginPlay() override;

	// 获取 GridManager 引用
	ALFPHexGridManager* GetGridManager() const;

	// 获取地图保存目录
	FString GetMapSaveDirectory() const;

protected:
	// 编辑器是否激活
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Editor")
	bool bEditorActive = false;

	// 当前工具
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Editor")
	ELFPMapEditorTool CurrentTool = ELFPMapEditorTool::MET_None;

	// 笔刷参数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Editor|Brush")
	ELFPTerrainType BrushTerrainType = ELFPTerrainType::TT_Grass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Editor|Brush")
	FName BrushDecorationID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Editor|Brush")
	ELFPSpawnFaction BrushSpawnFaction = ELFPSpawnFaction::SF_Player;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Editor|Brush")
	int32 BrushSpawnIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Editor|Brush")
	FGameplayTag BrushEventTag;

	// GridManager 缓存
	UPROPERTY()
	mutable TObjectPtr<ALFPHexGridManager> CachedGridManager;
};
