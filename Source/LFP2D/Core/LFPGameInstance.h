#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LFP2D/Unit/LFPUnitTypes.h"
#include "LFP2D/Shop/LFPRelicTypes.h"
#include "LFP2D/Shop/LFPHireMarketTypes.h"
#include "LFPGameInstance.generated.h"

class ALFPTacticsUnit;
class ULFPUnitRegistryDataAsset;
class ULFPRelicDataAsset;
class ULFPShopDataAsset;
class ULFPHireMarketDataAsset;

// 资源（金币/食物）变动委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResourceChanged, int32, NewGold, int32, NewFood);
// 遗物拥有列表变动委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOwnedRelicsChanged);

// 战斗请求：世界地图 → 战斗场景传递的数据
USTRUCT(BlueprintType)
struct FLFPBattleRequest
{
	GENERATED_BODY()

	// 触发战斗的节点 ID
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	int32 SourceNodeID = -1;

	// 战斗地图 CSV 文件名（对应 Saved/Maps/ 下的 CSV）
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	FString BattleMapName;

	// 敌人星级
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	int32 StarRating = 1;

	// 是否可逃跑
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bCanEscape = true;

	// 节点基础金币奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle")
	int32 BaseGoldReward = 0;

	// 节点基础食物奖励
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle")
	int32 BaseFoodReward = 0;

	// 是否有有效请求
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bIsValid = false;
};

// 战斗结果：战斗场景 → 世界地图传递的数据
USTRUCT(BlueprintType)
struct FLFPBattleResult
{
	GENERATED_BODY()

	// 对应的节点 ID
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	int32 SourceNodeID = -1;

	// 是否胜利
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bVictory = false;

	// 是否逃跑
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bEscaped = false;

	// 战斗中捕获的单位列表（背叛系统触发）
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	TArray<FLFPUnitEntry> CapturedUnits;

	// 总金币奖励（基础 + 击杀掉落）
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	int32 GoldReward = 0;

	// 总食物奖励（基础 + 击杀掉落）
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	int32 FoodReward = 0;

	// 是否有有效结果
	UPROPERTY(BlueprintReadWrite, Category = "Battle")
	bool bIsValid = false;
};

// 世界地图状态快照：跨关卡保持
USTRUCT(BlueprintType)
struct FLFPWorldMapSnapshot
{
	GENERATED_BODY()

	// 当前世界地图名
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	FString WorldMapName;

	// 玩家当前节点 ID
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	int32 CurrentNodeID = 0;

	// 已消耗回合数
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	int32 CurrentTurn = 0;

	// 已访问节点
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	TSet<int32> VisitedNodeIDs;

	// 已揭露节点
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	TSet<int32> RevealedNodeIDs;

	// 已触发节点（战斗/事件/Boss 首次触发后不再触发）
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	TSet<int32> TriggeredNodeIDs;

	// 是否有有效快照
	UPROPERTY(BlueprintReadWrite, Category = "World Map")
	bool bIsValid = false;
};

// 保存槽信息
USTRUCT(BlueprintType)
struct FLFPSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 SlotIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString SaveName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FDateTime Timestamp;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString WorldMapName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 CurrentTurn = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bIsValid = false;
};

// 存档数据
USTRUCT(BlueprintType)
struct FLFPSaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString SaveName;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FDateTime Timestamp;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FLFPWorldMapSnapshot WorldMapSnapshot;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 Food = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FLFPUnitEntry> PartyUnits;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FLFPUnitEntry> ReserveUnits;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FName> OwnedRelicIDs;

	// 雇佣市场已购买（扁平化存储，Key=Value 对）
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	TArray<FString> PurchasedHireMarketEntries;
};

/**
 * 游戏实例：跨关卡生命周期
 * 负责世界地图 ↔ 战斗场景之间的状态传递
 */
