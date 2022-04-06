// Copyright 2019 Elliot Gray. All Rights Reserved.

#include "UIPFInteractor.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "UIPFActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "WorldCollision.h"
#include "UIPFManager.h"
#include "TimerManager.h"
#include "Runtime/Launch/Resources/Version.h"

// Sets default values for this component's properties
UUIPFInteractor::UUIPFInteractor()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	//PrimaryComponentTick.TickInterval = (1/30);
	SetMobility(EComponentMobility::Movable);
}


// Called when the game starts
void UUIPFInteractor::BeginPlay()
{
	Super::BeginPlay();
	/**Get the uipf object**/
	LastUpdateLocation = GetComponentLocation();
	if(bLimitTickRate)
	{
		SetComponentTickInterval(1/FMath::Clamp(TickRate, 1.0f,240.0f));
	}
	TArray<AActor*> UIPFManagers;
	UGameplayStatics::GetAllActorsOfClass(this, AUIPFManager::StaticClass(), UIPFManagers);
	if (UIPFManagers.Num() > 0)
	{
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Found a light");
		AUIPFManager* obj = Cast<AUIPFManager>(UIPFManagers[0]);
		if (obj)
		{
			UIPFManagerect = obj;
			PlayerActivationRadius = obj->PlayerPhysActivationRadius;
			PlayerDeactivationRadius = obj->PlayerPhysDeactivationRadius;
			UIPFDT = UIPFManagerect->FoliageDataTable;
			UsingMassiveGrass = UIPFManagerect->UseShaderInteraction;
			UsingTruePhysics = UIPFManagerect->UseTruePhysics;
			DefaultIntActiveDist = UIPFManagerect->DefaultInteractorActivationDistance;
			if(!UsingMassiveGrass)
			{
				SetComponentTickEnabled(false);
				PrimaryComponentTick.bCanEverTick = false;
			}
		}
	}
	if(UsingTruePhysics && UIPFManagerect.IsValid())
	{
		// ...
		ActivationShape = FCollisionShape::MakeSphere(PlayerActivationRadius);
		FoliageDeactivationVolume = NewObject<USphereComponent>(this);
		FoliageDeactivationVolume->RegisterComponent();
		FoliageDeactivationVolume->SetSphereRadius(PlayerDeactivationRadius);
		//FoliageDeactivationVolume->SetHiddenInGame(false);
		FoliageDeactivationVolume->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		FoliageDeactivationVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		FoliageDeactivationVolume->SetCollisionObjectType(UIPFManagerect->bOverrideTraceChannel ? UIPFManagerect->FoliageTraceChannel.GetValue() : ECollisionChannel::ECC_WorldDynamic);
		if (UIPFManagerect->bOverrideTraceChannel)
		{
			FoliageDeactivationVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			FoliageDeactivationVolume->SetCollisionResponseToChannel(UIPFManagerect->FoliageTraceChannel.GetValue(), ECollisionResponse::ECR_Overlap);
		}
		else
		{
			FoliageDeactivationVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		}

		//FoliageDeactivationVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
		FoliageDeactivationVolume->OnComponentEndOverlap.AddDynamic(this, &UUIPFInteractor::OnDeactivationEndOverlap);


		TraceDelegate.BindUObject(this, &UUIPFInteractor::OnTraceCompleted);


		TArray<FName> Names = UIPFDT->GetRowNames();
		FString ContextString;
		for (auto& Itr : Names)
		{
			UIPFTypesArray.Add(UIPFDT->FindRow<FUIPFType>(Itr, ContextString));
		}

		FTimerDelegate TimerDel;
		TimerDel.BindUFunction(this, FName("PhysCheck"));
		GetWorld()->GetTimerManager().SetTimer(PhysTimerHandle, TimerDel, 1/ FMath::Clamp(PhysFoliageSwapTickRate, 1.0f, 240.0f), true);
	}


}


void UUIPFInteractor::SweepActivation()
{
	TArray<FHitResult> OutResults;
	static const FName SphereTraceMultiName(TEXT("SphereTraceMulti"));
	FCollisionQueryParams Params;
	GetWorld()->SweepMultiByChannel(OutResults, GetComponentLocation(), GetComponentLocation(), FQuat::Identity, UIPFManagerect->bOverrideTraceChannel? UIPFManagerect->FoliageTraceChannel.GetValue() : ECollisionChannel::ECC_Visibility, ActivationShape, Params);
	
	for (auto& CompItr : OutResults)
	{
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Hit Iterator");
		UHierarchicalInstancedStaticMeshComponent* ism = Cast<UHierarchicalInstancedStaticMeshComponent>(CompItr.Component);
		if (ism)
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Hit ISM");
			SwapISMforPhys(CompItr);
		}
	}

}


