// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Player/LFPTacticsPlayerController.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/LFPSkillComponent.h"
#include "LFP2D/Unit/Betrayal/LFPBetrayalCondition.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/Core/LFPTurnGameMode.h"
#include "LFP2D/Core/LFPGameInstance.h"
#include "LFP2D/Core/LFPUnitRegistryDataAsset.h"
#include "LFP2D/UI/Fighting/LFPHealthBarWidget.h"
#include "LFP2D/UI/Fighting/LFPPlannedSkillIconWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/TimelineComponent.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"

ALFPTacticsUnit::ALFPTacticsUnit()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建根组件
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    // 创建精灵组件
    SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
    SpriteComponent->SetupAttachment(RootComponent);
    SpriteComponent->SetRelativeLocation(FVector(0, 0, 0)); // 在根组件上方

    // 默认值
    ResetCurrentStatsToBase();
    CurrentPathIndex = -1;
    MoveProgress = 0.0f;

	// 创建血条组件
	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(RootComponent);
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawAtDesiredSize(true);
	// 设置相对位置（在单位上方）
	HealthBarComponent->SetRelativeLocation(FVector(0, 150, 0));

    // 创建技能组件
    SkillComponent = CreateDefaultSubobject<ULFPSkillComponent>(TEXT("SkillComponent"));

    // 创建头顶计划技能图标组件
    PlannedSkillIconComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PlannedSkillIconComponent"));
    PlannedSkillIconComponent->SetupAttachment(RootComponent);
    PlannedSkillIconComponent->SetWidgetSpace(EWidgetSpace::Screen);
    PlannedSkillIconComponent->SetDrawAtDesiredSize(true);
    PlannedSkillIconComponent->SetRelativeLocation(FVector(0, 250, 0)); // 在血条上方
    PlannedSkillIconComponent->SetVisibility(false); // 默认隐藏
}

bool ALFPTacticsUnit::InitializeFromRegistry(ULFPUnitRegistryDataAsset* Registry)
{
    if (bStatsInitialized)
    {
        return true;
    }

    if (!Registry || UnitTypeID == NAME_None)
    {
        return false;
    }

    FLFPUnitRegistryEntry Entry;
    if (!Registry->FindEntry(UnitTypeID, Entry))
    {
        return false;
    }

    ApplyRegistryEntry(Entry);
    bStatsInitialized = true;
    return true;
}

void ALFPTacticsUnit::ApplyRegistryEntry(const FLFPUnitRegistryEntry& Entry)
{
    UnitRace = Entry.Race;
    SpecialTags = Entry.SpecialTags;

    BaseAttack = Entry.BaseStats.Attack;
    BaseMaxHealth = Entry.BaseStats.MaxHealth;
    BaseMaxMovePoints = Entry.BaseStats.MaxMovePoints;
    BaseSpeed = Entry.BaseStats.Speed;

    BaseAttackCount = Entry.AdvancedStats.AttackCount;
    BaseActionCount = Entry.AdvancedStats.ActionCount;
    BasePhysicalBlock = Entry.AdvancedStats.PhysicalBlock;
    BaseSpellDefense = Entry.AdvancedStats.SpellDefense;
    BaseWeight = Entry.AdvancedStats.Weight;

    ResetCurrentStatsToBase();
}

void ALFPTacticsUnit::ResetCurrentStatsToBase(bool bResetHealth)
{
    CurrentAttack = BaseAttack;
    CurrentMaxHealth = BaseMaxHealth;
    CurrentMaxMovePoints = BaseMaxMovePoints;
    CurrentSpeed = BaseSpeed;
    CurrentAttackCount = BaseAttackCount;
    CurrentActionCount = BaseActionCount;
    CurrentPhysicalBlock = BasePhysicalBlock;
    CurrentSpellDefense = BaseSpellDefense;
    CurrentWeight = BaseWeight;

    AttackPower = CurrentAttack;
    MaxHealth = CurrentMaxHealth;
    MaxMovePoints = CurrentMaxMovePoints;
    Speed = CurrentSpeed;
    Defense = CurrentPhysicalBlock;

    if (bResetHealth)
    {
        CurrentHealth = CurrentMaxHealth;
    }
    else
    {
        CurrentHealth = FMath::Clamp(CurrentHealth, 0, CurrentMaxHealth);
    }

    CurrentMovePoints = CurrentMaxMovePoints;
}

