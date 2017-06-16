/**********************************************************************/
/*                Class Descriptor class for j2cps                    */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

public class ClassDesc extends TypeDesc  {

    // private static final int MAJOR_VERSION = 45;
    // private static final int MINOR_VERSION = 3;
    
    private static HashMap<String,ClassDesc> classMap = new HashMap<>();
    private static final String jlString = "java.lang.String";
    private static final String jlObject = "java.lang.Object";

    private static final int noAtt = 0;  // no record attribute in cp
    private static final int absR  = 1;  // ABSTRACT record in cp
    private static final int limR  = 2;  // LIMITED record in cp
                                         // No equivalent in java!
    private static final int extR  = 3;  // EXTENSIBLE record in cp
    private static final int iFace = 4;  // JAVA interface 

    public static boolean useJar = false;
    public static boolean verbose = false;
    public static boolean VERBOSE = false;
    public static boolean summary = false;
    public static boolean nocpsym = false;

    @Override
    public String toString() {    
        if (this.name != null)
            return this.name;
        else
            return "anon-ClassDesc";
    }
    
    ConstantPool cp;
    ClassDesc superClass;
    int access, outBaseTypeNum=0, superNum=0, numInts=0, intNums[];
    public String 
            /**
             * Qualified name of the class e.g. java/lang/Object
             */
            qualName, 
            /**
             * Java language name of the class e.g. java.lang.Object
             */
            javaName,
            /**
             *  Unqualified name of the class e.g. Object
             */
            objName;
    
    ClassDesc[] interfaces;
    
    boolean 
            isInterface = false, 
            read = false, 
            done = false;
    
    public boolean hasNoArgConstructor = false;
    
    /**
     *  Packages imported by this class
     */
    public ArrayList<PackageDesc> imports = new ArrayList<>();
    
    /**
     *  Fields declared in this class
     */
    public ArrayList<FieldInfo> fieldList = new ArrayList<>();
    
    /**
     *  Methods defined in this class
     */
    public ArrayList<MethodInfo> methodList = new ArrayList<>();
    
    /**
     *  HashMap of members declared locally in this class.
     */
    HashMap<String,MemberInfo> scope = new HashMap<>();

    public ClassDesc() {
      typeOrd = TypeDesc.classT;
    }
    
    /**
     * Find an existing ClassDescriptor object with the given
     * string name, or create one in the supplied package.
     * <p>
     * If the pack parameter is null, the package is found (or 
     * created) based on the prefix of the string class name.
     * @param name the full name of the class  
     * @param pack the package descriptor, or null if not known
     * @return     the (possibly newly created) class descriptor
     */
    public static ClassDesc GetClassDesc(String name, PackageDesc pack) {
        if (name.indexOf(Util.JAVADOT) != -1) { 
            name = name.replace(Util.JAVADOT,Util.FWDSLSH); 
        }
        ClassDesc aClass = (ClassDesc)classMap.get(name);
        if (aClass == null) {
            aClass = ClassDesc.MakeNewClassDesc(name,pack);
        } 
        return aClass;
    }

    /**
     * Create a new class descriptor with the given name, and added to
     * the given package descriptor.
     * <p>
     * If the pack parameter is null, create a new package descriptor 
     * with a name inferred from the full class name, add the new package
     * to the package list, add the new class to the new package.
     * @param name the full name of the new class
     * @param pack the package to which the new class belongs, or null
     * @return the newly created class descriptor.
     */
    public static ClassDesc MakeNewClassDesc(String name, PackageDesc pack) {
        ClassDesc desc = new ClassDesc(name, pack);
        desc.MakeJavaName();
        classMap.put(desc.qualName, desc);
        return desc;
    }

    private ClassDesc(String thisName, PackageDesc pack) {
        this.typeOrd = TypeDesc.classT;
        this.qualName = thisName;
        if (pack == null) {
            this.parentPkg = PackageDesc.getClassPackage(this.qualName);
        } else {
            this.parentPkg = pack;
        } 
    }