UCLASS()
class LFP2D_API ULFPGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ============== 保存/加载 ==============

	// 保存游戏到指定槽位（1-based）
	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool SaveGame(int32 SlotIndex, const FString& SaveName);

	// 从指定槽位加载游戏
	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool LoadGame(int32 SlotIndex);

	// 获取槽位信息
	UFUNCTION(BlueprintCallable, Category = "Save System")
	FLFPSaveSlotInfo GetSaveSlotInfo(int32 SlotIndex) const;

	// 获取所有有效槽位列表
	UFUNCTION(BlueprintCallable, Category = "Save System")
	TArray<FLFPSaveSlotInfo> GetValidSaveSlots() const;

	// 获取最近一次存档
	UFUNCTION(BlueprintCallable, Category = "Save System")
	FLFPSaveSlotInfo GetLatestSaveSlotInfo() const;

	// 检查槽位是否有存档
	UFUNCTION(BlueprintPure, Category = "Save System")
	bool DoesSaveExist(int32 SlotIndex) const;

	// 删除指定槽位存档
	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool DeleteSave(int32 SlotIndex);

	// 将当前游戏状态打包为保存数据
	FLFPSaveData PackSaveData(const FString& SaveName) const;

	// 从保存数据解包到当前游戏状态
	void UnpackSaveData(const FLFPSaveData& Data);

	// 重置所有游戏状态为新游戏
	UFUNCTION(BlueprintCallable, Category = "Save System")
	void ResetForNewGame();

	// 开始指定世界地图的新游戏
	UFUNCTION(BlueprintCallable, Category = "Save System")
	void StartNewWorldMapGame(const FString& WorldMapName, int32 StartNodeID = 0);

	// 扫描 Saved/WorldMaps/ 返回可用世界地图名列表（无需 Actor 依赖）
	UFUNCTION(BlueprintCallable, Category = "World Map")
	static TArray<FString> GetAvailableWorldMapNames();

	// ============== 战斗请求/结果 ==============

	// 设置战斗请求（世界地图调用，进入战斗前）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void SetBattleRequest(const FLFPBattleRequest& Request);

	// 获取战斗请求（战斗场景调用，读取后清除）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	FLFPBattleRequest ConsumeBattleRequest();

	// 设置战斗结果（战斗场景调用，结束战斗时）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void SetBattleResult(const FLFPBattleResult& Result);

	// 获取战斗结果（世界地图调用，读取后清除）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	FLFPBattleResult ConsumeBattleResult();

	// ============== 世界地图快照 ==============

	// 保存世界地图状态快照
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void SaveWorldMapSnapshot(const FLFPWorldMapSnapshot& Snapshot);

	// 获取世界地图快照（不清除，世界地图关卡可多次读取）
	UFUNCTION(BlueprintPure, Category = "Scene Transition")
	const FLFPWorldMapSnapshot& GetWorldMapSnapshot() const { return WorldMapSnapshot; }

	// 是否有待处理的战斗结果
	UFUNCTION(BlueprintPure, Category = "Scene Transition")
	bool HasPendingBattleResult() const { return PendingBattleResult.bIsValid; }

	// ============== 场景切换 ==============

	// 切换到战斗场景（带淡出过场）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void TransitionToBattle(const FString& BattleLevelName);

	// 切换回世界地图（带淡出过场）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void TransitionToWorldMap(const FString& WorldMapLevelName);

	// 切换到主菜单（带淡出过场）
	UFUNCTION(BlueprintCallable, Category = "Scene Transition")
	void TransitionToMainMenu(const FString& MainMenuLevelName = TEXT(""));

	// 过场动画时长（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Transition")
	float TransitionFadeDuration = 0.5f;

	// 是否正在过场中（防止重复触发）
	UPROPERTY(BlueprintReadOnly, Category = "Scene Transition")
	bool bIsTransitioning = false;

	// 世界地图关卡名（蓝图中配置，默认值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Transition")
	FString DefaultWorldMapLevelName = TEXT("Test_WorldMap");

	// 战斗关卡名（蓝图中配置，默认值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Transition")
	FString DefaultBattleLevelName = TEXT("Test_Fight");

	// 主菜单关卡名（蓝图中配置，默认值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Transition")
	FString DefaultMainMenuLevelName = TEXT("MainMenu");

	// ============== 资源系统 ==============

	// 金币
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Gold = 0;

	// 食物
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
	int32 Food = 0;

	// 增加金币
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddGold(int32 Amount);

	// 增加食物
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddFood(int32 Amount);

	// 获取金币
	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetGold() const { return Gold; }

	// 获取食物
	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetFood() const { return Food; }

	// 是否有足够金币
	UFUNCTION(BlueprintPure, Category = "Resources")
	bool CanAffordGold(int32 Amount) const { return Gold >= Amount; }

	// 扣除金币
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool SpendGold(int32 Amount);

	// 资源变动委托
	UPROPERTY(BlueprintAssignable, Category = "Resources")
	FOnResourceChanged OnResourceChanged;

	// 遗物变动委托
	UPROPERTY(BlueprintAssignable, Category = "Relic")
	FOnOwnedRelicsChanged OnOwnedRelicsChanged;

	// ============== 遗物 / 商店系统 ==============

	// 遗物数据资产
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relic")
	TObjectPtr<ULFPRelicDataAsset> RelicDataAsset;

	// 商店数据资产
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
	TObjectPtr<ULFPShopDataAsset> ShopDataAsset;

	// 雇佣市场数据资产
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HireMarket")
	TObjectPtr<ULFPHireMarketDataAsset> HireMarketDataAsset;

	// 已拥有遗物 ID 集合
	UPROPERTY(BlueprintReadOnly, Category = "Relic")
	TSet<FName> OwnedRelicIDs;

	// 是否已拥有遗物
	UFUNCTION(BlueprintPure, Category = "Relic")
	bool HasRelic(FName RelicID) const;

	// 尝试将遗物加入已拥有列表
	UFUNCTION(BlueprintCallable, Category = "Relic")
	bool TryAddOwnedRelic(FName RelicID);

	// 获取已拥有遗物 ID 列表
	UFUNCTION(BlueprintPure, Category = "Relic")
	TArray<FName> GetOwnedRelicIDsArray() const;

	// 查找遗物定义
	UFUNCTION(BlueprintPure, Category = "Relic")
	bool FindRelicDefinition(FName RelicID, FLFPRelicDefinition& OutDefinition) const;

	// 查找商店定义
	UFUNCTION(BlueprintPure, Category = "Shop")
	bool FindShopDefinition(FName ShopID, FLFPShopDefinition& OutDefinition) const;

	// 购买遗物
	UFUNCTION(BlueprintCallable, Category = "Shop")
	bool TryPurchaseRelic(FName RelicID, int32 Price);

	// 将已拥有遗物效果应用到单位当前属性
	UFUNCTION(BlueprintCallable, Category = "Relic")
	void ApplyOwnedRelicsToUnit(ALFPTacticsUnit* Unit) const;

	// ============== 雇佣市场 ==============

	// 已购买的雇佣市场单位（扁平化存储："HireMarketID=UnitTypeID" 格式）
	UPROPERTY()
	TArray<FString> PurchasedHireMarketEntries;

	// 查找雇佣市场定义
	UFUNCTION(BlueprintPure, Category = "HireMarket")
	bool FindHireMarketDefinition(FName HireMarketID, FLFPHireMarketDefinition& OutDefinition) const;

	// 是否已在指定雇佣市场购买过该单位
	UFUNCTION(BlueprintPure, Category = "HireMarket")
	bool HasPurchasedHireMarketUnit(FName HireMarketID, FName UnitTypeID) const;

	// 购买雇佣市场单位（同一市场内同种单位不可重复购买）
	UFUNCTION(BlueprintCallable, Category = "HireMarket")
	bool TryPurchaseHireMarketUnit(FName HireMarketID, FName UnitTypeID, int32 Price, FLFPUnitEntry& OutUnit);

	// 花金币购买单位（仅扣钱+生成 UnitEntry，不处理入队）
	// 返回 true 表示扣款成功，OutUnit 为生成的单位数据
	UFUNCTION(BlueprintCallable, Category = "HireMarket")
	bool TrySpendForUnit(FName UnitTypeID, int32 Price, FLFPUnitEntry& OutUnit);

	// ============== 编队系统 ==============

	// 单位注册表（蓝图中配置，全局唯一）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	TObjectPtr<ULFPUnitRegistryDataAsset> UnitRegistry;

	// 出战队伍
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	TArray<FLFPUnitEntry> PartyUnits;

	// 备战营
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	TArray<FLFPUnitEntry> ReserveUnits;

	// 出战队伍上限
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 MaxPartySize = 3;

	// 备战营上限
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Party")
	int32 MaxReserveSize = 3;

	// 尝试添加单位：先队伍 → 再备战 → 都满返回 false
	UFUNCTION(BlueprintCallable, Category = "Party")
	bool TryAddUnit(const FLFPUnitEntry& Unit);

	// 队伍是否已满
	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsPartyFull() const { return PartyUnits.Num() >= MaxPartySize; }

	// 备战营是否已满
	UFUNCTION(BlueprintPure, Category = "Party")
	bool IsReserveFull() const { return ReserveUnits.Num() >= MaxReserveSize; }

	// 替换队伍中的单位
	UFUNCTION(BlueprintCallable, Category = "Party")
	void ReplacePartyUnit(int32 SlotIndex, const FLFPUnitEntry& NewUnit);

	// 替换备战营中的单位
	UFUNCTION(BlueprintCallable, Category = "Party")
	void ReplaceReserveUnit(int32 SlotIndex, const FLFPUnitEntry& NewUnit);

	// 移除队伍单位
	UFUNCTION(BlueprintCallable, Category = "Party")
	void RemovePartyUnit(int32 SlotIndex);

	// 移除备战营单位
	UFUNCTION(BlueprintCallable, Category = "Party")
	void RemoveReserveUnit(int32 SlotIndex);

	// ============== 单位升阶 ==============

	// 查找可合并的 TypeID 列表（同 TypeID 出现 >= 2 次且注册表中有进化目标）
	UFUNCTION(BlueprintCallable, Category = "Party")
	TArray<FLFPUnitEntry> GetMergeablePairs() const;

	// 执行合并：移除指定的两个单位，添加进化目标新单位
	// TargetTypeID: 进化目标 TypeID（分支选择时由 UI 传入）
	UFUNCTION(BlueprintCallable, Category = "Party")
	bool MergeUnits(bool bSourceAIsParty, int32 SourceAIndex,
		bool bSourceBIsParty, int32 SourceBIndex, FName TargetTypeID);

protected:
	FString GetSaveSlotName(int32 SlotIndex) const;

	// 待处理的战斗请求
	UPROPERTY()
	FLFPBattleRequest PendingBattleRequest;

	// 待处理的战斗结果
	UPROPERTY()
	FLFPBattleResult PendingBattleResult;

	// 世界地图状态快照
	UPROPERTY()
	FLFPWorldMapSnapshot WorldMapSnapshot;
};
