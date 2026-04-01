#include "LFP2D/Turn/LFPBattleRelicRuntimeManager.h"

#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Shop/LFPRelicDataAsset.h"
#include "LFP2D/Shop/LFPRelicTypes.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "Kismet/GameplayStatics.h"

ALFPBattleRelicRuntimeManager::ALFPBattleRelicRuntimeManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALFPBattleRelicRuntimeManager::Initialize(ULFPGameInstance* InGameInstance, ALFPTurnManager* InTurnManager)
{
	CachedGameInstance = InGameInstance;
	CachedTurnManager = InTurnManager;
	CachedRelicDataAsset = InGameInstance ? InGameInstance->RelicDataAsset : nullptr;
	OwnedRelicIDs = InGameInstance ? InGameInstance->GetOwnedRelicIDsArray() : TArray<FName>();
}

void ALFPBattleRelicRuntimeManager::OnDeploymentFinished()
{
	ScanAndBindPlayerUnits();

	for (FLFPUnitRelicRuntimeState& State : BoundUnits)
	{
		if (State.Unit.IsValid())
		{
			RebuildUnitRelicStats(State.Unit.Get());
		}
	}

	// 触发战斗开始的即时效果
	FireInstantBattleRules(ELFPRelicTriggerType::RTT_BattleStart);
}

void ALFPBattleRelicRuntimeManager::OnRoundStarted()
{
	for (FLFPUnitRelicRuntimeState& State : BoundUnits)
	{
		if (State.Unit.IsValid() && State.Unit->IsAlive())
		{
			RebuildUnitRelicStats(State.Unit.Get());
		}
	}

	// 触发回合开始的即时效果
	FireInstantBattleRules(ELFPRelicTriggerType::RTT_RoundStart);
}

void ALFPBattleRelicRuntimeManager::OnRoundEnded(int32 CurrentRound)
{
	if (!CachedRelicDataAsset || LastProcessedRoundEndRound == CurrentRound)
	{
		return;
	}

	LastProcessedRoundEndRound = CurrentRound;

	// 触发回合结束的即时 BattleRule 效果
	FireInstantBattleRules(ELFPRelicTriggerType::RTT_RoundEnd);

	// 触发回合结束的组合规则效果
	for (const FLFPRelicSynergyRule& SynergyRule : CachedRelicDataAsset->SynergyRules)
	{
		if (SynergyRule.TriggerType != ELFPRelicTriggerType::RTT_RoundEnd)
		{
			continue;
		}

		if (SynergyRule.DurationType != ELFPRelicDurationType::RDT_Instant)
		{
			continue;
		}

		bool bHasAllRequiredRelics = true;
		for (const FName& RequiredRelicID : SynergyRule.RequiredRelicIDs)
		{
			if (!OwnedRelicIDs.Contains(RequiredRelicID))
			{
				bHasAllRequiredRelics = false;
				break;
			}
		}

		if (!bHasAllRequiredRelics)
		{
			continue;
		}

		for (FLFPUnitRelicRuntimeState& State : BoundUnits)
		{
			if (!State.Unit.IsValid() || !State.Unit->IsAlive() || !State.Unit->IsAlly())
			{
				continue;
			}

			if (!EvaluateConditions(SynergyRule.Conditions, State.Unit.Get()))
			{
				continue;
			}

			ApplyEffects(SynergyRule.Effects, State.Unit.Get());
		}
	}
}

void ALFPBattleRelicRuntimeManager::ScanAndBindPlayerUnits()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
		if (!Unit || !Unit->IsAlly())
		{
			continue;
		}

		bool bAlreadyBound = false;
		for (const FLFPUnitRelicRuntimeState& State : BoundUnits)
		{
			if (State.Unit.Get() == Unit)
			{
				bAlreadyBound = true;
				break;
			}
		}

		if (bAlreadyBound)
		{
			continue;
		}

		BindUnitEvents(Unit);

		FLFPUnitRelicRuntimeState NewState;
		NewState.Unit = Unit;
		NewState.LastHealthPercent = Unit->GetCurrentMaxHealth() > 0
			? static_cast<float>(Unit->GetCurrentHealth()) / Unit->GetCurrentMaxHealth()
			: 0.f;
		BoundUnits.Add(NewState);
	}
}

