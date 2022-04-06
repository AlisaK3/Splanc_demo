// Copyright 2019 Elliot Gray. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

//DECLARE_STATS_GROUP(TEXT("UIPF"), STATGROUP_UIPF, STATCAT_Advanced);

class FUIPFModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
