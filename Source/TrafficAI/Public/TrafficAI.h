// Copyright Anupam Sahu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define LOCTEXT_NAMESPACE "TrafficAI"

class FTrafficAIModule : public FDefaultGameModuleImpl
{
public:
	
	virtual bool SupportsDynamicReloading() override;

#if UE_EDITOR
	virtual void StartupModule() override;

	virtual void ShutdownModule() override;

private:
	bool HandleSettingsSaved();

	void RegisterSettings();

	void UnregisterSettings();
#endif
};
