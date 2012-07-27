/**********************************************************************/
/*                  Symbol File class for J2CPS                       */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.*;
import java.util.ArrayList;

class SymbolFile {
 
/************************************************************************/
/*             Symbol file reading/writing                              */
/************************************************************************/
  // Collected syntax ---
  // 
  // SymFile    = Header [String (falSy | truSy)]
  //		{Import | Constant | Variable | Type | Procedure} 
  //		TypeList Key.
  // Header     = magic modSy Name.
  // Import     = impSy Name [String] Key.
  // Constant   = conSy Name Literal.
  // Variable   = varSy Name TypeOrd.
  // Type       = typSy Name TypeOrd.
  // Procedure  = prcSy Name [String] [truSy] FormalType.
  // Method     = mthSy Name Byte Byte TypeOrd [String] FormalType.
  // FormalType = [retSy TypeOrd] frmSy {parSy Byte TypeOrd} endFm.
  // TypeOrd    = ordinal.
  // TypeHeader = tDefS Ord [fromS Ord Name].
  // TypeList   = start {Array | Record | Pointer | ProcType | 
  //                     NamedType | Enum} close.
  // Array      = TypeHeader arrSy TypeOrd (Byte | Number | ) endAr.
  // Pointer    = TypeHeader ptrSy TypeOrd.
  // ProcType   = TypeHeader pTpSy FormalType.
  // EventType  = TypeHeader evtSy FormalType.
  // Record     = TypeHeader recSy recAtt [truSy | falSy] [basSy TypeOrd] 
  //              [iFcSy basSy TypeOrd {basSy TypeOrd}]
  //              {Name TypeOrd} {Method} {Statics} endRc.
  // Statics    = ( Constant | Variable | Procedure ).
  // Enum       = TypeHeader eTpSy { Constant } endRc.
  // NamedType  = TypeHeader. 
  // Name       = namSy byte UTFstring.
  // Literal    = Number | String | Set | Char | Real | falSy | truSy.
  // Byte       = bytSy byte.
  // String     = strSy UTFstring.
  // Number     = numSy java.lang.long.
  // Real       = fltSy java.lang.double.
  // Set        = setSy java.lang.int.
  // Key        = keySy java.lang.int.
  // Char       = chrSy java.lang.char.
  //
  // Notes on the syntax:
  // All record types must have a Name field, even though this is often
  // redundant.  The issue is that every record type (including those that
  // are anonymous in CP) corresponds to a Java class, and the definer 
  // and the user of the class _must_ agree on the JVM name of the class.
  // The same reasoning applies to procedure types, which must have equal
  // interface names in all modules.
  //

  static final String[] mthAtt = {"", ",NEW", ",ABSTRACT", ",NEW,ABSTRACT",
                                  ",EMPTY", ",NEW,EMPTY",
                                  ",EXTENSIBLE", ",NEW,EXTENSIBLE"};
  static final String[] recAtt = {"RECORD ", "ABSTRACT RECORD ",
                                  "LIMITED RECORD ", "EXTENSIBLE RECORD "};
  static final String[] mark = {"", "*", "-", "!"};
  static final String[] varMark = {"", "IN", "OUT", "VAR"};

  private static final String spaces = "         ";
  private static final String recEndSpace = "      "; 
  private static final char qSepCh = '/';

  static final int modSy = (int) 'H';
  static final int namSy = (int) '$';
  static final int bytSy = (int) '\\';
  static final int numSy = (int) '#';
  static final int chrSy = (int) 'c';
  static final int strSy = (int) 's';
  static final int fltSy = (int) 'r';
  static final int falSy = (int) '0';
  static final int truSy = (int) '1';
  static final int impSy = (int) 'I';
  static final int setSy = (int) 'S';
  static final int keySy = (int) 'K';
  static final int conSy = (int) 'C';
  static final int typSy = (int) 'T';
  static final int tDefS = (int) 't';
  static final int prcSy = (int) 'P';
  static final int retSy = (int) 'R';
  static final int mthSy = (int) 'M';
  static final int varSy = (int) 'V';
  static final int parSy = (int) 'p';
  static final int iFcSy = (int) '~';
  
  static final int start = (int) '&';
  static final int close = (int) '!';
  
