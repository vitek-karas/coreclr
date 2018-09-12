using System.Reflection;

#if DEPENDENCY_ASSEMBLY_V1
[assembly: AssemblyVersion("1.0.0.0")]
#elif DEPENDENCY_ASSEMBLY_V2
[assembly: AssemblyVersion("2.0.0.0")]
#elif DEPENDENCY_ASSEMBLY_V2_5
[assembly: AssemblyVersion("2.5.0.0")]
#elif DEPENDENCY_ASSEMBLY_V2_7
[assembly: AssemblyVersion("2.7.0.0")]
#elif DEPENDENCY_ASSEMBLY_V3
[assembly: AssemblyVersion("3.0.0.0")]
#endif

public class DependencyClass
{
    public string GetDescription()
    {
        Assembly assembly = GetType().Assembly;
        return $"From DependencyAssembly {assembly.GetName()} in {assembly.Location}";
    }
}
