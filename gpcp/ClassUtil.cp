(* ============================================================ *)
(*  ClassUtil is the module which writes java classs file       *)
(*  structures  						*)
(*  Copyright (c) John Gough 1999, 2000.			*)
(*  Modified DWC September, 2000.				*)
(* ============================================================ *)

MODULE ClassUtil;

  IMPORT 
        GPCPcopyright,
        RTS,
        Console,
        L := LitValue,
        J := JavaUtil,
        FileNames,
        GPFiles,
        D := Symbols,
        G := Builtin,
        F := GPBinFiles,
        CSt := CompState,
        Jvm := JVMcodes,
        Id := IdDesc,
        Ty := TypeDesc;

(* ============================================================ *)

   CONST
        classPrefix = "CP";
        maxUnsignedByte = 255;
        pubStat = Jvm.acc_public + Jvm.acc_static;
        genSep = "/";

(* ============================================================ *)
(* ============================================================ *)
(*                  Java Class File Format                      *)
(*                                                              *)
(* Classfile { u4 magic;                                        *)
(*             u2 minor_version;                                *) 
(*             u2 major_version;                                *) 
(*             u2 constant_pool_count;                          *) 
(*             cp_info constant_pool[constant_pool_count];      *)
(*             u2 access_flags;                                 *) 
(*             u2 this_class;                                   *) 
(*             u2 super_class;                                  *) 
(*             u2 interfaces_count;                             *) 
(*             u2 interfaces[interfaces_count];                 *)
(*             u2 fields_count;                                 *) 
(*             field_info fields[field_count];                  *)
(*             u2 methods_count;                                *) 
(*             method_info methods[method_count];               *)
(*             u2 attributes_count;                             *) 
(*             attribute_info attributes[attribute_count];      *)
(*           }                                                  *)
(*                                                              *)
(* ============================================================ *)

  CONST 
        (* magic = -889275714;    (* 0xCAFEBABE *) *)
        magic = 0CAFEBABEH;
        minorVersion = 3;
        majorVersion = 45;
        initSize = 50;

(* ============================================================ *)

  TYPE CPEntry = POINTER TO ABSTRACT RECORD
                 END;

  TYPE ClassRef = POINTER TO EXTENSIBLE RECORD (CPEntry)
                    nameIx : INTEGER;
                  END;

  TYPE RecClass = POINTER TO RECORD (ClassRef)
                    rec : Ty.Record;
                  END;

  TYPE ModClass = POINTER TO RECORD (ClassRef)
                    mod : D.Scope;
                  END;

  TYPE Reference = POINTER TO EXTENSIBLE RECORD (CPEntry)
                     classIx : INTEGER;
                     nameAndTypeIx : INTEGER;
                   END;

  TYPE FieldRef = POINTER TO RECORD (Reference)
                  END;

  TYPE MethodRef = POINTER TO RECORD (Reference)
                   END;

  TYPE IntMethodRef = POINTER TO RECORD (Reference)
                      END;

  TYPE StringRef = POINTER TO RECORD (CPEntry)
                     stringIx : INTEGER;
                   END;

  TYPE Integer = POINTER TO RECORD (CPEntry)
                   iVal : INTEGER;
                 END;

  TYPE Float = POINTER TO RECORD (CPEntry)
                 fVal : SHORTREAL;
               END;

  TYPE Long = POINTER TO RECORD (CPEntry)
                lVal : LONGINT;
              END;

  TYPE Double = POINTER TO RECORD (CPEntry)
                  dVal : REAL;
                END;

  TYPE NameAndType = POINTER TO RECORD (CPEntry)
                       nameIx : INTEGER;
                       descIx : INTEGER;
                     END;

  TYPE UTF8 = POINTER TO RECORD (CPEntry)
                val : L.CharOpen;
                stringRef : INTEGER;
              END;

  TYPE ConstantPool = RECORD
                        pool : POINTER TO ARRAY OF CPEntry;
                        tide : INTEGER;
                      END;

  TYPE FieldInfo* = POINTER TO RECORD
                      access : INTEGER;
                      nameIx : INTEGER;
                      descIx : INTEGER;
                      constValIx : INTEGER;
                    END;
                      
  TYPE ExceptHandler = POINTER TO RECORD
                         start : INTEGER;
                         endAndHandler : INTEGER;
                       END;

  TYPE LineNumberTable = RECORD
                           start : POINTER TO ARRAY OF INTEGER;
                           lineNum : POINTER TO ARRAY OF INTEGER;
                           tide : INTEGER;
                         END;

  TYPE Op = POINTER TO EXTENSIBLE RECORD
              offset : INTEGER;
              op : INTEGER;
            END;

  TYPE OpI = POINTER TO RECORD (Op)  
               numBytes : INTEGER;
               val : INTEGER;
             END;
                   
  TYPE OpL = POINTER TO RECORD (Op)
               lab : J.Label;
             END;

  TYPE OpII = POINTER TO RECORD (Op)
               numBytes : INTEGER;
               val1 : INTEGER; 
               val2 : INTEGER;
             END;

  TYPE Op2IB = POINTER TO RECORD (Op)
                 val : INTEGER; 
                 bVal : INTEGER;
                 trailingZero : BOOLEAN;
               END;

  TYPE OpSwitch = POINTER TO RECORD (Op)
                    defLabel : J.Label;
                    padding : INTEGER;
                    low,high : INTEGER;
                    offs : POINTER TO ARRAY OF J.Label;
                  END;

  TYPE CodeList = RECORD
                    code : POINTER TO ARRAY OF Op;
                    tide : INTEGER;
                    codeLen : INTEGER;
                  END;
                   
  TYPE MethodInfo* = POINTER TO RECORD
                       methId- : D.Scope;
                       localNum : INTEGER;    (* current locals proc  *)
                       currStack  : INTEGER;  (* current depth proc.  *)
                       exLb  : INTEGER;
                       hnLb  : INTEGER;
                       access : INTEGER;
                       nameIx : INTEGER;
                       descIx : INTEGER;
                       maxStack : INTEGER;
                       maxLocals : INTEGER;
                       codes : CodeList;
                       except : ExceptHandler;
                       lineNumTab : LineNumberTable;
                     END;
                      
  TYPE ClassFile* = POINTER TO RECORD (J.JavaFile)
                      file* : GPBinFiles.FILE;
                      meth*  : MethodInfo;
                      nxtLb : INTEGER;
                      access : INTEGER;
                      cp : ConstantPool;
                      thisClassIx : INTEGER;
                      superClassIx : INTEGER;
                      interfaces : POINTER TO ARRAY OF INTEGER;
                      numInterfaces : INTEGER;
                      fields : POINTER TO ARRAY OF FieldInfo;
                      numFields : INTEGER;
                      methods : POINTER TO ARRAY OF MethodInfo;
                      numMethods : INTEGER;
                      srcFileIx : INTEGER;
                      srcFileAttIx : INTEGER;
                      codeAttIx : INTEGER; 
                      exceptAttIx : INTEGER; 
                      lineNumTabIx : INTEGER; 
                      jlExceptIx : INTEGER;
                    END;

(* ============================================================ *)

  TYPE  TypeNameString = ARRAY 12 OF CHAR;

(* ============================================================ *)

  VAR
        typeArr   : ARRAY 16 OF INTEGER;
        procNames : ARRAY 24 OF L.CharOpen;
        procSigs  : ARRAY 24 OF L.CharOpen;

        object-       : L.CharOpen;
        init-         : L.CharOpen;
        clinit-       : L.CharOpen;
        getCls-       : L.CharOpen;
        noArgVoid-    : L.CharOpen;
        noArgClass-   : L.CharOpen;
        errorClass-   : L.CharOpen;
        errorInitSig- : L.CharOpen;
        rtsClass-     : L.CharOpen;
        main-         : L.CharOpen;
        mainSig-      : L.CharOpen;
        CPmainClass-  : L.CharOpen;
        putArgs-      : L.CharOpen;
        srcFileStr    : L.CharOpen;
        codeStr       : L.CharOpen;
        lineNumTabStr : L.CharOpen;
        caseTrap      : L.CharOpen;
        caseTrapSig   : L.CharOpen;
        withTrap      : L.CharOpen;
        withTrapSig   : L.CharOpen;
        exceptType-   : L.CharOpen;
        srcFileName   : L.CharOpen;
        copy-         : L.CharOpen;
        sysClass      : L.CharOpen;
        charClass     : L.CharOpen;
        mathClass     : L.CharOpen;
        IIretI        : L.CharOpen;
        JJretJ        : L.CharOpen;

  VAR
    byte-    : L.CharOpen;
    char-    : L.CharOpen;
    double-  : L.CharOpen;
    float-   : L.CharOpen;
    int-     : L.CharOpen;
    long-    : L.CharOpen;
    short-   : L.CharOpen;
    boolean- : L.CharOpen;


(* ============================================================ *)

  PROCEDURE^ cat2(i,j : L.CharOpen) : L.CharOpen;
  PROCEDURE^ GetTypeName(typ : D.Type) : L.CharOpen;

  PROCEDURE^ (cf : ClassFile)Code2I*(code,val : INTEGER; updateS : BOOLEAN),NEW;