void ALFPBattleRelicRuntimeManager::BindUnitEvents(ALFPTacticsUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	Unit->OnHealthChangedWithUnitDelegate.AddDynamic(this, &ALFPBattleRelicRuntimeManager::OnUnitHealthChanged);
	Unit->OnDeathWithUnitDelegate.AddDynamic(this, &ALFPBattleRelicRuntimeManager::OnUnitDeath);
}

void ALFPBattleRelicRuntimeManager::UnbindUnitEvents(ALFPTacticsUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	Unit->OnHealthChangedWithUnitDelegate.RemoveDynamic(this, &ALFPBattleRelicRuntimeManager::OnUnitHealthChanged);
	Unit->OnDeathWithUnitDelegate.RemoveDynamic(this, &ALFPBattleRelicRuntimeManager::OnUnitDeath);
}

void ALFPBattleRelicRuntimeManager::RebuildUnitRelicStats(ALFPTacticsUnit* Unit)
{
	if (!Unit || !CachedRelicDataAsset)
	{
		return;
	}

	FLFPUnitRelicRuntimeState* FoundState = nullptr;
	for (FLFPUnitRelicRuntimeState& State : BoundUnits)
	{
		if (State.Unit.Get() == Unit)
		{
			FoundState = &State;
			break;
		}
	}

	if (!FoundState || FoundState->bIsRebuilding)
	{
		return;
	}

	FoundState->bIsRebuilding = true;

	Unit->ResetCurrentStatsToBase(false);

	for (const FName& RelicID : OwnedRelicIDs)
	{
		FLFPRelicDefinition Definition;
		if (!CachedGameInstance || !CachedGameInstance->FindRelicDefinition(RelicID, Definition))
		{
			continue;
		}

		for (const FLFPRelicEffectEntry& Effect : Definition.Effects)
		{
			switch (Effect.EffectType)
			{
			case ELFPRelicEffectType::RET_MaxHealthFlat:
				Unit->AddCurrentMaxHealth(Effect.Value);
				break;
			case ELFPRelicEffectType::RET_AttackFlat:
				Unit->AddCurrentAttack(Effect.Value);
				break;
			case ELFPRelicEffectType::RET_DefenseFlat:
				Unit->AddCurrentPhysicalBlock(Effect.Value);
				break;
			case ELFPRelicEffectType::RET_SpeedFlat:
				Unit->AddCurrentSpeed(Effect.Value);
				break;
			default:
				break;
			}
		}

		// 应用所有"条件成立时持续"的规则（不限触发时机，因为重建本身在多个时机发生）
		for (const FLFPRelicBattleRule& BattleRule : Definition.BattleRules)
		{
			if (BattleRule.DurationType != ELFPRelicDurationType::RDT_WhileConditionTrue)
			{
				continue;
			}

			if (!EvaluateConditions(BattleRule.Conditions, Unit))
			{
				continue;
			}

			ApplyEffects(BattleRule.Effects, Unit);
		}
	}

	FoundState->LastHealthPercent = Unit->GetCurrentMaxHealth() > 0
		? static_cast<float>(Unit->GetCurrentHealth()) / Unit->GetCurrentMaxHealth()
		: 0.f;
	FoundState->bIsRebuilding = false;
}

void ALFPBattleRelicRuntimeManager::FireInstantBattleRules(ELFPRelicTriggerType TriggerType)
{
	if (!CachedGameInstance)
	{
		return;
	}

	for (const FName& RelicID : OwnedRelicIDs)
	{
		FLFPRelicDefinition Definition;
		if (!CachedGameInstance->FindRelicDefinition(RelicID, Definition))
		{
			continue;
		}

		for (const FLFPRelicBattleRule& BattleRule : Definition.BattleRules)
		{
			if (BattleRule.TriggerType != TriggerType)
			{
				continue;
			}

			if (BattleRule.DurationType != ELFPRelicDurationType::RDT_Instant)
			{
				continue;
			}

			for (FLFPUnitRelicRuntimeState& State : BoundUnits)
			{
				if (!State.Unit.IsValid() || !State.Unit->IsAlive() || !State.Unit->IsAlly())
				{
					continue;
				}

				if (!EvaluateConditions(BattleRule.Conditions, State.Unit.Get()))
				{
					continue;
				}

				ApplyEffects(BattleRule.Effects, State.Unit.Get());
			}
		}
	}
}

