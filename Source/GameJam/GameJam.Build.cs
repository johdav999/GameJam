// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameJam : ModuleRules
{
	public GameJam(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GameJam",
			"GameJam/Variant_Platforming",
			"GameJam/Variant_Platforming/Animation",
			"GameJam/Variant_Combat",
			"GameJam/Variant_Combat/AI",
			"GameJam/Variant_Combat/Animation",
			"GameJam/Variant_Combat/Gameplay",
			"GameJam/Variant_Combat/Interfaces",
			"GameJam/Variant_Combat/UI",
			"GameJam/Variant_SideScrolling",
			"GameJam/Variant_SideScrolling/AI",
			"GameJam/Variant_SideScrolling/Gameplay",
			"GameJam/Variant_SideScrolling/Interfaces",
			"GameJam/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
