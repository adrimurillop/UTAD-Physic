// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/HitscanWeaponComponent.h"
#include <Kismet/KismetSystemLibrary.h>
#include <Kismet/GameplayStatics.h>
#include "PhysicsCharacter.h"
#include "PhysicsWeaponComponent.h"
#include <Camera/CameraComponent.h>
#include <Components/SphereComponent.h>

void UHitscanWeaponComponent::Fire()
{
	Super::Fire();
	m_WeaponDamageType->m_DamageType = EDamageType::HitScan;
	// @TODO: Add firing functionality

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	FVector StartLocation = GetOwner()->GetActorLocation() + PlayerController->PlayerCameraManager->GetCameraRotation().RotateVector(MuzzleOffset);
	FVector EndLocation = StartLocation + Character->FirstPersonCameraComponent->GetForwardVector() * m_fRange;
	FHitResult HitResult;

	DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red);
	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility))
	{
		onHitscanImpact.Broadcast(HitResult.GetActor(), HitResult.Location, HitResult.ImpactNormal * -1);
		HitResult.GetComponent()->AddImpulseAtLocation(
			HitResult.ImpactNormal * -1 * 100000,
			HitResult.ImpactPoint
		);
	}
}
