#include "VoxelTerrain/Chunk/Chunk.h"
#include "Kismet/KismetMathLibrary.h"
#include "../../Enums/BlockType.h"
#include "../../Enums/Direction.h"
#include "../../Structs/Block.h"
#include "../../FastNoiseLite.h"
#include "../World/ChunkManager.h"
#include "ProceduralMeshComponent.h"
#include "Components/SceneComponent.h"

AChunk::AChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	BlockSize = 100;
	Width = 32;
	Height = 32;

	Directions = {
		EFaceDirection::X,
		EFaceDirection::Y,
		EFaceDirection::nX,
		EFaceDirection::nY,
		EFaceDirection::nZ,
		EFaceDirection::Z,
	};

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetCastShadow(false);

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));

	RootComponent = RootSceneComponent;
}


void AChunk::InitBaseData(const TObjectPtr<AChunkManager>& InManager, int32 InBlockSize, int32 InWidth, int32 InHeight)
{
	Manager = InManager;
	BlockSize = InBlockSize;
	Width = InWidth;
	Height = InHeight;
}

void AChunk::GenerateChunk(const TSharedPtr<FastNoiseLite>& Noise)
{
	FVector NextBlockLocation = RootComponent->GetRelativeLocation();
	float X = NextBlockLocation.X;
	float Y = NextBlockLocation.Y;
	int CurrentHeight = 0;

	for (int i = 0; i < Height; i++)
	{
		CurrentHeight = i;
		
		for (int j = 0; j < Width; j++)
		{
			for (int k = 0; k < Width; k++)
			{
				NextBlockLocation = (NextBlockLocation + FVector(BlockSize, 0, 0)).GridSnap(BlockSize);
				float BlockHeight = Noise->GetNoise(NextBlockLocation.X / 100, NextBlockLocation.Y / 100);
				BlockHeight = LimitNoise(BlockHeight, 6, 32);

				(CurrentHeight >= BlockHeight) ?
					Blocks.Add(NextBlockLocation, EBlockType::Air):
					Blocks.Add(NextBlockLocation, EBlockType::Stone);
			}

			NextBlockLocation = FVector(X, NextBlockLocation.Y, NextBlockLocation.Z).GridSnap(BlockSize);
			NextBlockLocation = (NextBlockLocation + FVector(0, BlockSize, 0)).GridSnap(BlockSize);
		}

		NextBlockLocation = FVector(X, Y, NextBlockLocation.Z).GridSnap(BlockSize);
		NextBlockLocation = (NextBlockLocation + FVector(0, 0, BlockSize)).GridSnap(BlockSize);
		X = NextBlockLocation.X;
		Y = NextBlockLocation.Y;
	}

	TArray<FVector> BlockLocs;
	Blocks.GenerateKeyArray(BlockLocs);
	TArray<EBlockType> BlockTypes;
	Blocks.GenerateValueArray(BlockTypes);

	for (int i = 0; i < BlockLocs.Num(); i++)
	{
		if (BlockTypes[i] == EBlockType::Air) continue;

		for (int j = 0; j < Directions.Num(); j++)
		{
			if (!IsBlockNextToAir(Directions[j], BlockLocs[i])) continue;

			PotentialBlocks.Add(BlockLocs[i], BlockTypes[i]);
		}
	}
}

void AChunk::ModifyBlock(const FVector& Position, const EBlockType& NewType)
{
	auto Block = Blocks.Find(Position.GridSnap(BlockSize));
	if (!Block) return;

	*Block = NewType;
	PotentialBlocks.Add(Position, NewType);

	for (const EFaceDirection& Direction : Directions)
	{
		FVector BlockPosition;
		EBlockType BlockType;

		GetBlockInDirection(Position, Direction, BlockPosition, BlockType);

		PotentialBlocks.Add(BlockPosition, BlockType);
	}

	EmptyMeshData();
	CreateChunkMesh();
}

void AChunk::CreateChunkMesh()
{
	CreateChunkMeshData();

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

	EmptyMeshData();
}

void AChunk::CreateChunkMeshData()
{
	TArray<FVector> BlockLocs;
	PotentialBlocks.GenerateKeyArray(BlockLocs);
	TArray<EBlockType> BlockTypes;
	PotentialBlocks.GenerateValueArray(BlockTypes);

	for (int i = 0; i < BlockLocs.Num(); i++)
	{
		if (BlockTypes[i] == EBlockType::Air)
		{
			PotentialBlocks.Remove(BlockLocs[i]);
			continue;
		}

		bool IsFaceCreated = false;
		for (int j = 0; j < Directions.Num(); j++)
		{
			if (!IsBlockNextToAir(Directions[j], BlockLocs[i])) 
				continue;

			IsFaceCreated = true;
			CreateFaceData(Directions[j], BlockLocs[i]);
		}

		if(!IsFaceCreated)
			PotentialBlocks.Remove(BlockLocs[i]);
	}
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

bool AChunk::IsBlockNextToAir(const EFaceDirection& Direction, const FVector& Position) const
{
	FVector BlockInDirection = (GetDirectionAsValue(Direction) * BlockSize + Position).GridSnap(BlockSize);
	if (Blocks.Find(BlockInDirection) != nullptr)
	{
		return Blocks[BlockInDirection] == EBlockType::Air;
	}

	return Manager.Get()->IsBlockAir(GetActorLocation() + GetDirectionAsValue(Direction) * BlockSize * Width, BlockInDirection);
}

void AChunk::AddPotentialBlock(const FVector& Position)
{
	auto Block = Blocks.Find(Position.GridSnap(BlockSize));
	if (!Block) return;

	PotentialBlocks.Add(Position, *Block);
}

TMap<FVector, EBlockType>& AChunk::GetBlocks()
{
	return Blocks;
}

float AChunk::LimitNoise(float NoiseValue, int MinHeight, int MaxHeight) const
{
	float normalizedValue = (NoiseValue + 1.0f) / 2.0f;
	int voxelHeight = static_cast<int>(normalizedValue * (MaxHeight - MinHeight) + MinHeight);

	voxelHeight = FMath::Clamp(voxelHeight, MinHeight, MaxHeight);

	return voxelHeight;
}

void AChunk::GetBlockInDirection(const FVector& Position, const EFaceDirection& Direction, FVector& BlockPosition, EBlockType& BlockType) const
{
	FVector BlockInDirection = (GetDirectionAsValue(Direction) * BlockSize + Position).GridSnap(BlockSize);
	
	if (!Blocks.Find(BlockInDirection))
	{
		Manager->AddPotentialBlockAndRebuild(GetActorLocation() + GetDirectionAsValue(Direction) * BlockSize * Width, BlockInDirection);
		BlockType = EBlockType::Air;
		BlockPosition = BlockInDirection;
		return;
	}

	BlockType = *Blocks.Find(BlockInDirection);
	BlockPosition = BlockInDirection;
}

void AChunk::EmptyMeshData()
{
	Vertices.Empty();
	UVs.Empty();
	Normals.Empty();
	Triangles.Empty();
}

FVector AChunk::GetDirectionAsValue(const EFaceDirection& Direction) const
{
	TArray<FVector> Dire = {
		FVector(1, 0, 0),
		FVector(0, 1, 0),
		FVector(-1, 0, 0),
		FVector(0, -1, 0),
		FVector(0, 0, -1),
		FVector(0, 0, 1)
	};

	int32 Index = static_cast<int32>(Direction);

	return Dire[Index];
}
