/*
 * Copyright (c) John Gough 2016-2017
 */
package j2cpsfiles;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.FileOutputStream;
import java.io.DataOutputStream;
import java.io.File;

/**
 *
 * @author john
 */
public class j2cpsfiles /*implements FilenameFilter*/ {

  private static final String CLASSEXT = ".class";
  private static final String SYMFILEXT = ".cps";
  // private static final String CPEXT = ".cp";
  // private static final String dbName = "index.dbi";
  private static final char EOF = '\0';
  private static final char CR  = '\r';
  private static final char LF  = '\n';
  private static final char SP  = ' ';
  private static final char TAB  = '\t';
  private static final String CURRDIR = 
          System.getProperty("user.dir");
  private static final String FILESEP = 
          System.getProperty("file.separator");
  private static final String PATHSEPSTRING = 
           System.getProperty("path.separator");
  private static final char PATHSEP = 
          PATHSEPSTRING.charAt(0);
 
  /**
   *  Destination directory for symbol files.
   */
  private static String dstDir = ".";
  
  /**
   *  The installation root directory.
   */
  private static String rootName = ".";
  
  /**
   *  The root of the package class-file tree.
   */
  private static File pkgRoot = new File(CURRDIR);
  
  private static String[] classPath;
  
  /**
   *  Locations to search for symbol files.
   */
  private static String[] symPath;
  
  private static boolean verbose = false;
  private static boolean summary = false;
 
  public static void SetVerbose( boolean v ) { verbose = v; }
  public static void SetSummary( boolean v ) { summary = v; }

    public static void SetDstDir(String sDir) {
        if (!sDir.equals("."))
            dstDir = "." + FILESEP + sDir;
    }

    public static void SetPackageRootDir(String rDir) {
        rootName = "." + FILESEP + rDir;
        pkgRoot = new File(CURRDIR, rDir);
    }

    /**
     *  This method is called after all arguments have been parsed.
     */
    public static void GetPaths(boolean ignoreCpsym) {
        if (summary) {
            System.out.printf("Current directory \".\" is <%s>\n", CURRDIR);
            if (!rootName.equals("."))
                System.out.printf(
                    "Using <%s> as package-root directory\n", rootName);
            if (!dstDir.equals("."))
                System.out.printf("Using <%s> as symbol destination directory\n", dstDir);
        }      
        classPath = GetPathArray("java.class.path");
        if (ignoreCpsym) {
            symPath = new String[] { dstDir };
        } else {
            String[] tmp = GetPathArray("CPSYM");
            symPath = new String[tmp.length + 1];
            symPath[0] = dstDir;
            for (int i = 0; i < tmp.length; i++)
                symPath[i+1] = tmp[i];
        }
    }

  private static String GetPathFromProperty(String str) {
    String path = System.getProperty(str); 
    return path;
  }

  private static String GetPathFromEnvVar(String str) {
    String path = System.getenv(str);
    return path;
  }

  private static String[] GetPathArray(String prop) {
    // First look for the system property (preferred source)
    String cPath = GetPathFromProperty(prop);
    if (cPath == null)
      cPath = GetPathFromEnvVar(prop);

    if (cPath == null) {
        System.err.println("No variable for \"" + prop + "\", using \".\""); 
        cPath = ".";
    } else if (summary) 
        System.out.println("Using \"" + prop + "\" path \"" + cPath + "\""); 
    
    String[] splits = cPath.split(PATHSEPSTRING);
    return splits;
  }

  public static File getPackageFile(String name) { 
    File inFile = new File(pkgRoot,name);
    if (!inFile.exists()) {
        boolean found = false;
        for (int i=0; (i < classPath.length) && (!found); i++) {
            if (verbose) {
                System.out.println("<" + classPath[i] + FILESEP + name + ">");
            }
            inFile = new File(classPath[i],name);
            found = inFile.exists();
        }
        if (!found) {
          System.err.println(
                  "Cannot open package directory <" + name + ">, quitting");
          // 
          // Is this too severe?
          //
          System.exit(0);
        }
    } else {
        System.out.print("INFO: opened package directory <" + name + ">");
        if (summary)
            System.out.print(" from package-root <" + rootName + ">");
        System.out.println();
    }
    return inFile;
  }

  public static File OpenClassFile(String name) {
    if (!name.endsWith(CLASSEXT)) { name = name.concat(CLASSEXT); }
    File inFile = new File(CURRDIR,name);
    if (!inFile.exists()) {
      inFile = FindClassFile(name);
    }
    if (!inFile.exists()) {
      System.err.println("Cannot open class file <" + name + ">");
      System.exit(0);
    }
    return inFile;
  }


  public static File OpenClassFile(File dir, String fName) {
    File inFile = new File(dir,fName);
    if (!inFile.exists()) {
      System.err.println("Cannot open class file <" + dir.getName() +
                                                 FILESEP + fName + ">");
      System.exit(0);
    }
    return inFile;
  }
  

  public static File FindClassFile(String name) {
    File inFile = null;
    boolean found = false;
    if (!name.endsWith(CLASSEXT)) { name = name.concat(CLASSEXT); }
    for (int i=0; (i < classPath.length) && (!found); i++) {
      if (verbose) {
        System.out.println("<" + classPath[i] + FILESEP + name + ">");
      }
      inFile = new File(classPath[i],name);
      found = inFile.exists();
    }
    if (!found) {
      System.err.println("Cannot open class file <" + name + ">");
      System.exit(1);
    }
    return inFile;
  }

  public static File FindSymbolFile(String name) 
                                    throws FileNotFoundException, IOException {
    File inFile = null;
    boolean found = false;
    if (!name.endsWith(SYMFILEXT)) { 
        name = name.concat(SYMFILEXT); 
    }
    for (int i=0; (i < symPath.length) && (!found); i++) {
      if (verbose) {
        System.out.println("Seeking <" + symPath[i] + FILESEP + name + ">");
      }
      inFile = new File(symPath[i],name);
      found = inFile.exists();
    }
    if (!found) {
      if (verbose) {
        System.out.println("Cannot find symbol file <" + name + ">");
      }
      return null;
    } else {
        //char[] arr = inFile.getPath().toCharArray();
        return inFile;
    }
  }

  public static DataOutputStream CreateSymFile(String fileName) 
                                                          throws IOException {
    String dirName = (dstDir == null ? CURRDIR : dstDir);
    System.out.print("INFO: Creating symbolfile <" + fileName + SYMFILEXT + ">");
    if (summary) 
        System.out.print(" in directory <" + dirName + ">");
    System.out.println();
    return new DataOutputStream(new FileOutputStream(
                                new File(dirName,fileName + SYMFILEXT)));
  }    
}
