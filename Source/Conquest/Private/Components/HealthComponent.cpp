// Fill out your copyright notice in the Description page of Project Settings.

#include "HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	bReplicates = true;

	Health = 5;
	MaxHealth = 5;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
	DOREPLIFETIME(UHealthComponent, MaxHealth);
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

void UHealthComponent::InitHealth(int32 InHealth, int32 InMaxHealth)
{
	if (!HasBeenInitialized())
	{
		Health = InHealth;
		MaxHealth = InMaxHealth;
	}
}

int32 UHealthComponent::ApplyDamage(int32 Amount)
{
	if (GetOwnerRole() == ROLE_Authority && !IsDead())
	{
		if (Amount <= 0)
		{
			UE_LOG(LogConquest, Log, TEXT("UHealthComponent::ApplyDamage: Amount must be a positive value greater than zero. Amount was %i"), Amount);
			return 0;
		}

		int32 NewHealth = FMath::Max(0, Health - Amount);
		int32 Delta = NewHealth - Health;

		Health = NewHealth;

		// Send a negative delta to specify damage, but return 
		// positive as we return the amount of damage dealt
		OnHealthChanged.Broadcast(this, NewHealth, Delta);
		return FMath::Abs(Delta);
	}

	return 0;
}

int32 UHealthComponent::RestoreHealth(int32 Amount)
{
	if (GetOwnerRole() == ROLE_Authority && !(IsDead() || IsFullyHealed()))
	{
		if (Amount <= 0)
		{
			UE_LOG(LogConquest, Log, TEXT("UHealthComponent::RestoreHealth: Amount must be a positive value greater than zero. Amount = %i"), Amount);
			return 0;
		}

		int32 NewHealth = FMath::Min(MaxHealth, Health + Amount);
		int32 Delta = NewHealth - Health;

		Health = NewHealth;

		OnHealthChanged.Broadcast(this, NewHealth, Delta);
		return Delta;
	}

	return 0;
}

void UHealthComponent::IncreaseMaxHealth(int32 Amount, bool bIncreaseHealth)
{
	if (GetOwnerRole() == ROLE_Authority && Amount != 0)
	{
		int32 NewMaxHealth = FMath::Max(1, MaxHealth + Amount);

		// If increasing health as well, we want to increase it by the delta instead of given amount
		// since their is a possibility that the change is not actually the given amount (e.g. decreasing)
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
				ApplyDamage(-Delta);
			}
		}

		// We also need to consider that our health could now
		// be greater than our max health (this can arise from
		// not applying delta to current health)
		Health = FMath::Min(MaxHealth, Health);
	}
}

void UHealthComponent::Kill()
{
	ApplyDamage(Health);
}

void UHealthComponent::Revive(float Percent)
{
	if (GetOwnerRole() == ROLE_Authority && IsDead())
	{
		// Keep alpha valid
		Percent = FMath::Clamp(Percent, 0.f, 1.f);
		if (Percent > 0.f)
		{
			Health = FMath::RoundToInt(static_cast<float>(MaxHealth) * Percent);
			OnHealthChanged.Broadcast(this, Health, Health);
		}
		else
		{
			UE_LOG(LogConquest, Warning, TEXT("UHealthComponent::Revive: Revive called with a percent value of 0"));
		}
	}
}

void UHealthComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(this, Health, 0);
}