  static final int recSy = (int) '{';
  static final int endRc = (int) '}';
  static final int frmSy = (int) '(';
  static final int fromS = (int) '@';
  static final int endFm = (int) ')';
  static final int arrSy = (int) '[';
  static final int endAr = (int) ']';
  static final int pTpSy = (int) '%';
  static final int evtSy = (int) 'v';
  static final int ptrSy = (int) '^';
  static final int basSy = (int) '+';
  static final int eTpSy = (int) 'e';
  
  static final int magic = 0xdeadd0d0;

  static final int prvMode = 0;
  static final int pubMode = 1;
  static final int rdoMode = 2;
  static final int protect = 3;

  private static final int initTypeListSize = 128;
  public static TypeDesc[] typeList = new TypeDesc[initTypeListSize];
  private static int nextType = TypeDesc.ordT;
  private static int tListIx = 0;
  private static int sSym = 0;
  private static int acc = 0;
  private static String name;
  private static int iVal;
  private static long lVal;
  private static int tOrd;
  private static char cVal;
  private static double dVal;
  private static DataInputStream in;

// Symbol file writing 

  static void writeName(DataOutputStream out,int access, String name) 
                                                            throws IOException{
    out.writeByte(namSy);
    if (ConstantPool.isPublic(access))   { out.writeByte(pubMode); }
    else if (ConstantPool.isProtected(access)) { out.writeByte(protect); }
    else /* if (ConstantPool.isPrivate(access)) */ { out.writeByte(prvMode); }
    out.writeUTF(name);
  }

  static void writeString(DataOutputStream out,String str) throws IOException {
    out.writeByte(strSy);
    out.writeUTF(str);
  }

  static void writeLiteral(DataOutputStream out,Object val) throws IOException {
    if (val instanceof String) {
      writeString(out,(String) val);
    } else if (val instanceof Integer) {
      out.writeByte(numSy);
      out.writeLong(((Integer)val).longValue());
    } else if (val instanceof Long) {
      out.writeByte(numSy);
      out.writeLong(((Long)val).longValue());
    } else if (val instanceof Float) {
      out.writeByte(fltSy);
      out.writeDouble(((Float)val).doubleValue());
    } else if (val instanceof Double) {
      out.writeByte(fltSy);
      out.writeDouble(((Double)val).doubleValue());
    } else {
      System.out.println("Unknown constant type");
      System.exit(1);
    }
  }

  public static void writeOrd(DataOutputStream out,int i) throws IOException {
    // DIAGNOSTIC
    if (i < 0)
      throw new IOException(); 
    // DIAGNOSTIC
    if (i <= 0x7f) {
      out.writeByte(i);
    } else if (i <= 0x7fff) {
      out.writeByte(128 + i % 128);  
      out.writeByte(i / 128);
    } else {
      throw new IOException(); 
    }
  }

  private static void InsertType(TypeDesc ty) {
    if (ty.outTypeNum > 0) { return; }
    ty.outTypeNum = nextType++;
    if (tListIx >= typeList.length) {
      TypeDesc[] tmp = new TypeDesc[typeList.length + initTypeListSize];
      System.arraycopy(typeList, 0, tmp, 0, typeList.length);
      typeList = tmp;
    }
    typeList[tListIx++] = ty;
  }

  public static void AddType(TypeDesc ty) {
    InsertType(ty); 
    if (!ty.writeDetails) { return; }
    if (ty instanceof ClassDesc) {
      ClassDesc aClass = (ClassDesc)ty;
      if (aClass.outBaseTypeNum > 0) { return; }
      aClass.outBaseTypeNum = nextType++;
      if (aClass.superClass != null) {
        aClass.superClass.writeDetails = true; 
        AddType(aClass.superClass); 
      }
      if (aClass.isInterface) {
        for (int i=0; i < aClass.interfaces.length; i++) {
          aClass.interfaces[i].writeDetails = true;
          AddType(aClass.interfaces[i]);
        }
      }
    } else if (ty instanceof PtrDesc) { 
        ty = ((PtrDesc)ty).boundType; 
        if (ty.outTypeNum == 0) { AddType(ty); }
    } else if (ty instanceof ArrayDesc) {
      ty = ((ArrayDesc)ty).elemType;
      while (ty instanceof ArrayDesc) {
        ArrayDesc aTy = (ArrayDesc)ty;
        if (aTy.ptrType.outTypeNum == 0) { InsertType(aTy.ptrType); }
        if (aTy.outTypeNum == 0) { InsertType(aTy); }
        ty = aTy.elemType;
      }                   
      if (ty.outTypeNum == 0) { InsertType(ty); }
    }
  }

