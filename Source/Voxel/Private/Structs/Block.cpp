
#include "Structs/Block.h"
#include "../Enums/BlockType.h"

FBlock::FBlock()
{
	IsDestroyable = true;
	Light = 0;
	DecorationId = 0;
	Type = EBlockType::Air;
}

FBlock::FBlock(const EBlockType& InType, uint16 InDecorationId, uint8 InLight, bool InIsDestroyable)
{
	IsDestroyable = InIsDestroyable;
	Light = InLight;
	Type = InType;
	DecorationId = InDecorationId;
}

FBlock::FBlock(const EBlockType& InType, uint8 InLight, bool InIsDestroyable)
{
	IsDestroyable = InIsDestroyable;
	Light = InLight;
	Type = InType;
	DecorationId = 0;
}

FBlock::~FBlock()
{
}
