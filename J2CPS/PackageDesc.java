/**********************************************************************/
/*                Package Desscriptor class for j2cps                 */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import j2cpsfiles.j2cpsfiles;

public class PackageDesc {
  
    public static int pub = 0;
    public static int tot = 0;
    
    static final ExtFilter classFilter = ExtFilter.NewExtFilter(".class");
    /**
     *  Work-list of all the packages encountered during this
     *  invocation of <code>j2cps</code>, either directly or through reference.
     */
    static final ArrayList<PackageDesc> toDo = new ArrayList<>(2);
    
    /**
     *  List of packages that will be emitted as symbol files.
     */
    static final ArrayList<PackageDesc> syms = new ArrayList<>(2);
    
    /**
     *  Dictionary of packages known to this invocation of <code>j2cps</code>.
     */
    static final HashMap<String,PackageDesc> packageList = new HashMap<>();
    
    /**
     *  The file associated with this package.
     */
    File theFile;
    
    int status = Util.PRISTINE;

    /**
     *  This list contains all the classes defined in this package.
     */
    public ArrayList<ClassDesc> myClasses;
    
    /**
     *  The name of the package with <code>qSepCh</code> separators
     */
    public String name;
            
    /**
     *  The name of the package converted to a legal
     *  Component Pascal module identifier.
     */
    public String cpName;
            
    /**
     *  Package name in java language (dotted) format.
     */
    public String javaName;
            
    /**
     *  Directory name relative to class file hierarchy root with
     *  notation corrected for environment-specific file separators
     */
    public String dirName;
    
    /**
     *  List of imports into this package.
     */
    public ArrayList<PackageDesc> pkgImports = new ArrayList<>();
    Object blame;

        
    public int impNum = -1;
    public boolean anonPackage = false;
    
    /**
     *  Boolean true indicates that this package has been
     *  processed, and (if not just <i> referenced</i>) has
     *  been added to the <code>syms</code> list.
     */
    public boolean processed = false;
    
    public String blameStr() {
        if (this.blame == null)
            return "no-blame";
        else if (this.blame instanceof MemberInfo) {
            MemberInfo f = (MemberInfo)this.blame;
            return f.owner.objName + "::" + f.name;
        }
        else if (this.blame instanceof ClassDesc) {
            ClassDesc c = (ClassDesc)this.blame;
            return "ClassDesc " + c.name;
        }
        else
            return this.blame.toString();
    }

    /** 
     * Adds another package to the <code>toDo</code>
     * and package lists.
     * @param pName the full name of the package without file-extension
     * @param anon indicates if this package is the anonymous package
     * @return 
     */
    static PackageDesc MakeNewPackageDesc(String pName, boolean anon) {
        PackageDesc desc = new PackageDesc(pName, anon);
        if (!anon) {
            // Add to package list
            packageList.put(desc.name, desc);
            desc.set(Util.ONLIST);
        }
        // Add to toDo worklist
        toDo.add(desc);
        /*
        if (pName.startsWith("com") || pName.startsWith("sun"))
        System.err.println(pName);
        */
        desc.set(Util.ONTODO);
        return desc;
    }
  
    /**
     * Creates a new package descriptor, which will be the root 
     * (and element-zero) of the work-list for non-jar based
     * invocations.
     * <p>
     * Adds another package to the <code>toDo</code>
     * and package lists.
     * @param pName the full name of the package without file-extension
     * @param anon indicates if this package is the anonymous package
     */
    public static void MakeRootPackageDescriptor(String pName, boolean anon) {
      PackageDesc desc = new PackageDesc(pName, anon);
      if (!anon) { // Then insert in HashMap
          packageList.put(desc.name, desc);
          desc.set(Util.ONLIST);
      }
      // And in any case put on the worklist
      toDo.add(desc);
      /*        
      if (pName.startsWith("com") || pName.startsWith("sun"))
      System.err.println(pName);
      */
      desc.set(Util.ONTODO);
    }