(* ============================================================ *)
(*                   Constant Pool Stuff                        *)
(* ============================================================ *)

  PROCEDURE Add(VAR cp : ConstantPool; entry : CPEntry) : INTEGER;
  VAR
    i : INTEGER;
    tmp : POINTER TO ARRAY OF CPEntry;
  BEGIN
    IF LEN(cp.pool) <= cp.tide+1 THEN
      tmp := cp.pool;
      NEW(cp.pool,2 * cp.tide);
      FOR i := 1 TO cp.tide-1 DO
        cp.pool[i] := tmp[i];
      END;
    END;
    cp.pool[cp.tide] := entry;
    IF (entry IS Long) OR (entry IS Double) THEN
      INC(cp.tide,2);
      RETURN cp.tide-2;
    ELSE
      INC(cp.tide);
      RETURN cp.tide-1;
    END;
  END Add;

  PROCEDURE Equal(utf : UTF8; str2 : L.CharOpen) : BOOLEAN;
  VAR
    i : INTEGER;
    str1 : L.CharOpen;
  BEGIN
    IF utf.val = str2 THEN RETURN TRUE END;
    str1 := utf.val;
    IF (str1[0] # str2[0]) OR 
       (LEN(str1) # LEN(str2)) THEN RETURN FALSE END;
    FOR i := 1 TO LEN(str1) - 1 DO
      IF str1[i] # str2[i] THEN RETURN FALSE END;
    END;
    RETURN TRUE;
  END Equal;

  PROCEDURE AddUTF(VAR cp : ConstantPool; str : L.CharOpen) : INTEGER;
  VAR
    i : INTEGER;
    utf : UTF8;
  BEGIN
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS UTF8) & 
         Equal(cp.pool[i](UTF8), str) THEN
        RETURN i;
      END;
    END; 
    NEW(utf);
    utf.val := str;
    utf.stringRef := -1;
    RETURN Add(cp,utf);
  END AddUTF;

  PROCEDURE AddRecClassRef(VAR cp : ConstantPool; rec : Ty.Record) : INTEGER;
  VAR
    i : INTEGER;
    rc : RecClass;
  BEGIN
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS RecClass) & 
         (cp.pool[i](RecClass).rec = rec) THEN
        RETURN i;
      END;
    END; 
    NEW(rc);
    rc.rec := rec;
    IF rec.xName = NIL THEN J.MkRecName(rec); END;
    rc.nameIx := AddUTF(cp,rec.xName);
    RETURN Add(cp,rc);
  END AddRecClassRef;

  PROCEDURE AddModClassRef(VAR cp : ConstantPool; mod : Id.BlkId) : INTEGER;
  VAR
    i : INTEGER;
    mc : ModClass; 
  BEGIN
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS ModClass) & 
         (cp.pool[i](ModClass).mod = mod) THEN
        RETURN i;
      END;
    END; 
    NEW(mc);
    mc.mod := mod;
    mc.nameIx := AddUTF(cp,mod.xName);
    RETURN Add(cp,mc);
  END AddModClassRef;

  PROCEDURE AddClassRef(VAR cp : ConstantPool; clName : L.CharOpen) : INTEGER;
  VAR
    i,namIx : INTEGER;
    cr : ClassRef; 
  BEGIN
    namIx := AddUTF(cp,clName);
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS ClassRef) & 
         (cp.pool[i](ClassRef).nameIx = namIx) THEN
        RETURN i;
      END;
    END; 
    NEW(cr);
    cr.nameIx := namIx;
    RETURN Add(cp,cr);
  END AddClassRef;

  PROCEDURE AddStringRef(VAR cp : ConstantPool;  str : L.CharOpen) : INTEGER;
  VAR
    utfIx,strIx : INTEGER;
    strRef : StringRef;
  BEGIN
    utfIx := AddUTF(cp,str);
    strIx := cp.pool[utfIx](UTF8).stringRef;
    IF strIx = -1 THEN
      NEW(strRef);
      strRef.stringIx := utfIx;
      RETURN Add(cp,strRef);
    ELSE
      RETURN strIx;
    END; 
  END AddStringRef;

  PROCEDURE AddNameAndType(VAR cp : ConstantPool; nam : L.CharOpen;
                           typ : L.CharOpen) : INTEGER;
  VAR
    namIx,typIx,i : INTEGER;
    nt : NameAndType;
  BEGIN
    namIx := AddUTF(cp,nam);
    typIx := AddUTF(cp,typ);
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS NameAndType) THEN
        nt := cp.pool[i](NameAndType);
        IF (nt.nameIx = namIx) & (nt.descIx = typIx) THEN RETURN i; END;
      END;
    END; 
    NEW(nt);
    nt.nameIx := namIx; 
    nt.descIx := typIx; 
    RETURN Add(cp,nt);
  END AddNameAndType;

  PROCEDURE AddMethodRef(VAR cp : ConstantPool; classIx : INTEGER;
                         methName, signature : L.CharOpen) : INTEGER;
  VAR
    ntIx,mIx,i : INTEGER;
    meth : MethodRef;
  BEGIN
    ntIx := AddNameAndType(cp,methName,signature);
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS MethodRef) THEN
        meth := cp.pool[i](MethodRef);
        IF (meth.classIx = classIx) & (meth.nameAndTypeIx = ntIx) THEN 
          RETURN i; 
        END;
      END;
    END; 
    NEW(meth);
    meth.classIx := classIx; 
    meth.nameAndTypeIx := ntIx; 
    RETURN Add(cp,meth);
  END AddMethodRef;

  PROCEDURE AddInterfaceMethodRef(VAR cp : ConstantPool; classIx : INTEGER;
                         methName, signature : L.CharOpen) : INTEGER;
  VAR
    ntIx,mIx,i : INTEGER;
    meth : IntMethodRef;
  BEGIN
    ntIx := AddNameAndType(cp,methName,signature);
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS IntMethodRef) THEN
        meth := cp.pool[i](IntMethodRef);
        IF (meth.classIx = classIx) & (meth.nameAndTypeIx = ntIx) THEN 
          RETURN i; 
        END;
      END;
    END; 
    NEW(meth);
    meth.classIx := classIx; 
    meth.nameAndTypeIx := ntIx; 
    RETURN Add(cp,meth);
  END AddInterfaceMethodRef;

  PROCEDURE AddFieldRef(VAR cp : ConstantPool; classIx : INTEGER;
                        fieldName, signature : L.CharOpen) : INTEGER;
  VAR
    ntIx,mIx,i : INTEGER;
    field : FieldRef;
  BEGIN
    ntIx := AddNameAndType(cp,fieldName,signature);
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS FieldRef) THEN
        field := cp.pool[i](FieldRef);
        IF (field.classIx = classIx) & (field.nameAndTypeIx = ntIx) THEN 
          RETURN i; 
        END;
      END;
    END; 
    NEW(field);
    field.classIx := classIx; 
    field.nameAndTypeIx := ntIx; 
    RETURN Add(cp,field);
  END AddFieldRef;

  PROCEDURE AddConstInt(VAR cp : ConstantPool; val : INTEGER) : INTEGER;
  VAR
    i : INTEGER;
    conInt : Integer;
  BEGIN
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS Integer) & 
         (cp.pool[i](Integer).iVal = val) THEN
        RETURN i;
      END;
    END;
    NEW(conInt);
    conInt.iVal := val;
    RETURN Add(cp,conInt);
  END AddConstInt;

  PROCEDURE AddConstLong(VAR cp : ConstantPool; val : LONGINT) : INTEGER;
  VAR
    i : INTEGER;
    conLong : Long;
  BEGIN
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS Long) & 
         (cp.pool[i](Long).lVal = val) THEN
        RETURN i;
      END;
    END;
    NEW(conLong);
    conLong.lVal := val;
    RETURN Add(cp,conLong);
  END AddConstLong;

  PROCEDURE AddConstFloat(VAR cp : ConstantPool; val : SHORTREAL) : INTEGER;
  VAR
    i : INTEGER;
    conFloat : Float;
  BEGIN
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS Float) & 
         (cp.pool[i](Float).fVal = val) THEN
        RETURN i;
      END;
    END;
    NEW(conFloat);
    conFloat.fVal := val;
    RETURN Add(cp,conFloat);
  END AddConstFloat;

  PROCEDURE AddConstDouble(VAR cp : ConstantPool; val : REAL) : INTEGER;
  VAR
    i : INTEGER;
    conDouble : Double;
  BEGIN
    FOR i := 1 TO cp.tide-1 DO
      IF (cp.pool[i] # NIL) & (cp.pool[i] IS Double) &
         (cp.pool[i](Double).dVal = val) THEN
        RETURN i;
      END;
    END;
    NEW(conDouble);
    conDouble.dVal := val;
    RETURN Add(cp,conDouble);
  END AddConstDouble;

(* ============================================================ *)
(*			Constructor Method			*)
(* ============================================================ *)

  PROCEDURE newClassFile*(fileName : ARRAY OF CHAR) : ClassFile;
    VAR fil : ClassFile;
        ptr : L.CharOpen;
   (* -------------------------------------------------	*)
    PROCEDURE Warp(VAR s : ARRAY OF CHAR);
      VAR i : INTEGER;
    BEGIN
      FOR i := 0 TO LEN(s)-1 DO
        IF s[i] = "/" THEN s[i] := GPFiles.fileSep END;
      END;
    END Warp;
   (* -------------------------------------------------	*)
    PROCEDURE GetFullPath(IN fn : ARRAY OF CHAR) : L.CharOpen;
      VAR ps : L.CharOpen;
          ch : CHAR;
    BEGIN
      ps := BOX(CSt.binDir$);
      ch := ps[LEN(ps) - 2];
      IF (ch # "/") & (ch # "\") THEN
        ps := BOX(ps^ + genSep + fn);
      ELSE
        ps := BOX(ps^ + fn);
      END;
      RETURN ps;
    END GetFullPath;
   (* -------------------------------------------------	*)
  BEGIN
    IF CSt.binDir # "" THEN
      ptr := GetFullPath(fileName);
    ELSE
      ptr := BOX(fileName$);
    END;
    Warp(ptr);
(*
 *  IF GPFiles.fileSep # "/" THEN Warp(fileName) END;
 * 
 *  srcFileName := L.strToCharOpen(CSt.srcNam); 
 *  NEW(f);
 *
 *  f.file := GPBinFiles.createPath(fileName);
 *)
    srcFileName := BOX(CSt.srcNam$); 
    NEW(fil);
    fil.file := GPBinFiles.createPath(ptr);

    IF fil.file = NIL THEN RETURN NIL; END;
(*
 *  Console.WriteString("Creating file ");
 *  Console.WriteString(ptr);
 *  Console.WriteLn;
 *)
    fil.access := 0;
    NEW(fil.cp.pool,initSize); 
    fil.cp.tide := 1;
    fil.thisClassIx := 0;
    fil.superClassIx := 0;
    fil.numInterfaces := 0;
    fil.numFields := 0;
    fil.numMethods := 0;
    fil.srcFileIx := AddUTF(fil.cp,srcFileName);
    fil.srcFileAttIx := AddUTF(fil.cp,srcFileStr);
    fil.codeAttIx := AddUTF(fil.cp,codeStr);
    fil.exceptAttIx := 0;
    fil.lineNumTabIx := 0;
    fil.jlExceptIx := 0;
    RETURN fil;
  END newClassFile;

  PROCEDURE (cf : ClassFile) StartModClass*(mod : Id.BlkId);
  BEGIN
    cf.access := Jvm.acc_public + Jvm.acc_final + Jvm.acc_super; 
    cf.thisClassIx := AddModClassRef(cf.cp,mod);
    cf.superClassIx := AddClassRef(cf.cp,object);
  END StartModClass;

  PROCEDURE^ (cf : ClassFile) AddInterface*(interface : Ty.Record),NEW;
                                                
  PROCEDURE (cf : ClassFile)StartRecClass*(rec : Ty.Record);
  VAR
    clsId  : D.Idnt;
    impRec : D.Type;
    recAcc : INTEGER;
    index  : INTEGER;
  BEGIN
    recAcc := Jvm.acc_super;
    IF rec.recAtt = Ty.noAtt THEN
      recAcc := recAcc + Jvm.acc_final;
    ELSIF rec.recAtt = Ty.isAbs THEN
      recAcc := recAcc + Jvm.acc_abstract;
    END; 
    IF rec.bindTp = NIL THEN
      clsId := rec.idnt;
    ELSE
      clsId := rec.bindTp.idnt;
    END;  
    IF clsId # NIL THEN
      IF clsId.vMod = D.pubMode THEN
        recAcc := recAcc + Jvm.acc_public;
      ELSIF clsId.vMod = D.prvMode THEN
        recAcc := recAcc + Jvm.acc_package;
      END;
    END;
    cf.access := recAcc;
    cf.thisClassIx := AddRecClassRef(cf.cp,rec);
    IF rec.baseTp IS Ty.Record THEN
      IF rec.baseTp.xName = NIL THEN J.MkRecName(rec.baseTp(Ty.Record)); END;
      cf.superClassIx := AddClassRef(cf.cp,rec.baseTp.xName);
    ELSE
      cf.superClassIx := AddClassRef(cf.cp,object);
    END; 
   (*
    *   Emit interface declarations (if any)
    *)
    IF rec.interfaces.tide > 0 THEN
      FOR index := 0 TO rec.interfaces.tide-1 DO
        impRec := rec.interfaces.a[index];
        cf.AddInterface(impRec.boundRecTp()(Ty.Record));
      END;
    END;
  END StartRecClass;

(* ============================================================ *)
(*                   Java Class File Stuff                      *)
(* ============================================================ *)

  PROCEDURE (cf : ClassFile) InitFields*(numFields : INTEGER);
  BEGIN
    NEW(cf.fields,numFields);
  END InitFields;

  PROCEDURE (cf : ClassFile) AddField*(field : FieldInfo),NEW;
  CONST
    incSize = 10;
  VAR
    tmp : POINTER TO ARRAY OF FieldInfo;
    i : INTEGER;
  BEGIN
    IF cf.fields = NIL THEN
      NEW(cf.fields,incSize); 
    ELSIF cf.numFields >= LEN(cf.fields) THEN 
      tmp := cf.fields;
      NEW(cf.fields,cf.numFields+incSize); 
      FOR i := 0 TO cf.numFields-1 DO
        cf.fields[i] := tmp[i];
      END;
    END; 
    cf.fields[cf.numFields] := field;
    INC(cf.numFields);
  END AddField;

  PROCEDURE (cf : ClassFile) InitMethods*(numMethods : INTEGER);
  BEGIN
    NEW(cf.methods,numMethods);
  END InitMethods;

  PROCEDURE (cf : ClassFile)AddMethod*(method : MethodInfo),NEW;
  CONST
    incSize = 10;
  VAR
    tmp : POINTER TO ARRAY OF MethodInfo;
    i : INTEGER;
  BEGIN
    IF cf.methods = NIL THEN
      NEW(cf.methods,incSize); 
    ELSIF cf.numMethods >= LEN(cf.methods) THEN 
      tmp := cf.methods;
      NEW(cf.methods,cf.numMethods+incSize); 
      FOR i := 0 TO cf.numMethods-1 DO
        cf.methods[i] := tmp[i];
      END;
    END; 
    cf.methods[cf.numMethods] := method;
    INC(cf.numMethods);
  END AddMethod;

  PROCEDURE (cf : ClassFile) InitInterfaces*(numInterfaces : INTEGER),NEW;
  BEGIN
    NEW(cf.interfaces,numInterfaces);
  END InitInterfaces;

  PROCEDURE (cf : ClassFile) AddInterface*(interface : Ty.Record),NEW;
  CONST
    incSize = 10;
  VAR
    tmp : POINTER TO ARRAY OF INTEGER;
    i, intIx : INTEGER;
  BEGIN
    IF cf.interfaces = NIL THEN
      NEW(cf.interfaces,incSize); 
    ELSIF cf.numInterfaces >= LEN(cf.interfaces) THEN 
      tmp := cf.interfaces;
      NEW(cf.interfaces,cf.numInterfaces+incSize); 
      FOR i := 0 TO cf.numInterfaces-1 DO
        cf.interfaces[i] := tmp[i];
      END;
    END; 
    IF interface.xName = NIL THEN J.MkRecName(interface); END;
    intIx := AddRecClassRef(cf.cp,interface); 
    cf.interfaces[cf.numInterfaces] := intIx;
    INC(cf.numInterfaces);
  END AddInterface;

(* ============================================================ *)
(*			FieldInfo Methods			*)
(* ============================================================ *)

  PROCEDURE (cf : ClassFile) EmitField*(field : Id.AbVar);
  VAR  
    f : FieldInfo;
  BEGIN
    NEW(f);
    CASE field.vMod OF
    | D.prvMode : f.access := Jvm.acc_package;
    | D.pubMode : f.access := Jvm.acc_public;
    | D.rdoMode : f.access := Jvm.acc_public;
    | D.protect : f.access := Jvm.acc_protected;
    END;
    WITH field : Id.VarId DO
      f.access := f.access + Jvm.acc_static; 
      IF field.varNm = NIL THEN J.MkVarName(field(Id.VarId)); END;
      f.nameIx := AddUTF(cf.cp,field.varNm);
    | field : Id.FldId DO
      f.nameIx := AddUTF(cf.cp,D.getName.ChPtr(field));
    END;
    f.descIx := AddUTF(cf.cp, GetTypeName(field.type));
    f.constValIx := -1; (* constants not currently stored in class file *)
    cf.AddField(f);
  END EmitField;

(* ============================================================ *)
(*			MethodInfo Methods			*)
(* ============================================================ *)

  PROCEDURE newMethodInfo*(meth : Id.Procs) : MethodInfo;
    VAR m : MethodInfo;
  BEGIN
    NEW(m);
    m.methId := meth;
    IF meth = NIL THEN
      m.localNum := 0;
      m.maxLocals := 1;
    ELSE        (* Id.BlkId *)
      m.localNum := meth.rtsFram;
      m.maxLocals := MAX(meth.rtsFram, 1);
    END;
    m.currStack := 0;
    m.maxStack := 0;
    NEW(m.codes.code,initSize);
    m.codes.tide := 0;
    m.codes.codeLen := 0;
    m.lineNumTab.tide := 0;
    RETURN m;
  END newMethodInfo;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)StartProc*(proc : Id.Procs);
  VAR
    attr : INTEGER;
    method : Id.MthId;
  BEGIN
    cf.meth := newMethodInfo(proc);
    cf.AddMethod(cf.meth); 
   (*
    *  Compute the method attributes
    *)
    IF proc.kind = Id.conMth THEN
      method := proc(Id.MthId);
      attr := 0;
      IF method.mthAtt * Id.mask = {} THEN attr := Jvm.acc_final; END;
      IF method.mthAtt * Id.mask = Id.isAbs THEN
        attr := attr + Jvm.acc_abstract;
      END;
      IF Id.widen IN method.mthAtt THEN attr := attr + Jvm.acc_public END; 
    ELSE
      attr := Jvm.acc_static;
    END;
(*
 *  The following code fails for "implement-only" methods
 *  since the JVM places the "override method" in a different 
 *  slot! We must thus live with the insecurity of public mode.
 *
 *  IF proc.vMod = D.pubMode THEN	(* explicitly public *)
 *)
    IF (proc.vMod = D.pubMode) OR	(* explicitly public *)
       (proc.vMod = D.rdoMode) THEN     (* "implement only"  *)
      attr := attr + Jvm.acc_public;
    ELSIF proc.dfScp IS Id.PrcId THEN	(* nested procedure  *)
      attr := attr + Jvm.acc_private;
    END;
    cf.meth.access := attr;
    IF (cf.meth.access >= Jvm.acc_abstract) THEN
      cf.meth.maxLocals := 0;
    END;
    cf.meth.nameIx := AddUTF(cf.cp,proc.prcNm);
    cf.meth.descIx := AddUTF(cf.cp,proc.type.xName);
  END StartProc;

  PROCEDURE (cf : ClassFile)isAbstract*() : BOOLEAN;
  BEGIN
    RETURN (cf.meth.access >= Jvm.acc_abstract);
  END isAbstract;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)getScope*() : D.Scope;
  BEGIN
    RETURN cf.meth.methId;
  END getScope;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)newLocal*() : INTEGER;
    VAR ord : INTEGER;
  BEGIN
    ord := cf.meth.localNum;
    INC(cf.meth.localNum);
    IF cf.meth.localNum > cf.meth.maxLocals THEN 
      cf.meth.maxLocals := cf.meth.localNum;
    END;
    RETURN ord;
  END newLocal;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)ReleaseLocal*(i : INTEGER);
  BEGIN
   (*
    *  If you try to release not in LIFO order, the 
    *  location will not be made free again. This is safe!
    *)
    IF i+1 = cf.meth.localNum THEN DEC(cf.meth.localNum) END;
  END ReleaseLocal;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)markTop*() : INTEGER;
  BEGIN
    RETURN cf.meth.localNum;
  END markTop;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)ReleaseAll*(m : INTEGER);
  BEGIN
    cf.meth.localNum := m;
  END ReleaseAll;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)getDepth*() : INTEGER;
  BEGIN RETURN cf.meth.currStack END getDepth;

  (* ------------------------------------------ *)

  PROCEDURE (cf : ClassFile)setDepth*(i : INTEGER);
  BEGIN cf.meth.currStack := i END setDepth;


