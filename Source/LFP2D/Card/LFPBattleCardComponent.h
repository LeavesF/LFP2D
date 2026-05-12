#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/Card/LFPCardTypes.h"
#include "LFPBattleCardComponent.generated.h"

class ALFPTacticsUnit;
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

	/* 获取当前单位可以使用的手牌。单位专属卡只给来源单位使用，公共牌库卡临时绑定到当前单位。 */
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

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	int32 OpeningDrawCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	int32 MaxHandSize = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card")
	TSubclassOf<ULFPSkillBase> FallbackDefaultAttackSkillClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TArray<FLFPCardInstance> DrawPile;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TArray<FLFPCardInstance> Hand;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TArray<FLFPCardInstance> DiscardPile;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Card")
	TArray<FLFPCardInstance> ExhaustPile;

private:
	void AddCardToDrawPile(TSubclassOf<ULFPSkillBase> SkillClass, ALFPTacticsUnit* SourceUnit);
	void AddPlayerDeckCards(ULFPGameInstance* GameInstance);
	void AddUnitCards(ULFPGameInstance* GameInstance, const TArray<ALFPTacticsUnit*>& DeployedUnits);
	void AddConfiguredUnitCards(ALFPTacticsUnit* Unit, const TArray<TSubclassOf<ULFPSkillBase>>& CardSkillClasses);
	void AddDefaultAttackCard(ALFPTacticsUnit* Unit);
	void PrepareCardForUnit(FLFPCardInstance& Card, ALFPTacticsUnit* Unit);
	void ShuffleDrawPile();
	void ShuffleDiscardIntoDrawPile();
	TArray<FLFPCardInstance>* GetPileMutable(ELFPCardPile Pile);

	bool bInitialized = false;
	int32 NextCardInstanceID = 1;
};