    public ClassDesc(int inNum) { this.inBaseTypeNum = inNum; }

    @Override
    public String getTypeMnemonic() {
        switch (javaName) {
            case jlString:
                return "S";
            case jlObject:
                return "O";
            default:
                return "o";
        }
    }

    /**
     *  Read the details of the class.
     *  <p> 
     *  Parse the class file attached to the DataInputStream,
     *  creating new type, class and package descriptors for newly
     *  identified type names.
     * @param stream The input data stream. Could be either
     *               a FileStream or a zip stream from a jar file.
     * @return true if parse was successful.
     * @throws IOException 
     */
    private boolean ReadClassFileDetails(DataInputStream stream)
                                                         throws IOException {
      read = true;
      int count;
      ClassRef tmp;
      /* read and check the magic number */
      if (stream.readInt() != 0xCAFEBABE) {
        System.out.println("Bad magic number");
        System.exit(0);
      }
      /* read and check the minor and major version numbers  */
      int minorVersion = stream.readUnsignedShort();
  //   /* if (minorVersion > MINOR_VERSION) {
  //	System.out.println("Unsupported Java minor version " +
  //				String.valueOf(minorVersion));
  //	System.exit(0);
  //    }
  //*/
      int majorVersion = stream.readUnsignedShort();
  // /*   if (majorVersion != MAJOR_VERSION) {
  //      System.out.println("Unsupported Java major version " + 
  //			 String.valueOf(majorVersion));
  //      System.exit(0);
  //    }
  //*/
      cp = new ConstantPool(stream);
      access = stream.readUnsignedShort();
      // Experimental code to only transform packages that
      // are reachable from classes that are not private.
      // Under consideration for next version, controlled 
      // by a command line option.
      PackageDesc.tot++;
      if (!ConstantPool.isPublic(access) && !ConstantPool.isProtected(access)) {
         cp.EmptyConstantPool();
         PackageDesc.pub++;
         return true;
      }
      // End experimental code

      String clName = ((ClassRef) cp.Get(stream.readUnsignedShort())).GetName();
      if (!qualName.equals(clName)) {
        if (clName.startsWith(parentPkg.name)) {
          if (verbose) { System.out.println(clName + " IS PART OF PACKAGE " + 
                                            parentPkg.name + " but name is not "
                                            + qualName); }
        } else {
            System.err.println(clName + " IS NOT PART OF PACKAGE " + 
                           parentPkg.name + "  qualName = " + qualName);
            parentPkg = PackageDesc.getClassPackage(qualName);
            return false;
        } 
        classMap.remove(qualName);
        qualName = clName;
        this.MakeJavaName();
        ClassDesc put = classMap.put(qualName,this);
      }
      isInterface = ConstantPool.isInterface(access);
      int superIx = stream.readUnsignedShort();
      if (superIx > 0) {
        tmp = (ClassRef) cp.Get(superIx);
        superClass = tmp.GetClassDesc();
        //
        //  If superclass is not from the current package
        //  mark the superclass package as needed for API.
        //
        if (superClass.parentPkg != this.parentPkg) {
            //superClass.parentPkg.set(Util.APINEEDS);
            superClass.blame = this;
            this.AddImportToClass(superClass);
        }
      }
      /* get the interfaces implemented by this class */
      count = stream.readUnsignedShort();
      interfaces = new ClassDesc[count];
      for (int i = 0; i < count; i++) {
        tmp = (ClassRef) cp.Get(stream.readUnsignedShort());
        ClassDesc theInterface = tmp.GetClassDesc();
        //theInterface.parentPkg.set(Util.APINEEDS);
        this.AddImportToClass(theInterface);
        interfaces[i] = theInterface;
      }
      /* get the fields for this class */ 
      count = stream.readUnsignedShort();
      if (verbose) 
          System.out.println(count != 1 ?
                  "There are " + count + " fields" :
                  "There is one field");
      for (int i = 0; i < count; i++ ) {
        FieldInfo fInfo = new FieldInfo(cp,stream,this);
        // 
        //  The package to which this class belongs
        //  must place the package of this field's type
        //  on the import list IFF:
        //   *  the field is public or protected AND
        //   *  the field's package is not that of thisClass
        //
        if (fInfo.isExported()) {
            this.fieldList.add(fInfo);
            if (fInfo.type.parentPkg != this.parentPkg) {
                fInfo.type.blame = fInfo;
                this.TryImport(fInfo.type);
            }
        }
      }
      /* get the methods for this class */ 
      count = stream.readUnsignedShort();
      if (verbose) 
          System.out.println(count != 1 ?
                  "There are " + count + " methods" :
                  "There is one method");
      for (int i = 0; i < count; i++ ) {
        MethodInfo mInfo = new MethodInfo(cp,stream,this);
        if (mInfo.isExported())
            this.methodList.add(mInfo);
      }
      /* ignore the rest of the classfile (ie. the attributes) */ 
      if (verbose) { System.out.println("Finished reading class file"); }
      if (VERBOSE) { /*PrintClassFile();*/ Diag(); }
      this.AddImportToParentImports();
      cp.EmptyConstantPool();
      cp = null;
      return true;
    }

