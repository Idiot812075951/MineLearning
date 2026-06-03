// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MineLearning : ModuleRules
{
	public MineLearning(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput","AIModule","NavigationSystem" });
	}
}
