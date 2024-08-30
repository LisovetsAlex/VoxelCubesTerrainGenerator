#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChunkManager.generated.h"

class FastNoiseLite;
class AChunk;
enum class EBlockType : uint8;

/**
 * Manages chunk actions like drawing, generating, adding/removing blocks
   from chunks.
 */
UCLASS()
class AChunkManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AChunkManager();

	/**
	 * The draw distance in chunks, defining how far from the player chunks will be generated.
	 *
	 * This value determines the number of chunks to generate around the player in all directions.
	 * A higher value will result in more chunks being generated, and has performance impact.
	 * 
	 * Acceptable values: power of 2 (2, 4, 8, 16...)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ChunkManager")
	int32 DrawDistance;

	/**
	 * How big should a block be.
	 * Smaller blocks will have performance impact.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ChunkManager")
	int32 BlockSize;

	/**
	 * How wide should a chunk be.
	 * Wider chunks will take longer to draw and generate.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ChunkManager")
	int32 ChunkWidth;

	/**
	 * How tall should a chunk be.
	 * Taller chunks will take longer to draw and generate.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ChunkManager")
	int32 ChunkHeight;

	/**
	 * Generates chunks within the defined draw distance around the player.
	 *
	 * This function calculates the chunk positions around the player, spawns chunks at those positions,
	 * generates their terrain using noise, and then creates the mesh for each generated chunk.
	 */
	UFUNCTION(BlueprintCallable, Category = "ChunkManager")
	void GenerateChunks();

	/**
	 * Adds a new Block at a specified location.
	 *
	 * @param Position: The location of the new block to place.
	 * @param NewType: Type of the block.
	 */
	UFUNCTION(BlueprintCallable, Category = "ChunkManager")
	void AddBlock(const FVector& Position, const EBlockType& NewType);

	/**
	 * Removes a Block at a specified location.
	 *
	 * @param Position: The location of the block to remove.
	 */
	UFUNCTION(BlueprintCallable, Category = "ChunkManager")
	void RemoveBlock(const FVector& Position);

	/**
	 * Checks if a block at the given location in a chunk is an air block.
	 *
	 * Used in AChunk to create only visible faces.
	 *
	 * @param ChunkLocation: The location of the chunk containing the block.
	 * @param BlockLocation: The location of the block within the chunk.
	 * @return true if the block is adjacent to an air block, false otherwise.
	 */
	bool IsBlockAir(const FVector& ChunkLocation, const FVector& BlockLocation) const;

protected:
	TSharedPtr<FastNoiseLite> Noise;
	TMap<FVector, TObjectPtr<AChunk>> GeneratedChunks;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/**
	 * Spawns a chunk at the specified location in the world.
	 *
	 * @param Location: The location in the world where the chunk will be spawned.
	 * @return A pointer to the spawned chunk actor, or nullptr if the chunk could not be spawned.
	 */
	TObjectPtr<AChunk> SpawnChunk(const FVector& Location);

	/**
	 * Calculates and returns the positions of chunks within the draw distance around a center point.
	 *
	 * @param Center: The center point from which to calculate chunk positions.
	 * @param OutPositions: An array that will be filled with the calculated chunk positions.
	 */
	void GetChunkPositions(const FVector& Center, TArray<FVector>& OutPositions);

	/**
	 * Retrieves the location of the player in the world.
	 *
	 * @return The player's camera location as an FVector, or FVector::ZeroVector if the player cannot be found.
	 */
	FVector GetPlayerLocation() const;
};
