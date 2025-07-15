#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "WeaponDamageType.generated.h"


UENUM(BlueprintType)
enum class EDamageType : uint8
{
    Lineal UMETA(DisplayName = "Lineal"),
    Radial UMETA(DisplayName = "Radial"),
    HitScan UMETA(DisplayName = "HitScan"),
};

UCLASS(Blueprintable, EditInlineNew)
class PHYSICS_API UWeaponDamageType : public UObject
{
	GENERATED_BODY()

public:
	/** @TODO: Create damage data object */
    UPROPERTY(EditAnyWhere)
    EDamageType m_DamageType;

    UPROPERTY(EditAnyWhere)
    TSubclassOf<UDamageType> DamageType;
};