(* ============================================================ *)
(*			Init Methods				*)
(* ============================================================ *)

  PROCEDURE (cf : ClassFile)ClinitHead*();
  VAR 
    meth : MethodInfo;
    returned : BOOLEAN;
  BEGIN
    meth := newMethodInfo(NIL);
    cf.AddMethod(meth); 
    meth.access := pubStat;
    meth.nameIx := AddUTF(cf.cp,clinit);
    meth.descIx := AddUTF(cf.cp,noArgVoid);
    cf.meth := meth; 
  END ClinitHead;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)VoidTail*();
  BEGIN
    cf.Code(Jvm.opc_return);
  END VoidTail;

(* ============================================================ *)

  PROCEDURE^ (cf : ClassFile)CallS*(code : INTEGER; 
                            IN className : L.CharOpen;
                            IN procName  : L.CharOpen; 
                            IN signature : L.CharOpen; 
	                       argL,retL : INTEGER),NEW;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)MainHead*();
  VAR 
    meth : MethodInfo;
    returned : BOOLEAN;
  BEGIN
    meth := newMethodInfo(NIL);
    cf.AddMethod(meth); 
    meth.access := pubStat;
    meth.nameIx := AddUTF(cf.cp,main);
    meth.descIx := AddUTF(cf.cp,mainSig);
    cf.meth := meth; 
   (*
    *  Save the command-line arguments to the RTS.
    *)
    cf.Code(Jvm.opc_aload_0);
    cf.CallS(Jvm.opc_invokestatic,CPmainClass,putArgs,mainSig,1,0);
  END MainHead;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)ModNoArgInit*();
  VAR 
    meth : MethodInfo;
  BEGIN
    meth := newMethodInfo(NIL);
    cf.AddMethod(meth); 
    meth.access := Jvm.acc_public;
    meth.nameIx := AddUTF(cf.cp,init);
    meth.descIx := AddUTF(cf.cp,noArgVoid);
    cf.meth := meth; 
    cf.Code(Jvm.opc_aload_0);
    cf.CallS(Jvm.opc_invokespecial,object,init,noArgVoid,1,0);
    cf.Code(Jvm.opc_return);
  END ModNoArgInit;