    private String removeCLSU(String pName) {
        if (pName.startsWith("classes_"))
            return pName.substring(8);
        return pName;
    }

    private String removeCLSDOT(String pName) {
        if (pName.startsWith("classes."))
            return pName.substring(8);
        return pName;
    }
  
    private PackageDesc(String pName, boolean anon) {
        if (anon) {
          this.name = pName;
          this.cpName = removeCLSU(pName);
          this.javaName = removeCLSDOT(pName);
          this.anonPackage = true;
        } else {
          MakeName(pName); 
        }
        this.myClasses = new ArrayList<>();
    }

    private void MakeName(String pName) {
        this.name = pName.replace(Util.JAVADOT,Util.FWDSLSH);
        this.name = this.name.replace(Util.FILESEP,Util.FWDSLSH);  /* name is now .../... */
        this.cpName = removeCLSU(this.name.replace(Util.FWDSLSH,Util.LOWLINE));
        this.javaName = removeCLSDOT(this.name.replace(Util.FWDSLSH,Util.JAVADOT));
        if (Util.FWDSLSH != Util.FILESEP) {
            this.dirName = this.name.replace(Util.FWDSLSH,Util.FILESEP);
        } else {
            this.dirName = this.name;
        }
    }
 
    /** Create or fetch package descriptor by name */
  public static PackageDesc getPackage(String packName) {
    packName = packName.replace(Util.JAVADOT,Util.FWDSLSH); 
    PackageDesc pack = (PackageDesc)packageList.get(packName);
    if (pack == null) { 
        pack = PackageDesc.MakeNewPackageDesc(packName,false); 
    }
    return pack;
  }


  /**
   * Create or fetch the package descriptor object for the class 
   * with the given java (dotted) name.  In the case that the 
   * package has not been previously seen, the name of the newly
   * created package descriptor is the prefix of the className.
   * @param className the full java-name of the class.
   * @return the package descriptor to which the corresponding 
   * class descriptor should be added.
   */
  public static PackageDesc getClassPackage(String className) {
    className = className.replace(Util.JAVADOT,Util.FWDSLSH); 
    String pName = className.substring(0,className.lastIndexOf(Util.FWDSLSH));
    PackageDesc pack = (PackageDesc)packageList.get(pName);
    if (pack == null) { 
        pack = PackageDesc.MakeNewPackageDesc(pName,false);
    }
    return pack;
  }

  public void AddPkgImport(TypeDesc ty) {
    if (ty instanceof ClassDesc) {
      ClassDesc aClass = (ClassDesc)ty;
      if (aClass.parentPkg == null) {
        System.err.println("ERROR: Class <"+aClass.javaName+"> has no package");
        System.exit(0);
      } 
      if ((this!=aClass.parentPkg)&&
              (!this.pkgImports.contains(aClass.parentPkg))){ 
        this.pkgImports.add(aClass.parentPkg); 
      }
    } 
  }
 
  public void AddImport(PackageDesc pack) {
    if ((this != pack) && (!this.pkgImports.contains(pack))){ 
      boolean ignoreMe = this.pkgImports.add(pack); 
    }
  }

  public void ResetImports() {
    for (int i=0; i < this.pkgImports.size(); i++) {
      this.pkgImports.get(i).impNum = -1;
    }
  }

  private void AddImportList(ArrayList impList) {
    for (int i=0; i < impList.size(); i++) {
      // this.AddImport((PackageDesc)impList.get(i));
      PackageDesc p = (PackageDesc)impList.get(i);
      this.AddImport(p);
      System.out.printf("package %s, adding import %s, with APINEEDS=%s\n", 
              this.cpName, p.cpName, (p.has(Util.APINEEDS) ? "true" : "false"));
    }
  }