    public void TryImport(TypeDesc type){ 
        if (type instanceof ClassDesc) {
            this.AddImportToClass((ClassDesc)type);
        }
        else if (type instanceof ArrayDesc) {
            ((ArrayDesc)type).elemType.blame = type.blame;
            this.TryImport(((ArrayDesc)type).elemType); // Recursive!
        }
        else if (type instanceof PtrDesc) {
            ((PtrDesc)type).AddImportToPtr(this);
        }

    }

    public void AddImportToClass(ClassDesc aClass) {
     if ((aClass != this) && (aClass.parentPkg != this.parentPkg) &&
          (!this.imports.contains(aClass.parentPkg))) {
        aClass.parentPkg.set(Util.APINEEDS);
        this.imports.add(aClass.parentPkg);
        aClass.parentPkg.blame = aClass.blame;
        /*        
        if (aClass.parentPkg.cpName.contains("sun"))
            System.err.println(aClass.parentPkg.cpName);
        */
        }
    }
    
    public void AddImportToParentImports() {
        PackageDesc myPkg = this.parentPkg;
        for (PackageDesc pkg : this.imports) {
            if (!myPkg.pkgImports.contains(pkg)) {
                myPkg.pkgImports.add(pkg);// pkg should have been already blamed?
                if (VERBOSE)
                    System.out.printf(
                            "Adding %s to import list of package %s, blame %s\n",
                            pkg.name, myPkg.name,
                            pkg.blameStr());
                            //pkg.blame == null? "null" : pkg.blame.toString());
            }
        }
    }

    public boolean ReadPkgClassFile(File cFile) throws IOException {
        boolean result;
        try (DataInputStream in = new DataInputStream(new FileInputStream(cFile))) {
            if (verbose) { 
                System.out.println("Reading Pkg Class File <"+this.javaName+">"); 
            }
            this.parentPkg.set(Util.CURRENT);
            result = this.ReadClassFileDetails(in);
            if (result)
                this.parentPkg.set(Util.FROMCLS);
        }
        return result;
    }
  
    public boolean ReadJarClassFile(InputStream stream) throws IOException {
        boolean result;
        try (DataInputStream in = new DataInputStream(stream)) {
            if (verbose) { 
                System.out.println("Reading Jar Class File <"+qualName+">"); 
            }
            this.parentPkg.set(Util.CURRENT);
            result = this.ReadClassFileDetails(in);
            if (result)
                this.parentPkg.set(Util.FROMJAR);
            this.parentPkg.clear(Util.CURRENT);
        }    
        return result;      
    }
  
