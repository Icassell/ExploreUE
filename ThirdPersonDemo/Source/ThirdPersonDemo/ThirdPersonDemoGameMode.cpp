// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonDemoGameMode.h"

#include "MyPlayerState.h"
#include "ThirdPersonDemoCharacter.h"
#include "UObject/ConstructorHelpers.h"

AThirdPersonDemoGameMode::AThirdPersonDemoGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

FString AThirdPersonDemoGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	UE_LOG(LogTemp, Warning, TEXT("AThirdPersonDemoGameMode::InitNewPlayer,Result:%s"), *Result);

	//获取PlayerState
	AMyPlayerState* MyPlayerState = Cast<AMyPlayerState>(NewPlayerController->PlayerState);
	if(MyPlayerState==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyPlayerState==nullptr"));
		return Result;
	}

	//设置Camp
	MyPlayerState->CampId = MyPlayerState->GetPlayerId() % 2;
	UE_LOG(LogTemp, Warning, TEXT("PlayerId:%d,CampId:%d"), MyPlayerState->GetPlayerId(), MyPlayerState->CampId);
	
	return Result;
}