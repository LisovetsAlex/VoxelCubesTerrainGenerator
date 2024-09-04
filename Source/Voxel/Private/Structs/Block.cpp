
#include "Structs/Block.h"

FBlock::FBlock()
{

}

FBlock::FBlock(const EBlockType& InType, uint16 InDecorationId)
{
	Type = InType;
	DecorationId = InDecorationId;
}

FBlock::FBlock(const EBlockType& InType)
{
	Type = InType;
	DecorationId = 0;
}

FBlock::~FBlock()
{
}
