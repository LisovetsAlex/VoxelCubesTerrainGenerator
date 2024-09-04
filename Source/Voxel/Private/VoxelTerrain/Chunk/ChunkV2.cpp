#include "VoxelTerrain/Chunk/ChunkV2.h"
#include "Kismet/KismetMathLibrary.h"
#include "../../Enums/BlockType.h"
#include "../../Enums/Direction.h"
#include "../../Structs/Block.h"
#include "../../FastNoiseLite.h"
#include "../World/ChunkManager.h"
#include "ProceduralMeshComponent.h"
#include "Components/SceneComponent.h"

AChunkV2::AChunkV2()
{
	PrimaryActorTick.bCanEverTick = false;

	BlockSize = 100;
	Width = 16;
	Height = 32;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetCastShadow(false);

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));

	RootComponent = RootSceneComponent;
}

void AChunkV2::InitBaseData(const TObjectPtr<AChunkManager>& InManager, int32 InBlockSize, int32 InWidth, int32 InHeight)
{
	Manager = InManager;
	BlockSize = InBlockSize;
	Width = InWidth;
	Height = InHeight;
}

void AChunkV2::GenerateChunk(const TSharedPtr<FastNoiseLite>& Noise)
{
	FVector NextBlockLocation = RootComponent->GetRelativeLocation();
	float X = NextBlockLocation.X;
	float Y = NextBlockLocation.Y;
	int CurrentHeight = 0;

	for (int i = 0; i < Height; i++)
	{
		CurrentHeight = i;
		NextBlockLocation = FVector(X, Y, NextBlockLocation.Z).GridSnap(BlockSize);
		NextBlockLocation = (NextBlockLocation + FVector(0, 0, BlockSize)).GridSnap(BlockSize);
		X = NextBlockLocation.X;
		Y = NextBlockLocation.Y;

		for (int j = 0; j < Width; j++)
		{
			NextBlockLocation = FVector(X, NextBlockLocation.Y, NextBlockLocation.Z).GridSnap(BlockSize);
			NextBlockLocation = (NextBlockLocation + FVector(0, BlockSize, 0)).GridSnap(BlockSize);

			for (int k = 0; k < Width; k++)
			{
				NextBlockLocation = (NextBlockLocation + FVector(BlockSize, 0, 0)).GridSnap(BlockSize);
				float BlockHeight = Noise->GetNoise(NextBlockLocation.X / 100, NextBlockLocation.Y / 100);
				BlockHeight = LimitNoise(BlockHeight, 6, 32);

				(CurrentHeight >= BlockHeight) ?
					Blocks.Add(NextBlockLocation, EBlockType::Air) :
					Blocks.Add(NextBlockLocation, EBlockType::Stone);
			}
		}
	}
}

void AChunkV2::CreateChunkMesh()
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

	UE_LOG(LogTemp, Warning, TEXT("Number of iterations: %d"), GlobalIndex);
	UE_LOG(LogTemp, Warning, TEXT("Number of triangles: %d"), Triangles.Num());
	UE_LOG(LogTemp, Warning, TEXT("Number of verticies: %d"), Vertices.Num());
	GlobalIndex = 0;

	ClearMesh();
}

void AChunkV2::ModifyBlock(const FVector& Position, const EBlockType& NewType)
{
	auto Block = Blocks.Find(Position.GridSnap(BlockSize));
	if (!Block) return;

	*Block = NewType;

	ClearMesh();
	CreateChunkMesh();
}

void AChunkV2::AddPotentialBlock(const FVector& Position)
{
}

void AChunkV2::CreateChunkMeshData()
{
	FVector StartLocationX = RootComponent->GetRelativeLocation() + FVector(BlockSize, BlockSize, BlockSize);
	FVector StartLocationY = StartLocationX;
	float StartY_X = StartLocationX.Y;
	float StartX_X = StartLocationX.X;
	float StartY_Y = StartLocationX.Y;
	float StartX_Y = StartLocationX.X;

	for (int z = 0; z < Height; z++)
	{
		GlobalIndex++;

		for (int y = 0; y < Width; y++)
		{
			GlobalIndex++;

			CreateFacesY<EFaceDirection::X>(StartLocationY);
			CreateFacesY<EFaceDirection::nX>(StartLocationY);

			StartLocationY = (StartLocationY + FVector(BlockSize, 0, 0)).GridSnap(BlockSize);
		}

		for (int y = 0; y < Width; y++)
		{
			GlobalIndex++;

			CreateFacesX<EFaceDirection::Z>(StartLocationX);
			CreateFacesX<EFaceDirection::nZ>(StartLocationX);
			CreateFacesX<EFaceDirection::Y>(StartLocationX);
			CreateFacesX<EFaceDirection::nY>(StartLocationX);

			StartLocationX = (StartLocationX + FVector(0, BlockSize, 0)).GridSnap(BlockSize);
		}

		StartLocationX = (FVector(StartX_X, StartY_X, (z + 1) * BlockSize)).GridSnap(BlockSize);
		StartLocationY = (FVector(StartX_Y, StartY_Y, (z + 1) * BlockSize)).GridSnap(BlockSize);
	}
}

