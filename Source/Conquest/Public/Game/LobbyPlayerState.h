// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

/**
 * Tracks a players selected options while in lobby
 */
UCLASS(ClassGroup=(CSK))
class CONQUEST_API ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	ALobbyPlayerState();

protected:

	// Begin APlayerState Interface
	virtual void CopyProperties(APlayerState* PlayerState) override;
	// End APlayerState Interface

	// Begin UObject Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// End UObject Interface

public:

	/** Sets this players CSK player ID */
	void SetCSKPlayerID(int32 InPlayerID);

public:

	/** Get this players CSK player ID */
	FORCEINLINE int32 GetCSKPlayerID() const { return CSKPlayerID; }

protected:

	/** This players CSKPlayerID */
	UPROPERTY(BlueprintReadOnly, Transient, Replicated, Category = CSK)
	int32 CSKPlayerID;

public:

	/** Set if the player is ready to start the match */
	UFUNCTION(BlueprintCallable, Category = Lobby)
	void SetIsReady(bool bInIsReady);

	/** Sets this players assigned color */
	UFUNCTION(BlueprintCallable, Category = Lobby)
	void SetAssignedColor(FColor InColor);

public:

	/** Get if this player is ready to start the match */
	bool IsPlayerReady() const { return bIsPlayerReady; }

	/** Get this players chosen color */
	const FColor& GetAssignedColor() const { return AssignedColor; }
	
protected:
	
	/** If this player is ready to begin the match */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Replicated, Category = Lobby)
	uint32 bIsPlayerReady : 1;

	/** The color the player has chosen to go with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = Lobby)
	FColor AssignedColor;
};