void ALFPBattleRelicRuntimeManager::OnUnitHealthChanged(ALFPTacticsUnit* Unit, int32 CurrentHealth, int32 MaxHealth)
{
	if (!Unit || CurrentHealth < 0 || MaxHealth <= 0)
	{
		return;
	}

	RebuildUnitRelicStats(Unit);
}

void ALFPBattleRelicRuntimeManager::OnUnitDeath(ALFPTacticsUnit* Unit)
{
	if (Unit)
	{
		UnbindUnitEvents(Unit);
	}

	for (int32 Index = BoundUnits.Num() - 1; Index >= 0; --Index)
	{
		if (!BoundUnits[Index].Unit.IsValid() || BoundUnits[Index].Unit.Get() == Unit || !BoundUnits[Index].Unit->IsAlive())
		{
			BoundUnits.RemoveAt(Index);
		}
	}
}

bool ALFPBattleRelicRuntimeManager::EvaluateConditions(const TArray<FLFPRelicCondition>& Conditions, ALFPTacticsUnit* Unit) const
{
	if (!Unit)
	{
		return false;
	}

	for (const FLFPRelicCondition& Condition : Conditions)
	{
		switch (Condition.ConditionType)
		{
		case ELFPRelicConditionType::RCT_None:
			break;
		case ELFPRelicConditionType::RCT_HealthPercentBelow:
		{
			const float HealthPercent = Unit->GetCurrentMaxHealth() > 0
				? static_cast<float>(Unit->GetCurrentHealth()) / Unit->GetCurrentMaxHealth()
				: 0.f;
			if (HealthPercent >= Condition.FloatValue)
			{
				return false;
			}
			break;
		}
		case ELFPRelicConditionType::RCT_HasRelic:
		{
			bool bHasAnyRelic = false;
			for (const FName& RelicID : Condition.RelicIDs)
			{
				if (OwnedRelicIDs.Contains(RelicID))
				{
					bHasAnyRelic = true;
					break;
				}
			}
			if (!bHasAnyRelic)
			{
				return false;
			}
			break;
		}
		case ELFPRelicConditionType::RCT_HasAllRelics:
		{
			for (const FName& RelicID : Condition.RelicIDs)
			{
				if (!OwnedRelicIDs.Contains(RelicID))
				{
					return false;
				}
			}
			break;
		}
		default:
			return false;
		}
	}

	return true;
}

void ALFPBattleRelicRuntimeManager::ApplyEffects(const TArray<FLFPRelicBattleEffect>& Effects, ALFPTacticsUnit* Unit)
{
	if (!Unit)
	{
		return;
	}

	for (const FLFPRelicBattleEffect& Effect : Effects)
	{
		switch (Effect.EffectType)
		{
		case ELFPRelicBattleEffectType::RBET_HealFlat:
			Unit->Heal(Effect.IntValue);
			break;
		case ELFPRelicBattleEffectType::RBET_ModifyStatFlat:
			switch (Effect.StatType)
			{
			case ELFPRelicStatType::RST_Attack:
				Unit->AddCurrentAttack(Effect.IntValue);
				break;
			case ELFPRelicStatType::RST_Speed:
				Unit->AddCurrentSpeed(Effect.IntValue);
				break;
			case ELFPRelicStatType::RST_PhysicalBlock:
				Unit->AddCurrentPhysicalBlock(Effect.IntValue);
				break;
			default:
				break;
			}
			break;
		case ELFPRelicBattleEffectType::RBET_ModifyStatPercent:
			if (Effect.StatType == ELFPRelicStatType::RST_Attack)
			{
				const int32 Delta = FMath::RoundToInt(Unit->GetCurrentAttack() * Effect.FloatValue);
				Unit->AddCurrentAttack(Delta);
			}
			break;
		default:
			break;
		}
	}
}