  static void writeTypeOrd(DataOutputStream out,TypeDesc ty)throws IOException {
    if (ty.typeOrd < TypeDesc.ordT) { 
      out.writeByte(ty.typeOrd); 
    } else {
      if (ty.outTypeNum == 0) { AddType(ty); }
      if (ty.outTypeNum == 0) { 
        System.out.println("ERROR: type has number 0 for type " + ty.name); 
        System.exit(1); 
      }
      writeOrd(out,ty.outTypeNum);
    }
  }

  public static void WriteFormalType(MethodInfo m,DataOutputStream out) 
                                                     throws IOException {
    if ((m.retType != null) && (m.retType.typeOrd != 0)) {
      out.writeByte(retSy);
      writeTypeOrd(out,m.retType);
    } 
    out.writeByte(frmSy);
    for (int i=0; i < m.parTypes.length; i++) {
      out.writeByte(parSy);
      if (m.parTypes[i] instanceof ArrayDesc) {
        out.writeByte(1);   // array params are IN
      } else {
        out.writeByte(0);   // all other java parameters are value 
      }
      writeTypeOrd(out,m.parTypes[i]);
    }
    out.writeByte(endFm);
  }

  public static void WriteSymbolFile(PackageDesc thisPack) throws IOException{
    ClearTypeList();
    DataOutputStream out = J2CPSFiles.CreateSymFile(thisPack.cpName);

    System.out.println("INFO:  Creating symbol file " + thisPack.cpName);

    out.writeInt(magic);
    out.writeByte(modSy);
    writeName(out,0,thisPack.cpName);
    writeString(out,thisPack.javaName);
    out.writeByte(falSy); /* package is not an interface */
    for (int i=0; i < thisPack.imports.size(); i++) {
      out.writeByte(impSy);
      PackageDesc imp = (PackageDesc)thisPack.imports.get(i);
      imp.impNum = i+1;
      writeName(out,0,imp.cpName);
      writeString(out,imp.javaName);
      out.writeByte(keySy);
      out.writeInt(0);
    }
    for (int cNum=0; cNum < thisPack.classes.length; cNum++) {
      ClassDesc thisClass = thisPack.classes[cNum];
      if ((thisClass != null) && ConstantPool.isPublic(thisClass.access)) {
        thisClass.writeDetails = true;
        out.writeByte(typSy);
        writeName(out,thisClass.access,thisClass.objName);
        writeTypeOrd(out,thisClass);
      }
    }
    out.writeByte(start);
    for (int i=0; i < tListIx; i++) {
      out.writeByte(tDefS);
      writeOrd(out,typeList[i].outTypeNum);
      typeList[i].writeType(out,thisPack);
    }
    out.writeByte(close);
    out.writeByte(keySy);
    out.writeInt(0);
    thisPack.ResetImports();
  }

// Symbol file reading 

  private static void InsertType(int tNum,TypeDesc ty) {
    if (tNum >= typeList.length) {
      int newLen = 2 * typeList.length;
      while (tNum >= newLen) { newLen += typeList.length; }
      TypeDesc[] tmp = new TypeDesc[newLen];
      System.arraycopy(typeList, 0, tmp, 0, typeList.length);
      typeList = tmp;
    }
    typeList[tNum] = ty;
  }


  private static int readOrd() throws IOException {
    int b1 = in.readUnsignedByte();
    if (b1 <= 0x7f) { return b1; }
    else { int b2 = in.readByte();
           return b1 - 128 + b2 * 128; }
  }

