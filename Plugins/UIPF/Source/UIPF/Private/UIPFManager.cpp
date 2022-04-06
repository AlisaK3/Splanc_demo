// Copyright 2019 Elliot Gray. All Rights Reserved.

#include "UIPFManager.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleSystem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/DamageType.h"
#include "Kismet/KismetStringLibrary.h"
#include "UIPF.h"
#include "UIPFImpulse.h"


//DECLARE_CYCLE_STAT(TEXT("UIPF/Automatic Interaction"), STAT_AutoInteraction, STATGROUP_UIPF);
//DECLARE_CYCLE_STAT(TEXT("UIPF/Manual Interaction"), STAT_ManualInteraction, STATGROUP_UIPF);
//DECLARE_CYCLE_STAT(TEXT("UIPF/WaterBody"), STAT_WaterBody, STATGROUP_UIPF);

// Sets default values
AUIPFManager::AUIPFManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(false);
	// Box comp that might come in handy later for more aggressive culling/lodding
	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("ActivationCollision"));
	SetRootComponent(BoxComp);
	BoxComp->SetVisibility(false, false);


	//get all the content refs we need
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> MPCAsset(TEXT("/UIPF/MPC_UIPF.MPC_UIPF"));
	if (MPCAsset.Succeeded())
	{
		MPC_UIPF = MPCAsset.Object;
	}

	//get manual force splat material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ManSplatMat(TEXT("/UIPF/Simulation/Materials/UIPFManualSplat.UIPFManualSplat"));
	if (ManSplatMat.Succeeded())
	{
		ManForceSplatMat = ManSplatMat.Object;
	}

	//get heightsim material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PCMat(TEXT("/UIPF/Simulation/Materials/PersistentAccumulator.PersistentAccumulator"));
	if (PCMat.Succeeded())
	{
		PersistentAccumulator = PCMat.Object;
	}

	//get heightsim material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> splatvel(TEXT("/UIPF/Simulation/Materials/UIPFManualSplat_Additive.UIPFManualSplat_Additive"));
	if (splatvel.Succeeded())
	{
		VelSplatMat = splatvel.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> rv(TEXT("/UIPF/Simulation/Textures/UIPF_RT_Velocity.UIPF_RT_Velocity"));
	if (rv.Succeeded())
	{
		rtvel = rv.Object;
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Found gh2");
	}


	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> rp(TEXT("/UIPF/Simulation/Textures/UIPF_RT_Pos.UIPF_RT_Pos"));
	if (rp.Succeeded())
	{
		rtpos = rp.Object;
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Found gh2");
	}

	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> ro(TEXT("/UIPF/Simulation/Textures/UIPF_RT_Objects.UIPF_RT_Objects"));
	if (ro.Succeeded())
	{
		rtobj = ro.Object;
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Found gh2");
	}

	static ConstructorHelpers::FObjectFinder<UDataTable>UIPFTypeFind(TEXT("/UIPF/UIPFTypes.UIPFTypes"));
	if (UIPFTypeFind.Succeeded())
	{
		FoliageDataTable = UIPFTypeFind.Object;
	}
}



// Called when the game starts or when spawned
void AUIPFManager::BeginPlay()
{
	Super::BeginPlay();
	
	if (GetWorld()->GetNetMode() == NM_DedicatedServer || GetGameInstance()->GetWorldContext()->RunAsDedicated)
	{
		bIsInteractive = false;
	}
	if (!UseShaderInteraction)
	{
		SetActorTickEnabled(false);
	}

	/** Platform specific interaction type overrides*/
#if PLATFORM_SWITCH
	if (!UseTruePhysicsSwitch)
	{
		UseTruePhysics = false;
	}
#endif
#if PLATFORM_XBOXONE
	if (!UseTruePhysicsXbox)
	{
		UseTruePhysics = false;
	}
#endif
#if PLATFORM_PS4
	if (!UseTruePhysicsPS4)
	{
		UseTruePhysics = false;
	}
#endif
#if PLATFORM_ANDROID
	if (!UseTruePhysicsAndroid)
	{
		UseTruePhysics = false;
	}
#endif
#if PLATFORM_IOS
	if (!UseTruePhysicsIOS)
	{
		UseTruePhysics = false;
	}
#endif
	if (bIsInteractive)
	{
		if (bLimitTickRate)
		{
			ChangeBodyTickRate(TickRate);
		}
		InitializeRenderTargets(false);
	}

	AccumulatorInst = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, PersistentAccumulator);

	ManualSplatInst = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, ManForceSplatMat);
	VelocitySplatInst = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, VelSplatMat);
}

