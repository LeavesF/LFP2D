#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "LFPTownData.generated.h"

// 城镇建筑类型
UENUM(BlueprintType)
enum class ELFPTownBuildingType : uint8
{
	TBT_Shop            UMETA(DisplayName = "商店"),
	TBT_EvolutionTower  UMETA(DisplayName = "升华塔"),
	TBT_Teleport        UMETA(DisplayName = "传送阵"),
	TBT_QuestNPC        UMETA(DisplayName = "任务NPC"),
	TBT_SkillNode       UMETA(DisplayName = "技能节点"),
};

// 单个建筑配置
USTRUCT(BlueprintType)
struct FLFPTownBuildingEntry
{
	GENERATED_BODY()

	// 建筑类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ELFPTownBuildingType BuildingType = ELFPTownBuildingType::TBT_Shop;

	// 建筑图标
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> BuildingIcon;

	// 建筑显示名
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	// 是否可用（未解锁的建筑灰显）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;

	// 传送阵专用：目标节点 ID 列表（分号分隔）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TeleportTargetNodeIDs;
};

// ============== 工具函数 ==============

namespace LFPTownUtils
{
	// 建筑类型名 → 枚举
	inline bool StringToBuildingType(const FString& InStr, ELFPTownBuildingType& OutType)
	{
		static const TMap<FString, ELFPTownBuildingType> Map = {
			{TEXT("Shop"), ELFPTownBuildingType::TBT_Shop},
			{TEXT("EvolutionTower"), ELFPTownBuildingType::TBT_EvolutionTower},
			{TEXT("Teleport"), ELFPTownBuildingType::TBT_Teleport},
			{TEXT("QuestNPC"), ELFPTownBuildingType::TBT_QuestNPC},
			{TEXT("SkillNode"), ELFPTownBuildingType::TBT_SkillNode},
		};
		if (const ELFPTownBuildingType* Found = Map.Find(InStr))
		{
			OutType = *Found;
			return true;
		}
		return false;
	}

	// 枚举 → 建筑类型名
	inline FString BuildingTypeToString(ELFPTownBuildingType InType)
	{
		static const TMap<ELFPTownBuildingType, FString> Map = {
			{ELFPTownBuildingType::TBT_Shop, TEXT("Shop")},
			{ELFPTownBuildingType::TBT_EvolutionTower, TEXT("EvolutionTower")},
			{ELFPTownBuildingType::TBT_Teleport, TEXT("Teleport")},
			{ELFPTownBuildingType::TBT_QuestNPC, TEXT("QuestNPC")},
			{ELFPTownBuildingType::TBT_SkillNode, TEXT("SkillNode")},
		};
		if (const FString* Found = Map.Find(InType))
		{
			return *Found;
		}
		return TEXT("Unknown");
	}

	// 分号分隔字符串 → 建筑类型数组（如 "Shop;EvolutionTower;Teleport"）
	inline TArray<ELFPTownBuildingType> ParseBuildingList(const FString& InStr)
	{
		TArray<ELFPTownBuildingType> Result;
		TArray<FString> Tokens;
		InStr.ParseIntoArray(Tokens, TEXT(";"), true);
		for (const FString& Token : Tokens)
		{
			FString Trimmed = Token.TrimStartAndEnd();
			ELFPTownBuildingType Type;
			if (StringToBuildingType(Trimmed, Type))
			{
				Result.Add(Type);
			}
		}
		return Result;
	}

	// 建筑类型数组 → 分号分隔字符串
	inline FString SerializeBuildingList(const TArray<ELFPTownBuildingType>& InList)
	{
		TArray<FString> Strings;
		for (ELFPTownBuildingType Type : InList)
		{
			Strings.Add(BuildingTypeToString(Type));
		}
		return FString::Join(Strings, TEXT(";"));
	}
}
