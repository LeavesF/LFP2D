// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "LFPSkillBase.generated.h"

class ALFPTacticsUnit;
class ALFPHexTile;
class ALFPTacticsPlayerController;

struct FLFPHexCoordinates;

UENUM(BlueprintType)
enum class ESkillTargetType : uint8
{
    Self,           // ���Լ�ʩ��
    SingleAlly,     // �����ѷ�
    SingleEnemy,    // �����з�
    AreaAlly,       // �����ѷ�
    AreaEnemy,      // ����з�
    AreaAll,        // �������е�λ
    Tile            // �ض�����
};

USTRUCT(BlueprintType)
struct FSkillRange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MinRange = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxRange = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequireLineOfSight = true;
};
/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class LFP2D_API ULFPSkillBase : public UObject
{
	GENERATED_BODY()
	
public:
    ULFPSkillBase();

    // ����ִ��
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Skill")
    void Execute(ALFPHexTile* TargetTile = nullptr);

    // ��鼼���Ƿ����
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill")
    bool CanExecute(ALFPHexTile* TargetTile = nullptr);

    // ��ȡ������ȴ״̬
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    FString GetCooldownStatus() const;

    // ��ȡ���ܷ�Χ�ڵ�����Ŀ�����
    UFUNCTION(BlueprintCallable, Category = "Skill")
    TArray<ALFPHexTile*> GetTargetTiles(ALFPTacticsUnit* Caster) const;

    // �غϿ�ʼʱ����ȴ����
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void OnTurnStart();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Skill")
    void UpdateSkillRange();

    UFUNCTION(BlueprintCallable, Category = "Skill")
    TArray<FLFPHexCoordinates> GetReleaseRange() { return ReleaseRangeCoords; }

    // AI 目标选择：计算对目标的仇恨值（值越高，越优先攻击）
    // Caster：使用技能的敌方单位（自身属性/位置）
    // Target：候选的玩家单位（目标属性/位置）
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Skill|AI")
    float CalculateHatredValue(ALFPTacticsUnit* Caster, ALFPTacticsUnit* Target) const;

    /*UFUNCTION(BlueprintCallable, Category = "Skill")
    void ShowReleaseRange(bool bShow = true);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void ShowEffectRange(bool bShow = true);*/
public:
    // ��������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ALFPTacticsUnit* Owner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ALFPTacticsPlayerController* OwnerController;

    // ��������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FText SkillDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    UTexture2D* SkillIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 CooldownRounds; // ��ȴ�غ���

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
    int32 CurrentCooldown; // ��ǰ��ȴ����

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    ESkillTargetType TargetType;

    // ���ܱ�ǩ�����ڷ���͹���
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FGameplayTagContainer SkillTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    FSkillRange Range;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 ActionPointCost; // �ж�������

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bIsDefaultAttack; // �Ƿ�ΪĬ�Ϲ�������

    // �����ͷŷ�Χ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TArray<FLFPHexCoordinates> ReleaseRangeCoords;

    // ������Ч��Χ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    TArray<FLFPHexCoordinates> EffectRangeCoords;
};
