using UnrealBuildTool;
using System.Collections.Generic;
public class LunarG1EditorTarget : TargetRules
{
	public LunarG1EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		bOverrideBuildEnvironment = true;
		ExtraModuleNames.Add("LunarG1");
	}
}