  private static void GetSym() throws IOException {
    sSym = in.readByte();
    switch (sSym) {
      case namSy : acc = in.readByte();         // fall through 
      case strSy : name = in.readUTF(); break;
      case arrSy :
      case ptrSy :
      case retSy : 
      case fromS : 
      case tDefS : 
      case basSy : tOrd = readOrd(); break;
      case bytSy : iVal = in.readByte(); break;
      case keySy : 
      case setSy : iVal = in.readInt(); break;
      case numSy : lVal = in.readLong(); break;
      case fltSy : dVal = in.readDouble(); break;
      case chrSy : cVal = in.readChar(); break;
      case modSy : 
      case impSy : 
      case conSy : 
      case varSy : 
      case typSy : 
      case prcSy : 
      case mthSy : 
      case parSy : 
      case start : 
      case close : 
      case falSy : 
      case truSy : 
      case frmSy : 
      case endFm : 
      case recSy : 
      case endRc : 
      case endAr : 
      case eTpSy :
      case iFcSy :
      case evtSy :
      case pTpSy : break;
      default:  char ch = (char) sSym;
                System.out.println("Bad symbol file format." +ch+"  "+sSym);
                System.exit(1);
    }
  }

  private static void Expect(int expSym) throws IOException {
    if (expSym != sSym) {
      System.out.println("Error in symbol file:  expecting " + 
      String.valueOf((char) expSym) + " got " +
      String.valueOf((char) sSym));
      System.exit(1);
    }
    GetSym();
  }

  private static void Check(int expSym) {
    if (expSym != sSym) {
      System.out.println("Error in symbol file:  checking " + 
      String.valueOf((char) expSym) + " got " +
      String.valueOf((char) sSym));
      System.exit(1);
    }
  }

  private static void SkipToEndRec(DataInputStream in) throws IOException {
    while (sSym != endRc) { 
      if (sSym == mthSy) {
        GetSym(); // name
        in.readByte(); 
        in.readByte();
        readOrd();
      } else if (sSym == varSy) {
        GetSym(); // name
        readOrd();
      } else if (sSym == conSy) {
        GetSym(); // name
        GetSym(); // Literal
      } else if (sSym == prcSy) {
        GetSym(); // name
      } else if (sSym == parSy) {
        in.readByte();
        readOrd();
      } else if (sSym == namSy) {
        readOrd();
      } else {
      }
      GetSym(); 
    }
  }

  private static int GetAccess() {
    if (acc == prvMode) { return ConstantPool.ACC_PRIVATE; }
    else if (acc == pubMode) { return ConstantPool.ACC_PUBLIC; }
    else if (acc == protect) { return ConstantPool.ACC_PROTECTED; }
    return 0;
  }

  private static ClassDesc GetClassDesc(PackageDesc thisPack,String className) {
    ClassDesc aClass = ClassDesc.GetClassDesc(thisPack.name + qSepCh + 
                                              className,thisPack);
    if (aClass.fieldList == null){ aClass.fieldList = new ArrayList(); }
    if (aClass.methodList == null){ aClass.methodList = new ArrayList(); }
    return aClass;
  }

  private static void GetConstant(ClassDesc cClass) throws IOException {
  // Constant = conSy Name Literal.
  // Literal  = Number | String | Set | Char | Real | falSy | truSy.
    TypeDesc typ = null;
    Object val = null;
    Expect(conSy);
    String constName = name; 
    int fAcc = GetAccess();
    fAcc = fAcc + ConstantPool.ACC_STATIC + ConstantPool.ACC_FINAL;
    Expect(namSy); 
    switch (sSym) {
      case numSy : typ = TypeDesc.GetBasicType(TypeDesc.longT); 
                   val = new Long(lVal); break;
      case strSy : typ = TypeDesc.GetBasicType(TypeDesc.strT);
                   val = name; 
      case setSy : typ = TypeDesc.GetBasicType(TypeDesc.setT);
                   val = new Integer(iVal); break;
      case chrSy : typ = TypeDesc.GetBasicType(TypeDesc.charT);
                   val = new Character(cVal); break;
      case fltSy : typ = TypeDesc.GetBasicType(TypeDesc.dbleT);
                   val = new Double(dVal); break;
      case falSy : typ = TypeDesc.GetBasicType(TypeDesc.boolT);
                   val = false; break;
      case truSy : typ = TypeDesc.GetBasicType(TypeDesc.boolT);
                   val = true; break;
    }
    boolean ok = cClass.fieldList.add(new FieldInfo(cClass,fAcc,constName,typ,val));
    GetSym();
  }

