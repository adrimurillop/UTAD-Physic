// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include <Components/StaticMeshComponent.h>
#include <PhysicsEngine/PhysicsHandleComponent.h>

#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

APhysicsCharacter::APhysicsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	m_PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	
}

void APhysicsCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APhysicsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// @TODO: Stamina update
	if (m_IsSprinting)
	{
		m_Stamina = FMath::Clamp(m_Stamina - m_StaminaDepletionRate * DeltaSeconds, 0.0f, m_MaxStamina);
	}
	else
	{
		m_Stamina = FMath::Clamp(m_Stamina + m_StaminaRecoveryRate * DeltaSeconds, 0.0f, m_MaxStamina);
	}

	// @TODO: Physics objects highlight
	FHitResult OutHit;
	FCollisionQueryParams QueryParams;

	QueryParams.AddIgnoredActor(this);
	
	const FVector OriginLocation = GetActorLocation() + GetFirstPersonCameraComponent()->GetRelativeLocation(); 
	const FVector TargetLocation = OriginLocation + (GetFirstPersonCameraComponent()->GetForwardVector() * m_MaxGrabDistance);
		
	const bool TraceResult = GetWorld()->LineTraceSingleByChannel(
		OutHit,
		OriginLocation,
		TargetLocation,
		ECC_Visibility,
		QueryParams
	);

	if (TraceResult && IsValid(OutHit.GetComponent()) && OutHit.GetComponent()->Mobility)
	{
		SetHighlightedMesh(Cast<UMeshComponent>(OutHit.GetComponent()));
	}
	else
	{
		SetHighlightedMesh(nullptr);
	}

	// @TODO: Grabbed object update)
	const UPrimitiveComponent* GrabbedComponent = m_PhysicsHandle->GetGrabbedComponent();
	
	if (IsValid(GrabbedComponent))
	{ 
		const FVector GrabLocation = OriginLocation + (GetFirstPersonCameraComponent()->GetForwardVector() * m_CurrentGrabDistance);

		m_PhysicsHandle->SetTargetLocationAndRotation(GrabLocation, FRotator::ZeroRotator);
	}
}

void APhysicsCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void APhysicsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Look);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &APhysicsCharacter::Sprint);
		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Triggered, this, &APhysicsCharacter::GrabObject);
		EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Completed, this, &APhysicsCharacter::ReleaseObject);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void APhysicsCharacter::SetIsSprinting(bool NewIsSprinting)
{
	if (m_IsSprinting == NewIsSprinting) { return; }
	// @TODO: Enable/disable sprinting use CharacterMovementComponent
	if (m_Stamina <= 0.0f)
	{
		m_IsSprinting = false;
	}
	
	m_IsSprinting = NewIsSprinting;
	if (m_IsSprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed *= m_SprintSpeedMultiplier;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed /= m_SprintSpeedMultiplier;
	}

}

void APhysicsCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void APhysicsCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APhysicsCharacter::Sprint(const FInputActionValue& Value)
{
	SetIsSprinting(Value.Get<bool>());
}

void APhysicsCharacter::GrabObject(const FInputActionValue& Value)
{
	// @TODO: Grab objects using UPhysicsHandleComponent
	if (!IsValid(m_PhysicsHandle->GetGrabbedComponent()))
	{
		FHitResult OutHit;
		FCollisionQueryParams QueryParams;

		QueryParams.AddIgnoredActor(this);

		const FVector OriginLocation = GetActorLocation() + GetFirstPersonCameraComponent()->GetRelativeLocation(); 
		const FVector TargetLocation = OriginLocation + (GetFirstPersonCameraComponent()->GetForwardVector() * m_MaxGrabDistance);
		
		const bool TraceResult = GetWorld()->LineTraceSingleByChannel(
			OutHit,
			OriginLocation,
			TargetLocation,
			ECC_Visibility,
			QueryParams
		);

		if (TraceResult && IsValid(OutHit.GetComponent()))
		{
			GEngine->AddOnScreenDebugMessage(
				0,
				3.0f,
				FColor::Cyan,
				"Grabbed an Actor!"
			);
			
			m_PhysicsHandle->GrabComponentAtLocationWithRotation(OutHit.GetComponent(), OutHit.BoneName, OutHit.Location, FRotator::ZeroRotator);
			m_PhysicsHandle->SetInterpolationSpeed(m_BaseInterpolationSpeed / OutHit.GetComponent()->GetMass());

			m_CurrentGrabDistance = OutHit.Distance;
		}
	}
}

void APhysicsCharacter::ReleaseObject(const FInputActionValue& Value)
{
	// @TODO: Release grabbed object using UPhysicsHandleComponent
	if (IsValid(m_PhysicsHandle->GetGrabbedComponent()))
	{
		GEngine->AddOnScreenDebugMessage(
			0,
			3.0f,
			FColor::Cyan,
			"Released an Actor!"
		);

		m_PhysicsHandle->ReleaseComponent();
		m_CurrentGrabDistance = 0.0f;
	}
}

void APhysicsCharacter::SetHighlightedMesh(UMeshComponent* StaticMesh)
{
	if(m_HighlightedMesh)
	{
		m_HighlightedMesh->SetOverlayMaterial(nullptr);
	}
	m_HighlightedMesh = StaticMesh;
	if (m_HighlightedMesh)
	{
		m_HighlightedMesh->SetOverlayMaterial(m_HighlightMaterial);
	}
}
