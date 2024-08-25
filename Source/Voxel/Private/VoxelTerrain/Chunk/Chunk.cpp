#include "VoxelTerrain/Chunk/Chunk.h"
#include "Kismet/KismetMathLibrary.h"
#include "../../Enums/BlockType.h"
#include "../../Enums/Direction.h"
#include "../../Structs/Block.h"
#include "../../FastNoiseLite.h"
#include "ProceduralMeshComponent.h"

AChunk::AChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	BlockSize = 100;
	Width = 32;
	Height = 32;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Noise = new FastNoiseLite();
	Noise->SetFrequency(0.03f);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);

	Mesh->SetCastShadow(false);

	RootComponent = Mesh;
}

void AChunk::GenerateChunk(const int32& InBlockSize, const int32& InWidth, const int32& InHeight)
{
	FVector NextBlockLocation = Mesh->GetRelativeLocation().GridSnap(BlockSize);
	float X = NextBlockLocation.X;
	float Y = NextBlockLocation.Y;
	int CurrentHeight = 0;

    BlockSize = InBlockSize;
	Width = InWidth;
	Height = InHeight;

	TArray<EFaceDirection> Directions = {
		EFaceDirection::X,
		EFaceDirection::Y,
		EFaceDirection::nX,
		EFaceDirection::nY,
		EFaceDirection::nZ,
		EFaceDirection::Z,
	};

	UE_LOG(LogTemp, Warning, TEXT("Starting chunk generation!"));

	for (int i = 0; i < Height; i++)
	{
		CurrentHeight = i;
		NextBlockLocation = FVector(X, Y, NextBlockLocation.Z + BlockSize).GridSnap(BlockSize);
		X = NextBlockLocation.X;
		Y = NextBlockLocation.Y;

		for (int j = 0; j < Width; j++)
		{
			NextBlockLocation = FVector(X, NextBlockLocation.Y + BlockSize, NextBlockLocation.Z).GridSnap(BlockSize);

			for (int k = 0; k < Width; k++)
			{
				NextBlockLocation = NextBlockLocation + FVector(BlockSize, 0, 0).GridSnap(BlockSize);
				float BlockHeight = Noise->GetNoise(NextBlockLocation.X, NextBlockLocation.Y);
				BlockHeight = floor(BlockHeight + 1 * NoiseStrength) - 8;

				(CurrentHeight >= BlockHeight) ?
					Blocks.Add(NextBlockLocation, FBlock(EBlockType::Air, NextBlockLocation)):
					Blocks.Add(NextBlockLocation, FBlock(EBlockType::Stone, NextBlockLocation));
			}
		}
	}

	TArray<FBlock> BlockData;
	Blocks.GenerateValueArray(BlockData);

	for (int i = 0; i < BlockData.Num(); i++)
	{
		if (BlockData[i].Type == EBlockType::Air) continue;

		for (int j = 0; j < Directions.Num(); j++)
		{
			if (IsNextToAir(Directions[j], BlockData[i].Location))
			{
				CreateFaceData(Directions[j], BlockData[i].Location);
			}
		}
	}

	ApplyMesh();
}

void AChunk::CreateFaceData(const EFaceDirection& Direction, const FVector& Position)
{
	float HalfBlockSize = BlockSize / 2;
	auto DirectionAsValue = GetDirectionAsValue(Direction);

	switch (Direction)
	{
	case EFaceDirection::X:
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize * -1, HalfBlockSize * -1));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize, HalfBlockSize));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize * -1, HalfBlockSize));
		break;
		
	case EFaceDirection::Y:
		Vertices.Add(Position + FVector(HalfBlockSize * -1, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(Position + FVector(HalfBlockSize * -1, HalfBlockSize, HalfBlockSize));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize, HalfBlockSize));
		break;

	case EFaceDirection::nX:
		Vertices.Add(Position + FVector(HalfBlockSize*-1, HalfBlockSize*-1, HalfBlockSize*-1));
		Vertices.Add(Position + FVector(HalfBlockSize*-1, HalfBlockSize, HalfBlockSize*-1));
		Vertices.Add(Position + FVector(HalfBlockSize*-1, HalfBlockSize*-1, HalfBlockSize));
		Vertices.Add(Position + FVector(HalfBlockSize*-1, HalfBlockSize, HalfBlockSize));
		break;

	case EFaceDirection::nY:
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize*-1, HalfBlockSize*-1));
		Vertices.Add(Position + FVector(HalfBlockSize*-1, HalfBlockSize*-1, HalfBlockSize*-1));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize*-1, HalfBlockSize));
		Vertices.Add(Position + FVector(HalfBlockSize*-1, HalfBlockSize*-1, HalfBlockSize));
		break;

	case EFaceDirection::nZ:
		Vertices.Add(Position + FVector(HalfBlockSize * -1, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(Position + FVector(HalfBlockSize * -1, HalfBlockSize * -1, HalfBlockSize * -1));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize * -1, HalfBlockSize * -1));
		break;

	case EFaceDirection::Z:
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize, HalfBlockSize));
		Vertices.Add(Position + FVector(HalfBlockSize, HalfBlockSize * -1, HalfBlockSize));
		Vertices.Add(Position + FVector(HalfBlockSize * -1, HalfBlockSize, HalfBlockSize));
		Vertices.Add(Position + FVector(HalfBlockSize * -1, HalfBlockSize * -1, HalfBlockSize));
		break;
	}

	Normals.Append({
		DirectionAsValue,
		DirectionAsValue,
		DirectionAsValue,
		DirectionAsValue
	});
	UVs.Append({
		FVector2D(0, 0),
		FVector2D(0, 1),
		FVector2D(1, 0),
		FVector2D(1, 1)
	});
	Triangles.Append({
		Vertices.Num() - 4,
		Vertices.Num() - 3,
		Vertices.Num() - 2,
		Vertices.Num() - 2,
		Vertices.Num() - 3,
		Vertices.Num() - 1
	});
}



bool AChunk::IsNextToAir(const EFaceDirection& Direction, const FVector& Position) const
{
	FVector BlockInDirection = (GetDirectionAsValue(Direction) * BlockSize + Position).GridSnap(BlockSize);
	if (Blocks.Find(BlockInDirection) != nullptr)
	{
		return Blocks[BlockInDirection].Type == EBlockType::Air;
	}
	else 
	{
		return true;
	}
	
}

FVector AChunk::GetDirectionAsValue(const EFaceDirection& Direction) const 
{
	TArray<FVector> Directions = {
		FVector(1, 0, 0),
		FVector(0, 1, 0),
		FVector(-1, 0, 0),
		FVector(0, -1, 0),
		FVector(0, 0, -1),
		FVector(0, 0, 1)
	};

	int32 Index = static_cast<int32>(Direction);

	return Directions[Index];
}

void AChunk::ApplyMesh()
{
	Mesh->CreateMeshSection(
		0,
		Vertices, 
		Triangles, 
		Normals, 
		UVs, 
		TArray<FColor>(), 
		TArray<FProcMeshTangent>(), 
		true
	);
}
