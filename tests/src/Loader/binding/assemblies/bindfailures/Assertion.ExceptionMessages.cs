using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CoreFXTestLibrary
{
    public static partial class Assert
    {
        public static void Contains(string value, string substring, string message = "")
        {
            Assert.IsTrue(value.Contains(substring), $"Value '{value}' is expected to contain substring '{substring}'. {message}");
        }
    }
}
