// Copyright 2019 Elliot Gray. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "UIPFActor.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UIPFManager.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;
class UMaterialParameterCollectionInstance;
class UMaterialParameterCollection;
class UBoxComponent;
class UPostProcessComponent;
class USceneCaptureComponent2D;


UCLASS(hideCategories = (Cooking, Input, Rendering, Replication, "Actor Tick"))
class UIPF_API AUIPFManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AUIPFManager();

	//UFUNCTION(BlueprintCallable)

	void InitializeRenderTargets(bool bUpdate);

	void ChangeBodyTickRate(float fNewTickRate);

	bool nolocationset = false;

	/**	Called when ripples are created manually at a location.  Doesn't trigger for automatically detected interactions based on custom depth*/
	UFUNCTION(BlueprintImplementableEvent, Category = "UIPF Events")
	void ForceAppliedAtLocation(FVector Loc, float Strength = 1);

	/**	Event that triggers when point damage effects would be applied.  Will trigger even if automatic effects are disabled for ease of extension*/
	//UFUNCTION(BlueprintImplementableEvent, Category = "UIPF Events")
	//void OnPointDamageEffect(FVector Location, float RippleStrengthScaled, float RippleSizeScaled, float DamageAmount = 1);

	/**	Event that triggers when point damage effects would be applied.  Will trigger even if automatic effects are disabled for ease of extension*/
	//UFUNCTION(BlueprintImplementableEvent, Category = "UIPF Events")
	//void OnRadialDamageEffect(FVector RippleLocation, float RippleStrengthScaled, float RippleSizeScaled, float DamageAmount = 1);

	virtual float CalculateSizeFromWorld(float DesiredRadius);


	UFUNCTION(BlueprintCallable, Category = "UIPF Functions")
	virtual void ImpulseAtLocation(FVector Location, float ImpulseDiameter, float ImpulseStrength = 0.9f);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/**	Applies input for the ripple sim*/
	void ApplyInteractivityForces();

	void WritePersistent();

	/**	Convert world position to player relative 'UV' space.  Utility.*/
	FVector WorldPosToRelativeUV(FVector WorldPos);

	UBoxComponent* BoxComp;


