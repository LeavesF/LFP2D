# Architecture Decisions

## 1. Two-Phase Battle System (completed)
Replaced simple round-robin turns with:
- **EnemyPlanning phase**: enemies choose skills (by hatred), select targets, move to cast positions, display planned skill icons above heads. Player can hover to preview effect ranges.
- **Action phase**: all units act by speed. Player units get normal input; enemy units auto-execute pre-planned skills at pre-planned positions (even if target died).
- Deployment phase explicitly deferred to later.

Key types in `LFPBattleTypes.h`:
- `EBattlePhase`: BP_EnemyPlanning, BP_ActionPhase, BP_RoundEnd
- `FEnemyActionPlan`: stores EnemyUnit, PlannedSkill, TargetUnit, TargetTile, CasterPositionTile, EffectAreaTiles
- `EUnitAffiliation`: UA_Player, UA_Enemy, UA_Neutral (moved here from TacticsUnit.h to avoid circular deps)

## 2. Skill Check Three-Layer Split (completed)
Split ambiguous `CanExecute()` into:
- `IsAvailable()` — cooldown + AP check (AI skill selection)
- `CanReleaseFrom(CasterTile, TargetTile)` — range check, BlueprintNativeEvent (AI positioning)
- `CanExecute(TargetTile)` — full pre-execution check (player use)

## 3. Faction AP System (completed)
AP is per-faction (shared pool), not per-unit:
- Config: FactionInitialAP=1, FactionMaxAP=3, FactionAPRecovery=1/round
- TurnManager manages `TMap<EUnitAffiliation, int32> FactionCurrentAP`
- TacticsUnit's `HasEnoughActionPoints()`, `ConsumeActionPoints()`, `GetActionPoints()` all proxy to TurnManager
- Removed `MaxActionPoints` and `CurrentActionPoints` from TacticsUnit

## 4. Skill Priority System (completed)
Ensures fair AP allocation (not just fastest enemy):
- Each skill has `SkillPriority` (current), `BasePriority` (default=50), `PriorityDecreaseOnUse` (=30), `PriorityRecoveryPerRound` (=10)
- `GetEffectivePriority()` = SkillPriority + EvaluateConditionBonus() (virtual, override point)
- `OnSkillUsed()`: decreases priority. Called by SkillComponent after Execute.
- `RecoverPriority()`: restores priority each round. Called in OnTurnStart.

Two-step enemy planning:
1. `AllocateEnemySkills()`: collects all AP-costing skills from all enemies, sorts by GetEffectivePriority() descending, greedily allocates faction AP (one skill per unit max). Stores in `TMap<ALFPTacticsUnit*, ULFPSkillBase*> AllocatedSkills`.
2. `ProcessNextEnemyPlan()`: iterates enemies by speed order, passes pre-allocated skill to `AIController->CreateActionPlan(PreAllocatedSkill)`.

`CreateActionPlan(ULFPSkillBase* PreAllocatedSkill = nullptr)`: uses pre-allocated skill if provided, otherwise falls back to `SelectBestSkill()` (which returns default attack if nothing else available).

## 5. Delegates
- `OnPhaseChanged(EBattlePhase)` — TurnManager broadcasts phase transitions
- `OnFactionAPChanged(EUnitAffiliation, int32)` — TurnManager broadcasts AP changes
- `OnTurnChanged` — existing turn change delegate
- `OnHealthChangedSignature`, `OnUnitDeathSignature` — unit events
- `OnSkillExecutedSignature` — SkillComponent event

## 6. UI Bindings
- `LFPSkillSelectionWidget`: `ActionPointsText` (BindWidgetOptional) shows "AP: X/Y", bound to OnFactionAPChanged
- `LFPTurnSpeedListWidget`: `PhaseText` (BindWidgetOptional) shows current phase name
- `LFPPlannedSkillIconWidget`: shows skill icon above enemy head during planning phase
