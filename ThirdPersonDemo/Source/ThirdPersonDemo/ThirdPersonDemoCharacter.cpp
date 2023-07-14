// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonDemoCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "MyPlayerState.h"
#include "WeaponDataComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// AThirdPersonDemoCharacter

void AThirdPersonDemoCharacter::ServerFire_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("ServerFire_Implementation"));
	UWeaponDataComponent* WeaponDataComponent = FindComponentByClass<UWeaponDataComponent>();
	if (WeaponDataComponent==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponDataComponent==nullptr"));
		return;
	}
	WeaponDataComponent->AmmoCount--;
}

AThirdPersonDemoCharacter::AThirdPersonDemoCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AThirdPersonDemoCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AThirdPersonDemoCharacter::OnFire);
	
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AThirdPersonDemoCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AThirdPersonDemoCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AThirdPersonDemoCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AThirdPersonDemoCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AThirdPersonDemoCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AThirdPersonDemoCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AThirdPersonDemoCharacter::OnResetVR);
}

void AThirdPersonDemoCharacter::BeginPlay()
{
	Super::BeginPlay();
	// 打印日志到屏幕 PlayerId
	if (GetPlayerState())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("PlayerId:%d"), GetPlayerState()->GetPlayerId()));
	}
	

	// 获取Mesh
	USkeletalMeshComponent* SkeletalMeshComponent = GetMesh();
	if (SkeletalMeshComponent==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SkeletalMeshComponent==nullptr"));
		return;
	}
	// 设置渲染自定义深度
	SkeletalMeshComponent->SetRenderCustomDepth(true);
	SkeletalMeshComponent->SetCustomDepthStencilValue(1);//用模板值表示透视颜色
}

bool AThirdPersonDemoCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	UE_LOG(LogTemp, Warning, TEXT("IsNetRelevantFor,Self:%s,RealViewer:%s ViewTarget:%s SrcLocation:%s"),*GetName(), *RealViewer->GetName(), *ViewTarget->GetName(), *SrcLocation.ToString());
	if(ViewTarget==this)
	{
		UE_LOG(LogTemp, Warning, TEXT("ViewTarget==this"));
	}

	AMyPlayerState* SelfMyPlayerState = Cast<AMyPlayerState>(GetPlayerState());

	//同步规则：守方(1)能看到攻方(0) 攻方(0)能不看到守方(1)
	const AThirdPersonDemoCharacter* NetConnectionCharacter=Cast<AThirdPersonDemoCharacter>(ViewTarget);
	if(NetConnectionCharacter==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("NetConnectionCharacter==nullptr"));
		return false;
	}
	AMyPlayerState* NetConnectionPlayerState = Cast<AMyPlayerState>(NetConnectionCharacter->GetPlayerState());
	if(NetConnectionPlayerState==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("NetConnectionPlayerState==nullptr"));
		return false;
	}

	//以当前连接的玩家(ViewTarget)为消费者，当前Actor为商品，消费者在挑选符合它条件的商品。

	//消费者为守方，返回所有商品
	if(NetConnectionPlayerState->CampId==1)
	{
		return true;
	}

	//消费者为攻方，只返回攻方
	if(NetConnectionPlayerState->CampId==0)
	{
		if(SelfMyPlayerState->CampId==0)
		{
			return true;
		}
	}
	
	return false;
}

void AThirdPersonDemoCharacter::OnFire()
{
	UWeaponDataComponent* WeaponDataComponent = FindComponentByClass<UWeaponDataComponent>();
	if (WeaponDataComponent==nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponDataComponent==nullptr"));
		return;
	}
	
	ServerFire();
}

void AThirdPersonDemoCharacter::OnResetVR()
{
	// If ThirdPersonDemo is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in ThirdPersonDemo.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AThirdPersonDemoCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AThirdPersonDemoCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AThirdPersonDemoCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonDemoCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonDemoCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AThirdPersonDemoCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
