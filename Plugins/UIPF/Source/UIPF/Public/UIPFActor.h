// // Copyright 2019 Elliot Gray. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UIPFInteractor.h"
#include "UIPFActor.generated.h"


class UStaticMeshComponnt;
class USkeletalMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class USkeletalMesh;
class AUIPFManager;
UCLASS(NotPlaceable)
class UIPF_API AUIPFActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUIPFActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void SkeletonSleep(UPrimitiveComponent* Comp, FName Bone);

	UFUNCTION()
	virtual void UpdateTransition();

	float incrementer;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicInstance;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicInstanceStatic;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//UStaticMeshComponent* SMComp;

	UPROPERTY(Category = "uipf", BlueprintReadWrite, EditAnywhere)
	USkeletalMeshComponent* SkeletalMeshComp;

	UPROPERTY(Category = "uipf", BlueprintReadWrite, EditAnywhere)
	UStaticMeshComponent* BlockingCollisionOverideSMComp;
	UPROPERTY()
	UHierarchicalInstancedStaticMeshComponent* MyISM = nullptr;

	UPROPERTY()
	UStaticMesh* SM;
	UPROPERTY()
	USkeletalMesh* SkelMesh;

	bool ActivatePhys;

	UFUNCTION()
	void DelayPhysEnabled();

	UFUNCTION()
	void Init(bool bFromManual = false);

	UFUNCTION()
	void TrySleep();

	bool QueueImpulse = false;
	float QueuedForce;
	FHitResult QueuedHit;
	UPROPERTY()
	TWeakObjectPtr<AUIPFManager> UIPFManagerect;

	FUIPFType* MyType;

	float SleepThreshold = 100;
	
	FTimerHandle THS;

	FTimerHandle TransTimerHandle;

	float TransitionAlpha = 0.0f;

	int32 MyItem;

};
