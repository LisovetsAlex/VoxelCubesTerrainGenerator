
#include "Structs/Block.h"

FBlock::FBlock(const EBlockType& InType, const FVector& InLocation)
{
	Type = InType;
	Location = InLocation;
}

FBlock::FBlock()
{

}

FBlock::~FBlock()
{
}
