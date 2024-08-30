#pragma once

UENUM(BlueprintType)
enum class EBlockType : uint8
{
    Air = 0 UMETA(DisplayName="Air"),
    Grass = 1 UMETA(DisplayName="Grass"),
    Stone = 2 UMETA(DisplayName="Stone")
};