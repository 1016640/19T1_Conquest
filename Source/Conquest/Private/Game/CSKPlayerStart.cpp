// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKPlayerStart.h"

const FName ACSKPlayerStart::CSKPawnSpawnTag(TEXT("CSKPawnSpawn"));
const FName ACSKPlayerStart::CoinFlipCameraTag(TEXT("CoinFlipCameraTag"));

void ACSKPlayerStart::SetStartAsCSKPawnSpawn()
{
	PlayerStartTag = CSKPawnSpawnTag;
}

void ACSKPlayerStart::SetStartAsCoinFlipCamera()
{
	PlayerStartTag = CoinFlipCameraTag;
}
