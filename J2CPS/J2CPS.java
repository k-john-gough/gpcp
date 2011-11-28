/**********************************************************************/
/*                      Main class for J2CPS                          */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.util.*;
import java.io.*;

public class J2CPS {

  /**
   * Main program. Takes a package name as a parameter, produces the 
   * component pascal symbol file.
   */
  public static void main(String args[]) { 
    int argLen = args.length;
    boolean anonPack = false;
    J2CPSFiles.GetPaths();
    String filename = null;
    TypeDesc.InitTypes();
    if (argLen == 0) {
      System.err.println("J2CPS version 1.3.12 (Nov. 2011)");
      System.err.println("Usage: java J2CPS [options] packagename");
      System.err.println("Options may be in any order.");
      System.err.println("  -d dir  symbol file directory");
      System.err.println("  -u      use unique names");
      System.err.println("  -v      verbose diagnostic messages");
      System.exit(0);
    }
    else {
      int argIx = 0;
      filename = args[argIx];
      while (filename.startsWith("-")) { 
        /* parse options here */
        if (filename.charAt(1) == 'v') { 
          ClassDesc.verbose = true; 
        } else if (filename.charAt(1) == 'f') { 
          System.out.println("Class not package");
          anonPack = true; 
        } else if (filename.charAt(1) == 'u') { 
          System.out.println("Using unique names");
          ClassDesc.overloadedNames = false; 
        } else if (filename.charAt(1) == 'd') {
          if (argIx + 1 < argLen) {
            filename = args[++argIx];
            J2CPSFiles.SetSymDir(filename); 
          } else {
            System.err.println("-d option is missing directory name");
          }
        } else { 
          System.err.println("Unknown option " + filename); 
        }
        if (argIx + 1 < argLen) {
          filename = args[++argIx]; 
        } else {
          System.err.println("No package name given, terminating");
          System.exit(1);
        }
      }
    }
    try {
      PackageDesc thisPackage = new PackageDesc(filename, anonPack);
      PackageDesc.ReadPackages();
      PackageDesc.WriteSymbolFiles();
    }
    catch (IOException e) {
	System.err.println("IOException occurs while reading input file.");
	System.err.println("Aborting...");
	System.exit(1);
    }
  } 


}



