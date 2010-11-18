/* ------------------------------------------------------------ */
// Body of GPFiles interface.
// This file implements the code of the GPFiles.cp file.
// dwc August 1999. COOL version kjg May 2000
// kjg September 2000.  Stripped version as abstract base class.
// kjg March 2001.  Version for Beta-2 libraries.
/* ------------------------------------------------------------ */

namespace GPFiles {
public abstract class GPFiles {

  public static char pathSep = ';';
  public static char fileSep = '\\';
  public static char optChar = '/';

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
        return System.IO.Directory.GetCurrentDirectory().ToCharArray();
  }
  
  public static bool exists(char[] filName) {
	System.String path  = mkStr(filName);
	return System.IO.File.Exists(path);
    }

  } // end of class GPFiles

/* ------------------------------------------------------------ */

public abstract class FILE {
	public System.String path;
  }  // end of class GPFiles.FILE

/* ------------------------------------------------------------ */
}  // end of NameSpace GPFiles
/* ------------------------------------------------------------ */