  private static void GetVar(ClassDesc vClass) throws IOException {
  // Variable = varSy Name TypeOrd.
    Expect(varSy);
    String varName = name; 
    int fAcc = GetAccess();
    Check(namSy);
    FieldInfo f = new FieldInfo(vClass,fAcc,varName,null,null);
    f.typeFixUp = readOrd();
    vClass.fieldList.add(f);
    GetSym();
  }

  private static void GetType(PackageDesc thisPack) throws IOException {
  // Type = typSy Name TypeOrd.
    Expect(typSy);
    ClassDesc thisClass = GetClassDesc(thisPack,name); 
    thisClass.access = GetAccess();
    Check(namSy);
    int tNum = readOrd();
    thisClass.inTypeNum = tNum;
    InsertType(tNum,thisClass);
    GetSym();
  }

  private static void GetFormalType(ClassDesc thisClass,MethodInfo thisMethod) 
                                                           throws IOException {
  // FormalType = [retSy TypeOrd] frmSy {parSy Byte TypeOrd} endFm.
    int [] pars = new int[20];
    int numPars = 0;
    TypeDesc retType = TypeDesc.GetBasicType(TypeDesc.noTyp);
    if (sSym == retSy) { thisMethod.retTypeFixUp = tOrd; GetSym();} 
    Expect(frmSy);
    while (sSym != endFm) {
      Check(parSy);
      in.readByte();   /* ignore par mode */
      pars[numPars++] = readOrd(); 
      GetSym();
    }  
    Expect(endFm);
    thisMethod.parFixUps = new int[numPars];
    System.arraycopy(pars, 0, thisMethod.parFixUps, 0, numPars);
  }


  private static void GetMethod(ClassDesc thisClass) throws IOException {
  // Method = mthSy Name Byte Byte TypeOrd [String] FormalType.
    String jName = null;
    Expect(mthSy);
    Check(namSy);
    String nam = name; 
    int pAcc = GetAccess();
    int attr = in.readByte();
    int recMode = in.readByte();  
    int cNum = readOrd();
    if (cNum != thisClass.inTypeNum) {
      System.err.println("Method not part of THIS class!");
      System.exit(1);
    }  
    GetSym();
    if (sSym == strSy) { jName = name; GetSym();  }
    MethodInfo m = new MethodInfo(thisClass,nam,jName,pAcc); 
    switch (attr) {
      case 1 : if (!m.isInitProc) {
                 m.accessFlags += ConstantPool.ACC_FINAL; 
               }
               break;
      case 2 : m.overridding = true;
               m.accessFlags += (ConstantPool.ACC_ABSTRACT + 
                                 ConstantPool.ACC_FINAL); 
               break;
      case 3 : m.accessFlags += (ConstantPool.ACC_ABSTRACT +
                                 ConstantPool.ACC_FINAL); 
               break;
      case 6 : m.overridding = true;
               break;
      case 7 : break; 
    }
    GetFormalType(thisClass,m);
    thisClass.methodList.add(m);
    thisClass.scope.put(m.name,m);
  }

  private static void GetProc(ClassDesc pClass) throws IOException {
  // Proc = prcSy Name [String] [truSy] FormalType.
    String jName = null;
    Expect(prcSy);
    String procName = name; 
    int pAcc = GetAccess();
    pAcc = pAcc + ConstantPool.ACC_STATIC;
    Expect(namSy); 
    if (sSym == strSy) { jName = name; GetSym();  }
    MethodInfo m = new MethodInfo(pClass,procName,jName,pAcc); 
    if (sSym == truSy) { m.isInitProc = true; GetSym();  }
    GetFormalType(pClass,m);
    pClass.methodList.add(m);
  }