(* ---------------------------------------------------- *)

  PROCEDURE (cf : ClassFile)RecMakeInit*(rec : Ty.Record;
					 prc : Id.PrcId);
    VAR meth : MethodInfo;
	pTp : Ty.Procedure;
	signature : L.CharOpen;
  BEGIN
    IF (prc = NIL) & 
       ((D.noNew IN rec.xAttr) OR (D.xCtor IN rec.xAttr)) THEN 
      RETURN;                                       (* PREMATURE RETURN HERE *)
    END;
    meth := newMethodInfo(prc);
    cf.AddMethod(meth); 
    cf.meth := meth; 
    cf.Code(Jvm.opc_aload_0);
    meth.access := Jvm.acc_public;
    meth.nameIx := AddUTF(cf.cp,init);
   (*
    *  Get the procedure type, if any.
    *)
    IF prc # NIL THEN
      pTp := prc.type(Ty.Procedure);
      J.MkCallAttr(prc, pTp);
      signature := pTp.xName;
    ELSE
      pTp := NIL;
      signature := noArgVoid;
    END;
    meth.descIx := AddUTF(cf.cp,signature);
  END RecMakeInit;

(*
 *  IF pTp # NIL THEN
 *   (*
 *    *  Copy the args to the super-constructor
 *    *)
 *    FOR idx := 0 TO pNm-1 DO cf.GetLocal(pTp.formals.a[idx]) END;
 *  END;
 *)

  PROCEDURE (cf : ClassFile)CallSuperCtor*(rec : Ty.Record;
					   pTy : Ty.Procedure);
    VAR idx : INTEGER;
	fld : D.Idnt;
	pNm : INTEGER;
        initClass : L.CharOpen;
	signature : L.CharOpen;
  BEGIN
    IF pTy # NIL THEN
      pNm := pTy.formals.tide;
      signature := pTy.xName;
    ELSE
      pNm := 0;			(* was 1 *)
      signature := noArgVoid;
    END;
   (*
    *    Initialize the embedded superclass object.
    *)
    IF (rec.baseTp # NIL) & (rec.baseTp # G.anyRec) THEN
      initClass := rec.baseTp(Ty.Record).xName;
    ELSE
      initClass := object; 
    END;
    cf.CallS(Jvm.opc_invokespecial, initClass, init, signature, pNm+1, 0);
   (*
    *    Initialize fields, as necessary.
    *)
    FOR idx := 0 TO rec.fields.tide-1 DO
      fld := rec.fields.a[idx];
      IF (fld.type IS Ty.Record) OR (fld.type IS Ty.Array) THEN
	cf.Code(Jvm.opc_aload_0);
	cf.VarInit(fld);
        cf.PutGetF(Jvm.opc_putfield, rec, fld(Id.FldId));
      END;
    END;
(*
 *  cf.Code(Jvm.opc_return);
 *)
  END CallSuperCtor;

(* ============================================================ *)

  PROCEDURE makeClassVoidArgList(rec : Ty.Record) : L.CharOpen;
  BEGIN
    IF rec.xName = NIL THEN J.MkRecName(rec); END;
    RETURN J.cat3(J.lPar,rec.scopeNm,J.rParV); 
  END makeClassVoidArgList;
  
(* ---------------------------------------------------- *)

  PROCEDURE (cf : ClassFile)CopyProcHead*(rec : Ty.Record);
  VAR
    meth : MethodInfo;
  BEGIN
    meth := newMethodInfo(NIL);
    cf.AddMethod(meth); 
    meth.access := Jvm.acc_public;
    meth.nameIx := AddUTF(cf.cp,copy);
    meth.descIx := AddUTF(cf.cp,makeClassVoidArgList(rec));
    cf.meth := meth; 
  END CopyProcHead;

(* ============================================================ *)
(*			Private Methods				*)
(* ============================================================ *)

  PROCEDURE (meth : MethodInfo)FixStack(code : INTEGER),NEW;
  BEGIN
    INC(meth.currStack, Jvm.dl[code]);
    IF meth.currStack > meth.maxStack THEN meth.maxStack := meth.currStack END;
  END FixStack;

(* ============================================================ *)

  PROCEDURE GetTypeName*(typ : D.Type) : L.CharOpen;
  VAR
    arrayName : L.CharOpenSeq;
    arrayTy : D.Type;
  BEGIN
    WITH typ : Ty.Base DO
        RETURN typ.xName;
    | typ : Ty.Vector DO
        IF typ.xName = NIL THEN J.MkVecName(typ) END;
        RETURN typ.xName;
    | typ : Ty.Array DO
        IF typ.xName = NIL THEN
          L.InitCharOpenSeq(arrayName,3);
          arrayTy := typ;
          WHILE arrayTy IS Ty.Array DO 
            L.AppendCharOpen(arrayName,J.brac); 
            arrayTy := arrayTy(Ty.Array).elemTp;
          END;
          L.AppendCharOpen(arrayName,GetTypeName(arrayTy)); 
          typ.xName := L.arrayCat(arrayName);
        END;
        ASSERT(typ.xName # NIL);
        RETURN typ.xName;
    | typ : Ty.Record DO
	IF typ.xName = NIL THEN J.MkRecName(typ) END;
        RETURN typ.scopeNm;
    | typ : Ty.Enum DO
	RETURN G.intTp.xName;
    | typ : Ty.Pointer DO
        RETURN GetTypeName(typ.boundTp);
    | typ : Ty.Opaque DO
	IF typ.xName = NIL THEN J.MkAliasName(typ) END;
        RETURN typ.scopeNm;
    END;
  END GetTypeName;

(* ============================================================ *)
(*			Exported Methods			*)
(* ============================================================ *)

  PROCEDURE (cf : ClassFile)newLabel*() : J.Label;
  VAR
    lab : J.Label;
  BEGIN
    NEW(lab);
    lab.defIx := 0;
    RETURN lab;
  END newLabel;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)getLabelRange*(VAR labs : ARRAY OF J.Label);
  VAR
    i : INTEGER;
  BEGIN
    FOR i := 0 TO LEN(labs)-1 DO
      NEW(labs[i]);
      labs[i].defIx := 0;
    END;
  END getLabelRange;

(* ============================================================ *)

  PROCEDURE (VAR lst : CodeList)AddInstruction(op : Op),NEW;
  VAR
    tmp : POINTER TO ARRAY OF Op;
    i : INTEGER;
  BEGIN
    ASSERT(lst.code # NIL);
    IF lst.tide >= LEN(lst.code) THEN
      tmp := lst.code;
      NEW(lst.code,2 * lst.tide);
      FOR i := 0 TO lst.tide-1 DO
        lst.code[i] := tmp[i];
      END;
    END;
    lst.code[lst.tide] := op;
    INC(lst.tide); 
  END AddInstruction;

(* -------------------------------------------- *)
 
  PROCEDURE (cf : ClassFile)DefLab*(lab : J.Label);
  BEGIN
    ASSERT(lab.defIx = 0);
    lab.defIx := cf.meth.codes.codeLen;
  END DefLab;

  PROCEDURE (cf : ClassFile)DefLabC*(lab : J.Label; IN c : ARRAY OF CHAR);
  BEGIN
    ASSERT(lab.defIx = 0);
    lab.defIx := cf.meth.codes.codeLen;
  END DefLabC;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)AddSwitchLab*(lab : J.Label; pos : INTEGER);
  VAR
    sw : OpSwitch;
  BEGIN
    sw := cf.meth.codes.code[cf.meth.codes.tide-1](OpSwitch);
    sw.offs[pos] := lab;
  END AddSwitchLab;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)CodeLb*(code : INTEGER; lab : J.Label);
  VAR
    tmp : POINTER TO ARRAY OF INTEGER;
    i : INTEGER;
    op : OpL;
  BEGIN
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    op.lab := lab;
    INC(cf.meth.codes.codeLen,3);
    cf.meth.codes.AddInstruction(op);
    cf.meth.FixStack(code);
  END CodeLb;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)Code*(code : INTEGER);
  VAR
    op : Op;
  BEGIN
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    INC(cf.meth.codes.codeLen);
    cf.meth.codes.AddInstruction(op);
    cf.meth.FixStack(code);
  END Code;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)CodeI*(code,val : INTEGER);
  VAR
    op : OpI;
  BEGIN
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    op.val := val;
    IF (val > maxUnsignedByte) & 
       (((code >= Jvm.opc_iload) & (code <= Jvm.opc_aload)) OR
       ((code >= Jvm.opc_istore) & (code <= Jvm.opc_astore))) THEN
      cf.Code(Jvm.opc_wide);
      op.numBytes := 2;
      INC(cf.meth.codes.codeLen,3);
    ELSE
      op.numBytes := 1;
      INC(cf.meth.codes.codeLen,2);
    END; 
    cf.meth.codes.AddInstruction(op);
    cf.meth.FixStack(code);
  END CodeI;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)Code2I*(code,val : INTEGER; updateS : BOOLEAN),NEW;
  VAR
    op : OpI;
  BEGIN
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    op.val := val;
    op.numBytes := 2;
    INC(cf.meth.codes.codeLen,3);
    cf.meth.codes.AddInstruction(op);
    IF updateS THEN cf.meth.FixStack(code); END;
  END Code2I;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)Code4I*(code,val : INTEGER),NEW;
  VAR
    op : OpI;
  BEGIN
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    op.val := val;
    op.numBytes := 4;
    INC(cf.meth.codes.codeLen,5);
    cf.meth.codes.AddInstruction(op);
    cf.meth.FixStack(code);
  END Code4I;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)Code2IB*(code,val,bVal : INTEGER;
                                     endZero : BOOLEAN; updateS : BOOLEAN),NEW;
  VAR
    op : Op2IB;
    instSize : INTEGER;
  BEGIN
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    op.val := val;
    op.bVal := bVal;
    op.trailingZero := endZero;
    IF endZero THEN INC(cf.meth.codes.codeLen,5);
    ELSE INC(cf.meth.codes.codeLen,4); END;
    cf.meth.codes.AddInstruction(op);
    IF updateS THEN cf.meth.FixStack(code); END;
  END Code2IB;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)CodeL*(code : INTEGER; num : LONGINT);
  VAR
    conIx : INTEGER;
  BEGIN
    conIx := AddConstLong(cf.cp,num);
    cf.Code2I(Jvm.opc_ldc2_w, conIx, TRUE);
  END CodeL;

  PROCEDURE (cf : ClassFile)CodeR*(code : INTEGER; num : REAL; short : BOOLEAN);
  VAR
    conIx : INTEGER;
  BEGIN
    IF short THEN 
      conIx := AddConstFloat(cf.cp,SHORT(num));
      IF conIx > maxUnsignedByte THEN
        cf.Code2I(Jvm.opc_ldc_w, conIx, TRUE);
      ELSE
        cf.CodeI(Jvm.opc_ldc, conIx);
      END;
    ELSE 
      conIx := AddConstDouble(cf.cp,num);
      cf.Code2I(Jvm.opc_ldc2_w, conIx, TRUE);
    END;
  END CodeR;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)CodeInc*(localIx,incVal : INTEGER);
  VAR
    op : OpII;
    needWide : BOOLEAN;
  BEGIN
    needWide := (localIx > maxUnsignedByte) OR (incVal < MIN(BYTE)) OR 
                (incVal > MAX(BYTE));
    IF needWide THEN cf.Code(Jvm.opc_wide); END;
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := Jvm.opc_iinc;
    op.val1 := localIx;
    op.val2 := incVal;
    IF needWide THEN
      op.numBytes := 2;
      INC(cf.meth.codes.codeLen,5);
    ELSE
      op.numBytes := 1;
      INC(cf.meth.codes.codeLen,3);
    END;
    cf.meth.codes.AddInstruction(op);
  END CodeInc;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)CodeSwitch*(low,high : INTEGER; defLab : J.Label);
  VAR
    sw : OpSwitch;
    len : INTEGER;
  BEGIN
    NEW(sw);
    sw.offset := cf.meth.codes.codeLen;
    sw.op := Jvm.opc_tableswitch;
    sw.defLabel := defLab;
    sw.low := low;
    sw.high := high;
    len := high-low+1;
    NEW(sw.offs,len); 
    sw.padding := 3 - (sw.offset MOD 4);
    INC(cf.meth.codes.codeLen,13+sw.padding+4*len);
    cf.meth.codes.AddInstruction(sw);
    cf.meth.FixStack(Jvm.opc_tableswitch);
  END CodeSwitch;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)CodeT*(code : INTEGER; ty : D.Type);
  VAR
    op : OpI;
  BEGIN
    IF ty IS Ty.Pointer THEN ty := ty(Ty.Pointer).boundTp; END;
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
(* 
 *  // old code ...
 *  op.val := AddRecClassRef(cf.cp,ty(Ty.Record));
 *  // now new code ...
 *)
    IF ty IS Ty.Record THEN
      op.val := AddRecClassRef(cf.cp, ty(Ty.Record)); 
    ELSE
      op.val := AddClassRef(cf.cp, GetTypeName(ty)); 
    END;

    op.numBytes := 2;
    INC(cf.meth.codes.codeLen,3);
    cf.meth.codes.AddInstruction(op);
    cf.meth.FixStack(code);
  END CodeT;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)CodeC*(code : INTEGER; IN str : ARRAY OF CHAR);
  VAR
    op : Op;
  BEGIN
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    INC(cf.meth.codes.codeLen);
    cf.meth.codes.AddInstruction(op);
    cf.meth.FixStack(code);
  END CodeC;

