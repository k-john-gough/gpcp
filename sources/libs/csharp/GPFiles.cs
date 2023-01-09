/* ------------------------------------------------------------ */
// Body of GPFiles interface.
// This file implements the code of the GPFiles.cp file.
// dwc August 1999. COOL version kjg May 2000
// kjg September 2000.  Stripped version as abstract base class.
// kjg March 2001.  Version for Beta-2 libraries.
/* ------------------------------------------------------------ */

namespace GPFiles {
public abstract class GPFiles {

  private static bool unix = (System.Environment.NewLine == "\n");
  public static char pathSep = unix ? ':' : ';';
  public static char fileSep = unix ? '/' : '\\';
  public static char optChar = unix ? '-' : '/';

  /* ----------------------------------	*/

  private static System.String mkStr(char[] arr) {
    int  ix = 0;
    char ch;
    do {
	  ch = arr[ix]; ix++;
    } while (ch != '\0');
    return new System.String(arr,0,ix-1);
  }

  private static char[] mkArr(System.String str) {
    char[] rslt = new char[str.Length + 1];
    str.CopyTo(0, rslt, 0, str.Length);
    rslt[str.Length] = '\0';
    return rslt;
  }

  /* ----------------------------------	*/

  public static bool isOlder(FILE first, FILE second) {
	int comp = System.DateTime.Compare(
		     System.IO.File.GetLastWriteTime(first.path),
		     System.IO.File.GetLastWriteTime(second.path)
		);
	return comp == -1;
    }

  public static void MakeDirectory(char[] dirName) {
	System.String path = mkStr(dirName);
        System.IO.Directory.CreateDirectory(path);
    }

  public static char[] CurrentDirectory() {
        return mkArr(System.IO.Directory.GetCurrentDirectory());
  }
  
  public static bool exists(char[] filName) {
	System.String path  = mkStr(filName);
	return System.IO.File.Exists(path);
    }

   public static char[][] FileList(char[] dirPath) {
       string dirStr = mkStr(dirPath);
       string[] files = System.IO.Directory.GetFiles(dirStr);
       if (files == null || files.Length ==0) return null;
       else {
           char[][] rslt = new char[files.Length][];
           for (int i = 0; i < files.Length; i++)
               rslt[i] = mkArr(System.IO.Path.GetFileName(files[i]));
	   return rslt;
       }
   } 
  } // end of class GPFiles

/* ------------------------------------------------------------ */

public abstract class FILE {
	public System.String path;
  }  // end of class GPFiles.FILE

/* ------------------------------------------------------------ */
}  // end of NameSpace GPFiles
/* ------------------------------------------------------------ */

