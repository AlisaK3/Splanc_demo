// // Copyright 2019 Elliot Gray. All Rights Reserved.

#include "UIPFImpulse.h"
#include "TimerManager.h"
#include "UIPFManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"


// Sets default values
AUIPFImpulse::AUIPFImpulse()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	SetRootComponent(SphereComp);
	SetReplicates(false);
}

// Called when the game starts or when spawned
void AUIPFImpulse::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> UIPFManagers;
	UGameplayStatics::GetAllActorsOfClass(this, AUIPFManager::StaticClass(), UIPFManagers);
	if (UIPFManagers.Num() > 0)
	{
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Found a light");
		AUIPFManager* obj = Cast<AUIPFManager>(UIPFManagers[0]);
		if (obj)
		{
			UIPFManagerect = obj;
		}
	}
}

void AUIPFImpulse::InitializeImpulse(float ImpulseRadius, bool UsingTruePhys, bool UsingShaderInt, float ImpulseStrength)
{
	UsingShader = UsingShaderInt;
	UsingPhys = UsingTruePhys;
	if(UIPFManagerect.IsValid())
	{
		fTargetSize = UIPFManagerect->CalculateSizeFromWorld(ImpulseRadius);
	}
	fImpulseStrength = ImpulseStrength;
	SphereComp->SetSphereRadius(ImpulseRadius/2);

	StartImpulse();

}

void AUIPFImpulse::StartImpulse()
{
	if (UsingShader)
	{
		GetWorldTimerManager().SetTimer(ImpulseTicker, this, &AUIPFImpulse::ImpulseTick, 0.001f, true);
	}
	if (UsingPhys)
	{
		TArray<UPrimitiveComponent*> OverlappingComps;
		SphereComp->GetOverlappingComponents(OverlappingComps);
		for (auto& CompItr : OverlappingComps)
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Sphere overlapping ");
			USkeletalMeshComponent* skm = Cast<USkeletalMeshComponent>(CompItr);
			if (skm)
			{
				//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Added Impulse");
				skm->AddRadialImpulse(GetActorLocation(), SphereComp->GetScaledSphereRadius(), 800000.0*fImpulseStrength, ERadialImpulseFalloff::RIF_Linear);
			}
		}
	}
	
}

void AUIPFImpulse::ImpulseTick()
{
	float fSizeThisTick = UKismetMathLibrary::MapRangeClamped(fAccumulator, 0.0f, fImpulseLength,0.9f,fTargetSize);
	
	if(UIPFManagerect.IsValid())
		UIPFManagerect->FoliageForceAtLocation(fImpulseStrength,fSizeThisTick,GetActorLocation(),true);

	fAccumulator = (100*GetWorld()->GetDeltaSeconds())+fAccumulator;
	/*if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "ImpulseTick");
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::SanitizeFloat(fSizeThisTick));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::SanitizeFloat(fAccumulator));
	*/
	if(fAccumulator>fImpulseLength)
	{
		EndImpulse();
	}
}

void AUIPFImpulse::EndImpulse()
{
	GetWorld()->GetTimerManager().ClearTimer(ImpulseTicker);
//	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "ImpulseEnd");
	Destroy();
}



// Called every frame
void AUIPFImpulse::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