void ALFPTacticsUnit::AddCurrentAttack(int32 Delta)
{
    CurrentAttack = FMath::Max(0, CurrentAttack + Delta);
    AttackPower = CurrentAttack;
}

void ALFPTacticsUnit::AddCurrentMaxHealth(int32 Delta, bool bKeepHealthRatio)
{
    const int32 OldMaxHealth = CurrentMaxHealth;
    CurrentMaxHealth = FMath::Max(1, CurrentMaxHealth + Delta);
    MaxHealth = CurrentMaxHealth;

    if (bKeepHealthRatio && OldMaxHealth > 0)
    {
        const float HealthRatio = static_cast<float>(CurrentHealth) / OldMaxHealth;
        CurrentHealth = FMath::Clamp(FMath::RoundToInt(CurrentMaxHealth * HealthRatio), 0, CurrentMaxHealth);
    }
    else
    {
        CurrentHealth = FMath::Clamp(CurrentHealth + Delta, 0, CurrentMaxHealth);
    }
    OnHealthChangedDelegate.Broadcast(CurrentHealth, CurrentMaxHealth);
}

void ALFPTacticsUnit::AddCurrentSpeed(int32 Delta)
{
    CurrentSpeed = FMath::Max(0, CurrentSpeed + Delta);
    Speed = CurrentSpeed;
}

void ALFPTacticsUnit::AddCurrentPhysicalBlock(int32 Delta)
{
    CurrentPhysicalBlock = FMath::Max(0, CurrentPhysicalBlock + Delta);
    Defense = CurrentPhysicalBlock;
}

void ALFPTacticsUnit::BeginPlay()
{
    Super::BeginPlay();

    if (!bStatsInitialized)
    {
        if (ULFPGameInstance* GI = Cast<ULFPGameInstance>(GetGameInstance()))
        {
            InitializeFromRegistry(GI->UnitRegistry);
        }
        else
        {
            ResetCurrentStatsToBase();
        }
    }

    // 注册到回合管理器
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->RegisterUnit(this);
    }

    for (ULFPBetrayalCondition* Condition : BetrayalConditions)
    {
        if (Condition)
        {
            Condition->RegisterCondition(this);
        }
    }

    FLFPHexCoordinates SpawnPoint = FLFPHexCoordinates(StartCoordinates_Q, StartCoordinates_R);
    SetCurrentCoordinates(SpawnPoint);

	// 初始化血量
    CurrentHealth = FMath::Clamp(CurrentHealth, 0, GetCurrentMaxHealth());
	InitializeHealthBar();

    PlannedSkillIconComponent->SetRelativeLocation(FVector(0, SkillIconTopDist, 0)); // 在血条上方

    // 如果是敌方单位，创建AI控制器
    if (IsEnemy())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        if (AIControllerClass)
        {
            AIController = GetWorld()->SpawnActor<ALFPAIController>(AIControllerClass, SpawnParams);
            if (AIController)
            {
                AIController->Possess(this);
                AIController->SetControlledUnit(this);
            }
        }
    }
}

void ALFPTacticsUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->UnregisterUnit(this);
    }

	for (ULFPBetrayalCondition* Condition : BetrayalConditions)
	{
		if (Condition)
		{
			Condition->UnRegisterCondition(this);
		}
	}
}

void ALFPTacticsUnit::SetCurrentCoordinates(const FLFPHexCoordinates& NewCoords)
{
    // 清除旧位置
    if (ALFPHexGridManager* GridManager = GetGridManager())
    {
        if (ALFPHexTile* LastTile = GridManager->GetTileAtCoordinates(CurrentCoordinates))
        {
            LastTile->SetIsOccupied(false);
            LastTile->SetUnitOnTile(nullptr);
        }
        if (ALFPHexTile* Tile = GridManager->GetTileAtCoordinates(NewCoords))
        {
            SetActorLocation(Tile->GetActorLocation() + FVector(0, 0, 1));
            Tile->SetIsOccupied(true);
            Tile->SetUnitOnTile(this);
            CurrentCoordinates = NewCoords;
        }
    }
}