  public void PrintClassFile() {
    int i;
    System.out.println("ClassFile for " + qualName);
    cp.PrintConstantPool();
    System.out.print("THIS CLASS = ");
    System.out.print(ConstantPool.GetAccessString(access));
    System.out.println(qualName);      
    if (superClass != null) { 
      System.out.println("SUPERCLASS = " + superClass.qualName); 
    }
    System.out.println("INTERFACES IMPLEMENTED");
    for (i = 0; i < interfaces.length; i++) {
      System.out.println("  " + interfaces[i].qualName);
    }
    System.out.println("FIELDS");
    this.fieldList.forEach((fInfo) -> {
            System.out.println("  " + fInfo.toString() + ";");
        });
    System.out.println("METHODS");
    this.methodList.forEach((mInfo) -> {
            System.out.println("  " + mInfo.toString());
        });
    System.out.println();
  }

  public void Diag() {
    System.out.println("CLASSDESC");
    System.out.println("name = " + name);
    System.out.println("javaName = " + javaName);
    System.out.println("qualName = " + qualName);
    System.out.println();
  }

  private static void AddField(FieldInfo f,HashMap<String,MemberInfo> scope) throws IOException {
    int fNo = 1;
    String origName = f.name;
    while (scope.containsKey(f.name)) {
      f.name = origName + String.valueOf(fNo);
      fNo++;
    }
    scope.put(f.name,f);
  }

  private static void MakeMethodName(MethodInfo meth) {
    if (meth.isInitProc) { 
        meth.userName = "Init";
    } else {
      meth.userName = meth.name;
    }
  }

  private static void AddMethod(MethodInfo meth, HashMap<String,MemberInfo> scope) 
                                                          throws IOException {
    int methNo = 1;
    if (meth.userName == null) { MakeMethodName(meth); }
    String origName = meth.userName;
    while (scope.containsKey(meth.userName)) {
      meth.userName = origName + String.valueOf(methNo);
      methNo++;
    }
    scope.put(meth.userName,meth);
  }

  public void MakeJavaName() {
    this.javaName = qualName.replace(Util.FWDSLSH,Util.JAVADOT); // '/' --> '.'
    this.objName = javaName.substring(javaName.lastIndexOf(Util.JAVADOT)+1);
    this.name = javaName.replace(Util.JAVADOT,Util.LOWLINE); // '.' --> '_'
  }

  private void AddInterfaceImports(ClassDesc aClass) {
    // if (interfaces.length > 0) {
    if (interfaces != null && interfaces.length > 0) {
        for (ClassDesc intf : interfaces) {
            aClass.AddImportToClass(intf);
            intf.AddInterfaceImports(aClass);
        }
    }
  }

    public void GetSuperImports() {
        if (done) {
            return;
        }
        if (verbose) {
            System.out.println("GetSuperImports of " + javaName);
        }
        if (isInterface) {
            AddInterfaceImports(this);
        }
        if (this.superClass != null) {
            if (!this.superClass.done) {
                this.superClass.GetSuperImports();
            }
        }
        if (this.methodList.size() > 0) { // guard added
            for (MethodInfo mth : methodList) {
                MakeMethodName(mth);
                if (mth.isExported() && !mth.deprecated) {
                    if ((!mth.isInitProc) && (!mth.isStatic())) {
                        MethodInfo meth = GetOverridden(mth, mth.owner);
                        if (meth != null) {
                            mth.overridding = true;
                        }
                    }
                }
            }
        }
        done = true;
    }

    public void GetSuperFields(HashMap jScope) throws IOException {
        if (done) { return; }
        if (verbose) { 
            System.out.println("GetSuperFields of " + this.javaName); 
        }
        if (this.isInterface) { 
            this.AddInterfaceImports(this); 
        }
        if (this.superClass != null) {
          if (!this.superClass.done) { 
              this.superClass.GetSuperFields(jScope); }
          Iterator<String> enum1 = superClass.scope.keySet().iterator();
          while (enum1.hasNext()) {
            String methName = (String)enum1.next();
            this.scope.put(methName, this.superClass.scope.get(methName));
          }
        }
        for (FieldInfo f : this.fieldList) {
            if (f.isExported()) {
                AddField((FieldInfo)f,scope);
            } 
        }
        HashMap<String,MemberInfo> superScope = new HashMap<>();
        for (MethodInfo mth : this.methodList) {
            this.MakeMethodName(mth);
            if (mth.isExported() && !mth.deprecated) {
                if (mth.isInitProc) {
                    AddMethod(mth,superScope);
                } else if (mth.isStatic()) {
                    AddMethod(mth,scope);
                } else {
                    if (this.scope.containsKey(mth.userName)) {
                        MethodInfo meth = GetOverridden(mth,mth.owner);
                        if (meth != null) {
                            mth.overridding = true;
                            mth.userName = meth.userName;
                            this.scope.remove(mth.userName);
                            this.scope.put(mth.userName,mth);
                        } else {
                            AddMethod(mth,this.scope);
                        }
                    } else {
                        AddMethod(mth,this.scope);
                    } 
                }
            }
        }
        done = true;
    }