(* -------------------------------------------- *)
  PROCEDURE (cf : ClassFile)PushStr*(IN str : L.CharOpen);
  (* Use target quoting conventions for the literal string *)
  VAR
    strIx : INTEGER;
  BEGIN
    strIx := AddStringRef(cf.cp,str);
    IF strIx > maxUnsignedByte THEN  
      cf.Code2I(Jvm.opc_ldc_w, strIx, TRUE);
    ELSE
      cf.CodeI(Jvm.opc_ldc, strIx);
    END;
  END PushStr;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)CallS*(code : INTEGER; 
			  	   IN className : L.CharOpen;
                                   IN procName  : L.CharOpen;
                                   IN signature : L.CharOpen; 
				      argL,retL : INTEGER),NEW;
  VAR
    cIx,mIx : INTEGER;
  BEGIN
    ASSERT(code # Jvm.opc_invokeinterface);
    cIx := AddClassRef(cf.cp,className);
    mIx := AddMethodRef(cf.cp,cIx,procName,signature);
    cf.Code2I(code,mIx,FALSE);
    INC(cf.meth.currStack, retL-argL);
    IF cf.meth.currStack > cf.meth.maxStack THEN 
      cf.meth.maxStack := cf.meth.currStack;
    END;
  END CallS;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)CallIT*(code : INTEGER; 
				   proc : Id.Procs; 
				   type : Ty.Procedure);
    VAR cIx,mIx : INTEGER;
        scp : D.Scope;
  BEGIN
    IF proc.scopeNm = NIL THEN J.MkProcName(proc) END;
    WITH proc : Id.PrcId DO 
      cIx := AddClassRef(cf.cp,proc.clsNm);
    |    proc : Id.MthId DO 
      cIx := AddRecClassRef(cf.cp,proc.bndType(Ty.Record));
    END;
    IF code = Jvm.opc_invokeinterface THEN 
      mIx := AddInterfaceMethodRef(cf.cp,cIx,proc.prcNm,proc.type.xName); 
      cf.Code2IB(code,mIx,type.argN,TRUE,FALSE);
    ELSE 
      mIx := AddMethodRef(cf.cp,cIx,proc.prcNm,proc.type.xName);
      cf.Code2I(code,mIx,FALSE);
    END;
    INC(cf.meth.currStack, type.retN-type.argN);
    IF cf.meth.currStack > cf.meth.maxStack THEN 
      cf.meth.maxStack := cf.meth.currStack;
    END;
  END CallIT;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)MultiNew*(arrName : L.CharOpen;
				     dms : INTEGER),NEW;
   (* dsc is the array descriptor, dms the number of dimensions *)
  VAR
    classIx : INTEGER;
  BEGIN
    classIx := AddClassRef(cf.cp,arrName);
    cf.Code2IB(Jvm.opc_multianewarray,classIx,dms,FALSE,TRUE);
    DEC(cf.meth.currStack, dms-1);
  END MultiNew;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)PutGetS*(code : INTEGER;
				    blk  : Id.BlkId;
				    fld  : Id.VarId);
    VAR size : INTEGER;
        classIx : INTEGER;
        fieldIx : INTEGER;
        op : OpI;
  (* Emit putstatic and getstatic for static field *)
  BEGIN
    IF blk.xName = NIL THEN J.MkBlkName(blk) END;
    IF fld.varNm = NIL THEN J.MkVarName(fld) END;
    IF fld.recTyp = NIL THEN
      classIx := AddModClassRef(cf.cp,blk);
    ELSE
      classIx := AddRecClassRef(cf.cp,fld.recTyp(Ty.Record));
    END;
    fieldIx := AddFieldRef(cf.cp,classIx,fld.varNm,GetTypeName(fld.type));
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    op.val := fieldIx;
    op.numBytes := 2;
    INC(cf.meth.codes.codeLen,3);
    cf.meth.codes.AddInstruction(op);
    size := J.jvmSize(fld.type);
    IF    code = Jvm.opc_getstatic THEN INC(cf.meth.currStack, size);
    ELSIF code = Jvm.opc_putstatic THEN DEC(cf.meth.currStack, size);
    END;
    IF cf.meth.currStack > cf.meth.maxStack THEN 
      cf.meth.maxStack := cf.meth.currStack 
    END;
  END PutGetS;

