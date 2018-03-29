/**********************************************************************/
/*                      Main class for j2cps                          */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.io.IOException;
import java.util.jar.JarFile;
import j2cpsfiles.j2cpsfiles;

public class j2cps {

    static String pkgOrJar;
    static String argString;
    static final String versionStr = "j2cps version 1.4.07 (March 2018)";
  /**
   * Main program. Takes a package name as a parameter, produces the 
   * Component Pascal symbol file.  The class-files of the package must
   * be nested in a directory with the package name.
   * In the case of the -jar option the last argument is the path from
   * the current working directory to the target jar file.
   */
  public static void main(String args[]) { 
    int argLen = args.length;
    String argStr = null;
    boolean anonPack = false;
    boolean seenParg = false;
    MkArgString(args);
    TypeDesc.InitTypes();
    if (argLen == 0) {
	ShowHelp();
        System.exit(0);
    }
    else {
        int argIx = 0;
        argStr = args[argIx];
        while (argStr.startsWith("-")) {
            String optString = argStr.substring(1);
            /* parse options here */
            switch (argStr.charAt(1)) {
                case 'V':
                    if (optString.equalsIgnoreCase("version")) {
                        System.out.println(versionStr);
		    }
		    else if ("VERBOSE".startsWith(optString)) {
                        ClassDesc.VERBOSE = true;
                        ClassDesc.verbose = true;
                        ClassDesc.summary = true;
                    } else
                        BadOption(argStr);  
                    break;
                case 'v':
                    if ("version".equals(optString)) {
                        System.out.println(versionStr);
		    }
		    else if ("verbose".startsWith(optString)) {
                        ClassDesc.verbose = true;
                        ClassDesc.summary = true;
                        j2cpsfiles.SetVerbose( true );
                        j2cpsfiles.SetSummary( true );
                    } else
                        BadOption(argStr);
                    break;
                case 's':
                    if ("summary".startsWith(optString)) {
                        ClassDesc.summary = true;
                        j2cpsfiles.SetSummary( true );
                    } else
                        BadOption(argStr);
                    break;
                case 'd':
                    if (!"dst".startsWith(optString))
                        BadOption(argStr);
                    else if (argIx + 1 < argLen) {
                        argStr = args[++argIx];
                        j2cpsfiles.SetDstDir(argStr);
                    } else
                        System.err.println(
                            "-d option is missing symfile destination directory name");     
                    break;
                case 'p':
                    if (!"pkg".startsWith(optString))
                        BadOption(argStr);
                    else if (argIx + 1 < argLen) {
                        seenParg = true;
                        argStr = args[++argIx];
                        j2cpsfiles.SetPackageRootDir(argStr);
                    } else
                        System.err.println(
                            "-p option is missing package-root directory name");    
                    break;
                case 'h':
                case 'H':
                    if (optString.equalsIgnoreCase("help")) {
                        ShowHelp();
                    } else
                        BadOption(argStr);
                    break;
                case 'j':
                    if (optString.equalsIgnoreCase("jar")) {
                        ClassDesc.useJar = true;
                    } else
                        BadOption(argStr);
                    break;
                case 'n':
                    if (optString.equalsIgnoreCase("nocpsym")) {
                        ClassDesc.nocpsym = true;
                    } else
                        BadOption(argStr);
                    break;
                default:
                    BadOption(argStr);
                    break;
            }
            if (argIx + 1 < argLen) {
                argStr = args[++argIx];
            } else {
                System.err.println("No package-name or jar filename given, terminating");
                System.exit(1);
            }
        }
        //
        // Consistency checks ...
        //
        if (ClassDesc.useJar && seenParg) {
            System.out.println("Cannot use both -jar and -p. Ignoring -p");
        }
        if (ClassDesc.summary)
            System.out.println(argString);
        j2cpsfiles.GetPaths(ClassDesc.nocpsym);
    }
    try {
        if (ClassDesc.useJar) {
            if (ClassDesc.useJar && !argStr.toLowerCase().endsWith(".jar")) {
                System.err.println("After -jar, filename must end \".jar\"");
                System.exit(1);
            }
            pkgOrJar = "jar-file " + argStr;
            JarFile jf = new JarFile(argStr);
            JarHandler jh = new JarHandler();
            jh.ProcessJar(jf);
            PackageDesc.ProcessJarDependencies();           
        } else {
            pkgOrJar = "java package " + argStr;
            PackageDesc.MakeRootPackageDescriptor(argStr, anonPack);
            PackageDesc.ProcessPkgWorklist();
        }
        PackageDesc.WriteSymbolFiles();
    }
    catch (IOException e) {
	System.err.println("IOException occurs while reading input file.");
	System.err.println( e.getMessage() );
	System.err.println("Aborting...");
        e.printStackTrace(System.err);
	System.exit(1);
    }
  }
  
  static void MkArgString( String[] args ) {
      StringBuilder bldr = new StringBuilder( "J2cps args>");
      for (String arg : args) {
          bldr.append(' ');
          bldr.append(arg);
      }
      argString = bldr.toString();
  }

  static private void BadOption(String s) {
      System.out.println("Unknown option " + s);
  }
   
  static private void ShowHelp( ) {
        System.err.println(versionStr);
        System.err.println("Usage:");
        System.err.println("java [VM-opts] j2cps.j2cps [options] PackageNameOrJarFile");
        System.err.println("java [VM-opts] -jar j2cps.jar [options] PackageNameOrJarFile");
        System.err.println("J2cps options may be in any order.");
        System.err.println("  -d[st] dir => symbol file destination directory");
        System.err.println("  -p[kg] dir => package-root directory");
        System.err.println("  -jar       => process the named jar file");
        System.err.println("  -s[ummary] => summary of progress");
        System.err.println("  -v[erbose] => verbose diagnostic messages");
        System.err.println("  -version   => show version string");
        System.err.println("  -nocpsym   => only use sym-files from destination,");
        System.err.println("                (overrides any CPSYM path setting)");
  }
}

