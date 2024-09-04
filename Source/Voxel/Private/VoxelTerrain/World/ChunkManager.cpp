
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

	ChunkType = AChunk::StaticClass();

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
		auto ChunkActor = SpawnChunk(ChunkPositions[i]);
		auto Chunk = Cast<IChunkable>(ChunkActor);
		if (!Chunk) continue;

		Chunk->GenerateChunk(Noise);

		GeneratedChunks.Add(ChunkPositions[i], ChunkActor);
	}

	TArray<IChunkable*> Chunks;
	for (const TPair<FVector, TObjectPtr<AActor>>& Pair : GeneratedChunks)
	{
		auto Actor = Cast<IChunkable>(Pair.Value);
		if (!Actor) continue;

		Chunks.Add(Actor);
	}

	for (int i = 0; i < Chunks.Num(); i++)
	{
		Chunks[i]->CreateChunkMesh();
	}
}

bool AChunkManager::IsBlockAir(const FVector& ChunkLocation, const FVector& BlockLocation) const
{
	auto ChunkActor = GeneratedChunks.Find(ChunkLocation.GridSnap(BlockSize * DrawDistance));
	if (!ChunkActor) return false;

	auto Chunk = Cast<IChunkable>(*ChunkActor);
	if (!Chunk) return false;

	auto Block = Chunk->GetBlocks().Find(BlockLocation.GridSnap(BlockSize));
	if (!Block) return false;

	return *Block == EBlockType::Air;
}

void AChunkManager::AddPotentialBlockAndRebuild(const FVector& ChunkLocation, const FVector& BlockPosition)
{
	auto ChunkActor = GeneratedChunks.Find(ChunkLocation.GridSnap(BlockSize * DrawDistance));
	if (!ChunkActor) return;

	auto Chunk = Cast<IChunkable>(*ChunkActor);
	if (!Chunk) return;

	Chunk->AddPotentialBlock(BlockPosition);
	Chunk->CreateChunkMesh();
}

void AChunkManager::AddBlock(const FVector& Position, const EBlockType& NewType)
{
	for (auto& Pair : GeneratedChunks)
	{
		auto Chunk = Cast<IChunkable>(Pair.Value);
		if (!Chunk) continue;
		if (!Chunk->GetBlocks().Find(Position.GridSnap(BlockSize))) continue;

		Chunk->ModifyBlock(Position, NewType);
		return;
	}
}

void AChunkManager::RemoveBlock(const FVector& Position)
{
	for (auto& Pair : GeneratedChunks)
	{
		auto Chunk = Cast<IChunkable>(Pair.Value);
		if (!Chunk) continue;
		if (!Chunk->GetBlocks().Find(Position.GridSnap(BlockSize))) continue;

		Chunk->ModifyBlock(Position, EBlockType::Air);
		return;
	}
}

TObjectPtr<AActor> AChunkManager::SpawnChunk(const FVector& Location)
{
	FVector SpawnLocation = Location;
	FRotator SpawnRotation = FRotator(0.0f, 0.0f, 0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	AActor* SpawnedActor = World->SpawnActor<AActor>(ChunkType, SpawnLocation, SpawnRotation, SpawnParams);
	if (!SpawnedActor) return nullptr;

	if (!SpawnedActor->GetClass()->ImplementsInterface(UChunkable::StaticClass())) return nullptr;

	IChunkable* ChunkableActor = Cast<IChunkable>(SpawnedActor);
	if (!ChunkableActor) return nullptr;

	ChunkableActor->InitBaseData(this, BlockSize, ChunkWidth, ChunkHeight);
	
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