  private static MethodInfo GetOverridden(MethodInfo meth,ClassDesc thisClass) {
    ClassDesc aClass = thisClass;
    while (aClass.superClass != null) {
      aClass = aClass.superClass;
      if (aClass.methodList.isEmpty()) { // new guard
          for (MethodInfo method : aClass.methodList) {
              if (method.name.equals(meth.name)) {
                  if ((method.signature != null) && (meth.signature != null)) {
                      if (method.signature.equals(meth.signature)) {
                          return method;
                      }
                  } else if (method.parTypes.length == meth.parTypes.length) {
                      boolean ok = true;
                      for (int j = 0; (j < method.parTypes.length) & ok; j++) {
                          ok = method.parTypes[j] == meth.parTypes[j];
                      }
                      if (ok) {
                          return method;
                      }
                  }
              }
          }
      }
    }  
    return null;
  }

  public void CheckAccess() {
    if (ConstantPool.isAbstract(access)) {
      System.out.println(" is abstract ");
    } else if (ConstantPool.isFinal(access)) {
      System.out.println(" is final ");
    } else {
      System.out.println(" is default");
    }
  }

  public void setRecAtt(int recAtt) {
    if (recAtt >= 8) { recAtt -= 8; } else { hasNoArgConstructor = true; }
    if (recAtt == absR) {
      if (!ConstantPool.isAbstract(access)) {
        access = access + ConstantPool.ACC_ABSTRACT;
      }
    } else if (recAtt == noAtt) {
      if (!ConstantPool.isFinal(access)) {
        access = access + ConstantPool.ACC_FINAL;
      }
    }
  }