public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Only one body can have this enabled at a time.  Enabling will disable on all other bodies.  Setting modified at runtime (eventually this will be 100% transparent) */
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Water Simulation")
	bool bPrimary = true;

	//UPROPERTY()
	bool F = false;

	/** Enable or disable shader based 'fake' interaction.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features")
	bool UseShaderInteraction = true;

	/** Enable or disable true physics interaction.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features")
	bool UseTruePhysics = true;
	/** Optionally you can disable true physics on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|ShaderInteraction", meta = (EditCondition = "UseShaderInteraction"))
	bool UseShaderIntSwitch = true;
	/** Optionally you can disable true physics or shader interaction on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|ShaderInteraction", meta = (EditCondition = "UseShaderInteraction"))
	bool UseShaderIntPS4 = true;
	/** Optionally you can disable true physics or shader interaction on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|ShaderInteraction", meta = (EditCondition = "UseShaderInteraction"))
	bool UseShaderIntXbox = true;
	/** Optionally you can disable true physics or shader interaction on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|ShaderInteraction", meta = (EditCondition = "UseShaderInteraction"))
	bool UseShaderIntIOS = true;
	/** Optionally you can disable true physics or shader interaction on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|ShaderInteraction", meta = (EditCondition = "UseShaderInteraction"))
	bool UseShaderIntAndroid = true;

	/** Optionally you can disable true physics on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|TruePhysics", meta = (EditCondition = "UseTruePhysics"))
	bool UseTruePhysicsSwitch = true;
	/** Optionally you can disable true physics or shader interaction on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|TruePhysics", meta = (EditCondition = "UseTruePhysics"))
	bool UseTruePhysicsPS4 = true;
	/** Optionally you can disable true physics or shader interaction on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|TruePhysics", meta = (EditCondition = "UseTruePhysics"))
	bool UseTruePhysicsXbox = true;
	/** Optionally you can disable true physics or shader interaction on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|TruePhysics", meta = (EditCondition = "UseTruePhysics"))
	bool UseTruePhysicsIOS = true;
	/** Optionally you can disable true physics or shader interaction on a per platform basis.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features\|PerPlatform\|TruePhysics", meta = (EditCondition = "UseTruePhysics"))
	bool UseTruePhysicsAndroid = true;



	/**(Not yet implemented) Whether or not to allow impulses to activate foliage physics states between ActivationRadius and MaxManualActivationDistance. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Features")
	bool ActivateOnImpulse = false;


	/** Disable ripple interaction if you want*/
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Simulation")
	bool bIsInteractive = true;

	/** Enable this to manually set tick rate.  Seems to have a negligible performance effect but might be useful of low spec hardware*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Simulation", meta = (EditCondition = "bIsInteractive"))
	bool bLimitTickRate = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Simulation", meta = (EditCondition = "bLimitTickRate"))
	float TickRate = 60;

	/**If this component is on the player pawn, this is the radius it will check for any foliage with a physics partner asset to swap for**/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	float PlayerPhysActivationRadius = 1000;

	/**When a physics foliage asset is this far away it will be replaced with an instanced static mesh variation**/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	float PlayerPhysDeactivationRadius = 2000;

	/**Max distance to allow manual activation of plants for**/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	float MaxManualActivationDistance = 10000;

	/**For performance reasons, any interactor compants further than this distance away from the camera will be deactivated**/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	float DefaultInteractorActivationDistance = 20000;

	/****/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Data")
	UDataTable* FoliageDataTable;

	UPROPERTY()
	UMaterialParameterCollection* MPC_UIPF;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Data")
	TSubclassOf<AUIPFActor> UIPFActorClass = AUIPFActor::StaticClass();

	/*This will setup foliage traces to happen against this channel only.
	DON'T FORGET TO UPDATE YOUR FOLIAGE'S COLLISION SETTINGS TO BLOCK THIS CHANNEL, AND YOU PAWN PHYS ASSET TO BLOCK IT TOO*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Collision")
	bool bOverrideTraceChannel = false;
	/*This will setup foliage traces to happen against this channel only.
	DON'T FORGET TO UPDATE YOUR FOLIAGE'S COLLISION SETTINGS TO BLOCK THIS CHANNEL, AND YOU PAWN PHYS ASSET TO BLOCK IT TOO*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Collision", meta = (EditCondition = "bOverrideTraceChannel"))
	TEnumAsByte<ECollisionChannel> FoliageTraceChannel;


	/** Apply force manually to the water body.  Location in world space.
	Strength Clamped beteween -10 and 10.  Percent should be between 0 and 1
	Can modify these clamps in material: UIPFForceSplatManual
	*/
	UFUNCTION(BlueprintCallable, Category = "UIPF Functions")
	virtual void FoliageForceAtLocation(float fStrength, float fSizePercent, FVector HitLocation, bool bIsImpulse);


#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void BeginDestroy() override;


private:
	/** Sim Variables*/
	int iHeightState = 0;
	float fTimeAccumulator = 0.0f;
	float fInteractivityDistance;
	float fUpdateRate = 60.0f;

	UPROPERTY()
	UTextureRenderTarget2D* rtvel;

	UPROPERTY()
	UTextureRenderTarget2D* rtpos;

	UPROPERTY()
	UTextureRenderTarget2D* rtobj;

	UPROPERTY()
	UMaterialInterface* ForceSplatMat;
	UMaterialInterface* ManForceSplatMat;
	UMaterialInterface* VelSplatMat;

	UMaterialInterface* PersistentAccumulator;


	virtual void OnConstruction(const FTransform & Transform) override;

protected:
	UPROPERTY()
	UMaterialInstanceDynamic* AccumulatorInst;
	UPROPERTY()
	UMaterialInstanceDynamic* ManualSplatInst;
	UPROPERTY()
	UMaterialInstanceDynamic* VelocitySplatInst;
	UPROPERTY()
	UCanvas* Canvas;
	UPROPERTY()
	FVector2D Size;
	UPROPERTY()
	FDrawToRenderTargetContext Context;
	UPROPERTY()
	UCanvas* CanvasInteract;
	UPROPERTY()
	FVector2D SizeInteract;
	UPROPERTY()
	FDrawToRenderTargetContext ContextInteract;
};
