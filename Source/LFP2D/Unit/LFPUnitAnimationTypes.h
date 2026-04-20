#pragma once

#include "CoreMinimal.h"
#include "LFPUnitAnimationTypes.generated.h"

class ALFPHexTile;
class ALFPTacticsUnit;
class ULFPSkillBase;

UENUM(BlueprintType)
enum class ELFPUnitAnimState : uint8
{
    Idle,   // 常驻待机状态
    Move,   // 路径移动中
    Attack, // 近战/物理类动作
    Cast,   // 施法/远程类动作
    Hit,    // 受击硬直
    Death   // 死亡，优先级最高
};

UENUM(BlueprintType)
enum class ELFPSkillActionAnimType : uint8
{
    Attack,
    Cast
};

USTRUCT(BlueprintType)
struct FLFPPendingActionContext
{
    GENERATED_BODY()

    // 当前是否存在一个等待动画提交的动作
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bIsActive = false;

    // commit notify 是否已经触发过，防止重复结算
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bCommitted = false;

    // AP 是否已在动作请求阶段扣除
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    bool bActionPointsConsumed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<ULFPSkillBase> Skill = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<ALFPHexTile> TargetTile = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<ALFPTacticsUnit> TargetUnit = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    ELFPSkillActionAnimType ActionAnimType = ELFPSkillActionAnimType::Attack;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
    FName AnimationKey = NAME_None;

    void Reset()
    {
        bIsActive = false; // 统一清空动作上下文，避免单位保留上一段动作残留状态
        bCommitted = false;
        bActionPointsConsumed = false;
        Skill = nullptr;
        TargetTile = nullptr;
        TargetUnit = nullptr;
        ActionAnimType = ELFPSkillActionAnimType::Attack;
        AnimationKey = NAME_None;
    }
};
