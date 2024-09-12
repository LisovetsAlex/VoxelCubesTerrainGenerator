#include "VoxelTerrain/Chunk/Chunk.h"
#include "../../Enums/BlockType.h"
#include "../../Enums/Direction.h"
#include "../../Structs/Block.h"
#include "../../FastNoiseLite.h"
#include "../World/ChunkManager.h"
#include "ProceduralMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

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
	Mesh->SetCastShadow(true);
	Mesh->bSelfShadowOnly = true;
	Mesh->bCastContactShadow = true;
	Mesh->bCastStaticShadow = true;
	Mesh->bAffectDynamicIndirectLighting = true;

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

void AChunk::GenerateChunk(const TSharedPtr<FastNoiseLite>& InNoise)
{
	Noise = InNoise;
	FVector RootLocation = RootComponent->GetRelativeLocation();
	FVector NextBlockLocation;
	int CurrentHeight = 0;

	for (int Z = Height * BlockSize; Z > 0; Z -= BlockSize)
	{
		CurrentHeight = Z / BlockSize;

		for (int Y = 0; Y < Width * BlockSize; Y += BlockSize)
		{
			for (int X = 0; X < Width * BlockSize; X += BlockSize)
			{
				NextBlockLocation = RootLocation + FVector(X, Y, Z);
				float BlockHeight = Noise->GetNoise(NextBlockLocation.X / 100, NextBlockLocation.Y / 100);
				BlockHeight = LimitNoise(BlockHeight, 6, 32);

				EBlockType RandomBlockType = (FMath::RandRange(0, 1) == 0) ? 
					EBlockType::Stone : 
					EBlockType::Grass;

				(CurrentHeight >= BlockHeight) ?
					Blocks.Add(NextBlockLocation, FBlock(EBlockType::Air, 0, true)):
					Blocks.Add(NextBlockLocation, FBlock(RandomBlockType, 0, true));
			}
		}
	}

	//BuildLight();

	TArray<FVector> BlockLocs;
	Blocks.GenerateKeyArray(BlockLocs);
	TArray<FBlock> BlockTypes;
	Blocks.GenerateValueArray(BlockTypes);

	for (int i = 0; i < BlockLocs.Num(); i++)
	{
		if (BlockTypes[i].Type == EBlockType::Air) continue;

		for (int j = 0; j < Directions.Num(); j++)
		{
			if (!IsBlockNextToAirFast(Directions[j], BlockLocs[i])) continue;

			PotentialBlocks.Add(BlockLocs[i], BlockTypes[i].Type);
		}
	}
}

void AChunk::ModifyBlock(const FVector& Position, const EBlockType& NewType)
{
	auto Block = Blocks.Find(Position.GridSnap(BlockSize));
	if (!Block) return;

	Block->Type = NewType;
	PotentialBlocks.Add(Position, NewType);

	AddPotentialBlocksAround(Position);

	EmptyMeshData();
	CreateChunkMesh(false);
	ApplyMesh();
}

void AChunk::CreateChunkMesh(bool IsGenerating)
{
	CreateChunkMeshData(IsGenerating);
}

void AChunk::ApplyMesh()
{
	Mesh->CreateMeshSection(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		TArray<FProcMeshTangent>(),
		true
	);

	EmptyMeshData();
}

void AChunk::ClearChunk()
{
	Mesh->ClearMeshSection(0);
	Blocks.Empty(Width * Width * Height);
	PotentialBlocks.Empty();
}

void AChunk::CreateChunkMeshData(bool IsGenerating)
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
			bool IsNextToAir = IsGenerating ?
				IsBlockNextToAirFast(Directions[j], BlockLocs[i]) :
				IsBlockNextToAir(Directions[j], BlockLocs[i]);

			if (!IsNextToAir)
				continue;

			IsFaceCreated = true;
			CreateFaceData(Directions[j], BlockLocs[i]);
		}

		if(!IsFaceCreated)
			PotentialBlocks.Remove(BlockLocs[i]);
	}
}

void AChunk::BuildLight()
{
	FVector RootLocation = RootComponent->GetRelativeLocation();

	for (int Z = Height * BlockSize; Z > 0; Z -= BlockSize)
	{
		for (int Y = 0; Y < Width * BlockSize; Y += BlockSize)
		{
			for (int X = 0; X < Width * BlockSize; X += BlockSize)
			{
				FVector Position = RootLocation + FVector(X, Y, Z);
				FBlock* Block = Blocks.Find(Position);
				if (Block == nullptr)
					continue;

				FBlock BlockAbove;

				if (!GetBlockInDirection(Position, EFaceDirection::Z, BlockAbove))
				{
					UE_LOG(LogTemp, Log, TEXT("Block Light: 15"));
					Block->Light = 15;
					continue;
				}

				if (BlockAbove.Type != EBlockType::Air)
				{
					UE_LOG(LogTemp, Log, TEXT("Block Light: 14"));
					Block->Light = 14;
					continue;
				}

				if (BlockAbove.Type == EBlockType::Air && BlockAbove.Light <= 14)
				{
					Block->Light = BlockAbove.Light - 1;
					UE_LOG(LogTemp, Log, TEXT("Block Light: %d"), Block->Light);
					continue;
				}
			}
		}
	}
}

