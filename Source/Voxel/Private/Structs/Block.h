
#pragma once

#include "CoreMinimal.h"
#include "Block.generated.h"

enum class EBlockType : uint8;

USTRUCT(BlueprintType)
struct FBlock
{
	public:
		GENERATED_BODY()

		EBlockType Type;
		uint16 DecorationId;

		FBlock();
		FBlock(const EBlockType& InType);
		FBlock(const EBlockType& InType, uint16 InDecorationId);
		~FBlock();
};
