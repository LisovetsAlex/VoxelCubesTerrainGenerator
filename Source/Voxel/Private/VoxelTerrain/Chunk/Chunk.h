#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../../Structs/Block.h"
#include "Chunk.generated.h"

enum class EFaceDirection;
enum class EBlockType;
class FastNoiseLite;
class UProceduralMeshComponent;

UCLASS()
class AChunk : public AActor
{
	GENERATED_BODY()

public:
	AChunk();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chunk")
	int32 BlockSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chunk")
	int32 Width;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chunk")
	int32 Height;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Chunk")
	float NoiseStrength;

	UFUNCTION(BlueprintCallable, Category = "Chunk")
	void GenerateChunk(const int32& InBlockSize, const int32& InWidth, const int32& InHeight);

protected:
	TObjectPtr<UProceduralMeshComponent> Mesh;
	TObjectPtr<FastNoiseLite> Noise;

	TMap<FVector, FBlock> Blocks;
	
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;

	void CreateFaceData(const EFaceDirection& Direction, const FVector& Position);
	bool IsNextToAir(const EFaceDirection& Direction, const FVector& Position) const;
	FVector GetDirectionAsValue(const EFaceDirection& Direction) const;
	void ApplyMesh();

};
