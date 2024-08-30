#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../Structs/Block.h"
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
   block by block until it reaches the desired height. Width * Width * Height iterations.
 * After generating data, can create mesh based on that data.
 */
UCLASS()
class AChunk : public AActor
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

	TMap<FVector, FBlock> Blocks;

	/**
	 * Generates the chunk data based on noise. It does not create the mesh.
	 *
	 * @param Noise: A shared pointer to the FastNoiseLite instance used to generate noise values.
	 */
	void GenerateChunk(const TSharedPtr<FastNoiseLite>& Noise);

	/**
	 * Creates the mesh for the chunk using the generated chunk data.
	 */
	void CreateChunkMesh();

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	void ModifyBlock(const FVector& Position, const EBlockType& NewType);

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	void AddBlockFast(const FVector& Position, const EBlockType& NewType);

protected:
	TObjectPtr<UProceduralMeshComponent> Mesh;
	
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;

	/**
	 * Generates the chunk's mesh data (vertices, triangles, normals, UVs).
	 */
	void CreateChunkMeshData();

	/**
	 * Creates the vertex, normal, and triangle data for a single face of a block.
	 *
	 * @param Direction: The direction the face is facing (e.g., Up, Down, North).
	 * @param Position: The position of the block the face belongs to.
	 */
	void CreateFaceData(const EFaceDirection& Direction, const FVector& Position);

	/**
	 * Checks whether a block face is adjacent to an air block (empty space).
	 *
	 * @param Direction: The direction of the face being checked.
	 * @param Position: The position of the block being checked.
	 * @return true if the block face is next to air, false otherwise.
	 */
	bool IsBlockNextToAir(const EFaceDirection& Direction, const FVector& Position) const;

	/**
	 * Gets the vector value representing the direction of a block face.
	 *
	 * @param Direction: The face direction.
	 * @return A FVector corresponding to the face direction.
	 */
	FVector GetDirectionAsValue(const EFaceDirection& Direction) const;

	/**
	 * Limits the noise value to within a specified height range.
	 *
	 * @param NoiseValue: The original noise value.
	 * @param MinHeight: The minimum height limit.
	 * @param MaxHeight: The maximum height limit.
	 * @return The clamped noise value within the height range.
	 */
	float LimitNoise(float NoiseValue, int MinHeight, int MaxHeight) const;

	void ClearMesh();
};
