// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../Structs/Block.h"
#include "../../Interfaces/Chunkable.h"
#include "ChunkV2.generated.h"

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
 * 
 * Different meshing algorithm. Half cooked greedy meshing.
 * 3x slower, 3x less triangles, ~0% performance increase.
 */
UCLASS()
class AChunkV2 : public AActor, public IChunkable
{
	GENERATED_BODY()
public:
	AChunkV2();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	TObjectPtr<AChunkManager> Manager;
	int32 BlockSize;
	int32 Width;
	int32 Height;

	TMap<FVector, EBlockType> Blocks;
	TMap<FVector, EBlockType> PotentialFaces;

	void InitBaseData(
		const TObjectPtr<AChunkManager>& InManager,
		int32 InBlockSize,
		int32 InWidth,
		int32 InHeight
	) override;

	void GenerateChunk(const TSharedPtr<FastNoiseLite>& Noise) override;

	void CreateChunkMesh() override;

	void ModifyBlock(const FVector& Position, const EBlockType& NewType) override;

	void AddPotentialBlock(const FVector& Position) override;

	TMap<FVector, EBlockType>& GetBlocks() override;

	int32 GlobalIndex = 0;

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
	 * Will try to create faces where needed in Direction while
	   moving StartLocation in X axis.
	 * (With this function it would be possible to create faces in Y, -Y, Z and -Z directions)
	 */
	template <EFaceDirection Direction>
	void CreateFacesX(const FVector& StartLocation);

	/**
	 * Will try to create faces where needed in Direction while
	   moving StartLocation in Y axis.
	 * (With this function it would be possible to create faces in X, -X, Z and -Z directions)
	 */
	template <EFaceDirection Direction>
	void CreateFacesY(const FVector& StartLocation);

	/**
	 * Creates the vertex, normal, and triangle data for a single face of several blocks in a row.
	 * Extent in BlockSize * <how many blocks the face will take>
	 */
	void CreateFaceData(const EFaceDirection& Direction, float Extent, const FVector& StartLocation);

	/**
	 * Will walk in the X axis direction block by block
	   and stops when:
	 * There are no more blocks
	 * Stepped on Air
	 * Is adjacent to Air
	 *
	 * After stopping creates a face based on how many blocks it has traveled.
	 * Returns how many blocks it has traveled
	 */
	int32 WalkX(
		const FVector& StartLocation,
		const EBlockType& BlockType,
		const EFaceDirection& Direction
	);

	/**
	 * Will walk in the Y axis direction block by block
	   and stops when:
	 * There are no more blocks
	 * Stepped on Air
	 * Is adjacent to Air
	 *
	 * After stopping creates a face based on how many blocks it has traveled.
	 * Returns how many blocks it has traveled
	 */
	int32 WalkY(
		const FVector& StartLocation,
		const EBlockType& BlockType,
		const EFaceDirection& Direction
	);

	/**
	 * Gets the vector value representing the direction of a block face.
	 */
	FVector GetDirectionAsValue(const EFaceDirection& Direction) const;

	/**
	 * Limits the noise value to within a specified height range.
	 */
	float LimitNoise(float NoiseValue, int MinHeight, int MaxHeight) const;

	void ClearMesh();
};
