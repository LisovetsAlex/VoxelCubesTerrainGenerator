
#include "VoxelTerrain/World/ChunkManager.h"
#include "Engine/World.h"
#include "../Chunk/Chunk.h"
#include "../../FastNoiseLite.h"
#include "../../Enums/BlockType.h"

AChunkManager::AChunkManager()
{
	DrawDistance = 4;
	BlockSize = 100;
	ChunkWidth = 32;
	ChunkHeight = 32;

	MaxChunksPerTick = 8;
	MaxMeshesPerTick = 8;
	ChunkType = AChunk::StaticClass();

	Noise = MakeShared<FastNoiseLite>();
	Noise->SetFrequency(0.02f);
	Noise->SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	Noise->SetFractalType(FastNoiseLite::FractalType_FBm);
	Noise->SetFractalOctaves(3);
	Noise->SetFractalLacunarity(2.0f);
	Noise->SetFractalGain(0.3f); 
	Noise->SetCellularJitter(0.25f);
	Noise->SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
	Noise->SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
}

void AChunkManager::BeginPlay()
{
	Super::BeginPlay();

	int AmountOfChunks = DrawDistance * 2 * DrawDistance * 2;
	for (int i = 0; i < AmountOfChunks; i++)
	{
		ChunkPool.Add(SpawnChunk(FVector(0, 0, 0)));
	}
}

void AChunkManager::Tick(float DeltaTime)
{
	AdjustGenerateRate();
	RegenerateChunks();
	ProcessChunkGeneration();
	ProcessMeshGeneration();
}

void AChunkManager::RegenerateChunks()
{
	FVector Start = GetPlayerLocation();
	TArray<FVector> ChunkPositions;

	GetChunkPositions(FVector(Start.X, Start.Y, 0), ChunkPositions);

	EnqueueChunks(ChunkPositions);
}

void AChunkManager::ProcessMeshGeneration()
{
	int32 MeshesProcessed = 0;

	while (!MeshQueue.IsEmpty() && MeshesProcessed < MaxMeshesPerTick)
	{
		IChunkable* Chunk;
		if (MeshQueue.Dequeue(Chunk))
		{
			MeshQueueLength--;

			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Chunk]()
			{
				Chunk->CreateChunkMesh(true);

				AsyncTask(ENamedThreads::GameThread, [Chunk]()
				{
					Chunk->ApplyMesh();
				});
			});

			MeshesProcessed++;
		}
	}
}

void AChunkManager::ProcessChunkGeneration()
{
	int32 ChunksProcessed = 0;

	while (!ChunkQueue.IsEmpty() && ChunksProcessed < MaxChunksPerTick)
	{
		FVector ChunkPos;
		if (ChunkQueue.Dequeue(ChunkPos))
		{
			ChunkQueueLength--;

			if (ChunkPool.IsEmpty()) continue;

			auto ChunkActor = ChunkPool[0];
			ChunkActor->SetActorLocation(ChunkPos);
			ChunkPool.RemoveAt(0);
			auto Chunk = Cast<IChunkable>(ChunkActor);
			if (!Chunk) continue;

			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Chunk]()
			{
				Chunk->GenerateChunk(Noise);

				AsyncTask(ENamedThreads::GameThread, [this, Chunk]()
				{
					EnqueueMesh(Chunk);
				});
			});

			GeneratedChunks.Add(ChunkPos, ChunkActor);

			ChunksProcessed++;
		}
	}
}

void AChunkManager::EnqueueChunks(const TArray<FVector>& ChunkPositions)
{
	if (!GeneratedChunks.IsEmpty())
	{
		TArray<FVector> ChunksToRemove;
		TArray<FVector> ChunkLocs;

		GeneratedChunks.GenerateKeyArray(ChunkLocs);
		for (const FVector& ChunkLoc : ChunkLocs)
		{
			if (ChunkPositions.Contains(ChunkLoc)) continue;
			if (GeneratedChunks.Find(ChunkLoc) == nullptr) continue;

			ChunksToRemove.Add(ChunkLoc);
		}

		for (const FVector& ChunkLoc : ChunksToRemove)
		{
			auto ChunkActor = GeneratedChunks.Find(ChunkLoc);

			if (ChunkActor == nullptr) continue;
			if (ChunkActor->Get() == nullptr) continue;
				
			ChunkPool.Add(*ChunkActor);
			GeneratedChunks.Remove(ChunkLoc);

			auto Chunk = Cast<IChunkable>(ChunkActor->Get());
			if (!Chunk) continue;
			
			Chunk->ClearChunk();
		}
	}

	for (const FVector& ChunkPos : ChunkPositions)
	{
		if (!GeneratedChunks.Contains(ChunkPos))
		{
			GeneratedChunks.Add(ChunkPos, nullptr);
			ChunkQueue.Enqueue(ChunkPos);
			ChunkQueueLength++;
			continue;
		}
	}
}

void AChunkManager::EnqueueMesh(IChunkable* Chunk)
{
	MeshQueueLength++;
	MeshQueue.Enqueue(Chunk);
}

void AChunkManager::AdjustGenerateRate()
{
	if (ChunkQueueLength >= 600)
	{
		MaxChunksPerTick = 32;
	}
	else if (ChunkQueueLength >= 300)
	{
		MaxChunksPerTick = 16;
	}
	else if (ChunkQueueLength >= 100)
	{
		MaxChunksPerTick = 8;
	}
	else
	{
		MaxChunksPerTick = 4;
	}

	if (MeshQueueLength >= 600)
	{
		MaxMeshesPerTick = 16;
	}
	else if (MeshQueueLength >= 300)
	{
		MaxMeshesPerTick = 8;
	}
	else if (MeshQueueLength >= 100)
	{
		MaxMeshesPerTick = 4;
	}
	else if (MeshQueueLength >= 50)
	{
		MaxMeshesPerTick = 2;
	}
	else
	{
		MaxMeshesPerTick = 1;
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

	return Block->Type == EBlockType::Air;
}

void AChunkManager::AddPotentialBlockAndRebuild(const FVector& ChunkLocation, const FVector& BlockPosition)
{
	auto ChunkActor = GeneratedChunks.Find(ChunkLocation.GridSnap(BlockSize * DrawDistance));
	if (!ChunkActor) return;

	auto Chunk = Cast<IChunkable>(*ChunkActor);
	if (!Chunk) return;

	Chunk->AddPotentialBlock(BlockPosition);
	Chunk->CreateChunkMesh(false);
	Chunk->ApplyMesh();
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
	return CameraLocation.GridSnap(BlockSize * ChunkWidth);
}