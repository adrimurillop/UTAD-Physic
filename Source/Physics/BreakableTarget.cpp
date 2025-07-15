#include "BreakableTarget.h"
#include <GeometryCollection/GeometryCollectionComponent.h>

FTargetBroken ABreakableTarget::OnTargetBroken;

// Sets default values
ABreakableTarget::ABreakableTarget()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	SetRootComponent(StaticMesh);

	GeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	GeometryCollection->SetupAttachment(StaticMesh);
	GeometryCollection->OnChaosBreakEvent.AddDynamic(this, &ABreakableTarget::GeometryCollectionBroken);
	GeometryCollection->SetNotifyBreaks(true);

}

void ABreakableTarget::GeometryCollectionBroken(const FChaosBreakEvent& BreakEvent)
{
	// @TODO: Call this function when the geometry collection breaks
	if (!m_IsBroken)
	{
		m_IsBroken = true;

		OnTargetBroken.Broadcast(this);
		GeometryCollection->SetNotifyBreaks(false);
	}
}