  public void ReadPackage() throws IOException, FileNotFoundException {
    boolean ignoreMe;
    this.processed = true;
    ignoreMe = syms.add(this);
    if (this.anonPackage) {
        ClassDesc newClass;
        // this.classes = new ClassDesc[1];
        //
        // Create a new class descriptor with the given name
        // in the anonomous package represented by "this".
        // Then read the class file with the given name.
        //
        newClass = ClassDesc.GetClassDesc(this.name,this);
        this.myClasses.add( newClass );
        //
        // this.classes[0] = newClass;
        // 
        ignoreMe = newClass.ReadPkgClassFile(j2cpsfiles.OpenClassFile(this.name));
    } else {
        this.theFile = j2cpsfiles.getPackageFile(this.dirName);
        //
        //  classFilter gets files ending with ".class"
        //
        String[] clsNames = this.theFile.list(classFilter);
        //
        // Create a descriptor array and populate with the class
        // descriptors corresponding to the class files in the
        // directory for the current package.
        //
        // this.classes = new ClassDesc[clsNames.length];
        for (int i = 0; i < clsNames.length; i++) {
            String cName = name + Util.FWDSLSH + 
                    clsNames[i].substring(0,clsNames[i].lastIndexOf('.'));
            ClassDesc nextClass = ClassDesc.GetClassDesc(cName,this);
            if (nextClass.ReadPkgClassFile(j2cpsfiles.OpenClassFile(this.theFile, clsNames[i]))) {
                // this.classes[i] = nextClass;
                this.myClasses.add(nextClass);
            }
        }
    }
  }

  public static void ProcessPkgWorklist() throws IOException, FileNotFoundException {
    int j = 0;
    {
        PackageDesc pkg0 = toDo.get(0);
        pkg0.ReadPackage();
        if (ClassDesc.summary)      // Lightweight progress indicator ...
          System.out.printf("INFO: locating dependencies of package <%s>\n", pkg0.name);
    }
    //
    //  Reading package pkg0 may have added to the toDo list
    //
    for (int i=1; i < toDo.size(); i++) {
      PackageDesc pkgDesc = toDo.get(i);
      if (!pkgDesc.has(Util.APINEEDS)) {
          if (ClassDesc.verbose)
              System.out.println("Non API package skipped " + pkgDesc.cpName);
          continue;
      }          
      /* look for symbol file first */
      pkgDesc.theFile = j2cpsfiles.FindSymbolFile(pkgDesc.cpName);
      if (pkgDesc.theFile == null) { // No symbol file, look for package
        pkgDesc.ReadPackage();
      } else { // Symbol file was found ==> Read it.
        pkgDesc.set(Util.FOUNDCPS);
        if (ClassDesc.summary)  {
            System.out.println("Found Symbol file for dependency <" + pkgDesc.cpName + ">");
            if (ClassDesc.verbose)
                System.out.println("    Symbol file " + pkgDesc.theFile.getPath());
        }
      }
    }
  }
  
    public static void ProcessJarDependencies() throws IOException, FileNotFoundException  {
        for (PackageDesc desc : toDo) {
            if (desc.processed) {
                syms.add(desc);
            } else {
                desc.theFile = j2cpsfiles.FindSymbolFile(desc.cpName);
                System.out.println("INFO: Searching for imported package " + desc.javaName);
                if (desc.theFile == null)
                    System.err.printf(
                        "       Symbolfile %s not found on CPSYM path\n", desc.cpName);
                else if (ClassDesc.summary)
                    System.out.println(
                        "Found symbol file for dependency <" + desc.cpName  + ">");
                                
            }
        }
    }

    public static void WriteSymbolFiles() throws IOException {
        for (PackageDesc nextPack : syms) {
            SymbolFile.WriteSymbolFile(nextPack);
        }
    }
    
    public boolean has(int tag) {
        return (this.status & tag) != Util.PRISTINE;
    }
    
    public void clear(int tag) {
        this.status ^= tag;
    }
    
    public void set(int tag) {
        this.status |= tag;
        int mask = Util.FROMJAR | Util.FROMCLS;
        if (( this.status & mask) == mask)
            throw new IllegalArgumentException();
    }
}
