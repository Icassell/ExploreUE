// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ThirdPersonDemoGameMode.generated.h"

UCLASS(minimalapi)
class AThirdPersonDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AThirdPersonDemoGameMode();

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT(""));
};