(* -------------------------------------------- *)

  PROCEDURE (cf : ClassFile)PutGetF*(code : INTEGER;
				    rec  : Ty.Record;
				    fld  : Id.AbVar);
    VAR size : INTEGER;
        classIx : INTEGER;
        fieldIx : INTEGER;
        op : OpI;
  (* Emit putfield and getfield for record field *)
  BEGIN
    classIx := AddRecClassRef(cf.cp,rec);
    fieldIx := AddFieldRef(cf.cp,classIx,D.getName.ChPtr(fld),
                           GetTypeName(fld.type));
    NEW(op);
    op.offset := cf.meth.codes.codeLen;
    op.op := code;
    op.val := fieldIx;
    op.numBytes := 2;
    INC(cf.meth.codes.codeLen,3);
    cf.meth.codes.AddInstruction(op);
    size := J.jvmSize(fld.type);
    IF    code = Jvm.opc_getfield THEN INC(cf.meth.currStack, size-1);
    ELSIF code = Jvm.opc_putfield THEN DEC(cf.meth.currStack, size+1);
    END;
    IF cf.meth.currStack > cf.meth.maxStack THEN 
      cf.meth.maxStack := cf.meth.currStack;
    END;
  END PutGetF;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)Alloc1d*(elTp : D.Type);
  VAR
    tName : L.CharOpen;
    classIx : INTEGER;
  BEGIN
    WITH elTp : Ty.Base DO
      IF (elTp.tpOrd < Ty.anyRec) OR (elTp.tpOrd = Ty.uBytN) THEN
        cf.CodeI(Jvm.opc_newarray, typeArr[elTp.tpOrd]);
      ELSE
        classIx := AddClassRef(cf.cp,object);
        cf.Code2I(Jvm.opc_anewarray,classIx,TRUE); 
      END;
    ELSE
      IF elTp IS Ty.Pointer THEN elTp := elTp(Ty.Pointer).boundTp; END;
      IF elTp IS Ty.Record THEN
        classIx := AddRecClassRef(cf.cp,elTp(Ty.Record)); 
      ELSE
        classIx := AddClassRef(cf.cp,GetTypeName(elTp)); 
      END;
      cf.Code2I(Jvm.opc_anewarray,classIx,TRUE); 
    END;
  END Alloc1d;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)MkNewRecord*(typ : Ty.Record);
  VAR
    methIx,classIx : INTEGER;
  BEGIN
    classIx := AddRecClassRef(cf.cp,typ);
    cf.Code2I(Jvm.opc_new,classIx,TRUE);
    cf.Code(Jvm.opc_dup);
    methIx := AddMethodRef(cf.cp,classIx,init,noArgVoid); 
    cf.Code2I(Jvm.opc_invokespecial,methIx,TRUE); 
  END MkNewRecord;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)MkNewFixedArray*(topE : D.Type; len0 : INTEGER);
    VAR dims : INTEGER;
	arTp : Ty.Array;
	elTp : D.Type;
  BEGIN
    (*
    //  Fixed-size, possibly multi-dimensional arrays.
    //  The code relies on the semantic property in CP
    //  that the element-type of a fixed array type cannot
    //  be an open array. This simplifies the code somewhat.
    *)
    cf.PushInt(len0);
    dims := 1;
    elTp := topE;
   (*
    *  Find the number of dimensions ...
    *)
    LOOP
      WITH elTp : Ty.Array DO arTp := elTp ELSE EXIT END;
      elTp := arTp.elemTp;
      cf.PushInt(arTp.length);
      INC(dims);
    END;
    IF dims = 1 THEN
      cf.Alloc1d(elTp);
     (*
      *  Stack is (top) len0, ref...
      *)
      IF elTp.kind = Ty.recTp THEN cf.Init1dArray(elTp, len0) END;
    ELSE
     (*
      *  Allocate the array headers for all dimensions.
      *  Stack is (top) lenN, ... len0, ref...
      *)
      cf.MultiNew(cat2(J.brac,GetTypeName(topE)), dims);
     (*
      *  Stack is (top) ref...
      *)
      IF elTp.kind = Ty.recTp THEN cf.InitNdArray(topE, elTp) END;
    END;
  END MkNewFixedArray;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)MkNewOpenArray*(arrT : Ty.Array;dims : INTEGER);
    VAR elTp : D.Type;
	indx : INTEGER;
  BEGIN
   (* 
    *  Assert: lengths are pushed already...
    *  and we know from semantic analysis that
    *  the number of open array dimensions match
    *  the number of integer LENs in dims.
    *)
    elTp := arrT;
   (*
    *   Find the number of dimensions ...
    *)
    FOR indx := 0 TO dims-1 DO
      elTp := elTp(Ty.Array).elemTp;
    END;
   (*
    *   Allocate the array headers for all _open_ dimensions.
    *)
    IF dims = 1 THEN
      cf.Alloc1d(elTp);
     (*
      *  Stack is now (top) ref ...
      *  and we _might_ need to initialize the elements.
      *)
      IF (elTp.kind = Ty.recTp) OR 
	 (elTp.kind = Ty.arrTp) THEN 
	cf.Init1dArray(elTp, 0);
      END;
    ELSE
      cf.MultiNew(GetTypeName(arrT), dims);
     (*
      *    Stack is now (top) ref ...
      *    Now we _might_ need to initialize the elements.
      *)
      IF (elTp.kind = Ty.recTp) OR 
	 (elTp.kind = Ty.arrTp) THEN 
	cf.InitNdArray(arrT.elemTp, elTp);
      END;
    END;
  END MkNewOpenArray;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)MkArrayCopy*(arrT : Ty.Array);
    VAR dims : INTEGER;
        elTp : D.Type;
  BEGIN
   (*
    *	Assert: we must find the lengths from the runtime 
    *   descriptors.  Find the number of dimensions.  The 
    *   array to copy is on the top of stack, which reads -
    *		(top) aRef, ...
    *)
    elTp := arrT.elemTp;
    IF elTp.kind # Ty.arrTp THEN
      cf.Code(Jvm.opc_arraylength);     (* (top) len0, aRef,...	*)
      cf.Alloc1d(elTp);			     (* (top) aRef, ...		*)
      IF elTp.kind = Ty.recTp THEN cf.Init1dArray(elTp, 0) END; (*0 ==> open*)
    ELSE
      dims := 1;
      REPEAT
       (* 
        *  Invariant: an array reference is on the top of
        *  of the stack, which reads:
        *		(top) [arRf, lengths,] arRf ...
	*)
	INC(dims);
        elTp := elTp(Ty.Array).elemTp;
        cf.Code(Jvm.opc_dup);	     (*          arRf, arRf,... *)
        cf.Code(Jvm.opc_arraylength);   (*    len0, arRf, arRf,... *)
        cf.Code(Jvm.opc_swap);	     (*    arRf, len0, arRf,... *)
        cf.Code(Jvm.opc_iconst_0);	     (* 0, arRf, len0, arRf,... *)
        cf.Code(Jvm.opc_aaload);	     (*    arRf, len0, arRf,... *)
       (* 
        *  Stack reads:	(top) arRf, lenN, [lengths,] arRf ...
	*)
      UNTIL  elTp.kind # Ty.arrTp;
     (*
      *  Now get the final length...
      *)
      cf.Code(Jvm.opc_arraylength);  
     (* 
      *   Stack reads:	(top) lenM, lenN, [lengths,] arRf ...
      *   Allocate the array headers for all dimensions.
      *)
      cf.MultiNew(GetTypeName(arrT), dims);
     (*
      *  Stack is (top) ref...
      *)
      IF elTp.kind = Ty.recTp THEN cf.InitNdArray(arrT.elemTp, elTp) END;
    END;
  END MkArrayCopy;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)VarInit*(var : D.Idnt);
    VAR typ : D.Type;
  BEGIN
   (*
    *  Precondition: var is of a type that needs initialization
    *)
    typ := var.type;
    WITH typ : Ty.Record DO
	cf.MkNewRecord(typ);
    | typ : Ty.Array DO
	cf.MkNewFixedArray(typ.elemTp, typ.length);
    ELSE
      cf.Code(Jvm.opc_aconst_null);
    END;
  END VarInit;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)ValRecCopy*(typ : Ty.Record);
  BEGIN
   (*
    *     Stack at entry is (top) srcRef, dstRef...
    *)
    IF typ.xName = NIL THEN J.MkRecName(typ) END;
    cf.CallS(Jvm.opc_invokevirtual, typ.xName, copy, 
                                    makeClassVoidArgList(typ), 2, 0);
  END ValRecCopy;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)CallRTS*(ix,args,ret : INTEGER);
  VAR
    className : L.CharOpen;
  BEGIN
    IF ix = J.ToUpper THEN 
      className := charClass;
    ELSIF ix = J.DFloor THEN 
      className := mathClass;
    ELSIF ix = J.SysExit THEN 
      className := sysClass;
    ELSE
      className := rtsClass;
    END;
    cf.CallS(Jvm.opc_invokestatic,className,procNames[ix],procSigs[ix],args,ret);
  END CallRTS;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)CallGetClass*();
  BEGIN
    cf.CallS(Jvm.opc_invokevirtual, object, getCls, noArgClass, 1, 1);
  END CallGetClass;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)Trap*(IN str : ARRAY OF CHAR);
  VAR
   clIx : INTEGER;
  BEGIN
    clIx := AddClassRef(cf.cp,errorClass);
    cf.Code2I(Jvm.opc_new,clIx,TRUE);
    cf.Code(Jvm.opc_dup);
    cf.PushStr(L.strToCharOpen(str));
    cf.CallS(Jvm.opc_invokespecial,errorClass,init,errorInitSig,2,0);
    cf.Code(Jvm.opc_athrow);
  END Trap;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)CaseTrap*(i : INTEGER);
  VAR
   clIx : INTEGER;
  BEGIN
    clIx := AddClassRef(cf.cp,errorClass);
    cf.Code2I(Jvm.opc_new,clIx,TRUE);
    cf.Code(Jvm.opc_dup);
    cf.LoadLocal(i, G.intTp);
    cf.CallS(Jvm.opc_invokestatic,rtsClass,caseTrap,caseTrapSig,1,1);
    cf.CallS(Jvm.opc_invokespecial,errorClass,init,errorInitSig,2,0);
    cf.Code(Jvm.opc_athrow);
  END CaseTrap;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)WithTrap*(id : D.Idnt);
  VAR
   clIx : INTEGER;
  BEGIN
    clIx := AddClassRef(cf.cp,errorClass);
    cf.Code2I(Jvm.opc_new,clIx,TRUE);
    cf.Code(Jvm.opc_dup);
    cf.GetVar(id);
    cf.CallS(Jvm.opc_invokestatic,rtsClass,withTrap,withTrapSig,1,1);
    cf.CallS(Jvm.opc_invokespecial,errorClass,init,errorInitSig,2,0);
    cf.Code(Jvm.opc_athrow);
  END WithTrap;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)Line*(nm : INTEGER);
  VAR
    tmpStart, tmpNum : POINTER TO ARRAY OF INTEGER; 
    i : INTEGER;
  BEGIN
    IF cf.lineNumTabIx = 0 THEN
      cf.lineNumTabIx := AddUTF(cf.cp,lineNumTabStr);
    END;
    IF cf.meth.lineNumTab.start = NIL THEN
      NEW(cf.meth.lineNumTab.start,initSize);
      NEW(cf.meth.lineNumTab.lineNum,initSize);
    ELSIF cf.meth.lineNumTab.tide >= LEN(cf.meth.lineNumTab.start) THEN
      tmpStart := cf.meth.lineNumTab.start; 
      tmpNum := cf.meth.lineNumTab.lineNum; 
      NEW(cf.meth.lineNumTab.start,cf.meth.lineNumTab.tide + initSize);
      NEW(cf.meth.lineNumTab.lineNum,cf.meth.lineNumTab.tide + initSize);
      FOR i := 0 TO cf.meth.lineNumTab.tide-1 DO
        cf.meth.lineNumTab.start[i] := tmpStart[i];  
        cf.meth.lineNumTab.lineNum[i] := tmpNum[i];  
      END;
    END;
    cf.meth.lineNumTab.start[cf.meth.lineNumTab.tide] := cf.meth.codes.codeLen;
    cf.meth.lineNumTab.lineNum[cf.meth.lineNumTab.tide] := nm;
    INC(cf.meth.lineNumTab.tide);
  END Line;

