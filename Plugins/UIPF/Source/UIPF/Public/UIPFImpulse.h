// // Copyright 2019 Elliot Gray. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UIPFImpulse.generated.h"

class AUIPFManager;
class USphereComponent;

UCLASS(NotPlaceable)
class UIPF_API AUIPFImpulse : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUIPFImpulse();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void StartImpulse();

	virtual void ImpulseTick();

	virtual void EndImpulse();


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UFUNCTION(BlueprintCallable, Category = "UIPF Functions")
	virtual void InitializeImpulse(float ImpulseRadius, bool UsingTruePhys, bool UsingShaderInt, float ImpulseStrength =0.9f);

protected:
	FTimerHandle ImpulseTicker;

	float fAccumulator=0.0f;
	float fCurrentSize;
	float fTargetSize;
	float fImpulseLength = 16.0;
	float fImpulseStrength;

	bool UsingPhys = true;
	bool UsingShader = true;

	UPROPERTY()
	TWeakObjectPtr<AUIPFManager> UIPFManagerect = nullptr;

private:
	UPROPERTY()
	USphereComponent* SphereComp;
	
};
