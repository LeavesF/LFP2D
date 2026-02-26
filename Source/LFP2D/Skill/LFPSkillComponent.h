// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LFP2D/Skill/LFPSkillBase.h"
#include "LFPSkillComponent.generated.h"

// 委托签名
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillExecutedSignature, ALFPTacticsUnit*, Caster, ALFPHexTile*, TargetTile);

UCLASS(Blueprintable, ClassGroup=(Skill), meta=(BlueprintSpawnableComponent))
class LFP2D_API ULFPSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULFPSkillComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 初始化技能
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void InitializeSkills();

    // 获取所有可用技能
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    TArray<ULFPSkillBase*> GetAvailableSkills() const;

    // 执行技能
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool ExecuteSkill(ULFPSkillBase* Skill, ALFPHexTile* TargetTile);

    // 获取默认攻击技能
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    ULFPSkillBase* GetDefaultAttackSkill() const;

    // 回合开始处理
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void OnTurnStarted();

    // 技能执行事件
    UPROPERTY(BlueprintAssignable, Category = "Skill")
    FOnSkillExecutedSignature OnSkillExecuted;

protected:
    // 所有技能
    UPROPERTY(EditAnywhere, Category = "Skill")
    TArray<ULFPSkillBase*> Skills;

    // 默认攻击技能
    UPROPERTY(EditAnywhere, Category = "Skill")
    ULFPSkillBase* DefaultAttackSkill;

    // 技能数据资产
    UPROPERTY(EditAnywhere, Category = "Skill")
    class ULFPSkillDataAsset* SkillData;
};
