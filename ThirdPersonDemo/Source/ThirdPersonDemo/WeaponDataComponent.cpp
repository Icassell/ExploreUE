// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponDataComponent.h"

#include "MyPlayerState.h"
#include "ThirdPersonDemoCharacter.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UWeaponDataComponent::UWeaponDataComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UWeaponDataComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UWeaponDataComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	UE_LOG(LogTemp, Warning, TEXT("GetLifetimeReplicatedProps"));
	DOREPLIFETIME(UWeaponDataComponent, AmmoCount);
}

void UWeaponDataComponent::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	//限制当前Character的AmmoCount仅在当前玩家CampId为0时同步，即PreReplication是针对Actor/Component的条件同步，不是针对连接。
	const AThirdPersonDemoCharacter* ThirdPersonDemoCharacter = Cast<AThirdPersonDemoCharacter>(GetOwner());
	if (ThirdPersonDemoCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThirdPersonDemoCharacter==nullptr"));
		return;
	}
	const AMyPlayerState* PlayerState = Cast<AMyPlayerState>(ThirdPersonDemoCharacter->GetPlayerState());
	if (PlayerState == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerState==nullptr"));
		return;
	}
	const bool bCanReplicate = PlayerState->CampId == 0;
	UE_LOG(LogTemp, Warning, TEXT("PreReplication AmmoCount CampId:%d bCanReplicate:%d"), PlayerState->CampId, bCanReplicate);
	DOREPLIFETIME_ACTIVE_OVERRIDE(UWeaponDataComponent, AmmoCount, PlayerState->CampId == 0);
}

// Called every frame
void UWeaponDataComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UWeaponDataComponent::OnRep_AmmoCount()
{
	AThirdPersonDemoCharacter* ThirdPersonDemoCharacter = Cast<AThirdPersonDemoCharacter>(GetOwner());
	if (ThirdPersonDemoCharacter==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ThirdPersonDemoCharacter==nullptr"));
		return;
	}
	AMyPlayerState* PlayerState = ThirdPersonDemoCharacter->GetPlayerState<AMyPlayerState>();
	if (PlayerState==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerState==nullptr"));
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("AmmoCount: %d,PlayerId:%d,CampId:%d"), AmmoCount, PlayerState->GetPlayerId(), PlayerState->CampId));
}