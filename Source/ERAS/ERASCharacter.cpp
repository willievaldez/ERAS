// Copyright Epic Games, Inc. All Rights Reserved.

#include "ERASCharacter.h"
#include "ERASProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"

#include "Portal.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/UObjectIterator.h"

//////////////////////////////////////////////////////////////////////////
// AERASCharacter

AERASCharacter::AERASCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	TurnRateGamepad = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

}

void AERASCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	float ShortestDist = 999999.9f; // TODO works until it doesnt
	APortal* ClosestPortal = nullptr;
	for (TObjectIterator<APortal> It; It; ++It)
	{
		if (APortal* Portal = *It)
		{
			float Dist = UKismetMathLibrary::VSizeSquared(GetActorLocation() - Portal->GetActorLocation());
			if (Dist < ShortestDist)
			{
				ShortestDist = Dist;
				ClosestPortal = Portal;
			}

			Portals.Add(Portal);
		}
	}

	if (ClosestPortal)
	{
		UE_LOG(LogTemp, Warning, TEXT("%p: Initializing on portal %s"),
			(void*)ClosestPortal, * ClosestPortal->GetActorNameOrLabel());
		ClosestPortal->SetVisibleTemp(true);
	}
}

void AERASCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (OnUseItem.IsBound())
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			FVector StartPos = PC->PlayerCameraManager->GetCameraLocation();
			FVector EndPos = StartPos + (PC->PlayerCameraManager->GetCameraRotation().Vector() * 1000.0f);

			FHitResult HitResult;
			if (GetWorld()->LineTraceSingleByChannel(HitResult, StartPos, EndPos, ECollisionChannel::ECC_Visibility))
			{
				HitResult.Location.X = int32(HitResult.Location.X / 100.0f) * 100.0f;
				HitResult.Location.Y = int32(HitResult.Location.Y / 100.0f) * 100.0f;
				UKismetSystemLibrary::DrawDebugSphere(this, HitResult.Location);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////// Input

void AERASCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AERASCharacter::OnPrimaryAction);


	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AERASCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AERASCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "Mouse" versions handle devices that provide an absolute delta, such as a mouse.
	// "Gamepad" versions are for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AERASCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AERASCharacter::LookUpAtRate);

	// Scroll wheel
	PlayerInputComponent->BindAxis("Scroll Up / Down Mouse", this, &AERASCharacter::ScrollRate);
}

void AERASCharacter::OnPrimaryAction()
{
	// Trigger the OnItemUsed Event
	OnUseItem.ExecuteIfBound();
}

void AERASCharacter::ScrollRate(float Rate)
{
	// Trigger the OnScrollWeapon Event
	if (Rate != 0.0f)
	{
		OnScrollItem.ExecuteIfBound(Rate);
	}
}

void AERASCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnPrimaryAction();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AERASCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AERASCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AERASCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AERASCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AERASCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

bool AERASCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AERASCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AERASCharacter::EndTouch);

		return true;
	}
	
	return false;
}
