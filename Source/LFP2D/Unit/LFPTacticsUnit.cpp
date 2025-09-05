// Fill out your copyright notice in the Description page of Project Settings.


#include "LFP2D/Unit/LFPTacticsUnit.h"
#include "LFP2D/AI/LFPAIController.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFP2D/Skill/LFPSkillComponent.h"
#include "LFP2D/Unit/Betrayal/LFPBetrayalCondition.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "LFP2D/HexGrid/LFPHexGridManager.h"
#include "LFP2D/Turn/LFPTurnManager.h"
#include "LFP2D/UI/Fighting/LFPHealthBarWidget.h"
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
    SpriteComponent->SetRelativeLocation(FVector(0, 0, 0)); // 在格子上方

    // 默认值
    CurrentMovePoints = MaxMovePoints;
    CurrentActionPoints = MaxActionPoints;
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
    // 创建时间线组件
    //MoveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
}

void ALFPTacticsUnit::BeginPlay()
{
    Super::BeginPlay();

    // 注册到回合管理器
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->RegisterUnit(this);
    }

    for (ULFPBetrayalCondition* Condition : BetrayalConditions)
    {
        Condition->RegisterCondition(this);
    }

    FLFPHexCoordinates SpawnPoint = FLFPHexCoordinates(StartCoordinates_Q, StartCoordinates_R);
    SetCurrentCoordinates(SpawnPoint);

	// 初始化血条
    CurrentHealth = MaxHealth;
	InitializeHealthBar();

    // 设置移动时间线
    if (MoveCurve)
    {
        FOnTimelineFloat TimelineCallback;
        TimelineCallback.BindUFunction(this, FName("UpdateMoveAnimation"));
        //MoveTimeline->AddInterpFloat(MoveCurve, TimelineCallback);

        FOnTimelineEvent TimelineFinishedCallback;
        TimelineFinishedCallback.BindUFunction(this, FName("FinishMove"));
        //MoveTimeline->SetTimelineFinishedFunc(TimelineFinishedCallback);
    }

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
		Condition->UnRegisterCondition(this);
	}
}