  private static void ClearTypeList() {
    for (int i=0; i < typeList.length; i++) {
      if (typeList[i] != null) {
        if (typeList[i].typeOrd >= TypeDesc.specT) {
          typeList[i].inTypeNum = 0; 
          typeList[i].outTypeNum = 0; 
        }
        if (typeList[i] instanceof ClassDesc) {
          ((ClassDesc)typeList[i]).inBaseTypeNum = 0;
          ((ClassDesc)typeList[i]).outBaseTypeNum = 0;
          ((ClassDesc)typeList[i]).writeDetails = false;
        } else if (typeList[i] instanceof ArrayDesc) {
          ((ArrayDesc)typeList[i]).elemTypeFixUp = 0;
        }
      }
      typeList[i] = null; 
    }
    tListIx = 0;
    nextType = TypeDesc.ordT;
  }

  private static void FixArrayElemType(ArrayDesc arr) {
    if (arr.elemTypeFixUp == 0) { return; }
    TypeDesc elem = GetFixUpType(arr.elemTypeFixUp);
    if (elem instanceof ArrayDesc) {
      FixArrayElemType((ArrayDesc)elem); 
      arr.dim = ((ArrayDesc)elem).dim + 1;
      arr.ultimateElemType = ((ArrayDesc)elem).ultimateElemType;
    } else {
      arr.ultimateElemType = elem;
    }
    arr.elemType = elem;
  }

  private static TypeDesc GetFixUpType (int num) {
    if (num < TypeDesc.specT) { return TypeDesc.GetBasicType(num); }
    if (typeList[num] instanceof PtrDesc) { 
      return ((PtrDesc)typeList[num]).boundType;
    }
    return typeList[num];
  } 

