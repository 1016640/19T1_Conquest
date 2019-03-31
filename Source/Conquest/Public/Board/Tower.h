// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/Actor.h"
#include "Tower.generated.h"

/** 
 * Towers are board pieces that occupy a single tile and cannot move.
 */
UCLASS()
class CONQUEST_API ATower : public AActor
{
	GENERATED_BODY()
	
public:	

	ATower();

protected:

	// Begin UObject Interface
	//virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	#if WITH_EDITOR
	//virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	// End UObject Interface
};
