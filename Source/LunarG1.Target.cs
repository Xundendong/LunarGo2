using UnrealBuildTool;
using System.Collections.Generic;
public class LunarG1Target : TargetRules
{
	public LunarG1Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("LunarG1");
	}
}
