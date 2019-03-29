// Fill out your copyright notice in the Description page of Project Settings.

#include "CSKSpringArmComponent.h"
#include "..\..\Public\Components\CSKSpringArmComponent.h"

UCSKSpringArmComponent::UCSKSpringArmComponent()
{
	bInstantInterpArmLength = false;
	TargetArmLengthInterpSpeed = 5.f;
	MinTargetArmLength = 100.f;
	MaxTargetArmLength = 2000.f;
	DesiredTargetArmLength = 0.f;
}

void UCSKSpringArmComponent::OnRegister()
{
	Super::OnRegister();

	// This will validate min and max lengths
	InitTargetArmLengths(MinTargetArmLength, MaxTargetArmLength);
	DesiredTargetArmLength = TargetArmLength;
}

void UCSKSpringArmComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Interpolate towards desired length
	if (bInstantInterpArmLength)
	{
		TargetArmLength = DesiredTargetArmLength;
	}
	else if (!FMath::IsNearlyEqual(TargetArmLength, DesiredTargetArmLength))
	{
		TargetArmLength = FMath::FInterpTo(TargetArmLength, DesiredTargetArmLength, DeltaTime, TargetArmLengthInterpSpeed);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

#if WITH_EDITOR
void UCSKSpringArmComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCSKSpringArmComponent, MinTargetArmLength))
	{
		MaxTargetArmLength = FMath::Max(MinTargetArmLength, MaxTargetArmLength);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UCSKSpringArmComponent, MaxTargetArmLength))
	{
		MinTargetArmLength = FMath::Min(MinTargetArmLength, MaxTargetArmLength);
	}
}
#endif

void UCSKSpringArmComponent::AddDesiredArmLength(float ArmLength)
{
	SetDesiredTargetArmLength(DesiredTargetArmLength + ArmLength);
}

void UCSKSpringArmComponent::SetDesiredTargetArmLength(float DesiredArmLength, bool bInstant)
{
	DesiredTargetArmLength = FMath::Clamp(DesiredArmLength, MinTargetArmLength, MaxTargetArmLength);

	if (bInstant)
	{
		TargetArmLength = DesiredArmLength;
	}
}

void UCSKSpringArmComponent::InitTargetArmLengths(float MinArmLength, float MaxArmLength)
{
	MinTargetArmLength = MinArmLength;
	MaxTargetArmLength = MaxArmLength;

	if (MinTargetArmLength > MaxTargetArmLength)
	{
		Swap(MinTargetArmLength, MaxTargetArmLength);
	}
}

void UCSKSpringArmComponent::SetMinTargetArmLength(float NewArmLength)
{
	MinTargetArmLength = FMath::Max(0.f, NewArmLength);
	MaxTargetArmLength = FMath::Max(MinTargetArmLength, MaxTargetArmLength);
}

void UCSKSpringArmComponent::SetMaxTargetArmLength(float NewArmLength)
{
	MaxTargetArmLength = FMath::Max(0.f, NewArmLength);
	MinTargetArmLength = FMath::Min(MinTargetArmLength, MaxTargetArmLength);
}

void UCSKSpringArmComponent::BP_SetMinTargetArmLength(float NewArmLength)
{
	SetMinTargetArmLength(NewArmLength);
}

void UCSKSpringArmComponent::BP_SetMaxTargetArmLength(float NewArmLength)
{
	SetMaxTargetArmLength(NewArmLength);
}
