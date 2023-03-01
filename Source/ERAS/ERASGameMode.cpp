// Copyright Epic Games, Inc. All Rights Reserved.

#include "ERASGameMode.h"
#include "ERASCharacter.h"
#include "UObject/ConstructorHelpers.h"

AERASGameMode::AERASGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_ERASCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
