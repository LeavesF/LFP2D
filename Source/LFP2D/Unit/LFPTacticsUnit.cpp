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

    // ���������
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootSceneComponent;

    // �����������
    SpriteComponent = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteComponent"));
    SpriteComponent->SetupAttachment(RootComponent);
    SpriteComponent->SetRelativeLocation(FVector(0, 0, 0)); // �ڸ����Ϸ�

    // Ĭ��ֵ
    CurrentMovePoints = MaxMovePoints;
    CurrentActionPoints = MaxActionPoints;
    CurrentPathIndex = -1;
    MoveProgress = 0.0f;

	// ����Ѫ�����
	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(RootComponent);
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarComponent->SetDrawAtDesiredSize(true);
	// �������λ�ã��ڵ�λ�Ϸ���
	HealthBarComponent->SetRelativeLocation(FVector(0, 150, 0));

    // �����������
    SkillComponent = CreateDefaultSubobject<ULFPSkillComponent>(TEXT("SkillComponent"));
    // ����ʱ�������
    //MoveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
}

void ALFPTacticsUnit::BeginPlay()
{
    Super::BeginPlay();

    // ע�ᵽ�غϹ�����
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

	// ��ʼ��Ѫ��
    CurrentHealth = MaxHealth;
	InitializeHealthBar();

    // �����ƶ�ʱ����
    if (MoveCurve)
    {
        FOnTimelineFloat TimelineCallback;
        TimelineCallback.BindUFunction(this, FName("UpdateMoveAnimation"));
        //MoveTimeline->AddInterpFloat(MoveCurve, TimelineCallback);

        FOnTimelineEvent TimelineFinishedCallback;
        TimelineFinishedCallback.BindUFunction(this, FName("FinishMove"));
        //MoveTimeline->SetTimelineFinishedFunc(TimelineFinishedCallback);
    }

    // ����ǵз���λ������AI������
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

    // ����λ��
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

//// ��ȡ���ƶ���Χ
//TArray<ALFPHexTile*> ALFPTacticsUnit::GetMovementRangeTiles()
//{
//    if (MovementRangeTiles.Num() > 0)
//        return MovementRangeTiles;
//
//    if (ALFPHexGridManager* GridManager = GetGridManager())
//    {
//        if (ALFPHexTile* UnitTile = GridManager->GetTileAtCoordinates(CurrentCoordinates))
//        {
//            // �����ƶ���Χ
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

    // ��ȡ��ǰ���ڵĸ���
    ALFPHexTile* CurrentTile = GridManager->GetTileAtCoordinates(CurrentCoordinates);
    if (!CurrentTile) return false;

    // ����·��
    MovePath = GridManager->FindPath(CurrentTile, NewTargetTile);
    if (MovePath.Num() == 0|| MovePath.Num()>CurrentMovePoints) return false;

    // �����ƶ�״̬
    CurrentTile->SetIsOccupied(false);
    TargetTile = NewTargetTile;
    CurrentPathIndex = 0;
    MoveProgress = 0.0f;
    MovementRangeTiles.Empty();

    // �����ж���
    //ConsumeMovePoints(1);

    // ��ʼ�ƶ�����
    //if (MoveTimeline)
    //{
    //    MoveTimeline->PlayFromStart();
    //}
    //else
    //{
    //    // ���û��ʱ���ߣ�ֱ������ƶ�
    //    FinishMove();
    //}
    FinishMove();
    //NotifyMoveComplete();
    return true;
}

