
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
		FVector Location;

		FBlock();
		FBlock(const EBlockType& InType, const FVector& InLocation);
		~FBlock();
};