void UUIPFInteractor::AsyncSweep()
{
	//ShouldTrace
	if (LastTraceHandle._Data.FrameNumber != 0)
	{
		FTraceDatum OutData;
		if (GetWorld()->QueryTraceData(LastTraceHandle, OutData))
		{
			// Clear out handle so next tick we don't enter
			LastTraceHandle._Data.FrameNumber = 0;
			// trace is finished, do stuff with results
			DoWorkWithTraceResults(OutData);
		}
	}
	if (!GetWorld()->IsTraceHandleValid(LastTraceHandle, false))
	{
		LastTraceHandle = RequestAsyncTrace();
	}
}

AUIPFActor* UUIPFInteractor::SwapISMforPhys(FHitResult hit, bool bFromManual)
{
	AUIPFActor* PhysCopy = nullptr;
	UHierarchicalInstancedStaticMeshComponent* ism = Cast<UHierarchicalInstancedStaticMeshComponent>(hit.Component);
	if (ism)
	{
		FUIPFType* FolType = FindFoliageType(ism->GetStaticMesh());
		if (FolType != nullptr)
		{
			USkeletalMesh* SkeletalMesh = FolType->SkeletalMesh;
		
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			FTransform Trans;
			ism->GetInstanceTransform(hit.Item, Trans, true);
			if (Trans.GetLocation() == FVector(0, 0, 0))
			{
				//bugged ism
				return PhysCopy;
			}
			TSubclassOf<AUIPFActor> folclass = UIPFManagerect->UIPFActorClass;
			PhysCopy = GetWorld()->SpawnActor<AUIPFActor>(folclass, Trans.GetLocation(), Trans.GetRotation().Rotator(), SpawnParams);
			PhysCopy->SetActorScale3D(Trans.GetScale3D());
			PhysCopy->SkeletalMeshComp->SetSkeletalMesh(SkeletalMesh);
			if (FolType->StaticCollisionOverride)
			{
				PhysCopy->BlockingCollisionOverideSMComp->SetStaticMesh(FolType->StaticCollisionOverride);
				if (FolType->bDebugMakeColliderVisible)
				{
					PhysCopy->BlockingCollisionOverideSMComp->SetVisibility(true);
					PhysCopy->BlockingCollisionOverideSMComp->SetHiddenInGame(false);
				}
			}
			//PhysCopy->SMComp->SetStaticMesh(FolType->StaticMesh);
			//PhysCopy->SkeletalMeshComp->SetSimulatePhysics(true);
			PhysCopy->MyISM = ism;
			PhysCopy->MyType = FolType;

			bool bFoliageTraceOverridden = false;
			ECollisionChannel NewCollisionChannel;
			if (UIPFManagerect.IsValid())
			{
				if (UIPFManagerect->bOverrideTraceChannel)
				{
					NewCollisionChannel = UIPFManagerect->FoliageTraceChannel.GetValue();
					bFoliageTraceOverridden = true;
				}
			}

			if(FolType->CollideWithWorldDynamic)
			{
				PhysCopy->SkeletalMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Block);
			}
			if (FolType->CollideWithWorldStatic)
			{
				PhysCopy->SkeletalMeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECR_Block);
			}		
			if (FolType->CollideWithPhysicsFoliage)	
			{
				PhysCopy->SkeletalMeshComp->SetCollisionResponseToChannel(bFoliageTraceOverridden ? NewCollisionChannel : ECollisionChannel::ECC_Destructible, ECR_Block);
			}



			ism->RemoveInstance(hit.Item);
			PhysCopy->MyItem = hit.Item;
			PhysCopy->Init(bFromManual);
		}
		else
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "SkelNullPtr");
		}
	}

	return PhysCopy;
}



FUIPFType* UUIPFInteractor::FindFoliageType(UStaticMesh* StaticIn)
{
	FUIPFType* FoundType = nullptr;
	for (auto& itr : UIPFTypesArray)
	{
		if(itr->StaticMesh == StaticIn)
		{
			FoundType = itr;
		}
	}
	
	return FoundType;
}



void UUIPFInteractor::InteractWithGrass(float InteractRadius)
{
	if(UIPFManagerect.IsValid())
	{
		float fStrength;
		float size = InteractRadius;
		if (bForceFixedInteractionStrength)
		{
			fStrength = FixedStrength;
		}
		else
		{
			float Vel;

			Vel = bPerCompVelocity ? CalculatedVelocity.Size() : GetOwner()->GetVelocity().Size();
			if (Vel > VelocityDeadZone)
			{
				fStrength = UKismetMathLibrary::MapRangeUnclamped(Vel, 300, 600, 0.03, 0.8);
			}
			else
			{
				fStrength = UKismetMathLibrary::MapRangeUnclamped(Vel, 0, 300, 0, 0.03);
			}
		}

		UIPFManagerect->FoliageForceAtLocation(fStrength,size, bPerCompVelocity ? GetComponentLocation() : GetOwner()->GetActorLocation(), false);
	}
}