ALFPHexTile* ALFPTacticsUnit::GetCurrentTile()
{
    if (ALFPHexGridManager* GridManager = GetGridManager())
    {
        if (ALFPHexTile* Tile = GridManager->GetTileAtCoordinates(CurrentCoordinates))
        {
            return Tile;
        }
    }
    return nullptr;
}

//void ALFPTacticsUnit::SnapToGrid()
//{
//    if (ALFPHexGridManager* GridManager = GetGridManager())
//    {
//        if (ALFPHexTile* Tile = GridManager->GetTileAtCoordinates(CurrentCoordinates))
//        {
//            SetActorLocation(Tile->GetActorLocation() + FVector(0, 0, 50));
//        }
//    }
//}

//// 获取可移动范围
//TArray<ALFPHexTile*> ALFPTacticsUnit::GetMovementRangeTiles()
//{
//    if (MovementRangeTiles.Num() > 0)
//        return MovementRangeTiles;
//
//    if (ALFPHexGridManager* GridManager = GetGridManager())
//    {
//        if (ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(CurrentCoordinates))
//        {
//            // 计算移动范围
//            MovementRangeTiles = GridManager->GetMovementRange(UnitTile, MovementRange);
//        }
//    }
//    return MovementRangeTiles;
//}

bool ALFPTacticsUnit::MoveToTile(ALFPHexTile* NewTargetTile)
{
    if (!NewTargetTile || !HasEnoughMovePoints(1)) return false;

    ALFPHexGridManager* GridManager = GetGridManager();
    if (!GridManager) return false;

    // 获取当前所在的格子
    ALFPHexTile* CurrentTile = GridManager->GetTileAtCoordinates(CurrentCoordinates);
    if (!CurrentTile) return false;

    // 寻找路径（FindPath 返回的路径不包含起点）
    MovePath = GridManager->FindPath(CurrentTile, NewTargetTile);
    // 计算路径实际移动代价（考虑地形）
    int32 PathCost = 0;
    for (ALFPHexTile* Tile : MovePath)
    {
        PathCost += Tile->GetMovementCost();
    }
    if (MovePath.Num() == 0 || PathCost > CurrentMovePoints) return false;

    // 在路径开头插入当前格子，使 Tick 中 MovePath[0] → MovePath[1] 为第一段移动
    MovePath.Insert(CurrentTile, 0);

    // 设置移动状态
    CurrentTile->SetIsOccupied(false);
    TargetTile = NewTargetTile;
    CurrentPathIndex = 0;
    MoveProgress = 0.0f;
    MovementRangeTiles.Empty();

    // 启动移动动画（Tick 驱动逐格插值）
    bIsMoving = true;

    return true;
}

void ALFPTacticsUnit::UpdateMoveAnimation(float Value)
{
    // 已弃用，移动动画改为 Tick 驱动
}

void ALFPTacticsUnit::FinishMove()
{
    bIsMoving = false;

    // 更新到目标位置
    if (TargetTile)
    {
        SetCurrentCoordinates(TargetTile->GetCoordinates());
        // 计算路径实际移动代价（跳过 MovePath[0] 即起点格子）
        int32 TotalCost = 0;
        for (int32 i = 1; i < MovePath.Num(); i++)
        {
            TotalCost += MovePath[i]->GetMovementCost();
        }
        ConsumeMovePoints(TotalCost);
    }

    // 清除移动状态
    TargetTile = nullptr;
    MovePath.Empty();
    CurrentPathIndex = -1;
    MoveProgress = 0.0f;

    // 广播移动完成
    OnMoveFinished.Broadcast();
}

void ALFPTacticsUnit::ResetForNewRound()
{
    CurrentMovePoints = CurrentMaxMovePoints;
    bHasActed = false;

    // 重置精灵视觉效果
    if (SpriteComponent)
    {
        SpriteComponent->SetSpriteColor(FLinearColor::White);
    }

    // 清除上一轮的行动计划
    ClearActionPlan();
}

void ALFPTacticsUnit::OnTurnStarted()
{
    bOnTurn = true;

    if (SkillComponent)
    {
        SkillComponent->OnTurnStarted();
    }
}

void ALFPTacticsUnit::OnTurnEnded()
{
    bOnTurn = false;
}

void ALFPTacticsUnit::OnMouseEnter()
{
}

