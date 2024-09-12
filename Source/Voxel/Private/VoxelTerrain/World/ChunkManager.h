#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChunkManager.generated.h"

class FastNoiseLite;
class AChunk;
enum class EBlockType : uint8;
class IChunkable;
struct FBlock;

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
	 * Chunk type to spawn.
	 * Actor Chunk should implement interface IChunkable
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChunkManager")
	TSubclassOf<AActor> ChunkType;

	/**
	 * How many chunks can be generated per tick.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChunkManager")
	int32 MaxChunksPerTick;

	/**
	 * How many meshes of chunks can be generated per tick.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChunkManager")
	int32 MaxMeshesPerTick;

	/**
	 * Generates chunks within the defined draw distance around the player.
	 *
	 * This function calculates the chunk positions around the player, spawns chunks at those positions,
	 * generates their terrain using noise, and then creates the mesh for each generated chunk.
	 */
	UFUNCTION(BlueprintCallable, Category = "ChunkManager")
	void RegenerateChunks();

	/**
	 * Adds a new Block at a specified location.
	 */
	UFUNCTION(BlueprintCallable, Category = "ChunkManager")
	void AddBlock(const FVector& Position, const EBlockType& NewType);

	/**
	 * Removes a Block at a specified location.
	 */
	UFUNCTION(BlueprintCallable, Category = "ChunkManager")
	void RemoveBlock(const FVector& Position);

	/**
	 * Adds a potential block that might have faces to the chunk and rebuilds chunk mesh.
	 */
	void AddPotentialBlockAndRebuild(const FVector& ChunkLocation, const FVector& BlockPosition);

	/**
	 * Checks if given BlockLocation exists in given Chunk.
	 */
	bool IsBlockAir(const FVector& ChunkLocation, const FVector& BlockLocation) const;

protected:
	TSharedPtr<FastNoiseLite> Noise;
	TMap<FVector, TObjectPtr<AActor>> GeneratedChunks;
	TQueue<FVector> ChunkQueue;
	TQueue<IChunkable*> MeshQueue;
	TArray<TObjectPtr<AActor>> ChunkPool;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	void ProcessMeshGeneration();
	void ProcessChunkGeneration();
	void EnqueueChunks(const TArray<FVector>& ChunkPositions);
	void EnqueueMesh(IChunkable* Chunk);

	/**
	 * Spawns a chunk at the specified location in the world.
	 */
	TObjectPtr<AActor> SpawnChunk(const FVector& Location);

	/**
	 * Calculates and returns the positions of chunks within the draw distance around a center point.
	 */
	void GetChunkPositions(const FVector& Center, TArray<FVector>& OutPositions);

	/**
	 * Retrieves the location of the player in the world.
	 */
	FVector GetPlayerLocation() const;
};