void ALFPTacticsUnit::SetCurrentCoordinates(const FLFPHexCoordinates& NewCoords)
{
    CurrentCoordinates = NewCoords;

    // 更新位置
    if (ALFPHexGridManager* GridManager = GetGridManager())
    {
        if (ALFPHexTile* Tile = GridManager->GetTileAtCoordinates(NewCoords))
        {
            SetActorLocation(Tile->GetActorLocation() + FVector(0, 0, 1));
            Tile->SetIsOccupied(true);
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

    // 计算路径
    MovePath = GridManager->FindPath(CurrentTile, NewTargetTile);
    if (MovePath.Num() == 0|| MovePath.Num()>CurrentMovePoints) return false;

    // 设置移动状态
    CurrentTile->SetIsOccupied(false);
    TargetTile = NewTargetTile;
    CurrentPathIndex = 0;
    MoveProgress = 0.0f;
    MovementRangeTiles.Empty();

    // 消耗行动点
    //ConsumeMovePoints(1);

    // 开始移动动画
    //if (MoveTimeline)
    //{
    //    MoveTimeline->PlayFromStart();
    //}
    //else
    //{
    //    // 如果没有时间线，直接完成移动
    //    FinishMove();
    //}
    FinishMove();
    //NotifyMoveComplete();
    return true;
}

void ALFPTacticsUnit::UpdateMoveAnimation(float Value)
{
    if (CurrentPathIndex < 0 || CurrentPathIndex >= MovePath.Num() - 1) return;

    // 获取当前和下一个格子
    ALFPHexTile* CurrentTile = MovePath[CurrentPathIndex];
    ALFPHexTile* NextTile = MovePath[CurrentPathIndex + 1];

    if (!CurrentTile || !NextTile) return;

    // 计算位置
    FVector StartPos = CurrentTile->GetActorLocation() + FVector(0, 0, 50);
    FVector EndPos = NextTile->GetActorLocation() + FVector(0, 0, 50);

    // 应用曲线值
    FVector NewLocation = FMath::Lerp(StartPos, EndPos, Value);
    SetActorLocation(NewLocation);

    // 更新朝向
    FVector Direction = (EndPos - StartPos).GetSafeNormal();
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        SpriteComponent->SetWorldRotation(NewRotation);
    }

    // 检查是否移动到下一段
    if (Value >= 1.0f)
    {
        CurrentPathIndex++;
        MoveProgress = 0.0f;

        // 更新当前坐标
        if (CurrentPathIndex < MovePath.Num())
        {
            SetCurrentCoordinates(MovePath[CurrentPathIndex]->GetCoordinates());
        }

        // 如果还有路径，重新开始时间线
        if (CurrentPathIndex < MovePath.Num() - 1)
        {
            //MoveTimeline->PlayFromStart();
        }
        else
        {
            FinishMove();
        }
    }
}

void ALFPTacticsUnit::FinishMove()
{
    // 更新到目标位置
    if (TargetTile)
    {
        SetCurrentCoordinates(TargetTile->GetCoordinates());
        ConsumeMovePoints(MovePath.Num());
        //SetActorLocation(TargetTile->GetActorLocation() + FVector(0, 0, 50));
    }

    // 重置移动状态
    TargetTile = nullptr;
    MovePath.Empty();
    CurrentPathIndex = -1;
    MoveProgress = 0.0f;
}

void ALFPTacticsUnit::ResetForNewRound()
{
    CurrentMovePoints = MaxMovePoints;
    CurrentActionPoints = MaxActionPoints;
    bHasActed = false;

    // 可选：添加视觉反馈
    if (SpriteComponent)
    {
        SpriteComponent->SetSpriteColor(FLinearColor::White);
    }
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
    // 视觉反馈：高亮单位
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

    // 如果没有行动点了，标记为已行动
    if (CurrentMovePoints <= 0)
    {
        //bHasActed = true;
    }
}

void ALFPTacticsUnit::ConsumeActionPoints(int32 Amount)
{
    CurrentActionPoints = FMath::Max(0, CurrentActionPoints - Amount);
    CurrentMovePoints = 0;
}

bool ALFPTacticsUnit::HasEnoughMovePoints(int32 Required) const
{
    return CurrentMovePoints >= Required;
}

bool ALFPTacticsUnit::HasEnoughActionPoints(int32 Required) const
{
    return CurrentActionPoints >= Required;
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

    // 更新时间线
    /*if (MoveTimeline && MoveTimeline->IsPlaying())
    {
        MoveTimeline->TickComponent(DeltaTime, LEVELTICK_TimeOnly, nullptr);
    }*/
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
	int32 ActualDamage = FMath::Max(Damage - Defense, 1);
	int32 OldHealth = CurrentHealth;
	CurrentHealth = FMath::Max(CurrentHealth - ActualDamage, 0);

	// 触发血量变化事件
	OnHealthChangedDelegate.Broadcast(CurrentHealth, MaxHealth);

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

	int32 OldHealth = CurrentHealth;
	CurrentHealth = FMath::Min(CurrentHealth + Amount, MaxHealth);

	// 触发血量变化事件
	OnHealthChangedDelegate.Broadcast(CurrentHealth, MaxHealth);

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

    //// 实际伤害计算（延迟到动画结束）
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
    int32 Damage = AttackPower;

    // 添加随机波动 (10% 范围)
    float RandomFactor = FMath::RandRange(0.9f, 1.1f);
    Damage = FMath::RoundToInt(Damage * RandomFactor);

    // 应用伤害
    Target->TakeDamage(Damage);

    // 消耗行动点
    ConsumeMovePoints(1);
}

void ALFPTacticsUnit::HandleDeath()
{
    bIsDead = true;

	// 触发死亡事件
    OnDeathDelegate.Broadcast();

    // 蓝图事件
    OnDeath();

    // 从网格上移除
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
	Affiliation = NewAffiliation;

	// 触发事件
    //OnAffiliationChanged.Broadcast(OldAffiliation, NewAffiliation);
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
        // 跳过已有单位的格子
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
    // 基础威胁值 = 目标攻击力 * (1 - 目标当前血量/最大血量)
    float ThreatValue = Target->GetAttackPower();

    // 距离因子 (越近威胁越大)
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
    //        ThreatValue *= (2.0f - HealthRatio); // 血量越低，威胁值越高
    //    }

    //    // 应用攻击倾向
    //    ThreatValue *= BehaviorData->Aggressiveness;
    //}

    return ThreatValue * DistanceFactor;
}

float ALFPTacticsUnit::CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target)
{
    if (!Tile || !Target) return 0.0f;

    float PositionValue = 0.0f;

    // 1. 距离目标越近越好
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

    //// 3. 靠近其他敌人单位（团队协作）
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

    //// 4. 避免危险位置（如火焰、毒雾等）
    //if (Tile->IsDangerous())
    //{
    //    PositionValue -= 30.0f;
    //}

    //// 5. 高地优势
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

    // 近战攻击检查
    if (bMeleeAttack)
    {
        return Distance == 1; // 相邻格子
    }
    // 远程攻击检查
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
    // 在实际项目中，这里会更新单位的血条UI
    // 例如：HealthBarWidget->SetPercent((float)CurrentHealth / MaxHealth);
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