void ALFPTacticsUnit::SetSelected(bool bSelected)
{
    bIsSelected = bSelected;
    // 视觉反馈：高亮选中单位
    if (bSelected)
    {
        SpriteComponent->SetSpriteColor(FLinearColor::Yellow);
    }
    else
    {
        SpriteComponent->SetSpriteColor(FLinearColor::White);
    }
}

//void ALFPTacticsUnit::HighlightMovementRange(bool bHighlight)
//{
//    TArray<ALFPHexTile*> Tiles = GetMovementRangeTiles();
//
//    for (ALFPHexTile* Tile : Tiles)
//    {
//        if (bHighlight)
//        {
//            Tile->SetMovementHighlight(true);
//        }
//        else
//        {
//            Tile->SetMovementHighlight(false);
//        }
//    }
//
//    if (!bHighlight)
//    {
//        MovementRangeTiles.Empty();
//    }
//}

void ALFPTacticsUnit::ConsumeMovePoints(int32 Amount)
{
    CurrentMovePoints = FMath::Max(0, CurrentMovePoints - Amount);

    // 如果没有行动力了，标记为已行动
    if (CurrentMovePoints <= 0)
    {
        //bHasActed = true;
    }
}

void ALFPTacticsUnit::ConsumeActionPoints(int32 Amount)
{
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->ConsumeFactionAP(Affiliation, Amount);
    }
    CurrentMovePoints = 0;
}

bool ALFPTacticsUnit::HasEnoughMovePoints(int32 Required) const
{
    return CurrentMovePoints >= Required;
}

bool ALFPTacticsUnit::HasEnoughActionPoints(int32 Required) const
{
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        return TurnManager->HasEnoughFactionAP(Affiliation, Required);
    }
    return false;
}

int32 ALFPTacticsUnit::GetActionPoints()
{
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        return TurnManager->GetFactionAP(Affiliation);
    }
    return 0;
}

ALFPHexGridManager* ALFPTacticsUnit::GetGridManager() const
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPHexGridManager::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        return Cast<ALFPHexGridManager>(FoundActors[0]);
    }
    return nullptr;
}

ALFPTurnManager* ALFPTacticsUnit::GetTurnManager() const
{
    TArray<AActor*> FoundManagers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTurnManager::StaticClass(), FoundManagers);

    if (FoundManagers.Num() > 0)
    {
        return Cast<ALFPTurnManager>(FoundManagers[0]);
    }
    return nullptr;
}

void ALFPTacticsUnit::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 逐格平滑移动
    if (bIsMoving && CurrentPathIndex >= 0 && CurrentPathIndex < MovePath.Num() - 1)
    {
        MoveProgress += MoveSpeed * DeltaTime;

        ALFPHexTile* FromTile = MovePath[CurrentPathIndex];
        ALFPHexTile* ToTile = MovePath[CurrentPathIndex + 1];

        if (FromTile && ToTile)
        {
            FVector StartPos = FromTile->GetActorLocation() + FVector(0, 0, 1);
            FVector EndPos = ToTile->GetActorLocation() + FVector(0, 0, 1);

            float Alpha = FMath::Clamp(MoveProgress, 0.f, 1.f);
            SetActorLocation(FMath::Lerp(StartPos, EndPos, Alpha));
        }

        if (MoveProgress >= 1.0f)
        {
            CurrentPathIndex++;
            MoveProgress = 0.0f;

            if (CurrentPathIndex >= MovePath.Num() - 1)
            {
                // 到达终点
                FinishMove();
            }
        }
    }
}

void ALFPTacticsUnit::InitializeHealthBar()
{
	if (HealthBarComponent)
	{
		// 获取血条控件
		ULFPHealthBarWidget* HealthBarWidget = Cast<ULFPHealthBarWidget>(HealthBarComponent->GetWidget());
		if (HealthBarWidget)
		{
			// 绑定到单位
			HealthBarWidget->BindToUnit(this);
		}
	}
}

void ALFPTacticsUnit::TakeDamage(int32 Damage)
{
    if (bIsDead) return;

	// 计算实际伤害（考虑防御）
	int32 ActualDamage = FMath::Max(Damage - GetPhysicalBlock(), 1);
	CurrentHealth = FMath::Max(CurrentHealth - ActualDamage, 0);

	// 广播血量变化事件
	OnHealthChangedDelegate.Broadcast(CurrentHealth, GetCurrentMaxHealth());

	// 蓝图事件
	OnTakeDamage(ActualDamage);

	// 检查死亡
	if (CurrentHealth <= 0)
	{
		HandleDeath();
	}
}