void ALFPTacticsUnit::UpdateMoveAnimation(float Value)
{
    if (CurrentPathIndex < 0 || CurrentPathIndex >= MovePath.Num() - 1) return;

    // ��ȡ��ǰ����һ������
    ALFPHexTile* CurrentTile = MovePath[CurrentPathIndex];
    ALFPHexTile* NextTile = MovePath[CurrentPathIndex + 1];

    if (!CurrentTile || !NextTile) return;

    // ����λ��
    FVector StartPos = CurrentTile->GetActorLocation() + FVector(0, 0, 50);
    FVector EndPos = NextTile->GetActorLocation() + FVector(0, 0, 50);

    // Ӧ������ֵ
    FVector NewLocation = FMath::Lerp(StartPos, EndPos, Value);
    SetActorLocation(NewLocation);

    // ���³���
    FVector Direction = (EndPos - StartPos).GetSafeNormal();
    if (!Direction.IsNearlyZero())
    {
        FRotator NewRotation = Direction.Rotation();
        SpriteComponent->SetWorldRotation(NewRotation);
    }

    // ����Ƿ��ƶ�����һ��
    if (Value >= 1.0f)
    {
        CurrentPathIndex++;
        MoveProgress = 0.0f;

        // ���µ�ǰ����
        if (CurrentPathIndex < MovePath.Num())
        {
            SetCurrentCoordinates(MovePath[CurrentPathIndex]->GetCoordinates());
        }

        // �������·�������¿�ʼʱ����
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
    // ���µ�Ŀ��λ��
    if (TargetTile)
    {
        SetCurrentCoordinates(TargetTile->GetCoordinates());
        ConsumeMovePoints(MovePath.Num());
        //SetActorLocation(TargetTile->GetActorLocation() + FVector(0, 0, 50));
    }

    // �����ƶ�״̬
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

    // ��ѡ������Ӿ�����
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
    // �Ӿ�������������λ
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

    // ���û���ж����ˣ����Ϊ���ж�
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

    // ����ʱ����
    /*if (MoveTimeline && MoveTimeline->IsPlaying())
    {
        MoveTimeline->TickComponent(DeltaTime, LEVELTICK_TimeOnly, nullptr);
    }*/
}

void ALFPTacticsUnit::InitializeHealthBar()
{
	if (HealthBarComponent)
	{
		// ��ȡѪ���ؼ�
		ULFPHealthBarWidget* HealthBarWidget = Cast<ULFPHealthBarWidget>(HealthBarComponent->GetWidget());
		if (HealthBarWidget)
		{
			// �󶨵���λ
			HealthBarWidget->BindToUnit(this);
		}
	}
}

void ALFPTacticsUnit::TakeDamage(int32 Damage)
{
    if (bIsDead) return;

	// ����ʵ���˺������Ƿ�����
	int32 ActualDamage = FMath::Max(Damage - Defense, 1);
	int32 OldHealth = CurrentHealth;
	CurrentHealth = FMath::Max(CurrentHealth - ActualDamage, 0);

	// ����Ѫ���仯�¼�
	OnHealthChangedDelegate.Broadcast(CurrentHealth, MaxHealth);

	// ��ͼ�¼�
	OnTakeDamage(ActualDamage);

	// �������
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

	// ����Ѫ���仯�¼�
	OnHealthChangedDelegate.Broadcast(CurrentHealth, MaxHealth);

	// ��ͼ�¼�
	OnHeal(Amount);
}

bool ALFPTacticsUnit::AttackTarget(ALFPTacticsUnit* Target)
{
    if (!Target || bIsDead || Target->bIsDead)
    {
        return false;
    }

    // ��鹥����Χ
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

    //// ���Ź�������
    //PlayAttackAnimation(Target);

    //// ʵ���˺����㣨�ӳٵ�����������
    //FTimerDelegate TimerDelegate;
    //TimerDelegate.BindUFunction(this, FName("ApplyDamageToTarget"), Target);
    //GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, TimerDelegate, 0.5f, false);
    //NotifyAttackComplete();
    return true;
}

void ALFPTacticsUnit::ApplyDamageToTarget(ALFPTacticsUnit* Target)
{
    if (!Target || Target->bIsDead) return;

    // �����˺�����
    int32 Damage = AttackPower;

    // ���������� (10% ��Χ)
    float RandomFactor = FMath::RandRange(0.9f, 1.1f);
    Damage = FMath::RoundToInt(Damage * RandomFactor);

    // Ӧ���˺�
    Target->TakeDamage(Damage);

    // �����ж���
    ConsumeMovePoints(1);
}

void ALFPTacticsUnit::HandleDeath()
{
    bIsDead = true;

	// ���������¼�
    OnDeathDelegate.Broadcast();

    // ��ͼ�¼�
    OnDeath();

    // ���������Ƴ�
    ALFPHexTile* CurrentTile = GetCurrentTile();
    if (CurrentTile)
    {
        CurrentTile->SetUnitOnTile(nullptr);
    }

    // �ӻغ�ϵͳ���Ƴ�
    if (ALFPTurnManager* TurnManager = GetTurnManager())
    {
        TurnManager->UnregisterUnit(this);
    }

    // ������ײ
    SetActorEnableCollision(false);

    // �ӳ�����
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            Destroy();
        }, 2.0f, false);
}

