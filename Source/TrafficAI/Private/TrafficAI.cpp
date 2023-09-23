// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficAI.h"

#if UE_EDITOR
#include "ISettingsContainer.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif

#include "Modules/ModuleManager.h"
#include "TrafficRepresentationSystem/TrafficAIRepresentationSystem.h"

bool FTrafficAIModule::SupportsDynamicReloading()
{
	return true;
}

#if UE_EDITOR

void FTrafficAIModule::StartupModule()
{
	RegisterSettings();
}

void FTrafficAIModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		UnregisterSettings();
	}
}

bool FTrafficAIModule::HandleSettingsSaved()
{
	UTrafficAIRepresentationSystem* Settings = GetMutableDefault<UTrafficAIRepresentationSystem>();
	bool ResaveSettings = false;

	// You can put any validation code in here and resave the settings in case an invalid
	// value has been entered

	if (ResaveSettings)
	{
		Settings->SaveConfig();
	}

	return true;
}

void FTrafficAIModule::RegisterSettings()
{
	// Registering some settings is just a matter of exposing the default UObject of
	// your desired class, feel free to add here all those settings you want to expose
	// to your LDs or artists.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		// Create the new category
		ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Project");

		SettingsContainer->DescribeCategory("TrafficAI",
		                                    LOCTEXT("RuntimeWDCategoryName", "TrafficAI"),
		                                    LOCTEXT("RuntimeWDCategoryDescription", "Global settings for Traffic AI"));

		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Project", "TrafficAI", "General",
		                                                                       LOCTEXT("RuntimeGeneralSettingsName", "General"),
		                                                                       LOCTEXT("RuntimeGeneralSettingsDescription", "Base configuration for Traffic AI"),
		                                                                       GetMutableDefault<UTrafficAIRepresentationSystem>()
		);

		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FTrafficAIModule::HandleSettingsSaved);
		}
	}
}

void FTrafficAIModule::UnregisterSettings()
{
	// Ensure to unregister all of your registered settings here, hot-reload would
	// otherwise yield unexpected results.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "TrafficAISettings", "General");
	}
}

#endif

IMPLEMENT_PRIMARY_GAME_MODULE(FTrafficAIModule, TrafficAI, "TrafficAI")
