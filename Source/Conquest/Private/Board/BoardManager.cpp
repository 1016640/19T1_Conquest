// Fill out your copyright notice in the Description page of Project Settings.

#include "BoardManager.h"
#include "Tile.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ABoardManager::ABoardManager()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	bTestingToggle = false;
	HexSize = 200.f;
	DrawTime = 5.f;

	GridX = 10;
	GridY = 10;
}

void ABoardManager::OnConstruction(const FTransform& Transform)
{
	if (!bTestingToggle)
		return;

	FVector2D GridSize((float)HexSize, (float)HexSize);
	auto hex_corner_offset = [this, GridSize](float start_angle, int32 corner)->FVector
	{
		float angle = 2.f * PI * (start_angle + (float)corner) / 6;
		return FVector(GridSize.X * FMath::Cos(angle), GridSize.Y * sin(angle), this->GetActorLocation().Z);
	};
	
	auto hex_to_world = [this, GridSize](FIntVector hex)->FVector
	{
		float X = (FMath::Sqrt(3.f) * hex.X + (FMath::Sqrt(3.f) / 2.f) * hex.Y) * GridSize.X;
		float Y = (0.f * hex.X + (3.f / 2.f) * hex.Y) * GridSize.Y;

		return FVector(this->GetActorLocation().X + X, this->GetActorLocation().Y + Y, this->GetActorLocation().Z);
	};

	for (int32 Y = 0; Y < GridY; ++Y)
	{
		int32 YOffset = FMath::FloorToInt(Y / 2);
		for (int32 X = -YOffset; X < GridX - YOffset; ++X)
		{
				bool bSplitX = X % 2 == 1;
				bool bSplitY = Y % 2 == 1;

				FVector Pos = GetActorLocation();
				Pos.X += (FMath::Sqrt(3) * HexSize) * (float)X;
				Pos.Y += (2 * HexSize) * (float)Y;

				if (bSplitY)
				{
					Pos.X += (FMath::Sqrt(3) * HexSize) / 2.f;
				}

				if (bSplitY)
				{
					Pos.Y -= (HexSize) * 0.5f;
				}

				if (Y > 1)
				{
					Pos.Y -= (HexSize) * 0.5f;
				}

				Pos = hex_to_world(FIntVector(X, Y, 0));
				DrawDebugHexagon(Pos, HexSize, DrawTime);
		}
	}
}

void ABoardManager::DrawDebugHexagon(const FVector& Position, float Size, float Time)
{
	auto GetPoint = [this](const FVector& Pos, float Size, int32 Index)->FVector
	{
		float Angle = 60.f * (float)Index - 30.f;
		float Radians = FMath::DegreesToRadians(Angle);

		return FVector(Pos.X + Size * cos(Radians), Pos.Y + Size * sin(Radians), this->GetActorLocation().Z);
	};

	FVector CurrentPoint = GetPoint(Position, Size, 0);
	for (int32 I = 0; I <= 5; ++I)
	{
		FVector NextPoint = GetPoint(Position, Size, (I + 1) % 6);
		DrawDebugLine(GetWorld(), CurrentPoint, NextPoint, FColor::Green, false, Time, 0, 5.f);

		CurrentPoint = NextPoint;
	}

	DrawDebugSphere(GetWorld(), Position, Size / 2.f, 4, FColor::Red, false, Time, 0, 5.f);
}

void ABoardManager::RebuildGrid()
{
	OnConstruction(GetActorTransform());
}
