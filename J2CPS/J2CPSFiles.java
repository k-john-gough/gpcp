/**********************************************************************/
/*                  J2CPS Files class for J2CPS                       */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.*;

public class J2CPSFiles implements FilenameFilter {

  private static final String classExt = ".class";
  private static final String symExt = ".cps";
  private static final String intExt = ".cp";
  private static final String dbName = "index.dbi";
  private static final String sepCh = System.getProperty("file.separator");
  private static final char EOF = '\0';
  private static final char CR  = '\r';
  private static final char LF  = '\n';
  private static final char SP  = ' ';
  private static final char TAB  = '\t';
  private static String currDir = System.getProperty("user.dir");
  private static String symDir;
  private static String[] classPath;
  private static String[] symPath;
  private static final char pathSep = 
                            System.getProperty("path.separator").charAt(0);


/* 
 * Method for FilenameFilter
 */

  @Override
  public boolean accept (File dir, String name) {
    return name.endsWith(classExt); 
  }

  public static void SetSymDir(String sDir) {
    symDir = sDir;
    if (symDir == null) {
      symDir = symPath[0];
    }
  }

  public static void GetPaths() {
    classPath = GetPath("java.class.path");
    symPath = GetPath("CPSYM");
  }

  private static String GetPathFromProperty(String str) {
    String path = System.getProperty(str);
    //if (path != null)
    //  System.out.println("Property " + str + " = " + path); 
    return path;
  }

  private static String GetPathFromEnvVar(String str) {
    String path = System.getenv(str);
    //if (path != null)
    //  System.out.println("Env. variable " + str + " = " + path); 
    return path;
  }

  private static String[] GetPath(String prop) {
    String paths[];
    // First look for the system property (preferred source)
    String cPath = GetPathFromProperty(prop);
    if (cPath == null)
      cPath = GetPathFromEnvVar(prop);

    if (cPath == null) {
      System.out.println("No variable for \"" + prop + "\", using \".\""); 
      cPath = ".";
    } else 
      System.out.println("Using \"" + prop + "\" path \"" + cPath + "\""); 
      
    int i,count=1,start,end;
    for (i=0; i > -1 ; i++ ) {
      i = cPath.indexOf(pathSep,i); 
      if (i > -1) { count++; } else { i--; } 
    }
    paths = new String[count+1];
    paths[0] = currDir;
    start = 0; i = 1; 
    while (start < cPath.length()) {
      end = cPath.indexOf(pathSep,start);
      if (end == -1) { 
        end = cPath.length()+1; 
        paths[i] = cPath.substring(start);
      } else {
        paths[i] = cPath.substring(start,end);
      }
      if (paths[i].equals(".")) { paths[i] = currDir; }
      i++;
      start = end+1;
    }
    return paths;
  }

  public static File getPackageFile(String name) {
    File inFile = new File(currDir,name);
    if (!inFile.exists()) {
      boolean found = false;
      for (int i=0; (i < classPath.length) && (!found); i++) {
        if (ClassDesc.verbose) {
          System.out.println("<" + classPath[i] + sepCh + name + ">");
        }
        inFile = new File(classPath[i],name);
        found = inFile.exists();
      }
      if (!found) {
        System.err.println("Cannot open class directory <" + name + ">");
        System.exit(0);
      }
    }
    return inFile;
  }

  public static File OpenClassFile(String name) {
    if (!name.endsWith(classExt)) { name = name.concat(classExt); }
    File inFile = new File(currDir,name);
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
                                                 sepCh + fName + ">");
      System.exit(0);
    }
    return inFile;
  }
  

  public static File FindClassFile(String name) {
    File inFile = null;
    boolean found = false;
    if (!name.endsWith(classExt)) { name = name.concat(classExt); }
    for (int i=0; (i < classPath.length) && (!found); i++) {
      if (ClassDesc.verbose) {
        System.out.println("<" + classPath[i] + sepCh + name + ">");
      }
      inFile = new File(classPath[i],name);
      found = inFile.exists();
    }
    if (!found) {
      System.err.println("Cannot open class file <" + name + ">");
      System.exit(0);
    }
    return inFile;
  }

  public static File FindSymbolFile(String name) 
                                    throws FileNotFoundException, IOException {
    File inFile = null;
    boolean found = false;
    if (!name.endsWith(symExt)) { name = name.concat(symExt); }
    for (int i=0; (i < symPath.length) && (!found); i++) {
      if (ClassDesc.verbose) {
        System.out.println("<" + symPath[i] + sepCh + name + ">");
      }
      inFile = new File(symPath[i],name);
      found = inFile.exists();
    }
    if (!found) {
      if (ClassDesc.verbose) 
        { System.out.println("Cannot find symbol file <" + name + ">"); }
      return null;
    }
    return inFile;
  }

  public static DataOutputStream CreateSymFile(String fileName) 
                                                          throws IOException {
    String dirName;
    if (symDir == null) { dirName = currDir; } else { dirName = symDir; }
    if (ClassDesc.verbose) {  
      System.out.println("Creating symbolfile " + fileName + symExt +
                         " in directory " + dirName);
    }
    return new DataOutputStream(new FileOutputStream(
                                new File(dirName,fileName + symExt)));
  }
}



