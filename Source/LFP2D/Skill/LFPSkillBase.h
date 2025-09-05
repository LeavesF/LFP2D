// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "LFPSkillBase.generated.h"

class ALFPTacticsUnit;
class ALFPHexTile;

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
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void Execute(ALFPTacticsUnit* Caster, ALFPHexTile* TargetTile);

    // ��鼼���Ƿ����
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual bool CanExecute(ALFPTacticsUnit* Caster) const;

    // ��ȡ������ȴ״̬
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    FString GetCooldownStatus() const;

    // ��ȡ���ܷ�Χ�ڵ�����Ŀ�����
    UFUNCTION(BlueprintCallable, Category = "Skill")
    TArray<ALFPHexTile*> GetTargetTiles(ALFPTacticsUnit* Caster) const;

    // �غϿ�ʼʱ����ȴ����
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void OnTurnStart();

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
};
