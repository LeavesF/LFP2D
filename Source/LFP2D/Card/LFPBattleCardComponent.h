#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPBattleCardComponent.generated.h"

class ALFPTacticsUnit;
class ULFPCardDataAsset;
class ULFPGameInstance;
class ULFPSkillBase;

/* 玩家战斗牌堆运行时组件：维护抽牌堆、手牌、弃牌堆和销毁牌堆。 */
UCLASS(Blueprintable, ClassGroup = (Card), meta = (BlueprintSpawnableComponent))
class LFP2D_API ULFPBattleCardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULFPBattleCardComponent();

	/* 根据玩家牌库和当前出战单位组装本场战斗牌库。 */
	UFUNCTION(BlueprintCallable, Category = "Card")
	void InitializeBattleDeck(ULFPGameInstance* GameInstance, const TArray<ALFPTacticsUnit*>& DeployedUnits);

	/* 抽指定数量的牌；抽牌堆为空时会自动把弃牌堆洗回抽牌堆。 */
	UFUNCTION(BlueprintCallable, Category = "Card")
	int32 DrawCards(int32 Count);

	/* 抽到手牌上限，用于每个玩家行动阶段开始时补牌。 */
	UFUNCTION(BlueprintCallable, Category = "Card")
	int32 DrawUpToHandLimit();

	/* 获取当前单位可以使用的手牌。基于 CardCategory + RequiredTag + CanUseCard 过滤。 */
	UFUNCTION(BlueprintCallable, Category = "Card")
	TArray<FLFPCardInstance> GetPlayableHandCardsForUnit(ALFPTacticsUnit* Unit);

	/* 出牌成功后移动到默认目标牌堆；失败时外部不要调用，卡牌会留在手牌。 */
	UFUNCTION(BlueprintCallable, Category = "Card")
	bool FinishPlayingCard(int32 CardInstanceID);

	UFUNCTION(BlueprintCallable, Category = "Card")
	bool MoveHandCardToPile(int32 CardInstanceID, ELFPCardPile TargetPile);

	UFUNCTION(BlueprintPure, Category = "Card")
	bool IsInitialized() const { return bInitialized; }

	UFUNCTION(BlueprintPure, Category = "Card")
	int32 GetMaxHandSize() const { return MaxHandSize; }

	UFUNCTION(BlueprintPure, Category = "Card")
	TArray<FLFPCardInstance> GetDrawPile() const { return DrawPile; }

	UFUNCTION(BlueprintPure, Category = "Card")
	TArray<FLFPCardInstance> GetHand() const { return Hand; }

	UFUNCTION(BlueprintPure, Category = "Card")
	TArray<FLFPCardInstance> GetDiscardPile() const { return DiscardPile; }

	UFUNCTION(BlueprintPure, Category = "Card")
	TArray<FLFPCardInstance> GetExhaustPile() const { return ExhaustPile; }

	// 将卡牌从当前所在牌堆移动到目标牌堆（不限于手牌）。
	UFUNCTION(BlueprintCallable, Category = "Card")
	bool MoveCardToPile(int32 CardInstanceID, ELFPCardPile TargetPile);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	int32 OpeningDrawCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	int32 MaxHandSize = 5;

	// 三种通用普攻的兜底技能类，在蓝图中按攻击Tag分别配置。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|AttackDefaults")
	TSoftObjectPtr<ULFPCardDataAsset> FallbackMeleeAttackCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|AttackDefaults")
	TSoftObjectPtr<ULFPCardDataAsset> FallbackRangedAttackCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|AttackDefaults")
	TSoftObjectPtr<ULFPCardDataAsset> FallbackMagicAttackCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|AttackDefaults|Legacy")
	TSubclassOf<ULFPSkillBase> FallbackMeleeAttackClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|AttackDefaults|Legacy")
	TSubclassOf<ULFPSkillBase> FallbackRangedAttackClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card|AttackDefaults|Legacy")
	TSubclassOf<ULFPSkillBase> FallbackMagicAttackClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TArray<FLFPCardInstance> DrawPile;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TArray<FLFPCardInstance> Hand;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TArray<FLFPCardInstance> DiscardPile;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TArray<FLFPCardInstance> ExhaustPile;

private:
	bool AddCardToDrawPile(const FLFPCardDefinition& Definition, ALFPTacticsUnit* SourceUnit,
		FGameplayTag RequiredTagOverride = FGameplayTag());
	bool AddCardDataToDrawPile(const TSoftObjectPtr<ULFPCardDataAsset>& CardData, ALFPTacticsUnit* SourceUnit,
		FGameplayTag RequiredTagOverride = FGameplayTag());
	bool AddCardToDrawPile(TSubclassOf<ULFPSkillBase> SkillClass, ALFPTacticsUnit* SourceUnit,
		ELFPCardCategory Category = ELFPCardCategory::FullyGeneric, FGameplayTag RequiredTag = FGameplayTag());
	void AddPlayerDeckCards(ULFPGameInstance* GameInstance);
	void AddUnitCards(ULFPGameInstance* GameInstance, const TArray<ALFPTacticsUnit*>& DeployedUnits);
	void AddConfiguredUnitCards(ALFPTacticsUnit* Unit, const TArray<TSoftObjectPtr<ULFPCardDataAsset>>& Cards);
	void AddConfiguredUnitCards(ALFPTacticsUnit* Unit, const TArray<TSubclassOf<ULFPSkillBase>>& CardSkillClasses,
		ELFPCardCategory Category);
	void AddSharedAttackCards(const TArray<ALFPTacticsUnit*>& DeployedUnits);
	FGameplayTag ResolveRequiredTag(const FLFPCardDefinition& Definition, ALFPTacticsUnit* SourceUnit,
		FGameplayTag RequiredTagOverride) const;
	FGameplayTag FindFirstUnitTagWithPrefix(ALFPTacticsUnit* Unit, const FString& Prefix) const;
	void PrepareCardForUnit(FLFPCardInstance& Card, ALFPTacticsUnit* Unit);
	void ShuffleDrawPile();
	void ShuffleDiscardIntoDrawPile();
	TArray<FLFPCardInstance>* GetPileMutable(ELFPCardPile Pile);

	bool bInitialized = false;
	int32 NextCardInstanceID = 1;
};
