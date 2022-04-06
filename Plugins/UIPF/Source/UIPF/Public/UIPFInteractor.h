// Copyright 2019 Elliot Gray. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/DataTable.h"
#include "WorldCollision.h"
#include "UIPFInteractor.generated.h"

class AUIPFManager;
class AUIPFActor;
USTRUCT(BlueprintType)
struct FUIPFType : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIPF")
	UStaticMesh* StaticMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIPF")
	USkeletalMesh* SkeletalMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIPF")
	bool CollideWithWorldStatic = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIPF")
	bool CollideWithWorldDynamic = false;
	/*Not recommended, causes popping if any plants are intersecting on switch**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIPF")
	bool CollideWithPhysicsFoliage = false;
	/*Enable this if you want kinematic bones in the physics asset to block Pawns.  ie, enable if you want a tree trunk to stop a pawn running through it**/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIPF")
	bool KinematicBlocksPawn = false;
	/*If assigned, this mesh will be used to block all collision covering an area, and set to invisible.  Ideal for if you want to manually add hard collision to a tree trunk, for example*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIPF", AdvancedDisplay)
	UStaticMesh* StaticCollisionOverride = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UIPF", AdvancedDisplay)
	bool bDebugMakeColliderVisible = false;


};
UCLASS()
class UIPF_API UIPFTypes : public UDataTable
{
	GENERATED_BODY()

};

class USphereComponent;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), hideCategories = (Rendering, Tags, ComponentReplication, Activation, Cooking, Events, Physics, LOD, Collision))
class UIPF_API UUIPFInteractor : public UPrimitiveComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUIPFInteractor();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/**Does the sweep to check for any instanced static meshes we need to activate**/
	virtual void SweepActivation();

	/**Does the sweep to check for any instanced static meshes we need to activate**/
	virtual void AsyncSweep();

	/**Takes a hit result that we think is an ism, and if it is, it will switch it for the correct physicalised version of the asset**/
	virtual AUIPFActor * SwapISMforPhys(FHitResult hit, bool bFromManual = false);


	/**Checks to see if there's a skeletal mesh paired with the input static mesh and if there is returns it**/
	virtual void InteractWithGrass(float InteractRadius);


	FTraceHandle RequestAsyncTrace();
	void OnTraceCompleted(const FTraceHandle& Handle, FTraceDatum& Data);
	void DoWorkWithTraceResults(const FTraceDatum&TraceData);

	/**Checks to see if there's a skeletal mesh paired with the input static mesh and if there is returns it**/
	virtual FUIPFType* FindFoliageType(UStaticMesh* StaticIn);

	bool UsingMassiveGrass;

	bool UsingTruePhysics;

	FVector LastPhysCheckLocation;

	FVector LastUpdateLocation;

	FVector CalculatedVelocity;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Bound to activation volume end overlap*/
	UFUNCTION()
	virtual void OnDeactivationEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/**try to manually activate foliage with optional initial impulse*/
	UFUNCTION(BlueprintCallable, Category = "UIPF")
	virtual void ManuallyActivateFoliage(bool WithImpulse, float ImpulseStrength, FHitResult hit);

public:	
	/** Affects shader interaction update rate. Enable this to manually set tick rate.  Seems to have a negligible performance effect but might be useful of low spec hardware.  Won't tick is shader interaction disabled.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", meta = (EditCondition = "bIsInteractive"))
	bool bLimitTickRate = false;

	/** Affects shader interaction update rate. Enable this to manually set tick rate.  Seems to have a negligible performance effect but might be useful of low spec hardware.  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", meta = (EditCondition = "bLimitTickRate"))
	float TickRate = 60;

	/** The rate at which foliage swap checks will be carried out.  If you have a large interaction radius set on the manager you can set this to be quite low.  If you're using a small radius you'll want it to be higher to minimize
	the chance that foliage will transition while already colliding with the player causing artifacting.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	float PhysFoliageSwapTickRate = 10;

	/**If enabled this actor will interact with grass**/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	bool bInteractWithGrass = true;

	/**Grass interaction radius in world units**/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	float fGrassInteractionDiameter = 85;

	/**This is the velocity below which UIPF will consider this interactor as Stationary.**/
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", AdvancedDisplay)
	float VelocityDeadZone = 300.0f;

	/** if false the DefaultInteractorActivationDistance on the UIPF manager will be used*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	bool bOverrideInteractorActivationDistance = false;

	/** override value*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", meta = (EditCondition = "bOverrideInteractorActivationDistance"))
	float InteractorActivationDistance = 20000;
	
	/*Enable this if you have attached this component to something that won't have velocity, ie, a pawn without a move component or physics, or an animated mesh*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction")
	bool bPerCompVelocity = false;

	/*Not Recommended.  By default interaction scales with velocity, but incase you don't want this functionaly you can enable this and manually specify the strength*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", AdvancedDisplay)
	bool bForceFixedInteractionStrength = false;
	/** override value*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", meta = (EditCondition = "bForceFixedInteractionStrength"), AdvancedDisplay)
	float FixedStrength = 600;
	
	float DefaultIntActiveDist = 0.0f;
	
	/**Enable this to allow extra actors to be responsible for switching foliage out for physically simulated variants.  Highly not recommended.  By default true physics foliage will only be enabled near the player pawn**/
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", AdvancedDisplay)
	//bool bForceEnablePhys = false;

	/**Forced activation radius.  Player radius set on UIPFManager**/
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", meta = (EditCondition = "bForceEnablePhys"), AdvancedDisplay)
	//float ActivationRadius = 200;

	/**Forced deactivation radius.  Player radius set on UIPFManager**/
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UIPF Interaction", meta = (EditCondition = "bForceEnablePhys"), AdvancedDisplay)
	//float DeactivationRadius = 1500;

	float PlayerActivationRadius;
	float PlayerDeactivationRadius;

protected:
	//UPROPERTY(BlueprintReadWrite, Category = "UIPF Interaction", AdvancedDisplay)
	UPROPERTY()
	USphereComponent* FoliageDeactivationVolume;

	FCollisionShape ActivationShape;
	UPROPERTY()
	UDataTable* UIPFDT;

	TArray<FUIPFType*> UIPFTypesArray;


	FTraceHandle LastTraceHandle;
	FTraceDelegate TraceDelegate;
	uint32 bWantsTrace:1;

	UPROPERTY()
	TWeakObjectPtr<AUIPFManager> UIPFManagerect = nullptr;

	FTimerHandle PhysTimerHandle;

	UFUNCTION(Category = "UIPF")
	virtual void PhysCheck();
};
