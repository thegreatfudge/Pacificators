// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RunFromCamera : ModuleRules
{
	public RunFromCamera(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