void ALFPTacticsUnit::Heal(int32 Amount)
{
	if (bIsDead) return;

	CurrentHealth = FMath::Min(CurrentHealth + Amount, GetCurrentMaxHealth());

	// 广播血量变化事件
	OnHealthChangedDelegate.Broadcast(CurrentHealth, GetCurrentMaxHealth());

	// 蓝图事件
	OnHeal(Amount);
}

bool ALFPTacticsUnit::AttackTarget(ALFPTacticsUnit* Target)
{
    if (!Target || bIsDead || Target->bIsDead)
    {
        return false;
    }

    // 检查攻击范围
    if (!IsTargetInAttackRange(Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("Target is out of attack range!"));
        return false;
    }
    if (Affiliation == Target->Affiliation)
    {
        UE_LOG(LogTemp, Warning, TEXT("Target is Ally!"));
        return false;
    }

    ApplyDamageToTarget(Target);

    //// 播放攻击动画
    //PlayAttackAnimation(Target);

    //// 实际伤害计算（延迟到攻击动画后）
    //FTimerDelegate TimerDelegate;
    //TimerDelegate.BindUFunction(this, FName("ApplyDamageToTarget"), Target);
    //GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, TimerDelegate, 0.5f, false);
    //NotifyAttackComplete();
    return true;
}

void ALFPTacticsUnit::ApplyDamageToTarget(ALFPTacticsUnit* Target)
{
    if (!Target || Target->bIsDead) return;

    // 基础伤害计算
    int32 Damage = GetCurrentAttack();

    // 伤害随机浮动（10% 范围）
    float RandomFactor = FMath::RandRange(0.9f, 1.1f);
    Damage = FMath::RoundToInt(Damage * RandomFactor);

    // 应用伤害
    Target->TakeDamage(Damage);

    // 消耗行动力
    ConsumeMovePoints(1);
}

void ALFPTacticsUnit::HandleDeath()
{
    bIsDead = true;

	// 广播死亡事件
    OnDeathDelegate.Broadcast();

    // 蓝图事件
    OnDeath();

    // 通知 GameMode 敌方单位被击杀（掉落追踪）
    if (IsEnemy())
    {
        if (ALFPTurnGameMode* GM = Cast<ALFPTurnGameMode>(GetWorld()->GetAuthGameMode()))
        {
            GM->OnEnemyUnitKilled(this);
        }
    }

    // 从格子上移除
    ALFPHexTile* CurrentTile = GetCurrentTile();
    if (CurrentTile)
    {
        CurrentTile->SetUnitOnTile(nullptr);
    }

    // 从回合系统中移除
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->UnregisterUnit(this);
    }

    // 禁用碰撞
    SetActorEnableCollision(false);

    // 延迟销毁
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            Destroy();
        }, 2.0f, false);
}

void ALFPTacticsUnit::ChangeAffiliation(EUnitAffiliation NewAffiliation)
{
	EUnitAffiliation OldAffiliation = Affiliation;
	Affiliation = NewAffiliation;

    if (NewAffiliation == EUnitAffiliation::UA_Player)
    {
        if (AIController)
        {
            AIController->UnPossess();
            AIController->Destroy();
        }

        // 敌人→玩家：记录捕获
        if (OldAffiliation == EUnitAffiliation::UA_Enemy)
        {
            if (ALFPTurnGameMode* GM = Cast<ALFPTurnGameMode>(GetWorld()->GetAuthGameMode()))
            {
                GM->RecordCapturedUnit(this);
            }
        }
    }

    // 阵营变更后重新检查胜负条件（如最后一个敌人背叛）
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->CheckBattleEnd();
    }
}

