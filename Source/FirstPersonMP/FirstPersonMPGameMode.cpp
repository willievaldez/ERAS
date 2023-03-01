// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonMPGameMode.h"
#include "FirstPersonMPCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFirstPersonMPGameMode::AFirstPersonMPGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