  public static void ReadSymbolFile(File symFile,PackageDesc thisPack) 
                                     throws FileNotFoundException, IOException {

    if (ClassDesc.verbose)
      System.out.println("INFO:  Reading symbol file " + symFile.getName());

    ClearTypeList();
    ClassDesc aClass, impClass;
    PackageDesc impPack;
    int maxInNum = 0;
    FileInputStream fIn = new FileInputStream(symFile);
    in = new DataInputStream(fIn);
    if (in.readInt() != magic) {
      System.out.println(symFile.getName() + " is not a valid symbol file.");
      System.exit(1);
    }
    GetSym();
    Expect(modSy);
    if (!thisPack.cpName.equals(name)) {
      System.out.println("ERROR:  Symbol file " + symFile.getName() + 
      " does not contain MODULE " + thisPack.cpName + ", it contains MODULE " +
      name);
      System.exit(1);
    }
    Expect(namSy);
    if (sSym == strSy) { 
      if (!name.equals(thisPack.javaName)) { 
        System.out.println("Wrong name in symbol file.");
        System.exit(1); 
      }
      GetSym();
      if (sSym == truSy) {  
        System.out.println("ERROR:  Java Package cannot be an interface.");
        System.exit(1);
      }
      GetSym();
    } else {
      System.err.println("<" + symFile.getName() + 
                         "> NOT A SYMBOL FILE FOR A JAVA PACKAGE!");
      System.exit(1);
    }
    while (sSym != start) {
      switch (sSym) {
        case impSy : GetSym(); // name
                     String iName = name;
                     GetSym(); 
                     if (sSym == strSy) { 
                       PackageDesc pack = PackageDesc.getPackage(name);
                       thisPack.imports.add(pack);
                       GetSym(); 
                     }
                     Expect(keySy);  
                     break;
        case conSy : 
        case varSy : 
        case prcSy : System.out.println("Symbol File is not from a java class");
                     System.exit(1);
                     break;
        case typSy : GetType(thisPack); break;
      }
    }
    Expect(start);
    while (sSym != close) {
      int impNum = -1;
      impPack = null;
      impClass = null;
      String impName = null, impModName = null;
      int impAcc = 0, impModAcc = 0;
      Check(tDefS); 
      int tNum = tOrd; GetSym(); 
      if (tNum > maxInNum) { maxInNum = tNum; }
      if (sSym == fromS) { 
        impNum = tOrd - 1;
        GetSym(); 
        Check(namSy);
        impName = name;
        impAcc = acc; 
        if (impNum < 0) {
          impPack = thisPack;
        } else {
          impPack = (PackageDesc)thisPack.imports.get(impNum);
        }
        impClass = GetClassDesc(impPack,impName);
        GetSym(); 
      }
      switch (sSym) { 
        case arrSy : ArrayDesc newArr = null;
                     int elemOrd = tOrd;
                     GetSym();
                     Expect(endAr);
                     TypeDesc eTy = null;
                     if (elemOrd < typeList.length) {
                       if (elemOrd < TypeDesc.specT) { 
                         eTy = TypeDesc.GetBasicType(elemOrd); 
                       } else { 
                         eTy = typeList[elemOrd]; 
                       }
                       if ((eTy != null) && (eTy instanceof PtrDesc) &&
                          (((PtrDesc)eTy).boundType != null) && 
                          (((PtrDesc)eTy).boundType instanceof ClassDesc)) {
                         eTy = ((PtrDesc)eTy).boundType;   
                       } 
                     }
                     if (eTy != null) {
                       newArr = ArrayDesc.FindArrayType(1,eTy,true); 
                     } else {
                       newArr = new ArrayDesc(elemOrd); 
                     }
                     if ((tNum < typeList.length) && (typeList[tNum] != null)) {
                       PtrDesc desc = (PtrDesc) typeList[tNum];
                       if (desc.inBaseTypeNum != tNum) {
                         System.out.println("WRONG BASE TYPE FOR POINTER!");
                         System.exit(1);
                       }
                       desc.Init(newArr);
                       newArr.SetPtrType(desc);
                     }
                     InsertType(tNum,newArr);
                     break; 
        case ptrSy : TypeDesc ty = null;
                     if (impClass != null) { 
                       InsertType(tNum,impClass);
                       ty = impClass;
                       ty.inTypeNum = tNum;
                       ty.inBaseTypeNum = tOrd;
                       InsertType(tOrd,ty);
                     } else if ((tNum < typeList.length) && 
                                (typeList[tNum] != null) &&
                                (typeList[tNum] instanceof ClassDesc)) { 
                       ty = typeList[tNum];
                       ty.inTypeNum = tNum;
                       ty.inBaseTypeNum = tOrd;
                       InsertType(tOrd,ty);
                     } else {
                       ty = new PtrDesc(tNum,tOrd);
                       InsertType(tNum,ty);
                       if ((tOrd < typeList.length) && 
                                (typeList[tOrd] != null)) { 
                         ((PtrDesc)ty).Init(typeList[tOrd]);
                       }
                     }
                     GetSym();
                     break; 
        case recSy : if ((tNum >= typeList.length) || (typeList[tNum] == null)||
                         (!(typeList[tNum] instanceof ClassDesc))) {
                     /* cannot have record type that is not a base type
                        of a pointer in a java file                     */
                       System.err.println(
                         "RECORD TYPE " + tNum + " IS NOT POINTER BASE TYPE!");
                       System.exit(1);
                     }
                     aClass = (ClassDesc) typeList[tNum];
                     acc = in.readByte();
                     aClass.setRecAtt(acc);
                     if (aClass.read) { 
                       GetSym();
                       SkipToEndRec(in); 
                       GetSym();
                     } else {
                       GetSym();
                       if (sSym == truSy) { 
                         aClass.isInterface = true; 
                         GetSym();
                       } else if (sSym == falSy) { 
                         GetSym(); 
                       }
                       if (sSym == basSy) { 
                         aClass.superNum = tOrd; 
                         GetSym(); 
                       }
                       if (sSym == iFcSy) {
                         GetSym();
                         aClass.intNums = new int[10];
                         aClass.numInts = 0;
                         while (sSym == basSy) {
                           if (aClass.numInts >= aClass.intNums.length) {
                             int tmp[] = new int[aClass.intNums.length*2];
                             System.arraycopy(aClass.intNums, 0, tmp, 0, aClass.intNums.length);
                             aClass.intNums = tmp;
                           }
                           aClass.intNums[aClass.numInts] = tOrd;
                           aClass.numInts++;
                           GetSym();
                         }
                       }
                       while (sSym == namSy) {
                         FieldInfo f = new FieldInfo(aClass,GetAccess(),name,
                                       null,null); 
                         f.typeFixUp = readOrd(); 
                         GetSym();
                         boolean ok = aClass.fieldList.add(f);
                         aClass.scope.put(f.name,f);
                       } 
                       while ((sSym == mthSy) || (sSym == prcSy) ||
                              (sSym == varSy) || (sSym == conSy)) { 
                         switch (sSym) {
                           case mthSy : GetMethod(aClass); break; 
                           case prcSy : GetProc(aClass); break;
                           case varSy : GetVar(aClass); break;
                           case conSy : GetConstant(aClass); break;
                         }
                       }
                       Expect(endRc);
                     }
                     break; 
        case pTpSy : System.out.println("CANNOT HAVE PROC TYPE IN JAVA FILE!"); 
                     break;
        case evtSy :System.out.println("CANNOT HAVE EVENT TYPE IN JAVA FILE!"); 
                     break;
        case eTpSy : System.out.println("CANNOT HAVE ENUM TYPE IN JAVA FILE!"); 
                     break;
        case tDefS : 
        case close : InsertType(tNum,impClass);
                     break;
        default : char ch = (char) sSym; 
                  System.out.println("UNRECOGNISED TYPE!" + sSym + "  " + ch); 
                  System.exit(1);
      }
    }
    Expect(close);
    Check(keySy); 
    fIn.close();
    // do fix ups...
    for (int i = TypeDesc.specT; i <= maxInNum; i++) {
      int size = 0;
      if ((typeList[i] != null) && (typeList[i] instanceof ClassDesc)) {
        aClass = (ClassDesc)typeList[i];
        if (!((ClassDesc)typeList[i]).read) {
          aClass = (ClassDesc)typeList[i];
          if (aClass.superNum != 0) {
            aClass.superClass = (ClassDesc)typeList[aClass.superNum];
          }
          aClass.interfaces = new ClassDesc[aClass.numInts];
          for (int j=0; j < aClass.numInts; j++) {
            aClass.interfaces[j] = (ClassDesc) GetFixUpType(aClass.intNums[j]); 
          }
          if (aClass.fieldList == null) { size = 0; 
          } else {
            size = aClass.fieldList.size(); 
          }
          aClass.fields = new FieldInfo[size];
          for (int j=0; j < size; j++) {
            aClass.fields[j] = (FieldInfo)aClass.fieldList.get(j);
            aClass.fields[j].type = GetFixUpType(aClass.fields[j].typeFixUp); 
            if (aClass.fields[j].type instanceof ClassDesc) {
              aClass.AddImport((ClassDesc)aClass.fields[j].type);
            }  
          }
          aClass.fieldList = null;
          if (aClass.methodList == null) { size = 0;
          } else { size = aClass.methodList.size(); }
          aClass.methods = new MethodInfo[size];
          for (int k=0; k < size; k++) {
            aClass.methods[k] = (MethodInfo)aClass.methodList.get(k);
            aClass.methods[k].retType = GetFixUpType(
                                        aClass.methods[k].retTypeFixUp); 
            if (aClass.methods[k].retType instanceof ClassDesc) {
              aClass.AddImport((ClassDesc)aClass.methods[k].retType);
            }  
            aClass.methods[k].parTypes = new TypeDesc[
                                         aClass.methods[k].parFixUps.length];
            for (int j=0; j < aClass.methods[k].parFixUps.length; j++) {
              aClass.methods[k].parTypes[j] = GetFixUpType(
                                              aClass.methods[k].parFixUps[j]);
              if (aClass.methods[k].parTypes[j] instanceof ClassDesc) {
                aClass.AddImport((ClassDesc)aClass.methods[k].parTypes[j]);
              }  
            }
          }
          aClass.methodList = null;
          aClass.read = true;
          aClass.done = true;
        }
      } else if ((typeList[i] != null) && (typeList[i] instanceof ArrayDesc)) {
        FixArrayElemType((ArrayDesc)typeList[i]);
      } else if ((typeList[i] != null) && (typeList[i] instanceof PtrDesc)) {
        PtrDesc ptr = (PtrDesc)typeList[i];
        if (ptr.typeOrd == TypeDesc.arrPtr) { 
          ptr.Init(typeList[ptr.inBaseTypeNum]);
        }
      } else if (typeList[i] != null) { 
        System.out.println("Type " + i + " " + typeList[i].name + 
                           " is NOT array or class"); 
        System.exit(0);
      }
    }
  }
}