void ALFPTacticsUnit::ChangeAffiliation(EUnitAffiliation NewAffiliation)
{
	Affiliation = NewAffiliation;

	// �����¼�
    //OnAffiliationChanged.Broadcast(OldAffiliation, NewAffiliation);
}

ALFPTacticsUnit* ALFPTacticsUnit::FindBestTarget()
{
    // ��ȡ������ҵ�λ
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
    // ��ȡ���п��ƶ�λ��
    MovementRangeTiles = GridManager->GetTilesInRange(GetCurrentTile(), GetCurrentMovePoints());

    ALFPHexTile* BestTile = nullptr;
    float BestPositionValue = -MAX_FLT;

    for (ALFPHexTile* Tile : MovementRangeTiles)
    {
        // �������е�λ�ĸ���
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
    // ������вֵ = Ŀ�깥���� * (1 - Ŀ�굱ǰѪ��/���Ѫ��)
    float ThreatValue = Target->GetAttackPower();

    // �������� (Խ����вԽ��)
    int32 Distance = FLFPHexCoordinates::Distance(
        GetCurrentCoordinates(),
        Target->GetCurrentCoordinates()
    );
    float DistanceFactor = 1.0f / FMath::Max(Distance, 1);

    //// Ӧ����Ϊ����
    //if (BehaviorData)
    //{
    //    if (BehaviorData->bPrioritizeWeakTargets)
    //    {
    //        // ���ӶԵ�Ѫ��Ŀ���Ȩ��
    //        float HealthRatio = (float)Target->GetCurrentHealth() / Target->GetMaxHealth();
    //        ThreatValue *= (2.0f - HealthRatio); // Ѫ��Խ�ͣ���вֵԽ��
    //    }

    //    // Ӧ�ù�������
    //    ThreatValue *= BehaviorData->Aggressiveness;
    //}

    return ThreatValue * DistanceFactor;
}

float ALFPTacticsUnit::CalculatePositionValue(ALFPHexTile* Tile, ALFPTacticsUnit* Target)
{
    if (!Tile || !Target) return 0.0f;

    float PositionValue = 0.0f;

    // 1. ����Ŀ��Խ��Խ��
    int32 DistanceToTarget = FLFPHexCoordinates::Distance(
        Tile->GetCoordinates(),
        Target->GetCurrentCoordinates()
    );
    PositionValue += 10.0f / FMath::Max(DistanceToTarget, 1);

    // 2. ����ڹ�����Χ�ڶ���ӷ�
    if (DistanceToTarget <= GetAttackRange())
    {
        PositionValue += 20.0f;
    }

    //// 3. �����������˵�λ���Ŷ�Э����
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

    //// 4. ����Σ��λ�ã�����桢����ȣ�
    //if (Tile->IsDangerous())
    //{
    //    PositionValue -= 30.0f;
    //}

    //// 5. �ߵ�����
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
        // ��ս������Χ
        if (bMeleeAttack)
        {
            // ��ȡ���ڸ���
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
        // Զ�̹�����Χ
        else
        {
            // ��ȡ������Χ�ڵ����и���
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

    // �������
    int32 Distance = FLFPHexCoordinates::Distance(
        CurrentCoordinates,
        Target->GetCurrentCoordinates()
    );

    // ��ս�������
    if (bMeleeAttack)
    {
        return Distance == 1; // ���ڸ���
    }
    // Զ�̹������
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
        return FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // ��ɫ
    case EUnitAffiliation::UA_Enemy:
        return FLinearColor(1.0f, 0.1f, 0.1f, 1.0f); // ��ɫ
    case EUnitAffiliation::UA_Neutral:
        return FLinearColor(0.5f, 0.5f, 0.5f, 1.0f); // ��ɫ
    default:
        return FLinearColor::White;
    }
}

void ALFPTacticsUnit::UpdateHealthUI()
{
    // ��ʵ����Ŀ�У��������µ�λ��Ѫ��UI
    // ���磺HealthBarWidget->SetPercent((float)CurrentHealth / MaxHealth);
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