ALFPTacticsUnit* ALFPTacticsUnit::FindBestTarget()
{
    // 获取所有玩家单位
    TArray<AActor*> PlayerUnits;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), PlayerUnits);

    ALFPTacticsUnit* BestTarget = nullptr;
    float BestThreatValue = -MAX_FLT;

    for (AActor* Actor : PlayerUnits)
    {
        ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
        if (Unit && Unit->IsAlive() && Unit->IsAlly())
        {
            float ThreatValue = CalculateThreatValue(Unit);
            if (ThreatValue > BestThreatValue)
            {
                BestThreatValue = ThreatValue;
                BestTarget = Unit;
            }
        }
    }

    return BestTarget;
}

ALFPHexTile* ALFPTacticsUnit::FindBestMovementTile(ALFPTacticsUnit* Target)
{
    ALFPHexGridManager* GridManager = GetGridManager();
    if (!GridManager)
    {
        return nullptr;
    }
    // 获取所有可移动位置
    MovementRangeTiles = GridManager->GetTilesInRange(GetCurrentTile(), GetCurrentMovePoints());

    ALFPHexTile* BestTile = nullptr;
    float BestPositionValue = -MAX_FLT;

    for (ALFPHexTile* Tile : MovementRangeTiles)
    {
        // 跳过有单位的格子
        if (Tile->GetUnitOnTile()) continue;

        float PositionValue = CalculatePositionValue(Tile, Target);
        if (PositionValue > BestPositionValue)
        {
            BestPositionValue = PositionValue;
            BestTile = Tile;
        }
    }

    return BestTile;
}

float ALFPTacticsUnit::CalculateThreatValue(ALFPTacticsUnit* Target)
{
    // 威胁值 = 目标攻击力
    float ThreatValue = Target->GetCurrentAttack();

    // 距离因素（越近威胁越大）
    int32 Distance = FLFPHexCoordinates::Distance(
        GetCurrentCoordinates(),
        Target->GetCurrentCoordinates()
    );
    float DistanceFactor = 1.0f / FMath::Max(Distance, 1);

    //// 应用行为数据
    //if (BehaviorData)
    //{
    //    if (BehaviorData->bPrioritizeWeakTargets)
    //    {
    //        // 增加对低血量目标的权重
    //        float HealthRatio = (float)Target->GetCurrentHealth() / Target->GetMaxHealth();
    //        // 血量越低，威胁值越高
    //    }

    //    // 应用攻击性系数
    //    ThreatValue *= BehaviorData->Aggressiveness;
    //}

    return ThreatValue * DistanceFactor;
}

float ALFPTacticsUnit::CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target)
{
    if (!Tile || !Target) return 0.0f;

    float PositionValue = 0.0f;

    // 1. 离目标越近越好
    int32 DistanceToTarget = FLFPHexCoordinates::Distance(
        Tile->GetCoordinates(),
        Target->GetCurrentCoordinates()
    );
    PositionValue += 10.0f / FMath::Max(DistanceToTarget, 1);

    // 2. 如果在攻击范围内额外加分
    if (DistanceToTarget <= GetAttackRange())
    {
        PositionValue += 20.0f;
    }

    //// 3. 靠近其他敌方单位（团队协作加分）
    //TArray<AActor*> EnemyUnits;
    //UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALFPTacticsUnit::StaticClass(), EnemyUnits);

    //for (AActor* Actor : EnemyUnits)
    //{
    //    ALFPTacticsUnit* Unit = Cast<ALFPTacticsUnit>(Actor);
    //    if (Unit && Unit != this && Unit->IsEnemy() && Unit->IsAlive())
    //    {
    //        int32 DistanceToAlly = FLFPHexCoordinates::Distance(
    //            Tile->GetCoordinates(),
    //            Unit->GetCurrentCoordinates()
    //        );

    //        if (DistanceToAlly <= 2)
    //        {
    //            PositionValue += 5.0f / FMath::Max(DistanceToAlly, 1);
    //        }
    //    }
    //}

    //// 4. 避免危险位置（陷阱、火焰等）
    //if (Tile->IsDangerous())
    //{
    //    PositionValue -= 30.0f;
    //}

    //// 5. 高地加分
    //if (Tile->IsHighGround())
    //{
    //    PositionValue += 15.0f;
    //}

    return PositionValue;
}

