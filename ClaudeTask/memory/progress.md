# Implementation Progress

## Completed Features

### Two-Phase Battle System
- [x] EBattlePhase enum, FEnemyActionPlan struct in LFPBattleTypes.h
- [x] TurnManager phase state machine (SetPhase, BeginEnemyPlanningPhase, BeginActionPhase)
- [x] AIController: CreateActionPlan, SelectBestSkill, FindBestCasterPosition
- [x] TacticsUnit: SetActionPlan, ClearActionPlan, ShowPlannedSkillIcon
- [x] LFPPlannedSkillIconWidget created
- [x] PlayerController: hover preview (ShowEnemyPlanPreview/HideEnemyPlanPreview), OnPhaseChanged
- [x] TurnSpeedListWidget: phase text display

### Skill Check Refactoring
- [x] IsAvailable() in SkillBase
- [x] CanReleaseFrom() (BlueprintNativeEvent) in SkillBase
- [x] CanExecute() refactored to full check
- [x] AIController updated to use IsAvailable/CanReleaseFrom

### Faction AP + Skill Priority
- [x] SkillBase: priority properties + OnSkillUsed/RecoverPriority/GetEffectivePriority
- [x] TurnManager: faction AP management (TMap, Get/Has/Consume, AP recovery in BeginNewRound)
- [x] TurnManager: AllocateEnemySkills (priority-based global allocation)
- [x] TurnManager: two-step planning flow (allocate then move by speed)
- [x] TacticsUnit: removed per-unit AP, proxy to TurnManager
- [x] AIController: CreateActionPlan accepts PreAllocatedSkill parameter
- [x] SkillComponent: calls OnSkillUsed after Execute
- [x] SkillSelectionWidget: ActionPointsText (BindWidgetOptional), OnFactionAPChanged binding

## Editor TODO (manual)
- [ ] Create UMG Blueprint `WBP_PlannedSkillIcon` with UImage named `SkillIconImage`
- [ ] Set `PlannedSkillIconWidgetClass` on enemy unit blueprints
- [ ] Optionally add `ActionPointsText` TextBlock to SkillSelectionWidget UMG
- [ ] Optionally add `PhaseText` TextBlock to TurnSpeedListWidget UMG
- [ ] Configure skill priority values (BasePriority, PriorityDecreaseOnUse, PriorityRecoveryPerRound) on skill Blueprint assets
- [ ] Configure faction AP values (FactionMaxAP, FactionInitialAP, FactionAPRecovery) on TurnManager Blueprint

## Known Issues / Notes
- `FindBestCasterPosition` and `SelectBestSkill` cannot be `const` because `GetCurrentTile()` is non-const
- SkillBase `EvaluateConditionBonus()` returns 0 by default — override in Blueprint subclasses for context-aware priority bonuses
- AP is consumed during AllocateEnemySkills (planning), NOT during action phase execution
