
#include "VoxelTerrain/World/ChunkManager.h"
#include "Engine/World.h"
#include "../Chunk/Chunk.h"
#include "../../FastNoiseLite.h"
#include "../../Enums/BlockType.h"

AChunkManager::AChunkManager()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.TickInterval = 1.0f;

	DrawDistance = 4;
	BlockSize = 100;
	ChunkWidth = 32;
	ChunkHeight = 32;

	Noise = MakeShared<FastNoiseLite>();
	Noise->SetFrequency(0.02f);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);
	Noise->SetFractalOctaves(5);
	Noise->SetFractalLacunarity(2.0f);
	Noise->SetFractalGain(0.3f); 
	Noise->SetCellularJitter(0.25f);
	Noise->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
	Noise->SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
}

void AChunkManager::BeginPlay()
{
	Super::BeginPlay();

	GenerateChunks();
}

void AChunkManager::Tick(float DeltaTime)
{

}

void AChunkManager::GenerateChunks()
{
	FVector Start = GetPlayerLocation();
	TArray<FVector> ChunkPositions;

	GetChunkPositions(FVector(Start.X, Start.Y, 0), ChunkPositions);

	for (int i = 0; i < ChunkPositions.Num(); i++)
	{
		auto Chunk = SpawnChunk(ChunkPositions[i]);
		if (!Chunk) continue;

		Chunk->GenerateChunk(Noise);

		GeneratedChunks.Add(ChunkPositions[i], Chunk);
	}

	TArray<TObjectPtr<AChunk>> Chunks;
	GeneratedChunks.GenerateValueArray(Chunks);

	for (int i = 0; i < Chunks.Num(); i++)
	{
		Chunks[i]->CreateChunkMesh();
	}
}

bool AChunkManager::IsBlockAir(const FVector& ChunkLocation, const FVector& BlockLocation) const
{
	auto Chunk = GeneratedChunks.Find(ChunkLocation.GridSnap(BlockSize * DrawDistance));
	if (!Chunk) return false;

	auto Block = Chunk->Get()->Blocks.Find(BlockLocation.GridSnap(BlockSize));
	if (Block == nullptr) return false;

	return Block->Type == EBlockType::Air;
}

TObjectPtr<AChunk> AChunkManager::SpawnChunk(const FVector& Location)
{
	TSubclassOf<AChunk> ChunkToSpawn = AChunk::StaticClass();

	FVector SpawnLocation = Location;
	FRotator SpawnRotation = FRotator(0.0f, 0.0f, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	AChunk* SpawnedActor = World->SpawnActor<AChunk>(ChunkToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	if (!SpawnedActor) return nullptr;

	SpawnedActor->Manager = this;
	SpawnedActor->Height = ChunkHeight;
	SpawnedActor->Width = ChunkWidth;
	SpawnedActor->BlockSize = BlockSize;

	return SpawnedActor;
}

void AChunkManager::GetChunkPositions(const FVector& Center, TArray<FVector>& OutPositions)
{
	float ChunkSize = BlockSize * ChunkWidth;

	for (int Radius = 0; Radius <= DrawDistance; ++Radius)
	{
		for (int X = -Radius; X <= Radius; ++X)
		{
			for (int Y = -Radius; Y <= Radius; ++Y)
			{
				FVector GridPosition = Center + FVector(X * ChunkSize, Y * ChunkSize, 0);

				float DistanceFromCenter = sqrt(X * X + Y * Y);

				if (FMath::Abs(DistanceFromCenter - Radius) < 0.5f)
				{
					OutPositions.Add(GridPosition.GridSnap(ChunkSize));
				}
			}
		}
	}
}

FVector AChunkManager::GetPlayerLocation() const
{
	UWorld* World = GetWorld();
	if (!World) return FVector::ZeroVector;

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController) return FVector::ZeroVector;

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	return CameraLocation;
}