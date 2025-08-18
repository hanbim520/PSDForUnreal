// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;

public class PSDForUnreal : ModuleRules
{
	public PSDForUnreal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				Path.Combine(ModuleDirectory, "../ThirdParty", "Includes"),
                Path.Combine(ModuleDirectory, "../ThirdParty", "Includes","Psd")
            }
			);

        foreach (var path in PublicIncludePaths)
        {
            Console.WriteLine(path);
        }
        PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"EditorFramework",
				"EditorStyle",
				"UnrealEd",
				"UMG",
                "EditorScriptingUtilities",
                "UMGEditor",
                "LevelEditor",
                "AssetRegistry",
                "InteractiveToolsFramework",
				"EditorInteractiveToolsFramework",
                "ImageWrapper"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // Add the import library for Windows (replace 'YourWindowsLibName.lib' with your actual .lib file)
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "../ThirdParty", "Lib", "Win64",  "Debug", "Psd_MTd.lib"));

            // If you have a .dll, you need to ensure it's copied to the output directory
            // RuntimeDependencies.Add("$(ModuleDir)/ThirdParty/Win64/bin/YourWindowsRuntime.dll");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            // Add the static library for macOS (replace 'libYourMacLibName.a' with your actual .a file)
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "../ThirdParty", "Lib", "Mac", "Debug", "Psd_MTd.a"));

            // For dynamic libraries (.dylib), you might need to handle rpath settings
            // PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "ThirdParty", "Mac", "lib", "libYourMacLibName.dylib"));
            // RuntimeDependencies.Add("$(ModuleDir)/ThirdParty/Mac/lib/libYourMacLibName.dylib");
        }
    }
}