FTraceHandle UUIPFInteractor::RequestAsyncTrace()
{
	if(GetWorld()&& UIPFManagerect.IsValid())
	{
		FCollisionQueryParams Params;
		Params.bTraceComplex = false;

#if ENGINE_MINOR_VERSION >25
		return GetWorld()->AsyncSweepByChannel(EAsyncTraceType::Multi, GetComponentLocation(), GetComponentLocation() - FVector(0, 0, 1), FRotator::ZeroRotator.Quaternion(), UIPFManagerect->bOverrideTraceChannel ? UIPFManagerect->FoliageTraceChannel.GetValue() : ECollisionChannel::ECC_Visibility, ActivationShape, Params, FCollisionResponseParams::DefaultResponseParam, &TraceDelegate);
#else
		return GetWorld()->AsyncSweepByChannel(EAsyncTraceType::Multi, GetComponentLocation(), GetComponentLocation()-FVector(0,0,1), UIPFManagerect->bOverrideTraceChannel ? UIPFManagerect->FoliageTraceChannel.GetValue() : ECollisionChannel::ECC_Visibility, ActivationShape, Params, FCollisionResponseParams::DefaultResponseParam, &TraceDelegate);
#endif	

	}else return FTraceHandle();

}

void UUIPFInteractor::OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data)
{
	ensure(Handle == LastTraceHandle);
	DoWorkWithTraceResults(Data);
	LastTraceHandle._Data.FrameNumber = 0; // reset it
} 

void UUIPFInteractor::DoWorkWithTraceResults(const FTraceDatum&TraceData)
{
	for (auto& CompItr : TraceData.OutHits)
	{
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Hit Iterator");
		UHierarchicalInstancedStaticMeshComponent* ism = Cast<UHierarchicalInstancedStaticMeshComponent>(CompItr.Component);
		if (ism)
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Hit ISM");
			SwapISMforPhys(CompItr);
		}
	}
}

// Called every frame
void UUIPFInteractor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FVector CamLoc = FVector(0);
	if (APlayerCameraManager* CamMan = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		CamLoc = CamMan->GetCameraLocation();
	}
			
	bool inrange = (CamLoc - GetComponentLocation()).Size() < (bOverrideInteractorActivationDistance ? InteractorActivationDistance : DefaultIntActiveDist);

	if(UsingMassiveGrass && inrange)
	{
		if (bInteractWithGrass)
		{
			if (UIPFManagerect.IsValid())
			{
				if (bPerCompVelocity)
				{
					FVector CurrentLoc = GetComponentLocation();
					CalculatedVelocity = (CurrentLoc - LastUpdateLocation) / DeltaTime;
					LastUpdateLocation = CurrentLoc;
				}
				InteractWithGrass(UIPFManagerect->CalculateSizeFromWorld(fGrassInteractionDiameter));
			}

		}
	}
}


void UUIPFInteractor::OnDeactivationEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (GetOwner() == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		AUIPFActor* KillActor = Cast<AUIPFActor>(OtherActor);
		if (KillActor)
		{
			//float instance = KillActor->MyISM->AddInstanceWorldSpace(KillActor->GetActorTransform());
			//KillActor->Destroy();
			KillActor->TrySleep();
		}
	}

}

void UUIPFInteractor::ManuallyActivateFoliage(bool WithImpulse, float ImpulseStrength, FHitResult hit)
{
	if (UIPFManagerect.IsValid())
	{
		if (FMath::Abs((UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation() - hit.ImpactPoint).Size()) < UIPFManagerect->MaxManualActivationDistance)
		{
			AUIPFActor* PhysFoliage = SwapISMforPhys(hit, true);
			if (PhysFoliage != nullptr)
			{
				if (WithImpulse)
				{
					PhysFoliage->QueueImpulse = true;
					PhysFoliage->QueuedForce = -ImpulseStrength;
					PhysFoliage->QueuedHit = hit;
				}
			}
		}
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Tried to manually activate UIPF Foliage but couldn't find a manager in the scene.  Have you added a UIPFManager to the level?");
	}

}

void UUIPFInteractor::PhysCheck()
{
	if (GetOwner() == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (FVector::DistSquared(GetComponentLocation(), LastPhysCheckLocation)>FMath::Square(10.0f))
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Ticking as player");
			//SweepActivation();
			AsyncSweep();
		}
		LastPhysCheckLocation = GetComponentLocation();
	}
}

