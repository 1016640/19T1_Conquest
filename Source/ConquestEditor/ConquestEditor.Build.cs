// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class ConquestEditor : ModuleRules
{
	public ConquestEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(new string[]
        {
            "ConquestEditor/Public"
        });

        PrivateIncludePaths.AddRange(new string[]
        {
            "ConquestEditor/Private"
        });

		PublicDependencyModuleNames.AddRange(new string[] 
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

		PrivateDependencyModuleNames.AddRange(new string[] 
        {
            "Conquest",
            "EditorStyle",
            "InputCore",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "UnrealED"
        });
	}
}
