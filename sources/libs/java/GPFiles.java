//
// Body of GPFiles interface.
// This file implements the code of the GPFiles.cp file.
// dwc August 1999.


package CP.GPFiles;

import java.io.*;

public class GPFiles {

  public static char pathSep = System.getProperty("path.separator").charAt(0);
  public static char fileSep = System.getProperty("file.separator").charAt(0);
  public static char optChar = '-';

  public static boolean isOlder(GPFiles_FILE first, GPFiles_FILE second) {
    return (first.f.lastModified() < second.f.lastModified());
  }

  public static void MakeDirectory(char[] dirName) {
    File path = new File(CP.CPJ.CPJ.MkStr(dirName));
    if (!path.exists()) {
      boolean ok = path.mkdirs();
    }    
  }

  public static char[] CurrentDirectory() {
    String curDir = System.getProperty("user.dir");
    return curDir.toCharArray();
  }
  
  public static boolean exists(char[] dirName) {
    File path = new File(CP.CPJ.CPJ.MkStr(dirName));
    return path.exists();
  }

  public static char[][] FileList(char[] dirPath) throws IOException {
    File theDir = new File(CP.CPJ.CPJ.MkStr(dirPath));
    String[] files = theDir.list();
    if (files == null || files.length == 0 ) return null;
    else {
      char[][] rslt = new char[files.length][];
      for (int i = 0; i < files.length; i++)
        rslt[i] = CP.CPJrts.CPJrts.JavaStrToChrOpen(files[i]);
      return rslt;
    }
  }
}
