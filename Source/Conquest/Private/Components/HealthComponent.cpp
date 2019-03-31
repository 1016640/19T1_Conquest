// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	bReplicates = true;

	Health = 5;
	MaxHealth = 5;
	bIsDead = false;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
	DOREPLIFETIME(UHealthComponent, MaxHealth);
	DOREPLIFETIME(UHealthComponent, bIsDead);
}

#if WITH_EDITOR
void UHealthComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UHealthComponent, Health))
	{
		MaxHealth = FMath::Max(Health, MaxHealth);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UHealthComponent, MaxHealth))
	{
		Health = FMath::Min(Health, MaxHealth);
	}
}
#endif

int32 UHealthComponent::ApplyDamage(int32 Amount)
{
	if (!bIsDead)
	{
		if (Amount <= 0)
		{
			UE_LOG(LogConquest, Log, TEXT("UHealthComponent::ApplyDamage: Amount must be a positive value greater than zero. Amount was %i"), Amount);
			return 0;
		}

		int32 NewHealth = FMath::Max(0, Health - Amount);

		// Subtract the new health from current health to get the correct delta,
		// we want the delta health change for damage to be negative, so if our
		// health was 5 and is now 2, the delta should be -3 (2 - 5 = -3)
		int32 Delta = NewHealth - Health;

		Health = NewHealth;
		bIsDead = NewHealth == 0;

		OnHealthChanged.Broadcast(this, NewHealth, Delta);
		return Delta;
	}

	return 0;
}

int32 UHealthComponent::RestoreHealth(int32 Amount)
{
	if (!bIsDead)
	{

		if (Amount <= 0)
		{
			UE_LOG(LogConquest, Log, TEXT("UHealthComponent::RestoreHealth: Amount must be a positive value greater than zero. Amount = %i"), Amount);
			return 0;
		}

		int32 NewHealth = FMath::Max(MaxHealth, Health + Amount);
		int32 Delta = NewHealth - Health;

		Health = NewHealth;

		OnHealthChanged.Broadcast(this, NewHealth, Delta);
		return Delta;
	}

	return 0;
}

void UHealthComponent::IncreaseMaxHealth(int32 Amount, bool bIncreaseHealth)
{
	if (Amount != 0)
	{
		int32 NewMaxHealth = FMath::Max(1, MaxHealth + Amount);

		// If increasing health as well, we want to increase it by the delta instead of given amount
		// since their is a possibity that the change is not actually the given amount (decreasing)
		int32 Delta = NewMaxHealth - MaxHealth;

		MaxHealth = NewMaxHealth;
		
		if (bIncreaseHealth)
		{
			if (Delta > 0)
			{
				RestoreHealth(Delta);
			}
			else
			{
				ApplyDamage(Delta);
			}
		}
	}
}

void UHealthComponent::Kill()
{
	ApplyDamage(Health);
}

void UHealthComponent::Revive(float Percent)
{
	// Keep alpha valid
	Percent = FMath::Clamp(Percent, 0.f, 1.f);
	if (Percent > 0.f)
	{
		bIsDead = false;
		RestoreHealth(FMath::RoundToInt(static_cast<float>(MaxHealth) * Percent));
	}
	else
	{
		UE_LOG(LogConquest, Warning, TEXT("UHealthComponent::Revive: Revive called with a percent value of 0%"));
	}
}
