// Copyright Anupam Sahu. All Rights Reserved.

#include "TrafficAI.h"

#if UE_EDITOR
#include "ISettingsContainer.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#endif

#include "Modules/ModuleManager.h"
#include "Representation/TrRepresentationSystem.h"

bool FTrafficAIModule::SupportsDynamicReloading()
{
	return true;
}

#if UE_EDITOR

void FTrafficAIModule::StartupModule()
{
	RegisterSettings
	(
		GetMutableDefault<UTrRepresentationSystem>(),
		LOCTEXT("RuntimeWDCategoryDescription", "Configure Vehicle Representation Settings"), 
		"Representation",
		LOCTEXT("RuntimeGeneralSettingsName", "Representation Settings"),
		LOCTEXT("RuntimeGeneralSettingsDescription", "Configure settings such as LOD ranges, LOD update rate, Spawn Capacity and Delay")
	);
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
	GetMutableDefault<UTrRepresentationSystem>()->SaveConfig();
	
	return true;
}

void FTrafficAIModule::RegisterSettings
(
	const TWeakObjectPtr<>& SettingsObject,
	const FText& CategoryDescription,
	const FName& SectionName,
	const FText& DisplayName,
	const FText& Description
)
{
	// Registering some settings is just a matter of exposing the default UObject of
	// your desired class, feel free to add here all those settings you want to expose
	// to your LDs or artists.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		// Create the new category
		const ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Project");

		SettingsContainer->DescribeCategory
		(
			"TrafficAI",
			LOCTEXT("RuntimeWDCategoryName", "TrafficAI"),
			CategoryDescription
		);

		const ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings
		(
			"Project",
			"TrafficAI",
			SectionName,
			DisplayName,
			Description,
			SettingsObject
		);

		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindStatic(&FTrafficAIModule::HandleSettingsSaved);
		}
	}
}

void FTrafficAIModule::UnregisterSettings()
{
	// Ensure to unregister all of your registered settings here, hot-reload would
	// otherwise yield unexpected results.

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "TrafficAISettings", "Representation");
		SettingsModule->UnregisterSettings("Project", "TrafficAISettings", "Simulation");
	}
}

#endif

IMPLEMENT_PRIMARY_GAME_MODULE(FTrafficAIModule, TrafficAI, ProjectName)
