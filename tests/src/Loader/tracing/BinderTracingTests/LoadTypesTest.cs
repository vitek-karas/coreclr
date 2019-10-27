// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using TestLibrary;
using Xunit;

using Assert = Xunit.Assert;

namespace BinderTracingTests
{
    class LoadTypesTest : TestBase
    {
        private AssemblyName _assemblyToLoadName;
        private Type _typeToLoad;
        private Type _genericWithTypeToLoad;
        private string _typeToLoadFullyQualifiedName;
        private string _assemblyToLoadV1Path;
        private byte[] _assemblyToLoadRaw;

        protected override void Initialize()
        {
            _assemblyToLoadName = new AssemblyName("AssemblyToLoad");
            _typeToLoad = typeof(AssemblyToLoadNamespace.TestTypeToLoad);
            _genericWithTypeToLoad = typeof(System.Collections.Generic.List<AssemblyToLoadNamespace.TestTypeToLoad>);
            _typeToLoadFullyQualifiedName = _typeToLoad.AssemblyQualifiedName;
            _assemblyToLoadV1Path = _typeToLoad.Assembly.Location;
            
            using (FileStream s = new FileStream(_assemblyToLoadV1Path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
            {
                _assemblyToLoadRaw = new byte[s.Length];
                s.Read(_assemblyToLoadRaw, 0, (int)s.Length);
            }
        }

        public void Test_Assembly_Load_Name()
        {
            Assembly.Load(_assemblyToLoadName);
        }

        public void Test_Assembly_Load_Memory()
        {
            Assembly.Load(_assemblyToLoadRaw);
        }

        public void Test_Assembly_LoadFrom()
        {
            Assembly.LoadFrom(_assemblyToLoadV1Path);
        }

        public void Test_Assembly_LoadFile()
        {
            Assembly.LoadFile(_assemblyToLoadV1Path);
        }

        public void Test_Assembly_ALCLoadFromAssemblyName()
        {
            AssemblyLoadContext.Default.LoadFromAssemblyName(new AssemblyName("AssemblyToLoad"));
        }

        public void Test_Assembly_ALCLoadFromAssemblyPath()
        {
            AssemblyLoadContext.Default.LoadFromAssemblyPath(_assemblyToLoadV1Path);
        }

        public void Test_Assembly_ALCLoadFromStream()
        {
            AssemblyLoadContext.Default.LoadFromStream(new MemoryStream(_assemblyToLoadRaw));
        }

        public void Test_Type_GetType()
        {
            Type.GetType(_typeToLoadFullyQualifiedName);
        }

        public void Test_Type_GetType_Generic()
        {
            Type.GetType(_genericWithTypeToLoad.FullName);
        }

        public void Test_Activator_CreateInstance()
        {
            Activator.CreateInstance(_assemblyToLoadName.FullName, _typeToLoad.FullName);
        }

        public void Test_Assembly_GetType()
        {
            _typeToLoad.Assembly.GetType(_typeToLoad.FullName);
        }

        public void Test_Assembly_GetType_Generic()
        {
            _genericWithTypeToLoad.Assembly.GetType(_genericWithTypeToLoad.FullName);
        }
    }
}
