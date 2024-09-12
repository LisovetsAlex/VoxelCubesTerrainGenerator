
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Chunkable.generated.h"

struct FBlock;
class FastNoiseLite;
class AChunkManager;
enum class EBlockType : uint8;

UINTERFACE(MinimalAPI)
class UChunkable : public UInterface
{
	GENERATED_BODY()
};

class IChunkable
{
	GENERATED_BODY()

public:
	/**
	 * Sets Chunk Instance with essential data for chunks.
	 */
	virtual void InitBaseData(
		const TObjectPtr<AChunkManager>& InManager,
		int32 InBlockSize,
		int32 InWidth,
		int32 InHeight
	) = 0;

	/**
	 * Generates the chunk data based on noise. It does not create the mesh.
	 */
	virtual void GenerateChunk(const TSharedPtr<FastNoiseLite>& Noise) = 0;

	/**
	 * Creates data for the mesh for the chunk using the generated chunk data.
	 * 
	 * Set IsGenerating to true only when for the first generating chunk and mesh
	 */
	virtual void CreateChunkMesh(bool IsGenerating) = 0;

	/**
	 * Appllies the mesh date and creates the mesh.
	 */
	virtual void ApplyMesh() = 0;

	/**
	 * Changes the block type in a chunk.
	 */
	virtual void ModifyBlock(const FVector& Position, const EBlockType& NewType) = 0;

	/**
	 * Will add a block from all blocks in a chunk to a list of
	   potential blocks that might have faces.
	 */
	virtual void AddPotentialBlock(const FVector& Position) = 0;

	/**
	 * Returns all blocks in this chunk.
	 */
	virtual TMap<FVector, FBlock>& GetBlocks() = 0;
};
