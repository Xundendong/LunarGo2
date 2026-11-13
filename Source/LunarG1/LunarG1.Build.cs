using UnrealBuildTool;
using System.IO;
public class LunarG1 : ModuleRules {
    public LunarG1(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent" });
        string ThirdParty = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty"));
        foreach (string Library in new [] { "mujoco", "onnxruntime" }) {
            PublicIncludePaths.Add(Path.Combine(ThirdParty, Library, "include"));
            string Lib = Path.Combine(ThirdParty, Library, "lib", "lib" + Library + ".so");
            PublicAdditionalLibraries.Add(Lib);
            foreach (string File in Directory.GetFiles(Path.GetDirectoryName(Lib), "*.so*"))
                RuntimeDependencies.Add("$(TargetOutputDir)/" + Path.GetFileName(File), File);
        }
    }
}
