/**********************************************************************/
/*                Class Descriptor class for J2CPS                    */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

public class ClassDesc extends TypeDesc  {

  private static final int MAJOR_VERSION = 45;
  private static final int MINOR_VERSION = 3;
  private static final char qSepCh = '/';
  private static final char jSepCh = '.';
  private static final char nSepCh = '_';
  private static HashMap<String,ClassDesc> classList = new HashMap<String,ClassDesc>();
  private static final String jlString = "java.lang.String";
  private static final String jlObject = "java.lang.Object";

  private static final int noAtt = 0;  // no record attribute in cp
  private static final int absR  = 1;  // ABSTRACT record in cp
  private static final int limR  = 2;  // LIMITED record in cp
  private static final int extR  = 3;  // EXTENSIBLE record in cp
  private static final int iFace = 4;  // JAVA interface 
  private static HashMap<String,String> resWords = CPWords.InitResWords();

  public static boolean verbose = false;
  public static boolean overloadedNames = true;


  ConstantPool cp;
  ClassDesc superClass;
  int access, outBaseTypeNum=0, superNum=0, numInts=0, intNums[];
  public String qualName, javaName, objName;
  ClassDesc interfaces[];
  FieldInfo fields[];
  MethodInfo methods[];
  boolean isInterface = false, read = false, done = false;
  public boolean hasNoArgConstructor = false;
  public ArrayList imports = new ArrayList();
  public ArrayList fieldList = new ArrayList();
  public ArrayList methodList = new ArrayList();
  HashMap scope = new HashMap();

  public ClassDesc() {
    typeOrd = TypeDesc.classT;
  }

    public static ClassDesc GetClassDesc(String name, PackageDesc pack) {
    if (name.indexOf(jSepCh) != -1) { name = name.replace(jSepCh,qSepCh); }
    ClassDesc aClass = (ClassDesc)classList.get(name);
    if (aClass == null) 
        aClass = ClassDesc.MakeNewClassDesc(name,pack); 
    return aClass;
  }

  public static ClassDesc MakeNewClassDesc(String name, PackageDesc pack) {
      ClassDesc desc = new ClassDesc(name, pack);
      desc.MakeJavaName();
      classList.put(desc.qualName, desc);
      return desc;
  }
  
  private ClassDesc(String thisName, PackageDesc pack) {
    typeOrd = TypeDesc.classT;
    qualName = thisName;
    if (pack == null)
        packageDesc = PackageDesc.getClassPackage(qualName);
    else 
        packageDesc = pack; 
  }

  public ClassDesc(int inNum) {
    inBaseTypeNum = inNum; 
  }

