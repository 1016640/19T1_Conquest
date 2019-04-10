// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Engine/LocalPlayer.h"
#include "CSKLocalPlayer.generated.h"

/**
 * Local player class specialized for CSK
 */
UCLASS(ClassGroup = (CSK))
class CONQUEST_API UCSKLocalPlayer : public ULocalPlayer
{
	GENERATED_BODY()
};
