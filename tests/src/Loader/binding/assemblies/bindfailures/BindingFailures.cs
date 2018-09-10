using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;
using System.Text;
using System.Threading.Tasks;
using CoreFXTestLibrary;

namespace BindingFailures
{
    class BindingFailures
    {
        private string _binaryBasePath;

        public void TestDirectLoadIntoDefaultALCByPath()
        {
            Assembly assembly = LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            string description = GetDescriptionFromDependencyAssembly(assembly);
            Assert.IsTrue(description.Contains("DependencyAssembly, Version=2.5.0.0"), $"The loaded assembly is of the wrong version: {description}");

            // Try to load the exact same assembly again - this should work
            Assembly secondAssembly = AssemblyLoadContext.Default.LoadFromAssemblyPath(assembly.Location);
            Assert.AreSame(assembly, secondAssembly, "Second load of the exact same assembly path returned different Assembly object.");
        }

        public void TestDirectLoadIntoDefaultALCByPath_WithLowerMajorVersion()
        {
            // Make sure the assembly is loaded into default ALC (might be by previous tests).
            LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly with lower major version than what is already loaded should fail.
            Assert.Throws<FileLoadException>(() =>
                LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V1\DependencyAssembly.dll")));
        }

        public void TestDirectLoadIntoDefaultALCByPath_WithLowerMinorVersion()
        {
            // Make sure the assembly is loaded into default ALC (might be by previous tests).
            LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly with lower minor version should still fail since LoadFromAssemblyPath will only succeed
            // if the exact same assembly has already been loaded.
            FileLoadException exception = Assert.Throws<FileLoadException>(() =>
                LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2\DependencyAssembly.dll")));
            Assert.Contains(exception.Message, "foo");
        }

        private string GetDescriptionFromDependencyAssembly(Assembly assembly)
        {
            object dependencyInstance = assembly.CreateInstance("DependencyClass");
            return (string)dependencyInstance.GetType().GetMethod("GetDescription").Invoke(dependencyInstance, new object[0]);
        }

        private Assembly LoadIntoALCByPath(AssemblyLoadContext alc, string assemblyPath)
        {
            // If the exact assembly is already loaded into this ALC, return it.
            Assembly assembly = AppDomain.CurrentDomain.GetAssemblies()
                .FirstOrDefault(asm => asm.Location.Equals(assemblyPath, StringComparison.OrdinalIgnoreCase));
            if (assembly != null && AssemblyLoadContext.GetLoadContext(assembly) == alc)
            {
                return assembly;
            }

            // Otherwise try to load it
            return alc.LoadFromAssemblyPath(assemblyPath);
        }

        private int RunTests()
        {
            RunTest(TestDirectLoadIntoDefaultALCByPath);
            RunTest(TestDirectLoadIntoDefaultALCByPath_WithLowerMajorVersion);
            RunTest(TestDirectLoadIntoDefaultALCByPath_WithLowerMinorVersion);

            return _retValue;
        }

        public static int Main()
        {
            string testBasePath = Path.GetDirectoryName(typeof(BindingFailures).Assembly.Location);

            BindingFailures runner = new BindingFailures();
            runner._binaryBasePath = Path.GetDirectoryName(testBasePath);

            return runner.RunTests();
        }

        private int _retValue = 100;
        private void RunTest(Action test)
        {
            string testName = test.Method.Name;

            try
            {
                Console.WriteLine($"{testName} Start");
                test();
                Console.WriteLine($"{testName} PASSED.");
            }
            catch (Exception exe)
            {
                Console.WriteLine($"{testName} FAILED:");
                Console.WriteLine(exe.ToString());
                _retValue = -1;
            }
        }
    }
}