(* ============================================================ *)
(*			Namehandling Methods			*)
(* ============================================================ *)

  PROCEDURE cat2(i,j : L.CharOpen) : L.CharOpen;
  BEGIN
    L.ResetCharOpenSeq(J.nmArray);
    L.AppendCharOpen(J.nmArray, i);
    L.AppendCharOpen(J.nmArray, j);
    RETURN L.arrayCat(J.nmArray);
  END cat2;

(* ------------------------------------------------------------ *)


  PROCEDURE (cf : ClassFile)LoadConst*(num : INTEGER);
  VAR
    conIx : INTEGER;
  BEGIN
    IF (num >= MIN(SHORTINT)) & (num <= MAX(SHORTINT)) THEN
      cf.Code2I(Jvm.opc_sipush, num,TRUE);
    ELSE
      conIx := AddConstInt(cf.cp,num);
      IF conIx > maxUnsignedByte THEN  
        cf.Code2I(Jvm.opc_ldc_w, conIx,TRUE);
      ELSE
        cf.CodeI(Jvm.opc_ldc, conIx);
      END;
    END;
  END LoadConst;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)Try*();
    VAR start : INTEGER;
  BEGIN
    NEW(cf.meth.except);
    cf.meth.except.start := cf.meth.codes.codeLen;
    IF cf.jlExceptIx = 0 THEN
      cf.jlExceptIx := AddClassRef(cf.cp,exceptType);
    END;
  END Try;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)MkNewException*();
  BEGIN
    IF cf.jlExceptIx = 0 THEN
      cf.jlExceptIx := AddClassRef(cf.cp,exceptType);
    END;
    cf.Code2I(Jvm.opc_new, cf.jlExceptIx,TRUE);
  END MkNewException; 

  PROCEDURE (cf : ClassFile)InitException*();
  BEGIN
    cf.CallS(Jvm.opc_invokespecial, exceptType, init, errorInitSig, 2, 0);
  END InitException;

(* ------------------------------------------------------------ *)

  PROCEDURE (cf : ClassFile)Catch*(prc : Id.Procs);
  BEGIN
    cf.meth.except.endAndHandler := cf.meth.codes.codeLen;
    cf.StoreLocal(prc.except.varOrd, NIL);
   (*
    *  Now make sure that the overall stack
    *  depth computation is correctly initialized
    *)
    IF cf.meth.maxStack < 1 THEN cf.meth.maxStack := 1 END;
    cf.meth.currStack := 0;
  END Catch;

(* ============================================================ *)
(* ============================================================ *)
(*                Class File Writing Procedures                 *)
(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE u2 (file : F.FILE; val : INTEGER);
  VAR  
    b1,b2 : INTEGER;
  BEGIN
    b1 := val MOD 256; 
    b2 := val DIV 256;
    F.WriteByte(file,b2);
    F.WriteByte(file,b1);
  END u2;

(* ------------------------------------------------------------ *)

  PROCEDURE u4 (file : F.FILE; val : INTEGER);
  VAR
    b1,b2,b3,b4 : INTEGER;
  BEGIN
    b1 := val MOD 256; val := val DIV 256;
    b2 := val MOD 256; val := val DIV 256;
    b3 := val MOD 256; val := val DIV 256;
    b4 := val;
    F.WriteByte(file,b4);
    F.WriteByte(file,b3);
    F.WriteByte(file,b2);
    F.WriteByte(file,b1);
  END u4;

(* ============================================================ *)

  PROCEDURE WriteVal(file : F.FILE; val : INTEGER; numBytes : INTEGER);
  BEGIN
    CASE numBytes OF
    | 1 : F.WriteByte(file,val);
    | 2 : u2(file,val);
    | 4 : u4(file,val);
    END;
  END WriteVal;

  PROCEDURE (IN codes : CodeList)Dump(file : F.FILE),NEW;
  VAR
    i,j : INTEGER;
    op : Op;
    offset : INTEGER;
  BEGIN
    FOR i := 0 TO codes.tide-1 DO
      op := codes.code[i];
      F.WriteByte(file,op.op);
      WITH op : OpI DO
          WriteVal(file,op.val,op.numBytes); 
      | op : OpII DO 
          WriteVal(file,op.val1,op.numBytes); 
          WriteVal(file,op.val2,op.numBytes); 
      | op : OpL DO 
          offset := op.lab.defIx - op.offset;
          u2(file,offset); 
      | op : Op2IB DO 
          u2(file,op.val);
          F.WriteByte(file,op.bVal);
          IF op.trailingZero THEN F.WriteByte(file,0); END;
      | op : OpSwitch DO
          FOR j := 0 TO op.padding-1 DO F.WriteByte(file,0); END; 
          u4(file,(op.defLabel.defIx - op.offset));
          u4(file,op.low);
          u4(file,op.high);
          FOR j := 0 TO LEN(op.offs)-1 DO
            offset := op.offs[j].defIx - op.offset;
            u4(file,offset);
          END;
      ELSE (* nothing to do *)
      END;
    END;
  END Dump;

(* ============================================================ *)

  PROCEDURE (meth : MethodInfo)Dump(cf : ClassFile),NEW;
  VAR 
    i,len : INTEGER;
    linNumAttSize : INTEGER; 
  BEGIN
    u2(cf.file,meth.access);
    u2(cf.file,meth.nameIx);
    u2(cf.file,meth.descIx);
    IF (meth.access >= Jvm.acc_abstract) THEN
      u2(cf.file,0);         (* no attributes *)
    ELSE
      u2(cf.file,1);         (* only attribute is code *)
      (* Start of Code attribute *)
      (* Calculate size of code attribute *)
      IF meth.lineNumTab.tide > 0 THEN
        linNumAttSize := 8 + 4 * meth.lineNumTab.tide;
      ELSE
        linNumAttSize := 0;
      END;
      len := 12 + meth.codes.codeLen + linNumAttSize;
      IF meth.except # NIL THEN INC(len,8); END;
      u2(cf.file,cf.codeAttIx);
      u4(cf.file,len);
      u2(cf.file,meth.maxStack);
      u2(cf.file,meth.maxLocals);
      u4(cf.file,meth.codes.codeLen);
      meth.codes.Dump(cf.file);
      IF meth.except # NIL THEN
        u2(cf.file,1);
        u2(cf.file,meth.except.start);
        u2(cf.file,meth.except.endAndHandler);
        u2(cf.file,meth.except.endAndHandler);
        u2(cf.file,cf.jlExceptIx);
      ELSE
        u2(cf.file,0);
      END; 
      IF meth.lineNumTab.tide > 0 THEN
        u2(cf.file,1);
        (* Start of line number table attribute *)
        u2(cf.file,cf.lineNumTabIx);
        u4(cf.file,linNumAttSize-6);
        u2(cf.file,meth.lineNumTab.tide);
        FOR i := 0 TO meth.lineNumTab.tide-1 DO
          u2(cf.file,meth.lineNumTab.start[i]);
          u2(cf.file,meth.lineNumTab.lineNum[i]);
        END; 
        (* End of line number table attribute *)
      ELSE
        u2(cf.file,0);
      END;
      (* End of Code attribute *)
    END;
  END Dump;

(* ------------------------------------------------------------ *)

  PROCEDURE (field : FieldInfo)Dump(cf : ClassFile),NEW;
  BEGIN
    u2(cf.file,field.access);
    u2(cf.file,field.nameIx);
    u2(cf.file,field.descIx);
    u2(cf.file,0);  (* No attributes for fields.  ConstantValue is the     *)
                    (* only attribute recognized for fields, but constants *)
                    (* are not currently stored in the class file          *)
  END Dump;

(* ============================================================ *)

  PROCEDURE (e : CPEntry)Dump(file : F.FILE),NEW,ABSTRACT;

  PROCEDURE (u : UTF8)Dump(file : F.FILE);
  VAR 
    buf : POINTER TO ARRAY OF INTEGER;
    num : INTEGER;
    idx : INTEGER;
    chr : INTEGER;
   (* ================================= *)
    PROCEDURE Expand(VAR b : POINTER TO ARRAY OF INTEGER);
      VAR old : POINTER TO ARRAY OF INTEGER; len, idx : INTEGER;
    BEGIN
      len := LEN(b);
      old := b;
      NEW(b, len * 2);
      FOR idx := 0 TO len-1 DO b[idx] := old[idx] END;
    END Expand;
   (* ================================= *)
  BEGIN
    NEW(buf, 128);
    num := 0;
    idx := 0;
    FOR idx := 0 TO LEN(u.val) - 2 DO
      chr := ORD(u.val[idx]);
      IF num > LEN(buf) - 3 THEN Expand(buf) END;
      IF chr <= 7FH THEN
        IF chr = 0H THEN (* Modified UTF8! *)
          buf[num] := 0C0H; INC(num);
          buf[num] := 080H; INC(num);
        ELSE
          buf[num] := chr;  INC(num);
        END;
      ELSIF chr <= 7FFH THEN
        buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num  ] := 0C0H + chr; INC(num, 2);
      ELSE
        buf[num+2] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num  ] := 0E0H + chr; INC(num, 3);
      END;
    END;
    F.WriteByte(file,Jvm.const_utf8);
    u2(file,num);
    FOR idx := 0 TO num-1 DO F.WriteByte(file,buf[idx]) END;
  END Dump;

  PROCEDURE (c : ClassRef)Dump(file : F.FILE);
  BEGIN
    F.WriteByte(file,Jvm.const_class);
    u2(file,c.nameIx);
  END Dump;

  PROCEDURE (r : Reference)Dump(file : F.FILE);
  VAR
    tag : INTEGER;
  BEGIN
    IF r IS MethodRef THEN
      tag := Jvm.const_methodref;
    ELSIF r IS FieldRef THEN
      tag := Jvm.const_fieldref;
    ELSE
      tag := Jvm.const_interfacemethodref;
    END;
    F.WriteByte(file,tag);
    u2(file,r.classIx);
    u2(file,r.nameAndTypeIx);
  END Dump;

  PROCEDURE (n : NameAndType)Dump(file : F.FILE);
  BEGIN
    F.WriteByte(file,Jvm.const_nameandtype);
    u2(file,n.nameIx);
    u2(file,n.descIx);
  END Dump;

  PROCEDURE (s : StringRef)Dump(file : F.FILE);
  BEGIN
    F.WriteByte(file,Jvm.const_string);
    u2(file,s.stringIx);
  END Dump;

  PROCEDURE (i : Integer)Dump(file : F.FILE);
  BEGIN
    F.WriteByte(file,Jvm.const_integer);
    u4(file,i.iVal);
  END Dump;

  PROCEDURE (f : Float)Dump(file : F.FILE);
  BEGIN
    F.WriteByte(file,Jvm.const_float);
    u4(file,RTS.shortRealToIntBits(f.fVal));
  END Dump;

  PROCEDURE (l : Long)Dump(file : F.FILE);
  BEGIN
    F.WriteByte(file,Jvm.const_long);
    u4(file,RTS.hiInt(l.lVal));
    u4(file,RTS.loInt(l.lVal));
  END Dump;

  PROCEDURE (d : Double)Dump(file : F.FILE);
  VAR
    rBits : LONGINT;
  BEGIN
    F.WriteByte(file,Jvm.const_double);
    rBits := RTS.realToLongBits(d.dVal);
    u4(file,RTS.hiInt(rBits));
    u4(file,RTS.loInt(rBits));
  END Dump;