int32 AChunkV2::WalkX(const FVector& StartLocation, const EBlockType& BlockType, const EFaceDirection& Direction)
{
	FVector CurrentLocation = StartLocation.GridSnap(BlockSize);
	int32 DistanceTraveled = -1;

	for(int i = 0; i < Width; i++)
	{
		GlobalIndex++;
		if (!Blocks.Contains(CurrentLocation)) 
			break; 

		EBlockType* CurrentBlockType = Blocks.Find(CurrentLocation);

		if (CurrentBlockType == nullptr || *CurrentBlockType != BlockType || *CurrentBlockType == EBlockType::Air) 
			break;

		FVector NeighborLocation = (CurrentLocation + GetDirectionAsValue(Direction) * BlockSize).GridSnap(BlockSize);
		if (Manager && Manager->IsBlockAir(GetActorLocation() + GetDirectionAsValue(Direction) * BlockSize * Width, NeighborLocation))
		{
			CurrentLocation.X += BlockSize;
			CurrentLocation.X = FMath::GridSnap(CurrentLocation.Y, BlockSize);
			DistanceTraveled++;
			continue;
		}

		EBlockType* NeighborBlockType = Blocks.Find(NeighborLocation);
		if (!NeighborBlockType)
			continue;
			

		if (*NeighborBlockType != EBlockType::Air)
			break;

		CurrentLocation.X += BlockSize;
		CurrentLocation.X = FMath::GridSnap(CurrentLocation.X, BlockSize);
		DistanceTraveled++;
	}

	if (DistanceTraveled == -1)
		return 1;

	CreateFaceData(Direction, DistanceTraveled, StartLocation);

	return DistanceTraveled + 1;
}

int32 AChunkV2::WalkY(const FVector& StartLocation, const EBlockType& BlockType, const EFaceDirection& Direction)
{
	FVector CurrentLocation = StartLocation.GridSnap(BlockSize);
	int32 DistanceTraveled = -1;

	for (int i = 0; i < Width; i++)
	{
		GlobalIndex++;
		if (!Blocks.Contains(CurrentLocation))
			break;

		EBlockType* CurrentBlockType = Blocks.Find(CurrentLocation);
		if (CurrentBlockType == nullptr || *CurrentBlockType != BlockType || *CurrentBlockType == EBlockType::Air)
			break;

		FVector NeighborLocation = (CurrentLocation + GetDirectionAsValue(Direction) * BlockSize).GridSnap(BlockSize);
		if (Manager && Manager->IsBlockAir(GetActorLocation() + GetDirectionAsValue(Direction) * BlockSize * Width, NeighborLocation))
		{
			CurrentLocation.Y += BlockSize;
			CurrentLocation.Y = FMath::GridSnap(CurrentLocation.Y, BlockSize);
			DistanceTraveled++;
			continue;
		}
		
		EBlockType* NeighborBlockType = Blocks.Find(NeighborLocation);
		if (!NeighborBlockType)
			continue;


		if (*NeighborBlockType != EBlockType::Air)
			break;

		CurrentLocation.Y += BlockSize;
		CurrentLocation.Y = FMath::GridSnap(CurrentLocation.Y, BlockSize);
		DistanceTraveled++;
	}

	if (DistanceTraveled == -1)
		return 1;

	CreateFaceData(Direction, DistanceTraveled, StartLocation);

	return DistanceTraveled + 1;
}

