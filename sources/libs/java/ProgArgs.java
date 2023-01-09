//
// Body of ProgArgs interface.
// This file implements the code of the ProgArgs.cp file.
// kjg December 1999.
//
// The reason that this module is implemented as a Java class is
// that the name CPmain has special meaning to the compiler, so
// it must be imported secretly in the implementation.
//

package CP.ProgArgs;
import  CP.CPmain.CPmain;
import  java.io.File;
import  java.nio.file.*;
import  java.util.ArrayList;

public class ProgArgs
{
    public static int ArgNumber()
    {
        if (CP.CPmain.CPmain.args == null)
            return 0;
        else
            return CP.CPmain.CPmain.args.length;
    }

    public static void GetArg(int num, char[] str)
    {
        int i;
        if (CP.CPmain.CPmain.args == null) {
            str[0] = '\0';
        } else {
            for (i = 0; 
                i < str.length && i < CP.CPmain.CPmain.args[num].length();
                i++) {
                str[i] = CP.CPmain.CPmain.args[num].charAt(i);
            }
        if (i == str.length)
            i--;
        str[i] = '\0';
        }
    }

    public static void GetEnvVar(char[] ss, char[] ds) 
    {
        String path = CP.CPJ.CPJ.MkStr(ss);
        //
        //  getenv was deprecated between jave 1.1 and SE 5 (!)
        //
        String valu = System.getProperty(path);
        if (valu == null) // Try getenv instead
            valu = System.getenv(path);
            int i;
            for (i = 0; 
                 i < valu.length() && i < ds.length;
                 i++) {
                ds[i] = valu.charAt(i);
            }
            if (i == ds.length)
                i--;
            ds[i] = '\0';
        }

    public static void ExpandWildcards(int argsToSkip) {
        //
        // The Java launcher expands wildcards, but only
        // for simple filenames in the current directory.
        // 
        try {
            CP.CPmain.CPmain.args = ExpandArgs(CP.CPmain.CPmain.args, argsToSkip);
        } catch (Exception x) {
            System.err.println(x.toString());
        }
    }

    private static boolean needsExpansion(String arg) {
	return (arg.contains("*") || arg.contains("?"));
    }

    
    private static String[] ExpandArgs(String[] args, int first) throws Exception {
        ArrayList<String> list = new ArrayList<String>();
        for (int i = 0; i < args.length; i++) {
            String argS = args[i];
            if (i < first || argS.charAt(0) == '-' || !needsExpansion(argS)) {
                list.add(argS);
            } else {
                File argF = new File(args[i]);
                File parent = argF.getParentFile();
                String pattern = argF.getName();
                boolean implicitParent = (parent == null);
                if (implicitParent)
                    parent = new File(".");
                try (DirectoryStream<Path> stream = 
                        Files.newDirectoryStream(parent.toPath(), pattern)) {
                    for (Path entry: stream) {
                        if (implicitParent)
                            list.add(entry.toFile().getName());
                        else
                            list.add(entry.toString());
                    }
                } catch (DirectoryIteratorException x) {
                        throw x.getCause();
                }
            }
        }
        return list.toArray(new String[0]);
    }

} // end of public class ProgArgs
