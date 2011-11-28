//
// Body of GPFiles interface.
// This file implements the code of the GPFiles.cp file.
// dwc August 1999.


package CP.GPBinFiles;

import java.io.*;
import CP.CPJ.CPJ;
import CP.GPFiles.GPFiles.*;

public class GPBinFiles {

  public static int length(GPBinFiles_FILE cpf) {
    return (int) cpf.length;
  }

  public static GPBinFiles_FILE findLocal(char[] fileName) 
                                                  throws IOException {
    String currDir = System.getProperty("user.dir");
    GPBinFiles_FILE cpf = new GPBinFiles_FILE();
    cpf.f = new File(currDir, CP.CPJ.CPJ.MkStr(fileName));
    if (!cpf.f.exists()) {
      return null; 
    } else {
      cpf.rf = new RandomAccessFile(cpf.f,"r");
      cpf.length = cpf.rf.length();
      return cpf;
    }
  }
                               
  public static GPBinFiles_FILE findOnPath(char[] pathName, 
                                   char[] fileName) throws IOException { 
    //
    // Use MkStr, to trim space from end of char arrray.
    //
    String pName = CP.CPJ.CPJ.MkStr(pathName);
    String fName = CP.CPJ.CPJ.MkStr(fileName);

    String nextDir;
    String thisPath = System.getProperty(pName);
    GPBinFiles_FILE cpf = new GPBinFiles_FILE();
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
      cpf.rf = new RandomAccessFile(cpf.f,"r");
      cpf.length = cpf.rf.length();
      return cpf;
    } else {
      return null;
    }
  }
    
  public static char[] getFullPathName(GPBinFiles_FILE cpf) {
    return cpf.f.getPath().toCharArray();
  }

  public static GPBinFiles_FILE openFile(char[] fileName)throws IOException{
    GPBinFiles_FILE cpf = new GPBinFiles_FILE();
    cpf.f = new File(CP.CPJ.CPJ.MkStr(fileName));
    if (!cpf.f.exists()) {
      return null;
    } else {
      cpf.rf = new RandomAccessFile(cpf.f,"rw");
      cpf.length = cpf.rf.length();
      return cpf;
    }
  }

  public static GPBinFiles_FILE openFileRO(char[] fileName)throws IOException{
    GPBinFiles_FILE cpf = new GPBinFiles_FILE();
    cpf.f = new File(CP.CPJ.CPJ.MkStr(fileName));
    if (!cpf.f.exists()) {
      return null;
    } else {
      cpf.rf = new RandomAccessFile(cpf.f,"r");
      cpf.length = cpf.rf.length();
      return cpf;
    }
  }

  public static void CloseFile(GPBinFiles_FILE cpf) throws IOException {
    cpf.rf.close(); 
  }

  public static GPBinFiles_FILE createFile(char[] fileName)throws IOException {
    GPBinFiles_FILE cpf = new GPBinFiles_FILE();
    cpf.f = new File(CP.CPJ.CPJ.MkStr(fileName));
    cpf.rf = new RandomAccessFile(cpf.f,"rw");
    cpf.rf.setLength(0);
    cpf.length = 0;
    //  cpf.length = cpf.rf.length();
    return cpf;
  } 

  public static GPBinFiles_FILE createPath(char[] fileName)throws IOException {
    String fName = CP.CPJ.CPJ.MkStr(fileName);
    int ix = fName.lastIndexOf(File.separatorChar);
    if (ix > 0) {
      File path = new File(fName.substring(0,ix));
      if (!path.exists()) { boolean ok = path.mkdirs(); }
    } 
    GPBinFiles_FILE cpf = new GPBinFiles_FILE();
    cpf.f = new File(fName);
    cpf.rf = new RandomAccessFile(cpf.f,"rw");
    cpf.rf.setLength(0);
    cpf.length = 0;
    //    cpf.length = cpf.rf.length();
    return cpf;
  } 

  public static boolean EOF(GPBinFiles_FILE cpf) throws IOException {
    return cpf.rf.getFilePointer() >= cpf.length;
  }

  public static int readByte(GPBinFiles_FILE cpf) throws IOException {
    return cpf.rf.readUnsignedByte();
  } 

  public static int readNBytes(GPBinFiles_FILE cpf, byte[] buff, 
                               int numBytes) throws IOException {
    return cpf.rf.read(buff,0,numBytes);
  } 

  public static void WriteByte(GPBinFiles_FILE cpf,int b) throws IOException{
    cpf.rf.write(b);
  } 

  public static void WriteNBytes(GPBinFiles_FILE cpf,byte[] buff,
                                 int numBytes) throws IOException {
    cpf.rf.write(buff,0,numBytes);
  } 

  
}
