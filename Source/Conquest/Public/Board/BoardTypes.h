// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Conquest.h"

/** All elements present in CSK (bitset since some spells have 2 types) */
UENUM(BlueprintType, meta = (DisplayName = "ElementType", Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECSKElementType : uint8
{
	Fire	= 1,
	Water	= 2,
	Earth	= 4,
	Air		= 8,

	All		= 15	UMETA(Hidden="true"),
	None	= 0		UMETA(Hidden="true")
};

ENUM_CLASS_FLAGS(ECSKElementType);