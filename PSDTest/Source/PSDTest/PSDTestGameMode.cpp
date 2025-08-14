// Copyright Epic Games, Inc. All Rights Reserved.

#include "PSDTestGameMode.h"
#include "PSDTestCharacter.h"
#include "UObject/ConstructorHelpers.h"

APSDTestGameMode::APSDTestGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
