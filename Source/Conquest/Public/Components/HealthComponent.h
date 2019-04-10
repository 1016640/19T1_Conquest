// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class UHealthComponent;

/** Delegate for when health component has detected of a current health value */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthChangeSignature, UHealthComponent*, HealthComp, int32, NewHealth, int32, Delta);

/** 
 * A component that tracks a single health status
 */
UCLASS(ClassGroup = (CSK), meta = (BlueprintSpawnableComponent))
class CONQUEST_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UHealthComponent();

protected:

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif
	// End UObject Interface

public:

	/** Initializes health, should be called during component creation */
	void InitHealth(int32 InHealth, int32 InMaxHealth);

	/** Applies damage to the owner. Get how much damage was applied */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	int32 ApplyDamage(int32 Amount);

	/** Restores health to the owner. Get how much health was restored */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	int32 RestoreHealth(int32 Amount);

	/** Increases max health (Negative values are allowed and will decrease max healt).
	Can optionally increase health as well but health will be clamped at 1 if decreasing */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void IncreaseMaxHealth(int32 Amount, bool bIncreaseHealth = false);

	/** Kills the owner forcefully */
	UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void Kill();

	/** Revives the owner with given percent (0 to 1) of max health */
	UFUNCTION(Exec, BlueprintCallable, BlueprintAuthorityOnly, Category = "Health")
	void Revive(float Percent);

public:

	/** Get current health */
	FORCEINLINE int32 GetHealth() const { return Health; }

	/** Get max health */
	FORCEINLINE int32 GetMaxHealth() const { return MaxHealth; }

	/** If owner is dead */
	FORCEINLINE bool IsDead() const { return Health <= 0; }

public:

	/** Event for when health has changed (either from damaged or healing) */
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FHealthChangeSignature OnHealthChanged;

protected:

	/** Health of our owner */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Health", meta = (ClampMin = 1, EditCondition="!bIsDead"))
	int32 Health;

	/** Max health of our owner */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Health", meta = (ClampMin = 1, EditCondition = "!bIsDead"))
	int32 MaxHealth;
};
