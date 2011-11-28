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
// 
//  NOTE: this needs (as at 13-Jun-2000) to be compiled using
//
//  $ csc /t:library /r:System.Diagnostics.dll /r:RTS.dll MsilAsm.cs
//
//  NOTE: for Beta2 this finds System.Diagnostics in mscorlib.dll
//
//  $ csc /t:library /r:RTS.dll MsilAsm.cs
// 
#if !BETA1
  #define BETA2
#endif

using System.Diagnostics;

namespace MsilAsm {

public class MsilAsm {

    private static Process asm = null;


    public static string GetDotNetRuntimeInstallDirectory() 
    {
        // Get the path to mscorlib.dll
        string s = typeof(object).Module.FullyQualifiedName;

        // Remove the file part to get the directory
        return System.IO.Directory.GetParent(s).ToString() + "\\";
    }

    public static void Init() {
	if (asm == null) {
	    asm = new Process();
	    asm.StartInfo.FileName = GetDotNetRuntimeInstallDirectory() + "ilasm";
#if BETA1
	    asm.StartInfo.WindowStyle = ProcessWindowStyle.Minimized;
#else //BETA2
	    asm.StartInfo.CreateNoWindow = true;
	    asm.StartInfo.UseShellExecute = false;
#endif
	}
    }

    public static void Assemble(char[] fil, char[] opt, bool hasMain) {
	int retCode;
	System.String optNm;
	System.String suffx;
	System.String fName = CP_rts.mkStr(fil);
	if (hasMain) {
	    optNm ="/exe ";
	    suffx = ".exe";
	} else {
	    optNm = "/dll ";
	    suffx = ".dll";
	}
	optNm = optNm + CP_rts.mkStr(opt) + ' ';
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
	System.String fName = CP_rts.mkStr(fil);
	if (hasMain) {
	    optNm ="/exe ";
	    suffx = ".exe";
	} else {
	    optNm = "/dll ";
	    suffx = ".dll";
	}
	optNm = optNm + CP_rts.mkStr(opt) + ' ';
	if (verbose) {
	    System.Console.WriteLine("#gpcp: Calling " + asm.StartInfo.FileName);
#if BETA2
	    asm.StartInfo.CreateNoWindow = false;
#endif
	    asm.StartInfo.Arguments = optNm + "/nologo " + fName + ".il";
	} else {
#if BETA2
	    asm.StartInfo.CreateNoWindow = true;
#endif
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