template <EFaceDirection Direction>
void AChunkV2::CreateFacesX(const FVector& StartLocation)
{
	int32 TotalDistanceTraveled = 0;
	FVector NextLocation = StartLocation;

	if (!Blocks.Find(NextLocation))
		return;

	EBlockType Type = *Blocks.Find(NextLocation);

	while (TotalDistanceTraveled < Width)
	{
		GlobalIndex++;
		TotalDistanceTraveled += WalkX(NextLocation, Type, Direction);

		if (TotalDistanceTraveled >= Width)
			break;
		
		NextLocation = (StartLocation + FVector(BlockSize * (TotalDistanceTraveled), 0, 0)).GridSnap(BlockSize);
		if (!Blocks.Find(NextLocation)) 
			break;
		Type = *Blocks.Find(NextLocation);
	}
}

template<EFaceDirection Direction>
void AChunkV2::CreateFacesY(const FVector& StartLocation)
{
	int32 TotalDistanceTraveled = 0;
	FVector NextLocation = StartLocation;

	if (!Blocks.Find(NextLocation))
		return;

	EBlockType Type = *Blocks.Find(NextLocation);

	while (TotalDistanceTraveled < Width)
	{
		GlobalIndex++;
		TotalDistanceTraveled += WalkY(NextLocation, Type, Direction);

		if (TotalDistanceTraveled >= Width)
			break;

		NextLocation = (StartLocation + FVector(0, BlockSize * (TotalDistanceTraveled), 0)).GridSnap(BlockSize);
		if (!Blocks.Find(NextLocation)) 
			break;
		Type = *Blocks.Find(NextLocation);
	}
}

void AChunkV2::CreateFaceData(const EFaceDirection& Direction, float Extent, const FVector& StartLocation)
{
	float FullExtent = BlockSize * Extent;
	FVector Normal = GetDirectionAsValue(Direction);
	float HalfBlockSize = BlockSize / 2;

	switch (Direction)
	{
	case EFaceDirection::X:
		Vertices.Add(StartLocation + FVector(HalfBlockSize, HalfBlockSize + FullExtent, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize, HalfBlockSize * -1, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize, HalfBlockSize + FullExtent, HalfBlockSize));
		Vertices.Add(StartLocation + FVector(HalfBlockSize, HalfBlockSize * -1, HalfBlockSize));
		break;

	case EFaceDirection::Y:
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize + FullExtent, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize, HalfBlockSize));
		Vertices.Add(StartLocation + FVector(HalfBlockSize + FullExtent, HalfBlockSize, HalfBlockSize));
		break;

	case EFaceDirection::nX:
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize * -1 , HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize + FullExtent, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize * -1 , HalfBlockSize));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize + FullExtent, HalfBlockSize));
		break;

	case EFaceDirection::nY:
		Vertices.Add(StartLocation + FVector(HalfBlockSize + FullExtent, HalfBlockSize * -1, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize * -1, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize + FullExtent, HalfBlockSize * -1, HalfBlockSize));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize * -1, HalfBlockSize));
		break;

	case EFaceDirection::nZ:
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize * -1, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize + FullExtent, HalfBlockSize, HalfBlockSize * -1));
		Vertices.Add(StartLocation + FVector(HalfBlockSize + FullExtent, HalfBlockSize * -1, HalfBlockSize * -1));
		break;

	case EFaceDirection::Z:
		Vertices.Add(StartLocation + FVector(HalfBlockSize + FullExtent, HalfBlockSize, HalfBlockSize));
		Vertices.Add(StartLocation + FVector(HalfBlockSize + FullExtent, HalfBlockSize * -1, HalfBlockSize));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize, HalfBlockSize));
		Vertices.Add(StartLocation + FVector(HalfBlockSize * -1, HalfBlockSize * -1, HalfBlockSize));
		break;
	}

	Normals.Append({
		Normal,
		Normal,
		Normal,
		Normal
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

TMap<FVector, EBlockType>& AChunkV2::GetBlocks()
{
	return Blocks;
}

float AChunkV2::LimitNoise(float NoiseValue, int MinHeight, int MaxHeight) const
{
	float normalizedValue = (NoiseValue + 1.0f) / 2.0f;
	int voxelHeight = static_cast<int>(normalizedValue * (MaxHeight - MinHeight) + MinHeight);

	voxelHeight = FMath::Clamp(voxelHeight, MinHeight, MaxHeight);

	return voxelHeight;
}

void AChunkV2::ClearMesh()
{
	Vertices.Empty();
	UVs.Empty();
	Normals.Empty();
	Triangles.Empty();
}

FVector AChunkV2::GetDirectionAsValue(const EFaceDirection& Direction) const
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