(* ============================================================ *)

  PROCEDURE (cf : ClassFile)Dump*();
  VAR
    i,j : INTEGER;
  BEGIN
    u4(cf.file,RTS.loInt(magic));        
    u2(cf.file,minorVersion);
    u2(cf.file,majorVersion);
    u2(cf.file,cf.cp.tide);     (* constant pool count *)
    FOR i := 1 TO cf.cp.tide-1 DO
      IF cf.cp.pool[i] # NIL THEN cf.cp.pool[i].Dump(cf.file); END;
    END;
    u2(cf.file,cf.access);
    u2(cf.file,cf.thisClassIx); 
    u2(cf.file,cf.superClassIx); 
    u2(cf.file,cf.numInterfaces);
    FOR i := 0 TO cf.numInterfaces-1 DO
      u2(cf.file,cf.interfaces[i]);
    END;
    u2(cf.file,cf.numFields);
    FOR i := 0 TO cf.numFields-1 DO
      cf.fields[i].Dump(cf);
    END;
    u2(cf.file,cf.numMethods);
    FOR i := 0 TO cf.numMethods-1 DO
      cf.methods[i].Dump(cf);
    END;
    u2(cf.file,1);  (* only class file attribute is SourceFile *)
    u2(cf.file,cf.srcFileAttIx); 
    u4(cf.file,2);  (* length of source file attribute *) 
    u2(cf.file,cf.srcFileIx); 
    GPBinFiles.CloseFile(cf.file);
  END Dump;

(* ============================================================ *)

BEGIN
  srcFileStr := L.strToCharOpen("SourceFile");
  codeStr := L.strToCharOpen("Code");
  lineNumTabStr := L.strToCharOpen("LineNumberTable");
  object := L.strToCharOpen("java/lang/Object");
  init := L.strToCharOpen("<init>");
  clinit := L.strToCharOpen("<clinit>");
  noArgVoid := L.strToCharOpen("()V");
  noArgClass := L.strToCharOpen("()Ljava/lang/Class;");
(*
  errorClass := L.strToCharOpen("java/lang/Error"); 
 *)
  errorClass := L.strToCharOpen("java/lang/Exception"); 
  errorInitSig := L.strToCharOpen("(Ljava/lang/String;)V");
  rtsClass := L.strToCharOpen("CP/CPJrts/CPJrts");
  caseTrap := L.strToCharOpen("CaseMesg");
  caseTrapSig := L.strToCharOpen("(I)Ljava/lang/String;");
  withTrap := L.strToCharOpen("WithMesg");
  withTrapSig := L.strToCharOpen("(Ljava/lang/Object;)Ljava/lang/String;");
  exceptType := L.strToCharOpen("java/lang/Exception");
  main := L.strToCharOpen("main");
  mainSig := L.strToCharOpen("([Ljava/lang/String;)V");
  CPmainClass := L.strToCharOpen("CP/CPmain/CPmain");
  putArgs := L.strToCharOpen("PutArgs");
  copy := L.strToCharOpen("__copy__");
  sysClass := L.strToCharOpen("java/lang/System");
  charClass := L.strToCharOpen("java/lang/Character");
  mathClass := L.strToCharOpen("java/lang/Math");

  procNames[J.StrCmp]  := L.strToCharOpen("strCmp");
  procNames[J.StrToChrOpen] := L.strToCharOpen("JavaStrToChrOpen");
  procNames[J.StrToChrs] := L.strToCharOpen("JavaStrToFixChr");
  procNames[J.ChrsToStr] := L.strToCharOpen("FixChToJavaStr");
  procNames[J.StrCheck] := L.strToCharOpen("ChrArrCheck");
  procNames[J.StrLen] := L.strToCharOpen("ChrArrLength");
  procNames[J.ToUpper] := L.strToCharOpen("toUpperCase");
  procNames[J.DFloor] := L.strToCharOpen("floor");
  procNames[J.ModI] := L.strToCharOpen("CpModI");
  procNames[J.ModL] := L.strToCharOpen("CpModL");
  procNames[J.DivI] := L.strToCharOpen("CpDivI");
  procNames[J.DivL] := L.strToCharOpen("CpDivL");
  procNames[J.StrCatAA] := L.strToCharOpen("ArrArrToString");
  procNames[J.StrCatSA] := L.strToCharOpen("StrArrToString");
  procNames[J.StrCatAS] := L.strToCharOpen("ArrStrToString");
  procNames[J.StrCatSS] := L.strToCharOpen("StrStrToString");
  procNames[J.StrLP1] := L.strToCharOpen("ChrArrLplus1");
  procNames[J.StrVal] := L.strToCharOpen("ChrArrStrCopy");
  procNames[J.SysExit] := L.strToCharOpen("exit");
  procNames[J.LoadTp1] := L.strToCharOpen("getClassByOrd");
  procNames[J.LoadTp2] := L.strToCharOpen("getClassByName");
  getCls := L.strToCharOpen("getClass");

  IIretI := L.strToCharOpen("(II)I");
  JJretJ := L.strToCharOpen("(JJ)J");

  procSigs[J.StrCmp]   := L.strToCharOpen("([C[C)I");
  procSigs[J.StrToChrOpen] := L.strToCharOpen("(Ljava/lang/String;)[C");
  procSigs[J.StrToChrs] := L.strToCharOpen("([CLjava/lang/String;)V");
  procSigs[J.ChrsToStr] := L.strToCharOpen("([C)Ljava/lang/String;");
  procSigs[J.StrCheck] := L.strToCharOpen("([C)V");
  procSigs[J.StrLen] := L.strToCharOpen("([C)I");
  procSigs[J.ToUpper] := L.strToCharOpen("(C)C");
  procSigs[J.DFloor] := L.strToCharOpen("(D)D");
  procSigs[J.ModI] := IIretI;
  procSigs[J.ModL] := JJretJ;
  procSigs[J.DivI] := IIretI;
  procSigs[J.DivL] := JJretJ;
  procSigs[J.StrCatAA] := L.strToCharOpen("([C[C)Ljava/lang/String;");
  procSigs[J.StrCatSA] := L.strToCharOpen(
                                "(Ljava/lang/String;[C)Ljava/lang/String;");
  procSigs[J.StrCatAS] := L.strToCharOpen(
                                "([CLjava/lang/String;)Ljava/lang/String;");
  procSigs[J.StrCatSS] := L.strToCharOpen(
                   "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
  procSigs[J.StrLP1] := procSigs[J.StrLen];
  procSigs[J.StrVal] := L.strToCharOpen("([C[C)V");
  procSigs[J.SysExit] := L.strToCharOpen("(I)V");
  procSigs[J.LoadTp1] := L.strToCharOpen("(I)Ljava/lang/Class;");
  procSigs[J.LoadTp2] := L.strToCharOpen(
                               "(Ljava/lang/String;)Ljava/lang/Class;");

  typeArr[ Ty.boolN] := 4;
  typeArr[ Ty.sChrN] := 8;
  typeArr[ Ty.charN] := 5;
  typeArr[ Ty.byteN] := 8;
  typeArr[ Ty.uBytN] := 8;
  typeArr[ Ty.sIntN] := 9;
  typeArr[  Ty.intN] := 10;
  typeArr[ Ty.lIntN] := 11;
  typeArr[ Ty.sReaN] := 6;
  typeArr[ Ty.realN] := 7;
  typeArr[  Ty.setN] := 10;
END ClassUtil.
(* ============================================================ *)
(* ============================================================ *)

