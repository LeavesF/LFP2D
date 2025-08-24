// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LFP2D/HexGrid/LFPHexTile.h"
#include "Components/TimelineComponent.h"
#include "PaperSpriteComponent.h"
#include "LFPTacticsUnit.generated.h"

// ί��ǩ��
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, int32, CurrentHealth, int32, MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitDeathSignature);

// ��λ��Ӫö��
UENUM(BlueprintType)
enum class EUnitAffiliation : uint8
{
    UA_Player     UMETA(DisplayName = "Player"),
    UA_Enemy      UMETA(DisplayName = "Enemy"),
    UA_Neutral    UMETA(DisplayName = "Neutral")
};

class ALFPHexGridManager;
class ALFPTurnManager;

UCLASS()
class LFP2D_API ALFPTacticsUnit : public AActor
{
	GENERATED_BODY()
	
public:
    ALFPTacticsUnit();

    // ����/��ȡ��ǰ����
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetCurrentCoordinates(const FLFPHexCoordinates& NewCoords);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    FLFPHexCoordinates GetCurrentCoordinates() const { return CurrentCoordinates; }

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    ALFPHexTile* GetCurrentTile();
    //// ��ȡ���ƶ���Χ
    //UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    //TArray<ALFPHexTile*> GetMovementRangeTiles();

    // ���ƶ���Χ����
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    TArray<ALFPHexTile*> MovementRangeTiles;

    // �ƶ���Ŀ�����
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    bool MoveToTile(ALFPHexTile* TargetTile);

    // ����ѡ��״̬
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void SetSelected(bool bSelected);

    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool IsSelected() const { return bIsSelected; }

    // ��ȡ�ƶ���Χ
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    int32 GetMovementRange() const { return CurrentMovePoints; }

    // �����ƶ���
    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    void ConsumeMovePoints(int32 Amount);

	// �����ж���
	UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
	void ConsumeActionPoints(int32 Amount);

    // ����Ƿ����㹻�ƶ���
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughMovePoints(int32 Required) const;

    // ����Ƿ����㹻�ж���
    UFUNCTION(BlueprintPure, Category = "Tactics Unit")
    bool HasEnoughActionPoints(int32 Required) const;

    UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
    int32 GetMovePoints(int32 Amount) { return CurrentMovePoints; }

	UFUNCTION(BlueprintCallable, Category = "Tactics Unit")
	int32 GetActionPoints(int32 Amount) { return CurrentActionPoints; }

    ALFPHexGridManager* GetGridManager() const;
    ALFPTurnManager* GetTurnManager() const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // �ƶ���������
    UFUNCTION()
    void UpdateMoveAnimation(float Value);

    // ����ƶ�
    void FinishMove();

    // �������
    void SnapToGrid();

protected:
    // ��λ����
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MovementRange = 5;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 MaxMovePoints = 3;

    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    int32 CurrentMovePoints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
	int32 MaxActionPoints = 3;

	UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
	int32 CurrentActionPoints;

    // ��ǰ����
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 StartCoordinates_Q = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Unit Stats")
    int32 StartCoordinates_R = 0;

    FLFPHexCoordinates CurrentCoordinates;

    // ѡ��״̬
    UPROPERTY(VisibleInstanceOnly, Category = "Unit State")
    bool bIsSelected = false;

    // �ƶ�״̬
    UPROPERTY(Transient)
    ALFPHexTile* TargetTile;

    TArray<ALFPHexTile*> MovePath;
    int32 CurrentPathIndex;
    float MoveProgress;

    // ����
    FTimerHandle MoveTimerHandle;
    FTimeline MoveTimeline;

    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UCurveFloat* MoveCurve;