  /** Write a class definition to the typelist section of a symbol file
   * <p>
   * If the <code>writeDetails</code> flag is not true, or the
   * class belongs to a different package, then only the type 
   * ordinal is written. Otherwise a full class API is emitted.
   * 
   * @param out  the symbol file output stream
   * @param thisPack the package which this symbol file describes
   * @throws IOException 
   */
    @Override
    public void writeType(DataOutputStream out, PackageDesc thisPack)
            throws IOException {
        if (objName == null) {
            this.MakeJavaName();
        }
        if (this.parentPkg != thisPack) {
            out.writeByte(SymbolFile.fromS);
            // Diagnostic error message
            if (this.parentPkg.impNum < 0) {
                System.err.println("thisPack is " + thisPack.javaName);
                System.err.println("impNum is " + this.parentPkg.impNum);
                System.err.println("packageDesc " + this.parentPkg.javaName);
                System.err.println("objName " + objName);
                this.parentPkg.impNum = 0;
            }
            //
            SymbolFile.writeOrd(out, this.parentPkg.impNum);
            SymbolFile.writeName(out, access, objName);
        } else if (!ConstantPool.isPublic(access)) {
            out.writeByte(SymbolFile.fromS);
            SymbolFile.writeOrd(out, 0);
            SymbolFile.writeName(out, access, objName);
        }
        if (!this.writeDetails || (this.parentPkg != thisPack)) {
            return;
        }
        out.writeByte(SymbolFile.ptrSy);
        SymbolFile.writeOrd(out, outBaseTypeNum);
        out.writeByte(SymbolFile.tDefS);
        SymbolFile.writeOrd(out, outBaseTypeNum);
        out.writeByte(SymbolFile.recSy);
        int recAtt = 0;
        if (!hasNoArgConstructor) {
            recAtt = 8;
        }
        if (ConstantPool.isFinal(access)) {
            out.writeByte(noAtt + recAtt);
        } else if (isInterface) {
            out.writeByte(iFace + recAtt);
        } else if (ConstantPool.isAbstract(access)) {
            out.writeByte(absR + recAtt);
        } else {
            out.writeByte(extR + recAtt);
        }
        if (isInterface) {
            out.writeByte(SymbolFile.truSy);
        } else {
            out.writeByte(SymbolFile.falSy);
        }
        if (superClass != null) {
            out.writeByte(SymbolFile.basSy);
            SymbolFile.writeTypeOrd(out, superClass);
        }
        //if (interfaces.length > 0) {
        if (interfaces != null && interfaces.length > 0) {
            out.writeByte(SymbolFile.iFcSy);
            for (ClassDesc intf : interfaces) {
                out.writeByte(SymbolFile.basSy);
                SymbolFile.writeTypeOrd(out, intf);
            }
        }
        if (!this.fieldList.isEmpty()) {
            for (FieldInfo field : this.fieldList) {
                if (field.isExported() && !field.isStatic()) {
                    SymbolFile.writeName(out, field.accessFlags, field.name);
                    SymbolFile.writeTypeOrd(out, field.type);
                }
            }
        }
        if (!this.methodList.isEmpty()) {
            for (MethodInfo method : this.methodList) {
                if (method.isExported() && !method.deprecated
                        && !method.isStatic() && !method.isInitProc
                        && !method.isCLInitProc) {
                    out.writeByte(SymbolFile.mthSy);
                    // --------------------
                    //        if (methods[i].userName == null) {
                    //          System.out.println("packageDesc " + this.packageDesc.javaName);
                    //          System.out.println("objName " + objName);
                    //          for (int j=0; j < methods.length; j++) {
                    //            System.out.println("Method " + j +
                    //                (methods[i].userName == null ? " null" : methods[j].userName));
                    //          }
                    //        }
                    // --------------------
                    SymbolFile.writeName(out, method.accessFlags, method.userName);
                    int attr = 0;
                    if (!method.overridding) {
                        attr = 1;
                    }
                    if (method.isAbstract()) {
                        attr += 2;
                    } else if (!method.isFinal()) {
                        attr += 6;
                    }
                    out.writeByte(attr);
                    out.writeByte(0);
                    /* all java receivers are value mode */
                    SymbolFile.writeOrd(out, outTypeNum);
                    SymbolFile.writeString(out, method.name);
                    SymbolFile.WriteFormalType(method, out);
                }
            }
        }
        if (!this.fieldList.isEmpty()) {
            for (FieldInfo field : this.fieldList) {
                if (field.isConstant()) {
                    out.writeByte(SymbolFile.conSy);
                    SymbolFile.writeName(out, field.accessFlags, field.name);
                    SymbolFile.writeLiteral(out, field.GetConstVal());
                } else if (field.isExported() && field.isStatic()) {
                    out.writeByte(SymbolFile.varSy);
                    SymbolFile.writeName(out, field.accessFlags, field.name);
                    SymbolFile.writeTypeOrd(out, field.type);
                }
            }
        }
        if (!this.methodList.isEmpty()) {
            for (MethodInfo method : this.methodList) {
                if (method.isExported() && !method.deprecated
                        && method.isStatic() && !method.isCLInitProc) {
                    out.writeByte(SymbolFile.prcSy);
                    SymbolFile.writeName(out, method.accessFlags, method.userName);
                    SymbolFile.writeString(out, method.name);
                    if (method.isInitProc) {
                        out.writeByte(SymbolFile.truSy);
                    }
                    SymbolFile.WriteFormalType(method, out);
                }
            }
        }
        out.writeByte(SymbolFile.endRc);
    }
}