    @Override
  public String getTypeMnemonic() {
    if (javaName.equals(jlString)) {
      return "S";
    } else if (javaName.equals(jlObject)) {
      return "O";
    } else {
      return "o";
    }
  }

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
//    if (!ConstantPool.isPublic(access) && !ConstantPool.isProtected(access)) {
//        cp.EmptyConstantPool();
//        return true;
//    }
    // End experimental code
    ClassRef thisClass = (ClassRef) cp.Get(stream.readUnsignedShort());
    String clName = thisClass.GetName();
    if (!qualName.equals(clName)) {
      if (clName.startsWith(packageDesc.name)) {
        if (verbose) { System.out.println(clName + " IS PART OF PACKAGE " + 
                                          packageDesc.name + " but name is not "
                                          + qualName); }
      } else {
        if (verbose) { System.out.println(clName + " IS NOT PART OF PACKAGE " + 
                         packageDesc.name + "  qualName = " + qualName); }
        packageDesc = PackageDesc.getClassPackage(qualName);
        return false;
      } 
      classList.remove(qualName);
      qualName = clName;
      this.MakeJavaName();
      classList.put(qualName,this);
    }
    isInterface = ConstantPool.isInterface(access);
    int superIx = stream.readUnsignedShort();
    if (superIx > 0) {
      tmp = (ClassRef) cp.Get(superIx);
      superClass = tmp.GetClassDesc();
    }
    /* get the interfaces implemented by this class */
    count = stream.readUnsignedShort();
    interfaces = new ClassDesc[count];
    for (int i = 0; i < count; i++) {
      tmp = (ClassRef) cp.Get(stream.readUnsignedShort());
      interfaces[i] = tmp.GetClassDesc();
      AddImport(interfaces[i]);
    }
    /* get the fields for this class */ 
    count = stream.readUnsignedShort();
    if (verbose) {System.out.println("There are " + count + " fields");}
    fields = new FieldInfo[count];
    for (int i = 0; i < count; i++) {
      fields[i] = new FieldInfo(cp,stream,this);
    }
    /* get the methods for this class */ 
    count = stream.readUnsignedShort();
    if (verbose) { System.out.println("There are " + count + " methods"); }
    methods = new MethodInfo[count];
    for (int i = 0; i < count; i++) {
      methods[i] = new MethodInfo(cp,stream,this);
    }
    /* ignore the rest of the classfile (ie. the attributes) */ 
    if (verbose) { System.out.println("Finished reading class file"); }
    if (verbose) { PrintClassFile(); Diag(); }
    cp.EmptyConstantPool();
    cp = null;
    return true;
  }
  
  public void TryImport(TypeDesc type){
      if (type instanceof ClassDesc)
          this.AddImport((ClassDesc)type);
      else if (type instanceof ArrayDesc)
          this.TryImport(((ArrayDesc)type).elemType);
      else if (type instanceof PtrDesc)
          ((PtrDesc)type).AddImport(this);
      
  }

  public void AddImport(ClassDesc aClass) {
   if ((aClass != this) && (aClass.packageDesc != this.packageDesc) &&
        (!imports.contains(aClass.packageDesc))) {
      imports.add(aClass.packageDesc);
    }
  }

  public boolean ReadClassFile(File cFile) throws IOException {
    boolean result;
    DataInputStream in = new DataInputStream(new FileInputStream(cFile));
    if (verbose) { System.out.println("Reading Class File <"+qualName+">"); }
    result = ReadClassFileDetails(in);
    // close the file or run out of file handles!
    in.close();
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
    for (i=0; i < fields.length; i++) {
      System.out.println("  " + fields[i].toString() + ";");
    }
    System.out.println("METHODS");
    for (i=0; i < methods.length; i++) {
      System.out.println("  " + methods[i].toString());
    }
    System.out.println();
  }

  public void Diag() {
    System.out.println("CLASSDESC");
    System.out.println("name = " + name);
    System.out.println("javaName = " + javaName);
    System.out.println("qualName = " + qualName);
    System.out.println();
  }

  private static void AddField(FieldInfo f,HashMap scope) throws IOException {
    int fNo = 1;
    String origName = f.name;
    while (scope.containsKey(f.name)) {
      f.name = origName + String.valueOf(fNo);
      fNo++;
    }
    scope.put(f.name,f);
  }

  private static int HashSignature(MethodInfo meth) {
    int tot=0, sum=0, parNum = 1, end = meth.signature.indexOf(')');
    boolean inPar = false;
    for (int i=1; i < end; i++) {
      char c = meth.signature.charAt(i);
      sum += sum;
      if (sum < 0) { sum++; }
      sum += parNum * (int)c;
      if (!inPar) {
        if (c == 'L') { inPar = true; }
        else if (c != '[')  { parNum++; tot += sum; } 
      } else if (c == ';')  { inPar = false; parNum++; tot += sum; }
    }
    int hash = tot % 4099;
    if (hash < 0) { hash = -hash; }
    return hash;
  }

  private static void MakeMethodName(MethodInfo meth) {
    boolean needHash = false;
    if (meth.isInitProc) { meth.userName = "Init";
    } else {
      meth.userName = meth.name;
    }
    if (overloadedNames) { return; }
    if (meth.parTypes.length > 0) { meth.userName += "_"; }
    for (int i=0; i < meth.parTypes.length; i++) {
      String next = meth.parTypes[i].getTypeMnemonic();
      if (next.endsWith("o")) { needHash = true; }
      meth.userName += next;
    }
    if (needHash) {
      int hash = HashSignature(meth);
      meth.userName += ("_" + String.valueOf(hash)); 
    }
  }

  private static void AddMethod(MethodInfo meth, HashMap<String,MethodInfo> scope) 
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
    javaName = qualName.replace(qSepCh,jSepCh);
    objName = javaName.substring(javaName.lastIndexOf(jSepCh)+1);
    name = javaName.replace(jSepCh,nSepCh);
  }

  private void AddInterfaceImports(ClassDesc aClass) {
    // if (interfaces.length > 0) {
    if (interfaces != null && interfaces.length > 0) {
      for (int i=0; i < interfaces.length; i++) {
        aClass.AddImport(interfaces[i]);
        interfaces[i].AddInterfaceImports(aClass);
      }
    }
  }

  public void GetSuperImports() {
    if (done) { return; }
    if (verbose) { System.out.println("GetSuperImports of " + javaName); }
    if (isInterface) { AddInterfaceImports(this); }
    if (superClass != null) {
      if (!superClass.done) { superClass.GetSuperImports(); }
    }
    if (methods != null) { // guard added
      for (int i=0; i < methods.length; i++) {
        MethodInfo mth = methods[i];
        MakeMethodName(mth);
        if (mth.isExported() && !mth.deprecated) {
          if ((!mth.isInitProc) && (!mth.isStatic())) {
            MethodInfo meth = GetOverridden(mth,mth.owner);
            if (meth != null) { mth.overridding = true; }
          }
        }
      }
    }
    done = true;
  }

  public void GetSuperFields(HashMap jScope) throws IOException {
    if (done) { return; }
    if (verbose) { System.out.println("GetSuperFields of " + javaName); }
    if (isInterface) { AddInterfaceImports(this); }
    if (superClass != null) {
      if (!superClass.done) { superClass.GetSuperFields(jScope); }
      Iterator<String> enum1 = superClass.scope.keySet().iterator();
      while (enum1.hasNext()) {
        String methName = (String)enum1.next();
        scope.put(methName, superClass.scope.get(methName));
      }
    }
    for (int i=0; i < fields.length; i++) {
      FieldInfo f = fields[i];
      if (f.isExported()) { 
        AddField(f,scope); 
      }
    } 
    HashMap<String,MethodInfo> iScope = new HashMap<String,MethodInfo>();
    for (int i=0; i < methods.length; i++) {
      MethodInfo mth = methods[i];
      MakeMethodName(mth);
      if (mth.isExported() && !mth.deprecated) {
        if (mth.isInitProc) {
          AddMethod(mth,iScope); 
        } else if (mth.isStatic()) {
          AddMethod(mth,scope);
        } else {
          //if (scope.containsKey(mth.name)) {
          if (scope.containsKey(mth.userName)) {
            MethodInfo meth = GetOverridden(mth,mth.owner);
            if (meth != null) {
              mth.overridding = true;
              mth.userName = meth.userName;
              scope.remove(mth.userName);
              scope.put(mth.userName,mth); 
            } else {
              AddMethod(mth,scope);
            }
          } else { 
            AddMethod(mth,scope); 
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
      if (aClass.methods != null) { // new guard
        for (int i=0; i < aClass.methods.length; i++) {
          if (aClass.methods[i].name.equals(meth.name)) {
            if ((aClass.methods[i].signature != null)&&(meth.signature != null)){
              if (aClass.methods[i].signature.equals(meth.signature)) {
                return aClass.methods[i];
              }
            } else if (aClass.methods[i].parTypes.length == meth.parTypes.length){
              boolean ok = true;
              for (int j=0; (j < aClass.methods[i].parTypes.length)& ok; j++){
                ok = aClass.methods[i].parTypes[j] == meth.parTypes[j]; 
              }
              if (ok) { return aClass.methods[i]; }
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

    @Override
  public void writeType(DataOutputStream out,PackageDesc thisPack) 
                                                           throws IOException {
    if (objName == null)  { this.MakeJavaName(); }
    if (this.packageDesc != thisPack) {
      out.writeByte(SymbolFile.fromS);
// ------------
//      if (this.packageDesc.impNum < 0) {
//        System.out.println("impNum is " + this.packageDesc.impNum);
//        System.out.println("packageDesc " + this.packageDesc.javaName);
//        System.out.println("objName " + objName);
//        this.packageDesc.impNum = 0;
//      }
// ------------
      SymbolFile.writeOrd(out,this.packageDesc.impNum);
      SymbolFile.writeName(out,access,objName);
    } else if (!ConstantPool.isPublic(access)) {
      out.writeByte(SymbolFile.fromS);
      SymbolFile.writeOrd(out,0);
      SymbolFile.writeName(out,access,objName);
    }
    if (!writeDetails) { return; }
    out.writeByte(SymbolFile.ptrSy);
    SymbolFile.writeOrd(out,outBaseTypeNum);
    out.writeByte(SymbolFile.tDefS);
    SymbolFile.writeOrd(out,outBaseTypeNum);
    out.writeByte(SymbolFile.recSy);
    int recAtt = 0;
    if (!hasNoArgConstructor) { recAtt = 8; } 
    if (ConstantPool.isFinal(access)) { 
      out.writeByte(noAtt+recAtt); }
    else if (isInterface) { 
      out.writeByte(iFace+recAtt); }
    else if (ConstantPool.isAbstract(access)) { 
      out.writeByte(absR+recAtt); }
    else { 
      out.writeByte(extR+recAtt); }
    if (isInterface) { out.writeByte(SymbolFile.truSy); } 
    else { out.writeByte(SymbolFile.falSy); }
    if (superClass != null) { 
      out.writeByte(SymbolFile.basSy);
      SymbolFile.writeTypeOrd(out,superClass); 
    }
    //if (interfaces.length > 0) {
    if (interfaces != null && interfaces.length > 0) {
      out.writeByte(SymbolFile.iFcSy);
      for (int i = 0; i < interfaces.length; i++) {
        out.writeByte(SymbolFile.basSy);
        SymbolFile.writeTypeOrd(out,interfaces[i]); 
      }
    }
    if (fields != null && fields.length > 0) {
      for (int i=0; i < fields.length; i++) {
        if (fields[i].isExported() && !fields[i].isStatic()) {
            SymbolFile.writeName(out,fields[i].accessFlags,fields[i].name);
            SymbolFile.writeTypeOrd(out,fields[i].type);
        }
      }
    }
    if (methods != null && methods.length > 0) {
      for (int i=0; i < methods.length; i++) {
        if (methods[i].isExported() && !methods[i].deprecated &&
            !methods[i].isStatic() && !methods[i].isInitProc &&
            !methods[i].isCLInitProc) {
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
            SymbolFile.writeName(out,methods[i].accessFlags,methods[i].userName);
            int attr = 0;
            if (!methods[i].overridding) { attr = 1; }
            if (methods[i].isAbstract()) { attr += 2; }
            else if (!methods[i].isFinal()){ attr += 6; } 
            out.writeByte(attr); 
            out.writeByte(0); /* all java receivers are value mode */
            SymbolFile.writeOrd(out,outTypeNum);
            SymbolFile.writeString(out,methods[i].name);
            SymbolFile.WriteFormalType(methods[i],out);
        }
      }
    }
    if (fields != null && fields.length > 0) {
      for (int i=0; i < fields.length; i++) {
        if (fields[i].isConstant()) {
            out.writeByte(SymbolFile.conSy);
            SymbolFile.writeName(out,fields[i].accessFlags,fields[i].name);
            SymbolFile.writeLiteral(out,fields[i].GetConstVal());
        } else if (fields[i].isExported() && fields[i].isStatic()) {
            out.writeByte(SymbolFile.varSy);
            SymbolFile.writeName(out,fields[i].accessFlags,fields[i].name);
            SymbolFile.writeTypeOrd(out,fields[i].type);
        }
      }
    }
    if (methods != null && methods.length > 0) {
      for (int i=0; i < methods.length; i++) {
        if (methods[i].isExported() && !methods[i].deprecated &&
            methods[i].isStatic() && !methods[i].isCLInitProc) {
            out.writeByte(SymbolFile.prcSy);
            SymbolFile.writeName(out,methods[i].accessFlags,methods[i].userName);
            SymbolFile.writeString(out,methods[i].name);
            if (methods[i].isInitProc) { out.writeByte(SymbolFile.truSy); }
            SymbolFile.WriteFormalType(methods[i],out);
        }
      }
    }
    out.writeByte(SymbolFile.endRc);
  }
}