    // ���
    UPROPERTY(VisibleAnywhere, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UPaperSpriteComponent* SpriteComponent;

public:
    // �غ�ϵͳ����
    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void ResetForNewRound();

    UFUNCTION(BlueprintPure, Category = "Turn System")
    bool CanAct() const { return !bHasActed && bOnTurn; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    int32 GetSpeed() const { return Speed; }

    UFUNCTION(BlueprintPure, Category = "Turn System")
    bool HasActed() const { return bHasActed; }

    UFUNCTION(BlueprintCallable, Category = "Turn System")
    void SetHasActed(bool bActed) { bHasActed = bActed; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn System")
    UTexture2D* GetUnitIcon() const { return UnitIconTexture; }

    // �غ��¼�
    UFUNCTION()
    void OnTurnStarted();

    UFUNCTION()
    void OnTurnEnded();

public:
    // �غ�ϵͳ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Speed = 5; // �ٶ�ֵ�������ж�˳��

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    bool bHasActed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State")
    bool bOnTurn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn System")
    UTexture2D* UnitIconTexture;

public:
    UFUNCTION(BlueprintCallable, Category = "Mouse Input")
    void OnMouseEnter();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mouse Input")
    void OnMouseExit();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mouse Input")
    void OnMouseClick();

	// �ڵ�λ�������Ѫ���仯�¼�
public:
	// �¼���Ѫ���仯
	UPROPERTY(BlueprintAssignable, Category = "Unit Events")
	FOnHealthChangedSignature OnHealthChangedDelegate;

	// �¼�����λ����
	UPROPERTY(BlueprintAssignable, Category = "Unit Events")
    FOnUnitDeathSignature OnDeathDelegate;

	// ��ȡ��ǰѪ��
	UFUNCTION(BlueprintPure, Category = "Unit Combat")
	int32 GetCurrentHealth() const { return CurrentHealth; }

	// ��ȡ���Ѫ��
	UFUNCTION(BlueprintPure, Category = "Unit Combat")
	int32 GetMaxHealth() const { return MaxHealth; }

	// ��ȡ��Ӫ��ʶ
	UFUNCTION(BlueprintPure, Category = "Unit Combat")
    EUnitAffiliation GetAffiliation() const { return Affiliation; }

	// Ѫ�����
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWidgetComponent* HealthBarComponent;

	// ��ʼ��Ѫ��
	UFUNCTION(BlueprintCallable, Category = "Unit Display")
	void InitializeHealthBar();

public:
    // Ѫ������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 MaxHealth = 100;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit Stats")
    int32 CurrentHealth = 100;

    // ����������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 AttackPower = 10;

    // ����������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    int32 Defense = 5;

    // ��Ӫ��ʶ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit Stats")
    EUnitAffiliation Affiliation = EUnitAffiliation::UA_Player;

    // ==== �������� ====

    // Ӧ���˺�
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    void TakeDamage(int32 Damage);

    // ���Ƶ�λ
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    void Heal(int32 Amount);

    // ����Ƿ���
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsAlive() const { return CurrentHealth > 0; }

    // ����Ƿ�Ϊ�з���λ
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsEnemy() const { return Affiliation == EUnitAffiliation::UA_Enemy; }

    // ����Ƿ�Ϊ�ѷ���λ
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsAlly() const { return Affiliation == EUnitAffiliation::UA_Player; }

    // ����Ƿ�Ϊ������λ
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsNeutral() const { return Affiliation == EUnitAffiliation::UA_Neutral; }

    // ����Ŀ�굥λ
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    bool AttackTarget(ALFPTacticsUnit* Target);

    void ApplyDamageToTarget(ALFPTacticsUnit* Target);

    // ��������
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void OnDeath();

    // �˺�����
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void OnTakeDamage(int32 DamageTaken);

    // ���ƴ���
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void OnHeal(int32 HealAmount);

    // ��������
    UFUNCTION(BlueprintImplementableEvent, Category = "Unit Combat")
    void PlayAttackAnimation(ALFPTacticsUnit* Target);

    // ��ȡ������Χ
    UFUNCTION(BlueprintCallable, Category = "Unit Combat")
    TArray<ALFPHexTile*> GetAttackRangeTiles();

    // ���Ŀ���Ƿ��ڹ�����Χ��
    UFUNCTION(BlueprintPure, Category = "Unit Combat")
    bool IsTargetInAttackRange(ALFPTacticsUnit* Target) const;

    // ��Ӫ��ɫ
    UFUNCTION(BlueprintPure, Category = "Unit Display")
    FLinearColor GetAffiliationColor() const;

    void UpdateHealthUI();

protected:
    // ��������
    void HandleDeath();

    // ������Χ
    UPROPERTY(EditAnywhere, Category = "Unit Combat")
    int32 AttackRange = 2;

    // ������Χģʽ
    UPROPERTY(EditAnywhere, Category = "Unit Combat")
    bool bMeleeAttack = true;

    // ����״̬
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit State", meta = (AllowPrivateAccess = "true"))
    bool bIsDead = false;
};