TArray<ALFPHexTile*> ALFPTacticsUnit::GetAttackRangeTiles()
{
    TArray<ALFPHexTile*> AttackRangeTiles;

    if (ALFPHexGridManager* GridManager = GetGridManager())
    {
        // 近战攻击范围
        if (bMeleeAttack)
        {
            // 获取相邻格子
            TArray<FLFPHexCoordinates> Neighbors = CurrentCoordinates.GetNeighbors();
            for (const FLFPHexCoordinates& Coord : Neighbors)
            {
                FLFPHexCoordinates Key(Coord.Q, Coord.R);
                if (ALFPHexTile* Tile = GridManager->GetTileAtCoordinates(Key))
                {
                    AttackRangeTiles.Add(Tile);
                }
            }
        }
        // 远程攻击范围
        else
        {
            // 获取攻击范围内的所有格子
            TArray<ALFPHexTile*> TilesInRange = GridManager->GetTilesInRange(GetCurrentTile(), AttackRange);

            for (ALFPHexTile* Tile : TilesInRange)
            {
                AttackRangeTiles.Add(Tile);
            }
        }
    }

    return AttackRangeTiles;
}

bool ALFPTacticsUnit::IsTargetInAttackRange(ALFPTacticsUnit* Target) const
{
    if (!Target || !Target->GetCurrentTile()) return false;

    // 计算距离
    int32 Distance = FLFPHexCoordinates::Distance(
        CurrentCoordinates,
        Target->GetCurrentCoordinates()
    );

    // 近战攻击判定
    if (bMeleeAttack)
    {
        return Distance == 1; // 相邻格子
    }
    // 远程攻击判定
    else
    {
        return Distance >= 2 && Distance <= AttackRange;
    }
}

FLinearColor ALFPTacticsUnit::GetAffiliationColor() const
{
    switch (Affiliation)
    {
    case EUnitAffiliation::UA_Player:
        return FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // 蓝色
    case EUnitAffiliation::UA_Enemy:
        return FLinearColor(1.0f, 0.1f, 0.1f, 1.0f); // 红色
    case EUnitAffiliation::UA_Neutral:
        return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f); // 灰色
    default:
        return FLinearColor::White;
    }
}

void ALFPTacticsUnit::UpdateHealthUI()
{
    // 在实际项目中，可以更新单位的血量UI
    // 例如：HealthBarWidget->SetPercent((float)CurrentHealth / GetCurrentMaxHealth());
}

TArray<ULFPSkillBase*> ALFPTacticsUnit::GetAvailableSkills()
{
    if (SkillComponent)
    {
        return SkillComponent->GetAvailableSkills();
    }
    return TArray<ULFPSkillBase*>();
}

bool ALFPTacticsUnit::ExecuteSkill(ULFPSkillBase* Skill, ALFPHexTile* NewTargetTile)
{
    if (SkillComponent)
    {
        return SkillComponent->ExecuteSkill(Skill, NewTargetTile);
    }
    return false;
}

ULFPSkillBase* ALFPTacticsUnit::GetDefaultAttackSkill()
{
    if (SkillComponent)
    {
        return SkillComponent->GetDefaultAttackSkill();
    }
    return nullptr;
}

// ==== 行动计划 ====

void ALFPTacticsUnit::SetActionPlan(const FEnemyActionPlan& Plan)
{
    CurrentActionPlan = Plan;
    if (Plan.bIsValid)
    {
        ShowPlannedSkillIcon(true);
    }
}

void ALFPTacticsUnit::ClearActionPlan()
{
    CurrentActionPlan.Reset();
    ShowPlannedSkillIcon(false);
}

void ALFPTacticsUnit::ShowPlannedSkillIcon(bool bShow)
{
    if (!PlannedSkillIconComponent) return;

    PlannedSkillIconComponent->SetVisibility(bShow);

    if (bShow && CurrentActionPlan.PlannedSkill && CurrentActionPlan.PlannedSkill->SkillIcon)
    {
        // 确保 Widget 已创建
        if (PlannedSkillIconWidgetClass && !PlannedSkillIconComponent->GetWidget())
        {
            PlannedSkillIconComponent->SetWidgetClass(PlannedSkillIconWidgetClass);
        }

        if (ULFPPlannedSkillIconWidget* IconWidget = Cast<ULFPPlannedSkillIconWidget>(PlannedSkillIconComponent->GetWidget()))
        {
            IconWidget->SetSkillIcon(CurrentActionPlan.PlannedSkill->SkillIcon);
        }
    }
}
