// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "CSKSpringArmComponent.generated.h"

/**
 * Spring arm specialized to work with CSK pawn
 */
UCLASS()
class CONQUEST_API UCSKSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()
	
public:

	UCSKSpringArmComponent();

public:

	// Begin UActorComponent Interface
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	// End UActorComponent Interface

protected:

	// Begin UObject Interface
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	// End UObject Interface

public:

	/** Appends length to desired target arm length. Negative values are allowed */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	void AddDesiredArmLength(float ArmLength);

public:

	/** Set the desired target arm length to interp to */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	void SetDesiredTargetArmLength(float DesiredArmLength, bool bInstant = false);

	/** Init the min and max target arm length */
	void InitTargetArmLengths(float MinArmLength, float MaxArmLength);

	/** Set the min possible target arm length */
	void SetMinTargetArmLength(float NewArmLength);

	/** Set the max possible target arm length */
	void SetMaxTargetArmLength(float NewArmLength);

private:

	/** Blueprint setter for min target arm length */
	UFUNCTION(BlueprintCallable)
	void BP_SetMinTargetArmLength(float NewArmLength);

	/** Blueprint setter for max target arm length */
	UFUNCTION(BlueprintCallable)
	void BP_SetMaxTargetArmLength(float NewArmLength);

public:

	/** If target arm length should be immediately set to new desired arm length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera)
	uint8 bInstantInterpArmLength : 1;

	/** Speed for interpolating to target arm length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (ClampMin = 0.1, EditCondition="!bInstantInterpArmLength"))
	float TargetArmLengthInterpSpeed;

protected:

	/** The min desired target arm length we can use */
	UPROPERTY(EditAnywhere, BlueprintSetter = "BP_SetMinTargetArmLength", Category = Camera, meta = (ClampMin = 0))
	float MinTargetArmLength;

	/** The max desired target arm length we can use */
	UPROPERTY(EditAnywhere, BlueprintSetter = "BP_SetMaxTargetArmLength", Category = Camera, meta = (ClampMin = 0))
	float MaxTargetArmLength;

private:

	/** The target arm length we are lerping to */
	float DesiredTargetArmLength;
};
