
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
		uint8 Light;
		bool IsDestroyable;

		FBlock();
		FBlock(const EBlockType& InType, uint8 InLight, bool InIsDestroyable);
		FBlock(const EBlockType& InType, uint16 InDecorationId, uint8 InLight, bool InIsDestroyable);
		~FBlock();
};