void AUIPFManager::ApplyInteractivityForces()//probably delete this function
{


}

void AUIPFManager::WritePersistent()
{
		//AccumulatorInst->SetTextureParameterValue(TEXT("Texture"), globalheight0);
		//UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, rtpos, Canvas, Size, Context);
		//Canvas->K2_DrawMaterial(ManualSplatInst, FVector2D(0, 0), Size, FVector2D(0, 0), FVector2D(1, 1), 0.0f, FVector2D(0, 0));
		//UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
		AccumulatorInst->SetTextureParameterValue(TEXT("Texture"), rtvel);
		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, rtvel, Canvas, Size, Context);
		Canvas->K2_DrawMaterial(AccumulatorInst, FVector2D(0, 0), Size, FVector2D(0, 0), FVector2D(1, 1), 0.0f, FVector2D(0, 0));
		UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}

FVector AUIPFManager::WorldPosToRelativeUV(FVector WorldPos)
{
	float IntDistance = UKismetMaterialLibrary::GetScalarParameterValue(this, MPC_UIPF, TEXT("InteractiveDistance"));

	float x = (UKismetMathLibrary::GenericPercent_FloatFloat(WorldPos.X + (IntDistance / 2), IntDistance)) / -IntDistance;
	float y = (UKismetMathLibrary::GenericPercent_FloatFloat(WorldPos.Y + (IntDistance / 2), IntDistance)) / IntDistance;
	return FVector(x, y, 0);
}


void AUIPFManager::InitializeRenderTargets(bool bUpdate)
{

	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Orange, "InitializeRenderTargets() Update = " + UKismetStringLibrary::Conv_BoolToString(bUpdate));
	if (bPrimary == true)
	{
		//Clear global render targets ready for use
		UKismetRenderingLibrary::ClearRenderTarget2D(this, rtvel);
		UKismetRenderingLibrary::ClearRenderTarget2D(this, rtpos);
		UKismetRenderingLibrary::ClearRenderTarget2D(this, rtobj);
	}
	else
	{
		//create local render targets if they don't already exist
		
	}
	/** Platform specific interaction type overrides*/
#if PLATFORM_SWITCH

#endif
#if PLATFORM_XBOXONE

#endif
#if PLATFORM_PS4

#endif
#if PLATFORM_ANDROID

#endif
#if PLATFORM_IOS

#endif

}

void AUIPFManager::ChangeBodyTickRate(float fNewTickRate)
{
	if (fNewTickRate == 0)
	{
		SetActorTickInterval(0);
	}
	else
	{
		SetActorTickInterval(1 / fNewTickRate);
	}
}

float AUIPFManager::CalculateSizeFromWorld(float DesiredRadius)
{
	DesiredRadius = FMath::Clamp(DesiredRadius, 85.0f, 1708.0f);

	if(DesiredRadius <= 150)
	{
		return UKismetMathLibrary::MapRangeUnclamped(DesiredRadius,85.0f, 150.0f, 0.4f, 0.7f);
	}
	else if (DesiredRadius <= 290)
	{
		return UKismetMathLibrary::MapRangeUnclamped(DesiredRadius, 150.0f, 290.0f, 0.7f, 0.9f);
	}
	else if (DesiredRadius <= 509)
	{
		return UKismetMathLibrary::MapRangeUnclamped(DesiredRadius, 290.0f, 509.0f, 0.9f, 0.98f);
	}
	else if (DesiredRadius <= 730)
	{
		return UKismetMathLibrary::MapRangeUnclamped(DesiredRadius, 509.0f, 730.0f, 0.98f, 0.99f);
	}
	else //if (DesiredRadius <= 1708)
	{
		return UKismetMathLibrary::MapRangeUnclamped(DesiredRadius, 730.0f, 1708.0f, 0.99f, 1.0f);
	}

}

void AUIPFManager::ImpulseAtLocation(FVector Location, float ImpulseDiameter, float ImpulseStrength /*= 0.9f*/)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AUIPFImpulse* SpawnedImpulse = GetWorld()->SpawnActor<AUIPFImpulse>(Location, FRotator::ZeroRotator, SpawnParams);
	SpawnedImpulse->InitializeImpulse(ImpulseDiameter, UseTruePhysics, UseShaderInteraction, ImpulseStrength);
}




