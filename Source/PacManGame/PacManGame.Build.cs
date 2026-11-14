// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PacManGame : ModuleRules
{
	public PacManGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
	// 	PublicDependencyModuleNames.AddRange(new string[] 
	// 	{ 
	// 		"Core", "CoreUObject", "Engine", 
	// 		"InputCore", "EnhancedInput","UMG" ,
	// 		"AIModule","StateTreeModule" ,"GameplayStateTreeModule"
	// });
    //
    
        // Public: modules whose types appear in OUR public headers.
		//   UMG              -> Blueprint/UserWidget.h (PacHUDWidget.h)
		//   AIModule         -> AIController.h (PacGhostAIController.h)
		//   StateTreeModule  -> StateTreeTaskBase.h (PacGhostSetBehaviorModeTask.h)
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"UMG", "AIModule", "StateTreeModule"
		});

        // Private: only used inside .cpp files.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EnhancedInput", "GameplayStateTreeModule","Niagara"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
