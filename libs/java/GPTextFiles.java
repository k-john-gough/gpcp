//
// Body of GPTextFiles interface.
// This file implements the code of the GPTextFiles.cp file.
// dwc August 1999.


package CP.GPTextFiles;

import java.io.*;
import CP.CPJ.CPJ;
import CP.GPFiles.GPFiles.*;

public class GPTextFiles {


  public static GPTextFiles_FILE findLocal(char[] fileName) 
                                               throws IOException {
    String currDir = System.getProperty("user.dir");
    GPTextFiles_FILE cpf = new GPTextFiles_FILE();
    cpf.f = new File(currDir, CP.CPJ.CPJ.MkStr(fileName));
    if (!cpf.f.exists()) {
      return null;
    } else {
      cpf.r = new BufferedReader(new FileReader(cpf.f));
      return cpf;
    }
  }
                               
  public static GPTextFiles_FILE findOnPath(char[] pathName, 
                                 char[] fileName) throws IOException { 
    //
    // Use MkStr, to trim space from end of char arrray.
    //
    String pName = CP.CPJ.CPJ.MkStr(pathName);
    String fName = CP.CPJ.CPJ.MkStr(fileName);

    String nextDir;
    String thisPath = System.getProperty(pName);
    GPTextFiles_FILE cpf = new GPTextFiles_FILE();
    boolean found = false; 
    boolean pathFinished = false;
    int length = thisPath.length();
    int nextPathStart = -1, nextPathEnd = -1;

    while (!found && !pathFinished) {
      nextPathStart = nextPathEnd + 1;
      nextPathEnd = thisPath.indexOf(CP.GPFiles.GPFiles.pathSep,nextPathStart);
      if (nextPathEnd < 0)
	  nextPathEnd = length;
      nextDir = thisPath.substring(nextPathStart,nextPathEnd);
      cpf.f = new File(nextDir,fName);
      found = cpf.f.exists();
      pathFinished = nextPathEnd >= length; 
    } 
    if (found) {
      cpf.r = new BufferedReader(new FileReader(cpf.f));
      return cpf;
    } else {
      return null;
    }
  }
    

  public static char[] GetFullpathName(GPTextFiles_FILE cpf) {
    return cpf.f.getPath().toCharArray();
  }

  public static GPTextFiles_FILE openFile(char[] fileName) 
                                              throws IOException{
    GPTextFiles_FILE cpf = new GPTextFiles_FILE();
    cpf.f = new File(CP.CPJ.CPJ.MkStr(fileName));
    if (!cpf.f.exists()) {
      return null;
    } else {
      cpf.r = new BufferedReader(new FileReader(cpf.f));
      return cpf;
    }
  }

  public static GPTextFiles_FILE openFileRO(char[] fileName) 
                                              throws IOException{
    return openFile(fileName);  // always read only in java?
  }

  public static void CloseFile(GPTextFiles_FILE cpf) throws IOException {
    if (cpf.w != null) { cpf.w.flush(); cpf.w.close(); 
    } else { cpf.r.close(); }
  }

  public static GPTextFiles_FILE createFile(char[] fileName) 
  {
    try {
	GPTextFiles_FILE cpf = new GPTextFiles_FILE();
        cpf.f = new File(CP.CPJ.CPJ.MkStr(fileName));
	cpf.w = new PrintWriter(new FileWriter(cpf.f));
	return cpf;
    } catch (IOException e) {
	return null;
    }
  } 

  public static GPTextFiles_FILE createPath(char[] fileName) 
  {
    try {
        String fName = CP.CPJ.CPJ.MkStr(fileName);
        int ix = fName.lastIndexOf(File.separatorChar);
        if (ix > 0) {
          File path = new File(fName.substring(0,ix));
          if (!path.exists()) { boolean ok = path.mkdirs(); }
        }
	GPTextFiles_FILE cpf = new GPTextFiles_FILE();
        cpf.f = new File(fName);
	cpf.w = new PrintWriter(new FileWriter(cpf.f));
	return cpf;
    } catch (IOException e) {
	return null;
    }
  } 

  public static char readChar(GPTextFiles_FILE cpf) throws IOException {
    if (cpf.r.ready()) { return (char) cpf.r.read(); }
    return (char) 0;
  } 

  public static int readNChars(GPTextFiles_FILE cpf, char[] buff, 
                               int numChars) throws IOException {
    return cpf.r.read(buff,0,numChars);
  } 

  public static void WriteChar(GPTextFiles_FILE cpf,char ch) 
                                                       throws IOException { 
    cpf.w.write(ch);
  } 

  public static void WriteEOL(GPTextFiles_FILE cpf) 
                     throws IOException {
    cpf.w.write('\n');
  } 

  public static void WriteNChars(GPTextFiles_FILE cpf, char[] buff, 
                     int numChars) throws IOException {
    cpf.w.write(buff,0,numChars);
  } 

  
}
