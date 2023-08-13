// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define LOCTEXT_NAMESPACE "TrafficAI"

class FTrafficAIModule : public FDefaultGameModuleImpl
{
public:
	
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

	virtual bool SupportsDynamicReloading() override;

private:
	
	bool HandleSettingsSaved();

	void RegisterSettings();

	void UnregisterSettings();
};