// Called every frame
void AUIPFManager::Tick(float DeltaTime)
{
	//SCOPE_CYCLE_COUNTER(STAT_WaterBody);
	Super::Tick(DeltaTime);
	if (bIsInteractive)
	{
		//update player position in the mpc.  if there's a valid pawn
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (PlayerPawn)
		{
			UKismetMaterialLibrary::SetVectorParameterValue(this, MPC_UIPF, TEXT("playerpos"), FLinearColor(PlayerPawn->GetActorLocation()));
		}
		else if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Pawn was invalid.  Centering on camera");
			UKismetMaterialLibrary::SetVectorParameterValue(this, MPC_UIPF, TEXT("playerpos"), FLinearColor(PC->PlayerCameraManager->GetCameraLocation()));
		}
		else
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Camera was invalid, not updating sim center position");
			UE_LOG(LogTemp, Warning, TEXT("UIPFManager failed to find a pawn OR camera to center water simulation on.  UIPFManager.CPP AUIPFManager::Tick()"));
		}

		//ApplyInteractivityForces();
		UKismetRenderingLibrary::ClearRenderTarget2D(this, rtobj);
		WritePersistent();
	}


}


#if WITH_EDITOR
void AUIPFManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

}
#endif

void AUIPFManager::BeginDestroy()
{
	Super::BeginDestroy();
}


void AUIPFManager::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);

}


void AUIPFManager::FoliageForceAtLocation(float fStrength, float fSizePercent, FVector HitLocation, bool bIsImpulse)
{
	/*Don't run on dedicated server*/
	if (GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		//SCOPE_CYCLE_COUNTER(STAT_ManualInteraction);
		FLinearColor UVLC = UKismetMaterialLibrary::GetVectorParameterValue(this, MPC_UIPF, TEXT("playerpos"));;
		float IntDistance = UKismetMaterialLibrary::GetScalarParameterValue(this, MPC_UIPF, TEXT("InteractiveDistance"));
		FVector WPVec = FVector(UVLC.R, UVLC.G, 0);

		//if the ripple is within interactive bounds draw it
		if (HitLocation.X > WPVec.X - (IntDistance - 400) / 2 && HitLocation.X<WPVec.X + (IntDistance - 400) / 2 && HitLocation.Y>WPVec.Y - (IntDistance - 400) / 2 && HitLocation.Y < WPVec.Y + (IntDistance - 400) / 2)
		{
			FVector HitVec = HitLocation;
			FVector UVVec = WorldPosToRelativeUV(HitVec);
			ManualSplatInst->SetVectorParameterValue(TEXT("ForcePosition"), FLinearColor(UVVec));
			ManualSplatInst->SetVectorParameterValue(TEXT("InteractorWorldPos"), FLinearColor(HitLocation));
			ManualSplatInst->SetScalarParameterValue(TEXT("ForceSizePercent"), fSizePercent);
			ManualSplatInst->SetScalarParameterValue(TEXT("ForceStrength"), fStrength);

			FVector impulseboost;
			if (bIsImpulse)
			{
				impulseboost = FVector(1, 1, 1);
			}
			else
			{
				impulseboost = FVector(0, 0, 0);
			}
			VelocitySplatInst->SetVectorParameterValue(TEXT("ForcePosition"), FLinearColor(UVVec));
			VelocitySplatInst->SetVectorParameterValue(TEXT("InteractorWorldPos"), impulseboost);
			VelocitySplatInst->SetScalarParameterValue(TEXT("ForceSizePercent"), fSizePercent);
			VelocitySplatInst->SetScalarParameterValue(TEXT("ForceStrength"), fStrength);


			UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, rtvel, CanvasInteract, SizeInteract, ContextInteract);
			Canvas->K2_DrawMaterial(VelocitySplatInst, FVector2D(0, 0), SizeInteract, FVector2D(0, 0), FVector2D(1, 1), 0.0f, FVector2D(0, 0));
			UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, ContextInteract);
			//writepostarget
			UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, rtpos, CanvasInteract, SizeInteract, ContextInteract);
			Canvas->K2_DrawMaterial(ManualSplatInst, FVector2D(0, 0), SizeInteract, FVector2D(0, 0), FVector2D(1, 1), 0.0f, FVector2D(0, 0));
			UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, ContextInteract);

			//Writeobjtarget
			ManualSplatInst->SetScalarParameterValue(TEXT("ForceStrength"), 1);
			UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, rtobj, CanvasInteract, SizeInteract, ContextInteract);
			Canvas->K2_DrawMaterial(ManualSplatInst, FVector2D(0, 0), SizeInteract, FVector2D(0, 0), FVector2D(1, 1), 0.0f, FVector2D(0, 0));
			UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, ContextInteract);
		}
	}
	ForceAppliedAtLocation(HitLocation, fStrength);
}

