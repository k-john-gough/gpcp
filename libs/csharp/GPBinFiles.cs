//
// Body of GPBinFiles interface.
// This file implements the code of the GPBinFiles.cp file.
// dwc August 1999. COOL version kjg May 2000
// kjg September 2000. Renamed from GPFiles to GPBinFiles.
// kjg March 2001.  Version for Beta-2 libraries.
// 
// Compile with: csc /t:library /r:GPFiles.dll GPBinFiles.cs
//
/* ------------------------------------------------------------ */
using System;
using GPFiles;

namespace GPBinFiles {
public class GPBinFiles {

  /* ----------------------------------	*/

  private static System.String mkStr(char[] arr) {
    int  ix = 0;
    char ch;
    do {
	ch = arr[ix]; ix++;
    } while (ch != '\0');
    return new System.String(arr,0,ix-1);
  }

  /* ----------------------------------	*/

  public static int length(FILE cpf) {
    return (int) cpf.bufS.Length;
  }

  /* ----------------------------------	*/

  private static FILE open(System.String name) {
  // Opens buffered filestream for reading.
    try {
	FILE cpf = new FILE();
	cpf.path = name;
	System.IO.FileStream fStr = 
			System.IO.File.Open(name, System.IO.FileMode.Open);
	cpf.bufS = new System.IO.BufferedStream(fStr);
	return cpf;
    } catch {
	return null;
    }
  }

  private static FILE openRead(System.String name) {
  // Opens buffered filestream for reading.
    try {
	FILE cpf = new FILE();
	cpf.path = name;
	System.IO.FileStream fStr = System.IO.File.OpenRead(name);
	cpf.bufS = new System.IO.BufferedStream(fStr);
	return cpf;
    } catch {
	return null;
    }
  }

 /* =========================== */
                               
  /* ----------------------------------	*/

  public static FILE findLocal(char[] fileName) 
  {
    System.String name = mkStr(fileName);
    return open(name);
  }
                               
  /* ----------------------------------	*/

  public static FILE findOnPath(
			char[] pathName, 
			char[] fileName) 
  { 
    //
    // Use mkStr, to trim space from end of char arrray.
    //
    System.String pName = mkStr(pathName);
    System.String fName = mkStr(fileName);
    System.String nName = "";

    System.String nextDir;
    System.String thisPath = System.Environment.GetEnvironmentVariable(pName);
//
//  System.Console.WriteLine(pName);
//  System.Console.WriteLine(thisPath);
//
    FILE cpf = new FILE();
    bool found = false; 
    bool pathFinished = false;
    int length = thisPath.Length;
    int nextLength;
    int nextPathStart;
    int nextPathEnd   = -1;

    while (!found && !pathFinished) {
      nextPathStart = nextPathEnd + 1;
      nextPathEnd   = thisPath.IndexOf(GPFiles.GPFiles.pathSep, nextPathStart);
      if (nextPathEnd < 0)
	  nextPathEnd = length;
      nextLength    = nextPathEnd - nextPathStart;
      nextDir = thisPath.Substring(nextPathStart, nextLength);
      nName = nextDir + GPFiles.GPFiles.fileSep + fName;
      found = System.IO.File.Exists(nName);
      pathFinished = nextPathEnd >= length; 
    } 
    if (found) {
        return openRead(nName);
    } else
        return null;
  }
   
  /* ----------------------------------	*/

  public static char[] getFullPathName(FILE cpf) {
    return cpf.path.ToCharArray(); // not really correct!
  }

  /* ----------------------------------	*/

  public static FILE openFile(char[] fileName) {
    System.String name = mkStr(fileName);
    return open(name);
  }

  public static FILE openFileRO(char[] fileName) {
    System.String name = mkStr(fileName);
    return openRead(name);
  }
                               
  /* ----------------------------------	*/

  public static void CloseFile(FILE cpf) {
    if (cpf.bufS != null)
	cpf.bufS.Close();
  }

  /* ----------------------------------	*/

  public static FILE createFile(char[] arr) {
    FILE cpf = new FILE();
    try {
	System.String name = mkStr(arr);
	System.IO.FileStream fStr = System.IO.File.Create(name);
      cpf.path = name;
	cpf.bufS = new System.IO.BufferedStream(fStr);
	return cpf;
    } catch {
	return null;
    }
  } 

  /* ----------------------------------	*/

  public static FILE createPath(char[] fileName)
  {
    System.String fName = mkStr(fileName);
    try {
	int ix = fName.LastIndexOf(GPFiles.GPFiles.fileSep);
	if (ix > 0) {
        System.String path = fName.Substring(0,ix);
        // Check if exists first?
	  if (!System.IO.Directory.Exists(path)) {
	    System.IO.DirectoryInfo junk = System.IO.Directory.CreateDirectory(path);
        }
	}
	FILE cpf = new FILE();
      cpf.path = fName;
	System.IO.FileStream fStr = System.IO.File.Create(fName);
	cpf.bufS = new System.IO.BufferedStream(fStr);
	return cpf;
    } catch {
	return null;
    }
  }

  /* ----------------------------------	*/

  public static bool EOF(FILE cpf) {
    return cpf.bufS.Position >= cpf.bufS.Length;
  }

  /* ----------------------------------	*/

  public static int readByte(FILE cpf) {
    if (cpf.bufS != null)
	return (int) cpf.bufS.ReadByte();
    else
	throw new System.Exception("File not open for reading");
  } 

  /* ----------------------------------	*/

  public static int  readNBytes(FILE cpf, 
				byte[] buff, 
				int    want)
  {
    if (cpf.bufS != null)
	return cpf.bufS.Read(buff, 0, want);
    else
	throw new System.Exception("File not open for reading");
  } 

  /* ----------------------------------	*/

  public static void WriteByte(FILE cpf, int b) {
    if (cpf.bufS != null)
	cpf.bufS.WriteByte((byte) b);
    else
	throw new System.Exception("File not open for reading");
  } 

  /* ----------------------------------	*/

  public static void WriteNBytes(FILE cpf,
				 byte[] buff,
				 int    want)
  {
    if (cpf.bufS != null)
	cpf.bufS.Write(buff, 0, want);
    else
	throw new System.Exception("File not open for reading");
  } 
}	// end of class GPBinFiles

/* ------------------------------------------------------------ */
/*		File-descriptor for GPBinFiles			*/
/* ------------------------------------------------------------ */

  public class FILE : GPFiles.FILE
  {
     public System.IO.BufferedStream bufS;
  }	// end of class FILE

/* ------------------------------------------------------------ */
} // end of GPBinFiles.
/* ------------------------------------------------------------ */