void AChunk::CreateFaceData(const EFaceDirection& Direction, const FVector& Position)
{
	FBlock Block = *Blocks.Find(Position);
	uint8 Index = GetTextureIndex(Block.Type);
	FColor VertexColor = FColor(Index, Block.Light, 0, 0);
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

	VertexColors.Append({
		VertexColor,
		VertexColor,
		VertexColor,
		VertexColor
	});
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

void AChunk::AddPotentialBlocksAround(const FVector& BlockPosition)
{
	for (int32 XOffset = -1; XOffset <= 1; XOffset++)
	{
		for (int32 YOffset = -1; YOffset <= 1; YOffset++)
		{
			for (int32 ZOffset = -1; ZOffset <= 1; ZOffset++)
			{
				if (XOffset == 0 && YOffset == 0 && ZOffset == 0)
					continue;

				FVector NeighborPosition = BlockPosition + FVector(XOffset, YOffset, ZOffset) * BlockSize;

				if (Blocks.Contains(NeighborPosition))
				{
					FBlock& NeighborBlock = Blocks[NeighborPosition];
					PotentialBlocks.Add(NeighborPosition, NeighborBlock.Type);
					continue;
				}

				Manager.Get()->AddPotentialBlockAndRebuild(GetActorLocation() + FVector(XOffset, YOffset, ZOffset) * BlockSize * Width, NeighborPosition);
			}
		}
	}
}

bool AChunk::IsBlockNextToAirFast(const EFaceDirection& Direction, const FVector& Position) const
{
	FVector BlockInDirection = (GetDirectionAsValue(Direction) * BlockSize + Position).GridSnap(BlockSize);
	if (Blocks.Find(BlockInDirection) != nullptr)
	{
		return Blocks[BlockInDirection].Type == EBlockType::Air;
	}

	float BlockHeight = Noise->GetNoise(BlockInDirection.X / 100, BlockInDirection.Y / 100);
	BlockHeight = LimitNoise(BlockHeight, 6, 32);

	return (BlockInDirection.Z / BlockSize) >= BlockHeight;
}

bool AChunk::IsBlockNextToAir(const EFaceDirection& Direction, const FVector& Position) const
{
	FVector BlockInDirection = (GetDirectionAsValue(Direction) * BlockSize + Position).GridSnap(BlockSize);
	if (Blocks.Find(BlockInDirection) != nullptr)
	{
		return Blocks[BlockInDirection].Type == EBlockType::Air;
	}

	return Manager.Get()->IsBlockAir(GetActorLocation() + GetDirectionAsValue(Direction) * BlockSize * Width, BlockInDirection);
}

uint8 AChunk::GetTextureIndex(const EBlockType& Type) const
{
	return static_cast<uint8>(Type) - 1;
}

void AChunk::AddPotentialBlock(const FVector& Position)
{
	auto Block = Blocks.Find(Position.GridSnap(BlockSize));
	if (!Block) return;

	PotentialBlocks.Add(Position, Block->Type);
}

TMap<FVector, FBlock>& AChunk::GetBlocks()
{
	return Blocks;
}

void AChunk::LogBlocks()
{
	for (const auto& Elem : Blocks)
	{
		FVector Key = Elem.Key;
		const FBlock& Block = Elem.Value;

		UE_LOG(LogTemp, Log, TEXT("Block Location: X=%f, Y=%f, Z=%f"), Key.X, Key.Y, Key.Z);
	}
}

float AChunk::LimitNoise(float NoiseValue, int MinHeight, int MaxHeight) const
{
	float normalizedValue = (NoiseValue + 1.0f) / 2.0f;
	int voxelHeight = static_cast<int>(normalizedValue * (MaxHeight - MinHeight) + MinHeight);

	voxelHeight = FMath::Clamp(voxelHeight, MinHeight, MaxHeight);

	return voxelHeight;
}

bool AChunk::GetBlockInDirection(const FVector& Position, const EFaceDirection& Direction, FBlock& Block) const
{
	FVector BlockInDirection = (GetDirectionAsValue(Direction) * BlockSize + Position).GridSnap(BlockSize);
	
	if (Blocks.Find(BlockInDirection) != nullptr)
	{
		Manager->AddPotentialBlockAndRebuild(GetActorLocation() + GetDirectionAsValue(Direction) * BlockSize * Width, BlockInDirection);
		Block = *Blocks.Find(BlockInDirection);
		return true;
	}

	return false;
}

void AChunk::EmptyMeshData()
{
	Vertices.Empty();
	UVs.Empty();
	Normals.Empty();
	Triangles.Empty();
	VertexColors.Empty();
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
