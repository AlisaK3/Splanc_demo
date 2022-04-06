// // Copyright 2019 Elliot Gray. All Rights Reserved.

#include "UIPFActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UIPFManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "UIPFInteractor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SphereComponent.h"



// Sets default values
AUIPFActor::AUIPFActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	//SMTEMP 
	//SetRootComponent(SMTEMP);
	//SMTEMP->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//SMTEMP->SetCollisionResponseToAllChannels(ECR_Overlap);
	

	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
	SetRootComponent(SkeletalMeshComp);
	SkeletalMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SkeletalMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SkeletalMeshComp->SetCollisionObjectType(ECC_Destructible);
	SkeletalMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	SkeletalMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Block);
	SkeletalMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	SkeletalMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);
	SkeletalMeshComp->SetMobility(EComponentMobility::Movable);


	BlockingCollisionOverideSMComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Blocking Collision Override Mesh"));
	BlockingCollisionOverideSMComp->SetupAttachment(RootComponent);
	BlockingCollisionOverideSMComp->SetVisibility(false);
	BlockingCollisionOverideSMComp->SetHiddenInGame(true);
	BlockingCollisionOverideSMComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingCollisionOverideSMComp->SetCollisionResponseToAllChannels(ECR_Block);
	BlockingCollisionOverideSMComp->SetMobility(EComponentMobility::Movable);
	/*
	SMComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SMComp"));
	SMComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SMComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SMComp->SetupAttachment(RootComponent);
	SMComp->SetMobility(EComponentMobility::Movable);
	*/
#if ENGINE_MINOR_VERSION >19
	SkeletalMeshComp->SetGenerateOverlapEvents(true);
#endif

#if ENGINE_MINOR_VERSION == 19
	SkeletalMeshComp->bGenerateOverlapEvents = true;
#endif


}

// Called when the game starts or when spawned
void AUIPFActor::BeginPlay()
{
	Super::BeginPlay();
	//SkeletalMeshComp->SetSimulatePhysics(true);

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

	if (UIPFManagerect.IsValid() && UIPFManagerect->bOverrideTraceChannel)
	{
		SkeletalMeshComp->SetCollisionObjectType(UIPFManagerect->FoliageTraceChannel.GetValue());
	}


}

void AUIPFActor::SkeletonSleep(UPrimitiveComponent* Comp, FName Bone)
{
	if (!UIPFManagerect.IsValid())
		return;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Sleep detected");
	if(GetDistanceTo(UGameplayStatics::GetPlayerPawn(this, 0))>UIPFManagerect->PlayerPhysDeactivationRadius)
	{
		MyISM->AddInstance(GetActorTransform());
		Destroy();
	}

}

void AUIPFActor::UpdateTransition()
{
	TransitionAlpha += 0.016;
	DynamicInstance->SetScalarParameterValue("DitherAlpha", FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.0f,2.0f), TransitionAlpha));
	DynamicInstanceStatic->SetScalarParameterValue("DitherAlpha", 1.0f- FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.0f, 2.0f), TransitionAlpha));
	if(TransitionAlpha>1)
	{
		GetWorld()->GetTimerManager().ClearTimer(TransTimerHandle);
		

		//MyISM->RemoveInstance(MyItem);
	}
}

// Called every frame
void AUIPFActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUIPFActor::DelayPhysEnabled()
{
	SkeletalMeshComp->SetSimulatePhysics(true);
	//SkeletalMeshComp->SetAllBodiesBelowSimulatePhysics(MyType->AnchorBone, true, false);
	//SkeletalMeshComp->PutAllRigidBodiesToSleep();
	TArray<FName> Names;
	Names = SkeletalMeshComp->GetAllSocketNames();
	for (auto& Itr : Names)
	{
		if(!SkeletalMeshComp->IsSimulatingPhysics(Itr)&&MyType->KinematicBlocksPawn)
		{
			USphereComponent* comp = NewObject<USphereComponent>(this);
			comp->RegisterComponent();			
			comp->AttachToComponent(SkeletalMeshComp, FAttachmentTransformRules::SnapToTargetIncludingScale,Itr);
			comp->SetCollisionResponseToAllChannels(ECR_Ignore);
			comp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
			comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			comp->SetSphereRadius(5);
			//comp->SetHiddenInGame(false);
		}
	}
	if(QueueImpulse)
	{
		SkeletalMeshComp->AddImpulseAtLocation(QueuedForce*QueuedHit.Normal,QueuedHit.ImpactPoint, SkeletalMeshComp->FindClosestBone(QueuedHit.ImpactPoint, (FVector*)0, 0.0f, true));
	}

}

void AUIPFActor::Init(bool bFromManual)
{
	//DynamicInstance = SkeletalMeshComp->CreateDynamicMaterialInstance(0);
	//DynamicInstanceStatic = SMComp->CreateDynamicMaterialInstance(0);

	//FTimerDelegate TransTimerDelegate;
	//TransTimerDelegate.BindUFunction(this, FName("UpdateTransition"));
	//GetWorldTimerManager().SetTimer(TransTimerHandle, TransTimerDelegate, 0.016f, true);

	FTimerHandle TH;
	FTimerDelegate TimerDel;
	TimerDel.BindUFunction(this, FName("DelayPhysEnabled"));
	GetWorldTimerManager().SetTimer(TH, TimerDel, 0.1f, false);

	if (bFromManual)
	{
		FTimerDelegate TimerDelSleep;
		TimerDelSleep.BindUFunction(this, FName("TrySleep"));
		GetWorldTimerManager().SetTimer(THS, TimerDelSleep, 5.0f, true);
		//SkeletalMeshComp->OnComponentSleep.AddDynamic(this, &AUIPFActor::SkeletonSleep);
	}

	
}

void AUIPFActor::TrySleep()
{
	if (UIPFManagerect.IsValid() && GetDistanceTo(UGameplayStatics::GetPlayerPawn(this, 0)) > UIPFManagerect->PlayerPhysDeactivationRadius)
	{
		bool Sleep = true;
		//SkeletalMeshComp->GetPhysicsLinearVelocity()
		TArray<FName> Names;
		Names = SkeletalMeshComp->GetAllSocketNames();
		for (auto& Itr : Names)
		{
			if(SkeletalMeshComp->GetPhysicsAngularVelocityInDegrees(Itr).Size()>SleepThreshold)
			{
				Sleep = false;
			}
		}

		if(Sleep)
		{

			MyISM->AddInstance(GetActorTransform());
			GetWorld()->GetTimerManager().ClearTimer(THS);
			Destroy();
		
		}
		else
		{
			if(THS.IsValid() ==false)
			{
			//try sleep again in 3 seconds
				FTimerDelegate TimerDelSleep;
				TimerDelSleep.BindUFunction(this, FName("TrySleep"));
				GetWorldTimerManager().SetTimer(THS, TimerDelSleep, 2.0f, true);
				//SkeletalMeshComp->OnComponentSleep.AddDynamic(this, &AUIPFActor::SkeletonSleep);
			}
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(THS);
	}

}

