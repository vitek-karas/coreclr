using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;
using TestLibrary;

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
            LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly with lower major version than what is already loaded should fail.
            Assert.Throws<FileLoadException>(() =>
                AssemblyLoadContext.Default.LoadFromAssemblyPath(Path.Join(_binaryBasePath, @"DependencyAssembly_V1\DependencyAssembly.dll")));
        }

        public void TestDirectLoadIntoDefaultALCByPath_WithHigherMajorVersion()
        {
            LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly with higher major version than what is already loaded should fail.
            Assert.Throws<FileLoadException>(() =>
                AssemblyLoadContext.Default.LoadFromAssemblyPath(Path.Join(_binaryBasePath, @"DependencyAssembly_V3\DependencyAssembly.dll")));
        }

        public void TestDirectLoadIntoDefaultALCByName_WithLowerMajorVersion()
        {
            Assembly assembly = LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly with lower major version than what is already loaded by name should work.
            Assembly secondAssembly = AssemblyLoadContext.Default.LoadFromAssemblyName(new AssemblyName("DependencyAssembly, version=1.0.0.0"));

            // The original 2.5 assembly should be returned.
            Assert.AreSame(assembly, secondAssembly);
        }

        public void TestDirectLoadIntoDefaultALCByName_WithHigherMajorVersion()
        {
            Assembly assembly = LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly with higher major version than what is already loaded by name should fail.
            Assert.Throws<FileNotFoundException>(() =>
                AssemblyLoadContext.Default.LoadFromAssemblyName(new AssemblyName("DependencyAssembly, version=3.0.0.0")));
        }

        public void TestDirectLoadIntoDefaultALCByPath_WithLowerMinorVersion()
        {
            LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly with lower minor version should still fail since LoadFromAssemblyPath will only succeed
            // if the exact same assembly has already been loaded.
            FileLoadException exception = Assert.Throws<FileLoadException>(() =>
                AssemblyLoadContext.Default.LoadFromAssemblyPath(Path.Join(_binaryBasePath, @"DependencyAssembly_V2\DependencyAssembly.dll")));
        }

        public void TestDirectLoadIntoDefaultALCByPath_WithHigherMinorVersion()
        {
            LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly with higher minor version should fail.
            Assert.Throws<FileNotFoundException>(() =>
                AssemblyLoadContext.Default.LoadFromAssemblyPath(Path.Join(_binaryBasePath, @"DependencyAssembly_V2_7\DependencyAssembly.dll")));
        }

        public void TestDirectLoadIntoDefaultALCByName_WithLowerMinorVersion()
        {
            Assembly assembly = LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly by name should work since it is of lower version than already loaded.
            Assembly secondAssembly = AssemblyLoadContext.Default.LoadFromAssemblyName(new AssemblyName("DependencyAssembly, version=2.0.0.0"));

            // The original 2.5 assembly should be returned.
            Assert.AreSame(assembly, secondAssembly);
        }

        public void TestDirectLoadIntoDefaultALCByName_WithHigherMinorVersion()
        {
            Assembly assembly = LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            // Trying to load assembly by name should work since it is of lower version than already loaded.
            Assert.Throws<FileNotFoundException>(() =>
                AssemblyLoadContext.Default.LoadFromAssemblyName(new AssemblyName("DependencyAssembly, version=2.7.0.0")));
        }

        public class EmptyAssemblyLoadContext : AssemblyLoadContext
        {
            protected override Assembly Load(AssemblyName assemblyName)
            {
                return null;
            }
        }

        public void TestResolveEventForHigherMinorVersionThanTPAOnCustomALC()
        {
            Assembly assembly = LoadIntoALCByPath(AssemblyLoadContext.Default, Path.Join(_binaryBasePath, @"DependencyAssembly_V2.5\DependencyAssembly.dll"));

            AssemblyName targetAssemblyName = new AssemblyName("DependencyAssembly, version=2.7.0.0");

            AssemblyLoadContext customALC = new EmptyAssemblyLoadContext();
            customALC.Resolving += (AssemblyLoadContext alc, AssemblyName assemblyName) =>
            {
                if (assemblyName.Name == targetAssemblyName.Name && assemblyName.Version == targetAssemblyName.Version)
                {
                    using (Stream s = new FileStream(Path.Join(_binaryBasePath, @"DependencyAssembly_V2.7\DependencyAssembly.dll"), FileMode.Open))
                    {
                        return alc.LoadFromStream(s);
                    }
                }

                return null;
            };

            Assembly targetAssembly = customALC.LoadFromAssemblyName(targetAssemblyName);
            Assert.AreEqual(targetAssemblyName.Name, targetAssembly.GetName().Name);
            Assert.AreEqual(targetAssemblyName.Version, targetAssembly.GetName().Version);
        }

        public void TestAssemblyLoadWithNullAssemblyName_ShouldFail()
        {
            Assert.Throws<ArgumentNullException>(() => Assembly.Load((AssemblyName)null));
        }

        public void TestAssemblyLoadWithEmptyAssemblyName_ShouldFail()
        {
            AssemblyName assemblyName = new AssemblyName
            {
                // Setting these explicitly to make it clear what we're testing
                Name = null,
                CodeBase = null
            };

            Assert.Throws<ArgumentException>(() => Assembly.Load(assemblyName));
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

        public static int Main()
        {
            string testBasePath = Path.GetDirectoryName(typeof(BindingFailures).Assembly.Location);

            BindingFailures runner = new BindingFailures();
            runner._binaryBasePath = Path.GetDirectoryName(testBasePath);

            runner.RunTests(runner);
            return runner._retValue;
        }

        private int _retValue = 100;
        private void RunTest(Action test, string testName = null)
        {
            testName = testName ?? test.Method.Name;

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

        private void RunTests(object testClass)
        {
            foreach (MethodInfo m in testClass.GetType()
                .GetMethods(BindingFlags.Instance | BindingFlags.Public)
                .Where(m => m.Name.StartsWith("Test") && m.GetParameters().Length == 0))
            {
                RunTest(() => m.Invoke(testClass, new object[0]), m.Name);
            }
        }
    }
}
