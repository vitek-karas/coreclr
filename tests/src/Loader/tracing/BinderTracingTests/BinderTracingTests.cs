// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.IO;
using System.Reflection;
using System.Runtime.Loader;
using TestLibrary;
using Xunit;

using Assert = Xunit.Assert;

namespace BinderTracingTests
{
    class BinderTracingTests
    {
        public static int Main()
        {
            return TestBase.RunTests(
                typeof(LoadTypesTest));
        }
    }
}
