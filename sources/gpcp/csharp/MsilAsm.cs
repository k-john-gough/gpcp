// (* ========================================================= *)
// (**	Interface to the ILASM Byte-code assembler.		*)
// (*	K John Gough, 10th June 1999				*)
// (*	Modifications:						*)
// (*		Version for GPCP V0.3 April 2000 (kjg)		*)
// (* ========================================================= *)
// (*	The real code is in MsilAsm.cs				*)	
// (* ========================================================= *)
//
//MODULE MsilAsm;
//
//  PROCEDURE Init*(); BEGIN END Init;
//
//  PROCEDURE Assemble*(IN fil, opt : ARRAY OF CHAR; main : BOOLEAN); 
//  BEGIN END Assemble;
//
//END MsilAsm.
//
// Compile with :
//
//  $ csc /t:library /debug MsilAsm.cs
// 

using System;
using System.Text;
using System.Diagnostics;

namespace MsilAsm {

public class MsilAsm {

    private static Process asm = null;


    public static string GetDotNetRuntimeInstallDirectory() 
    {
        // Get the path to mscorlib.dll
        string s = typeof(object).Module.FullyQualifiedName;
        // Remove the file part to get the directory
        return System.IO.Directory.GetParent(s).ToString();
    }

        public static void Init() {
            if (asm == null) {
                asm = new Process();
                System.String frameworkDir = GetDotNetRuntimeInstallDirectory();
                //System.String frameworkDir = Environment.GetEnvironmentVariable("NET40", EnvironmentVariableTarget.User);
                asm.StartInfo.FileName = frameworkDir
                  + (frameworkDir[0] == '/' ?
                        "/../../../Commands/" : "\\") + "ilasm";

                asm.StartInfo.CreateNoWindow = true;
                asm.StartInfo.UseShellExecute = false;
            }
        }

        private static int CPlen(char[] arr) {
            int len = arr.Length;
            for (int ix = 0; ix < len; ix++)
                if (arr[ix] == '\0')
                    return ix;
            return len;
        }

        public static void Assemble(char[] fil, char[] opt, bool hasMain) {
            int retCode;
            System.String optNm;
            System.String suffx;
            System.String fName = new String(fil, 0, CPlen(fil));
            if (hasMain) {
                optNm = "/exe ";
                suffx = ".exe";
            }
            else {
                optNm = "/dll ";
                suffx = ".dll";
            }
            optNm = optNm + new String(opt, 0, CPlen(opt)) + ' ';
            asm.StartInfo.Arguments = optNm + "/nologo /quiet " + fName + ".il";
            asm.Start();
            asm.WaitForExit();
            retCode = asm.ExitCode;
            if (retCode != 0)
                System.Console.WriteLine("#gpcp: ilasm FAILED " + retCode);
            else
                System.Console.WriteLine("#gpcp: created " + fName + suffx);
        }

        public static void DoAsm(char[] fil, char[] opt,
                    bool hasMain,
                    bool verbose,
                    ref int rslt) {
            System.String optNm;
            System.String suffx;
            System.String fName = new String(fil, 0, CPlen(fil));

            if (hasMain) {
                optNm = "/exe ";
                suffx = ".exe";
            }
            else {
                optNm = "/dll ";
                suffx = ".dll";
            }
            optNm = optNm + new String(opt, 0, CPlen(opt)) + ' ';
            if (verbose) {
                asm.StartInfo.Arguments = optNm + "/nologo " + fName + ".il";
                System.Console.WriteLine("#gpcp: Calling " + asm.StartInfo.FileName + ' ' + asm.StartInfo.Arguments);
            }
            else {
                asm.StartInfo.Arguments = optNm + "/nologo /quiet " + fName + ".il";
            }
            asm.Start();
            asm.WaitForExit();
            rslt = asm.ExitCode;
            if (rslt == 0)
                System.Console.WriteLine("#gpcp: Created " + fName + suffx);
        }


        public static void Assemble(char[] fil, bool hasMain) {
        char[] opt = {'/', 'd', 'e', 'b', 'u', 'g', '\0' };
        Assemble(fil, opt, hasMain);
    }

  }
}
