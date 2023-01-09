
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace PeToCpsUtils
{
    // This has to be wrapped, as the CLR v2 CPS file has it as an 
    // instance method. Since 4.0 it is virtual and MUST be called with callvirt
    //
	public class Utils {
		public static AssemblyName[] GetDependencies(Assembly asm) {
            return asm.GetReferencedAssemblies();
		}

        public static String typName(System.Type typ) {
            return typ.Name;
        }
	}
}


