// Some copyright should be here...

using UnrealBuildTool;

public class NAI : ModuleRules
{
	public NAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		// Use this if you need to access them in public files
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"NavigationSystem",
				"PropertyEditor",
				"Projects",
				"AssetTools",
				"UnrealEd",
				//"AlembicLibrary",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		// Use this if you only need access to them in private files
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
