// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_FindSkillTarget.generated.h"

/**
 * 根据技能的仇恨值公式为 AI 选择最优攻击目标。
 * 输入：黑板中的技能对象（ULFPSkillBase）
 * 输出：仇恨值最高的玩家单位（写入黑板 TargetUnitKey）
 */
UCLASS()
class LFP2D_API UBTTask_FindSkillTarget : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FindSkillTarget();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // 黑板键（输入）：要使用的技能对象（Object 类型，指向 ULFPSkillBase）
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector SkillKey;

    // 黑板键（输出）：仇恨值最高的目标单位（Object 类型，指向 ALFPTacticsUnit）
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetUnitKey;
};
