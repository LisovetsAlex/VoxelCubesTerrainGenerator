#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../Structs/Block.h"
#include "../../Interfaces/Chunkable.h"
#include "Chunk.generated.h"

enum class EFaceDirection;
enum class EBlockType : uint8;
class UProceduralMeshComponent;
class FastNoiseLite;
class AChunkManager;
class USceneComponent;

/**
 * Chunk of blocks
 * 
 * It can generate blocks based on the given Noise. It will iterate through the cube
   block by block until it reaches the desired height.
 * After generating data, can create mesh based on that data.
 */
UCLASS()
class AChunk : public AActor, public IChunkable
{
	GENERATED_BODY()

public:
	AChunk();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	TObjectPtr<AChunkManager> Manager;
	int32 BlockSize;
	int32 Width;
	int32 Height;

	//All Blocks in a chunk
	TMap<FVector, EBlockType> Blocks;

	//Blocks that will most likely have faces
	TMap<FVector, EBlockType> PotentialBlocks;

	/**
	 * Sets Chunk Instance with essential data for chunks.
	 */
	void InitBaseData(
		const TObjectPtr<AChunkManager>& InManager,
		int32 InBlockSize,
		int32 InWidth,
		int32 InHeight
	) override;

	/**
	 * Generates the chunk data based on noise. It does not create the mesh.
	 */
	void GenerateChunk(const TSharedPtr<FastNoiseLite>& Noise) override;

	/**
	 * Creates the mesh for the chunk using the generated chunk data.
	 */
	void CreateChunkMesh() override;

	/**
	 * Changes the block type in a chunk.
	 */
	void ModifyBlock(const FVector& Position, const EBlockType& NewType) override;

	/**
	 * Will add a block from all blocks to a list of
	   potential blocks that might have faces.
	 */
	void AddPotentialBlock(const FVector& Position) override;

	/**
	 * Returns all blocks in this chunk.
	 */
	TMap<FVector, EBlockType>& GetBlocks() override;

protected:
	TObjectPtr<UProceduralMeshComponent> Mesh;
	
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;

	TArray<EFaceDirection> Directions;

	/**
	 * Generates the chunk's mesh data (vertices, triangles, normals, UVs).
	 */
	void CreateChunkMeshData();

	/**
	 * Creates the vertex, normal, and triangle data for a single face of a block.
	 */
	void CreateFaceData(const EFaceDirection& Direction, const FVector& Position);

	/**
	 * Checks whether a block face is adjacent to an air block (empty space).
	 */
	bool IsBlockNextToAir(const EFaceDirection& Direction, const FVector& Position) const;

	/**
	 * Gets the vector value representing the direction of a block face.
	 */
	FVector GetDirectionAsValue(const EFaceDirection& Direction) const;

	/**
	 * Limits the noise value to within a specified height range.
	 */
	float LimitNoise(float NoiseValue, int MinHeight, int MaxHeight) const;

	/**
	 * Returns block in given direction relative to certain block.
	 */
	void GetBlockInDirection(const FVector& Position, const EFaceDirection& Direction, FVector& BlockPosition, EBlockType& BlockType) const;

	/**
	 * Empties arrays with vertex, normal, and triangle data
	 */
	void EmptyMeshData();
};
