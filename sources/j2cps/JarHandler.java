/*
 *  (c) copyright John Gough, 2012-2017   
 */
package j2cps;

import java.io.InputStream;
import java.io.IOException;
//import java.util.HashMap;
//import java.util.ArrayList;
import java.util.Enumeration;
import java.util.jar.JarFile;
import java.util.jar.JarEntry;

/**
 *
 * @author john
 */
public class JarHandler {
    
    public JarHandler( ) { }
    
    public void ProcessJar( JarFile jf ){
        System.out.printf("INFO: opened jar file <%s>\n", jf.getName());
        Enumeration<JarEntry> entries = jf.entries();
        String classFileName;
        String className;
        while (entries.hasMoreElements()) {
            JarEntry entry = entries.nextElement();
            classFileName = entry.getName();
            //System.out.println(entryName);
            if (classFileName.toLowerCase().endsWith(".class")) {
                className = classFileName.substring(0, classFileName.length() - 6);
                try {
                    ClassDesc desc = ClassDesc.MakeNewClassDesc(className, null);
                    desc.parentPkg.myClasses.add(desc);
                    desc.parentPkg.processed = true;
                    InputStream classStream = jf.getInputStream( entry );
                    boolean ok = desc.ReadJarClassFile(classStream);
                    if (ClassDesc.verbose)
                        System.out.printf( "Read jar class %s ... %s\n", className, (ok ? "OK" : "error"));
                } catch (IOException x) {
                    // Message
                    System.err.println( "ProcessJar threw IOException");
                    System.err.println( x.getMessage() );
                    // x.printStackTrace();
                    System.exit(1);                    
                }
            }
        }
        //
        //  At this point all the class files in the jar have
        //  been processed, and all packages listed in the todo
        //  collection. However, dependent packages have not been
        //  processed. 
        //
    } 
}
