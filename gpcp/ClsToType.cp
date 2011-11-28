
(* ================================================================ *)
(*                                                                  *)
(*  Module of the V1.4+ gpcp tool to create symbol files from       *)
(*  the metadata of .NET assemblies, using the PERWAPI interface.   *)
(*                                                                  *)
(*  Copyright QUT 2004 - 2005.                                      *)
(*                                                                  *)
(*  This code released under the terms of the GPCP licence.         *)
(*                                                                  *)
(*  This Module:   <ClsToType>                                      *)
(*       Transforms PERWAPI classes to GPCP TypeDesc structures.    *)
(*       Original module, kjg December 2004                         *)
(*                                                                  *)
(* ================================================================ *)

MODULE ClsToType;
  IMPORT
(*
 *  Rfl := mscorlib_System_Reflection,  (* temporary *)
 *  Sio := mscorlib_System_IO,          (* temporary *)
 *)
    FNm := FileNames,
    Mng := ForeignName,
    Per := "[QUT.PERWAPI]QUT.PERWAPI",
    Glb := N2State,
    Ltv := LitValue,
    Cst := CompState,
    Nh  := NameHash,
    Id  := IdDesc,
    Ty  := TypeDesc,
    Xp  := ExprDesc,
    Sy  := Symbols,
    Bi  := Builtin,
    Console,
    ASCII,
    RTS;

 (* ------------------------------------------------------------ *)

  CONST anon*      = 0; (* The anonymous hash index *)

  CONST (* class kind enumeration *)
        default* =  0; refCls*  =  1; valCls*  =  2;
        enuCls*  =  3; evtCls*  =  4; dlgCls*  =  5;
        primTyp* =  6; arrTyp*  =  7; voidTyp* =  8;
        strTyp*  =  9; objTyp*  = 10; sysValT* = 11;
        sysEnuT* = 12; sysDelT* = 13; sysExcT* = 14; voidStar* = 15;

  CONST (* type attribute enumeration bits   *)
        absTp = 7; intTp = 5; sldTp = 8;

        (* field attribute enumeration bits  *)
        stFld = 4; ltFld = 6; 

        (* method attribute enumeration bits *)
        stMth = 4; fnMth = 5; vrMth = 6; nwMth = 8; abMth = 10;

 (* ------------------------------------------------------------ *)

  TYPE  Namespace*    = POINTER TO ABSTRACT RECORD
                          hash  : INTEGER;
                          bloc* : Id.BlkId;
                          tIds  : VECTOR OF Id.TypId;
                        END;

        DefNamespace* = POINTER TO RECORD (Namespace)
                          clss  : VECTOR OF Per.ClassDef;
                        END;

        RefNamespace* = POINTER TO RECORD (Namespace)
                          clss  : VECTOR OF Per.ClassRef;
                        END;

 (* ------------------------------------------------------------ *)

  VAR   ntvObj : Sy.Type; 
        ntvStr : Sy.Type; 
        ntvExc : Sy.Type;
        ntvTyp : Sy.Type; 
        ntvEnu : Sy.Type;
        ntvEvt : Sy.Type;
        ntvVal : Sy.Type;
        sysU16 : Sy.Type;
        sysU32 : Sy.Type;
        sysU64 : Sy.Type;
        voidSt : Sy.Type;

        intPtr : Sy.Type;
        uIntPt : Sy.Type;
        tpdRef : Sy.Type;

        corLib : Id.BlkId;

        baseArrs : ARRAY 18 OF Sy.Type;

 (* ------------------------------------------------------------ *)
 (*                   Utilities and Predicates                   *)
 (* ------------------------------------------------------------ *)

  PROCEDURE^ cpTypeFromCTS(peT : Per.Type; spc : DefNamespace) : Sy.Type;

 (* ------------------------------------------------ *)

  PROCEDURE isExportedType(attr : Per.TypeAttr) : BOOLEAN;
    VAR bits : SET;
  BEGIN
    bits := BITS(attr) * {0..2};
    CASE ORD(bits) OF
    | 1, 2, 4, 7 : RETURN TRUE;
    ELSE RETURN FALSE;
    END;
  END isExportedType;

 (* ------------------------------------------------ *)

  PROCEDURE isProtectedType(attr : Per.TypeAttr) : BOOLEAN;
    VAR bits : SET;
  BEGIN
    bits := BITS(attr) * {0..2};
    CASE ORD(bits) OF
    | 4, 7 : RETURN TRUE;
    ELSE RETURN FALSE;
    END;
  END isProtectedType;

 (* ------------------------------------------------ *)

  PROCEDURE isGenericClass(cls : Per.ClassDesc) : BOOLEAN;
  BEGIN
    RETURN LEN(cls.GetGenericParams()) > 0;
  END isGenericClass;

 (* ------------------------------------------------ *)

  PROCEDURE isGenericType(typ : Per.Type) : BOOLEAN;
  BEGIN
    WITH typ : Per.ClassSpec DO RETURN TRUE;
       | typ : Per.ClassDesc DO RETURN isGenericClass(typ);
       | typ : Per.Array     DO RETURN isGenericType(typ.ElemType());
    ELSE RETURN FALSE;
    END;
  END isGenericType;

 (* ------------------------------------------------ *)

  PROCEDURE isPublicClass(cls : Per.Class) : BOOLEAN;
  BEGIN
    WITH cls : Per.NestedClassDef DO
      RETURN isExportedType(cls.GetAttributes()) &
             ~isGenericType(cls) &
             isPublicClass(cls.GetParentClass());
    |    cls : Per.ClassDef DO
      RETURN isExportedType(cls.GetAttributes()) & 
             ~isGenericType(cls);
    ELSE (* cls : Per.ClassRef ==> exported *)
      RETURN TRUE;
    END;
  END isPublicClass;

 (* ------------------------------------------------ *)

  PROCEDURE hasGenericArg(mth : Per.Method) : BOOLEAN;
    VAR idx : INTEGER;
        par : Per.Type;
        prs : POINTER TO ARRAY OF Per.Type;
  BEGIN
    prs := mth.GetParTypes();
    FOR idx := 0 TO LEN(prs) - 1 DO
      par := prs[idx];
      IF isGenericType(par) THEN RETURN TRUE END;
    END;
    RETURN FALSE;
  END hasGenericArg;

 (* ------------------------------------------------ *)

  PROCEDURE isGenericMethod(mth : Per.Method) : BOOLEAN;
  BEGIN
    RETURN (mth.GetGenericParam(0) # NIL) OR hasGenericArg(mth);
  END isGenericMethod;

 (* ------------------------------------------------ *)

  PROCEDURE isVarargMethod(mth : Per.Method) : BOOLEAN;
  BEGIN
    RETURN mth.GetCallConv() = Per.CallConv.Vararg;
  END isVarargMethod;

 (* ------------------------------------------------ *)
 (*
  PROCEDURE isNestedType(attr : Per.TypeAttr) : BOOLEAN;
    VAR bits : INTEGER;
  BEGIN
    bits := ORD(BITS(attr) * {0..2});
    RETURN (bits >= 2) & (bits <= 7);
  END isNestedType;
  *)
 (* ------------------------------------------------ *)

  PROCEDURE gpName(typ : Per.Class) : RTS.NativeString;
  BEGIN
    WITH typ : Per.NestedClassDef DO
        RETURN gpName(typ.GetParentClass()) + "$" + typ.Name();
    | typ : Per.NestedClassRef DO
        RETURN gpName(typ.GetParentClass()) + "$" + typ.Name();
    ELSE
      RETURN typ.Name();
    END;
  END gpName;

 (* ------------------------------------------------ *)

  PROCEDURE gpSpce(typ : Per.Class) : RTS.NativeString;
  BEGIN
    WITH typ : Per.NestedClassDef DO
        RETURN gpSpce(typ.GetParentClass());
    | typ : Per.NestedClassRef DO
        RETURN gpSpce(typ.GetParentClass());
    ELSE
      RETURN typ.NameSpace();
    END;
  END gpSpce;

 (* ------------------------------------------------ *)

  PROCEDURE ilName(mth : Per.Method) : RTS.NativeString;
    VAR cls : Per.Class;
  BEGIN
    cls := mth.GetParent()(Per.Class);
    RETURN gpSpce(cls) + "." + gpName(cls) + "::'" + mth.Name() + "'";
  END ilName;

 (* ------------------------------------------------ *)

  PROCEDURE isCorLibRef(res : Per.ResolutionScope) : BOOLEAN;
    VAR str : RTS.NativeString;
  BEGIN
    IF Glb.isCorLib THEN 
      RETURN FALSE; (* ==> this is corlib DEFINITION! *)
    ELSIF res = NIL THEN 
      RETURN FALSE;
    ELSE
      str := res.Name();
      RETURN ((str = "mscorlib") OR (str = "CommonLanguageRuntimeLibrary"));
    END;
  END isCorLibRef;

 (* ------------------------------------------------ *)

  PROCEDURE SayWhy(cls : Per.Class);
    VAR str : Glb.CharOpen;
  BEGIN
    WITH cls : Per.ClassSpec DO
      str := BOX(" Hiding generic class -- "); 
    |   cls : Per.NestedClassDef DO
      IF ~isExportedType(cls.GetAttributes()) THEN RETURN; (* just private! *)
      ELSIF isGenericType(cls) THEN
        str := BOX(" Hiding generic class -- "); 
      ELSE (* ~isPublicClass(cls.GetParentClass()); *)
        str := BOX(" Hiding public child of private class -- "); 
      END;
    |   cls : Per.ClassDef DO
      IF ~isExportedType(cls.GetAttributes()) THEN RETURN; (* just private! *)
      ELSE (* isGenericType(cls) *)
        str := BOX(" Hiding generic class -- "); 
      END;
    END;
    Glb.Message(str^ + gpSpce(cls) + "." + gpName(cls));
  END SayWhy;

 (* ------------------------------------------------ *)

  PROCEDURE getKind(typ : Per.Type) : INTEGER;
    VAR pEnu : INTEGER;
        pTyp : Per.Class;
        name : RTS.NativeString;
        rScp : Per.ResolutionScope;
  BEGIN
    WITH typ : Per.Array DO (* --------------- *) RETURN arrTyp;
    | typ : Per.UnmanagedPointer DO (* ------- *) RETURN voidStar;
    | typ : Per.PrimitiveType DO
        IF    typ = Per.PrimitiveType.Object THEN RETURN objTyp;
        ELSIF typ = Per.PrimitiveType.String THEN RETURN strTyp;
        ELSIF typ = Per.PrimitiveType.Void   THEN RETURN voidTyp;
        ELSE                                      RETURN primTyp;
        END;
    | typ : Per.ClassDef DO
        rScp := typ.GetScope();
        pTyp := typ.get_SuperType();
       (*
        *  If this is *not* mscorlib, then check the kind of the parent.
        *)
        IF ~Glb.isCorLib THEN
          pEnu := getKind(pTyp);
          name := gpName(typ);
       (*
        *  If it has no parent, then it must be Object, or some ref class.
        *)
        ELSIF pTyp = NIL THEN                     RETURN refCls;
       (*
        *  Since "ntvObj" and the others have not been initialized
        *  for the special case of processing mscorlib, we must look
        *  at the names of the parents.
        *)
        ELSE 
          name := gpName(pTyp);
          IF    name = "ValueType"               THEN RETURN valCls;
          ELSIF name = "Enum"                    THEN RETURN enuCls;
          ELSIF name = "MulticastDelegate"       THEN RETURN dlgCls;
          ELSE (* -------------------------------- *) RETURN refCls;
          END;
        END;
    | typ : Per.ClassRef DO
        rScp := typ.GetScope();
        name := gpName(typ);
        pEnu := default;
    ELSE (* ---------------------------------- *) RETURN default; 
    END;

    IF isCorLibRef(rScp) THEN
      IF    name = "Object"                  THEN RETURN objTyp;
      ELSIF name = "ValueType"               THEN RETURN sysValT;
      ELSIF name = "Enum"                    THEN RETURN sysEnuT;
      ELSIF name = "MulticastDelegate"       THEN RETURN sysDelT;
      ELSIF name = "Exception"               THEN RETURN sysExcT;
      END;
    END;

    IF    pEnu = sysValT                     THEN RETURN valCls;
    ELSIF pEnu = sysDelT                     THEN RETURN dlgCls;
    ELSIF pEnu = sysEnuT                     THEN RETURN enuCls;
    ELSE (* ---------------------------------- *) RETURN refCls;
    END;
  END getKind;

 (* ------------------------------------------------ *)

  PROCEDURE kindStr(kind : INTEGER) : Glb.CharOpen;
  BEGIN
    CASE kind OF
    | default  : RETURN BOX("opaque               ");
    | refCls   : RETURN BOX("reference class      ");
    | valCls   : RETURN BOX("value class          ");
    | enuCls   : RETURN BOX("enumeration class    ");
    | evtCls   : RETURN BOX("event class          ");
    | dlgCls   : RETURN BOX("delegate             ");
    | primTyp  : RETURN BOX("primitive            ");
    | arrTyp   : RETURN BOX("array type           ");
    | voidTyp  : RETURN BOX("void type            ");
    | strTyp   : RETURN BOX("Sys.String           ");
    | objTyp   : RETURN BOX("Sys.Object           ");
    | sysValT  : RETURN BOX("Sys.ValueType        ");
    | sysEnuT  : RETURN BOX("Sys.Enum Type        ");
    | sysDelT  : RETURN BOX("Sys.MulticastDelegate");
    | sysExcT  : RETURN BOX("Sys.Exception        ");
    | voidStar : RETURN BOX("Sys.Void*            ");
    ELSE         RETURN BOX("unknown              ");
    END;
  END kindStr;

 (* ------------------------------------------------ *)

  PROCEDURE mapPrimitive(peT : Per.Type) : Sy.Type;
  BEGIN
    IF    peT = Per.PrimitiveType.Int32    THEN RETURN Bi.intTp;
    ELSIF peT = Per.PrimitiveType.Char     THEN RETURN Bi.charTp;
    ELSIF peT = Per.PrimitiveType.Boolean  THEN RETURN Bi.boolTp;
    ELSIF peT = Per.PrimitiveType.Int16    THEN RETURN Bi.sIntTp;
    ELSIF peT = Per.PrimitiveType.Float64  THEN RETURN Bi.realTp;
    ELSIF peT = Per.PrimitiveType.Int64    THEN RETURN Bi.lIntTp;
    ELSIF peT = Per.PrimitiveType.Float32  THEN RETURN Bi.sReaTp;
    ELSIF peT = Per.PrimitiveType.Int8     THEN RETURN Bi.byteTp;
    ELSIF peT = Per.PrimitiveType.UInt8    THEN RETURN Bi.uBytTp;
    ELSIF peT = Per.PrimitiveType.UInt16   THEN RETURN sysU16;
    ELSIF peT = Per.PrimitiveType.UInt32   THEN RETURN sysU32;
    ELSIF peT = Per.PrimitiveType.UInt64   THEN RETURN sysU64;
    ELSIF peT = Per.PrimitiveType.IntPtr   THEN RETURN intPtr;
    ELSIF peT = Per.PrimitiveType.UIntPtr  THEN RETURN uIntPt;
    ELSIF peT = Per.PrimitiveType.TypedRef THEN RETURN tpdRef;
    ELSE (* ------------------------------- *) RETURN NIL;
    END;
  END mapPrimitive;

 (* ------------------------------------------------ *)

  PROCEDURE makeNameType(blk : Id.BlkId; hsh : INTEGER) : Id.TypId;
    VAR tId : Id.TypId;
  BEGIN
    tId := Id.newTypId(Ty.newNamTp());
    tId.hash := hsh;
    tId.dfScp := blk;
    tId.type.idnt := tId;
    tId.SetMode(Sy.pubMode);
    Glb.ListTy(tId.type);
    IF Sy.refused(tId, blk) THEN Glb.AbortMsg("bad TypId insert") END;
    RETURN tId;
  END makeNameType;

 (* ------------------------------------------------ *)

  PROCEDURE lookup(peT : Per.Class; nSp : DefNamespace) : Sy.Type;
    VAR asm : Glb.CharOpen;  (* assembly file name *)
        spc : Glb.CharOpen;  (* namespace name str *)
        mNm : Glb.CharOpen;  (* CP module name     *)
        cNm : Glb.CharOpen;  (* PE file class name *)
        blk : Sy.Idnt;       (* The Blk descriptor *)
        bId : Id.BlkId;      (* The Blk descriptor *)
        tId : Sy.Idnt;       (* TypId descriptor   *)
        hsh : INTEGER;       (* Class name hash    *)
   (* -------------------------------------------- *)
    PROCEDURE NoteImport(spc : DefNamespace; imp : Id.BlkId);
    BEGIN
      IF (spc # NIL) & (spc.bloc # imp) THEN
        IF ~Sy.refused(imp, spc.bloc) THEN
          IF Glb.superVb THEN
            Console.WriteString("Inserting import <");
            Console.WriteString(Nh.charOpenOfHash(imp.hash));
            Console.WriteString("> in Namespace ");
            Console.WriteString(Nh.charOpenOfHash(spc.bloc.hash));
            Console.WriteLn;
          END;
        END;
      END; 
    END NoteImport;
   (* -------------------------------------------- *)
  BEGIN
    bId := NIL;
   (*
    *  First we establish the (mangled) name of the defining scope.
    *)
    WITH peT : Per.ClassDef DO
        asm := BOX(Glb.basNam^); (* Must do a value copy *)
    | peT : Per.ClassRef DO
        asm := BOX(peT.GetScope().Name());
    ELSE 
        RETURN NIL;
    END;
(*
 *  FNm.StripExt(asm, asm);
 *  spc := BOX(peT.NameSpace());
 *)
    spc := BOX(gpSpce(peT));
    mNm := Mng.MangledName(asm, spc);
   (*
    *  Check if this name is already known to PeToCps
    *)
    blk := Glb.thisMod.symTb.lookup(Nh.enterStr(mNm));
    cNm := BOX(gpName(peT));
    hsh := Nh.enterStr(cNm);
    WITH blk : Id.BlkId DO
       (*
        *  The module name is known to PeToCps.
        *  However, it may not have been listed as an import
        *  into the current namespace, in the case of multiple
        *  namespaces defined in the same source PEFile.
        *)
        NoteImport(nSp, blk);

        tId := blk.symTb.lookup(hsh);
        IF (tId # NIL) & (tId IS Id.TypId) THEN 
          RETURN tId.type;
        ELSE
          bId := blk;
        END;
    ELSE 
    END;
   (*
    *  Could not find the type identifier descriptor.
    *)
    IF bId = NIL THEN
     (*
      *  Create a BlkId for the namespace.
      *)
      NEW(bId);
      INCL(bId.xAttr, Sy.need);
      Glb.BlkIdInit(bId, asm, spc);
     (*
      *  ... and in any case, this new BlkId is an
      *  import into the current namespace scope.
      *)
      NoteImport(nSp, bId);
    END;
   (*
    *  Now create a TypId, and insert in block symTab.
    *)
    tId := makeNameType(bId, hsh);
    RETURN tId.type;
  END lookup;

 (* ------------------------------------------------ *)

  PROCEDURE ptrToArrayOf(elTp : Sy.Type) : Sy.Type;
    VAR ptrT : Sy.Type;
   (* -------------------------------------------- *)
    PROCEDURE getPtr(elT : Sy.Type) : Sy.Type;
      VAR arT, ptT : Sy.Type;
    BEGIN
      arT := Ty.mkArrayOf(elT); Glb.ListTy(arT);
      ptT := Ty.mkPtrTo(arT);   Glb.ListTy(ptT); RETURN ptT;
    END getPtr;
   (* -------------------------------------------- *)
  BEGIN
    WITH elTp : Ty.Base DO
      ptrT := baseArrs[elTp.tpOrd];
      IF ptrT = NIL THEN
        ptrT := getPtr(elTp);
        baseArrs[elTp.tpOrd] := ptrT;
      END;
    ELSE
      ptrT := getPtr(elTp);
    END;
    RETURN ptrT;
  END ptrToArrayOf;

 (* ------------------------------------------------ *)

  PROCEDURE cpTypeFromCTS(peT : Per.Type; spc : DefNamespace) : Sy.Type;
    VAR kind : INTEGER;
  BEGIN
    kind := getKind(peT);
    CASE kind OF
    | voidTyp  : RETURN NIL;
    | arrTyp   : RETURN ptrToArrayOf(
                             cpTypeFromCTS(peT(Per.Array).ElemType(), spc));
    | primTyp  : RETURN mapPrimitive(peT);
    | strTyp   : RETURN ntvStr;
    | objTyp   : RETURN ntvObj;
    | sysValT  : RETURN ntvVal;
    | sysEnuT  : RETURN ntvEnu;
    | sysDelT  : RETURN ntvEvt;
    | voidStar : RETURN voidSt;

    ELSE (* default, refCls, valCls, enuCls, evtCls, dlgCls *)  
      WITH peT : Per.Class DO
        RETURN lookup(peT, spc);
      ELSE
        IF peT # NIL THEN
          Console.WriteString("Not a class -- ");
          Console.WriteLn;
        END;
        RETURN NIL;
      END;
    END;
  END cpTypeFromCTS;

 (* ------------------------------------------------ *)

  PROCEDURE modeFromMbrAtt(att : SET) : INTEGER;
  BEGIN
    CASE ORD(att * {0,1,2}) OF
    | 4, 5 : RETURN Sy.protect;
    | 6    : RETURN Sy.pubMode;
    ELSE     RETURN Sy.prvMode;
    END;
  END modeFromMbrAtt;

 (* ------------------------------------------------ *)

  PROCEDURE mkParam(IN nam : ARRAY OF CHAR; 
                       mod : INTEGER;
                       typ : Sy.Type; 
                       rcv : BOOLEAN) : Id.ParId;
    VAR par : Id.ParId;
  BEGIN
    par := Id.newParId();
    par.parMod := mod;
    par.type   := typ;
    par.hash   := Nh.enterStr(nam);
    par.isRcv  := rcv;
    RETURN par;
  END mkParam;

 (* ------------------------------------------------------------ *)

  PROCEDURE isValClass(cls : Per.Type) : BOOLEAN;
  BEGIN
    RETURN getKind(cls) = valCls;
  END isValClass;

 (* ------------------------------------------------------------ *)
 (*                   Main processing code                       *)
 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)AddRecFld(rec : Ty.Record; 
                                          fld : Per.FieldDef), NEW;
    VAR   mod : INTEGER;
          hsh : INTEGER;
          bts : SET;
          res : BOOLEAN;
          fId : Id.FldId;
          vId : Id.VarId;
          cId : Id.ConId;
   (* ------------------------------------ *)
    PROCEDURE conExp(val : Per.Constant) : Sy.Expr;
      VAR byts : POINTER TO ARRAY OF UBYTE;
          chrs : POINTER TO ARRAY OF CHAR;
          indx : INTEGER;
    BEGIN
      WITH val : Per.DoubleConst DO
          RETURN Xp.mkRealLt(val.GetDouble());
      | val : Per.FloatConst DO
          RETURN Xp.mkRealLt(val.GetDouble());
      | val : Per.CharConst DO
          RETURN Xp.mkCharLt(val.GetChar());
      | val : Per.IntConst DO
          RETURN Xp.mkNumLt(val.GetLong());
      | val : Per.UIntConst DO
          RETURN Xp.mkNumLt(val.GetULongAsLong());
      | val : Per.StringConst DO
          byts := val.GetStringBytes();
          NEW(chrs, LEN(byts) DIV 2 + 1);
          FOR indx := 0 TO (LEN(byts) DIV 2)-1 DO
            chrs[indx] := CHR(byts[indx*2] + byts[indx*2 + 1] * 256);
          END;
          (* RETURN Xp.mkStrLt(chrs); *)
          RETURN Xp.mkStrLenLt(chrs, LEN(chrs) - 1); (* CHECK THIS! *)
      END;
    END conExp;
   (* ------------------------------------ *)
  BEGIN
    bts := BITS(fld.GetFieldAttr());
    mod := modeFromMbrAtt(bts);
    IF mod > Sy.prvMode THEN
      hsh := Nh.enterStr(fld.Name());
      IF ltFld IN bts THEN                 (* literal field  *)
        cId := Id.newConId();
        cId.hash := hsh;
        cId.SetMode(mod);
        cId.recTyp := rec;
        cId.type := cpTypeFromCTS(fld.GetFieldType(), spc);
        cId.conExp := conExp(fld.GetValue());
        res := rec.symTb.enter(hsh, cId);
        Sy.AppendIdnt(rec.statics, cId);
      ELSIF stFld IN bts THEN              (* static field   *)
        vId := Id.newVarId();
        vId.hash := hsh;
        vId.SetMode(mod);
        vId.recTyp := rec;
        vId.type := cpTypeFromCTS(fld.GetFieldType(), spc);
        res := rec.symTb.enter(hsh, vId);
        Sy.AppendIdnt(rec.statics, vId);
      ELSE                                 (* instance field *)
        fId := Id.newFldId();
        fId.hash := hsh;
        fId.SetMode(mod);
        fId.recTyp := rec;
        fId.type := cpTypeFromCTS(fld.GetFieldType(), spc);
        res := rec.symTb.enter(hsh, fId);
        Sy.AppendIdnt(rec.fields, fId);
      END;
    END;
  END AddRecFld;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)AddFormals(typ : Ty.Procedure; 
                                           mth : Per.MethodDef), NEW;
    VAR indx : INTEGER;
        pMod : INTEGER;
        thsP : Per.Param;
        thsT : Per.Type;
        pPar : Id.ParId;
        pars : POINTER TO ARRAY OF Per.Param;

  BEGIN
    typ.retType := cpTypeFromCTS(mth.GetRetType(), spc);
    pars := mth.GetParams();
    FOR indx := 0 TO LEN(pars) - 1 DO
      pMod := Sy.val;
      thsP := pars[indx];
      thsT := thsP.GetParType();
      IF thsT IS Per.ManagedPointer THEN
        thsT := thsT(Per.PtrType).GetBaseType(); pMod := Sy.var;
      END;
      pPar := mkParam(thsP.GetName(), pMod, cpTypeFromCTS(thsT, spc), FALSE);
      Id.AppendParam(typ.formals, pPar);
    END;
  END AddFormals;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)AddRecMth(rec : Ty.Record; 
                                          mth : Per.MethodDef), NEW;
    VAR   mod : INTEGER;
          hsh : INTEGER;
          pMd : INTEGER;
          bts : SET;
          res : BOOLEAN;
          pId : Id.PrcId;
          mId : Id.MthId;
          rcv : Per.Type;        (* Receiver type *)
          pTp : Ty.Procedure;
  BEGIN
   (* SPECIAL FOR PRE 1.4 VERSION *)
    IF isGenericMethod(mth) THEN
      Glb.CondMsg(" Hiding generic method -- " + ilName(mth));
      RETURN;
    ELSIF isVarargMethod(mth) THEN
      Glb.CondMsg(" Hiding Vararg call method -- " + ilName(mth));
      RETURN;
    END;
    bts := BITS(mth.GetMethAttributes());
    mod := modeFromMbrAtt(bts);
    IF mod > Sy.prvMode THEN
      hsh := Nh.enterStr(mth.Name());

      IF stMth IN bts THEN                (* static method *)
        pId := Id.newPrcId(); 
        pId.SetKind(Id.conPrc);
        pId.hash := hsh;
        pId.SetMode(mod);
        pTp := Ty.newPrcTp();
        pTp.idnt := pId;
        pId.type := pTp;
        spc.AddFormals(pTp, mth);
        res := rec.symTb.enter(hsh, pId);
        Sy.AppendIdnt(rec.statics, pId);
        Glb.ListTy(pTp);

      ELSIF hsh = Glb.ctorBkt THEN        (* constructor method *)
        pId := Id.newPrcId(); 
        pId.SetKind(Id.ctorP);
        pId.hash := Glb.initBkt;
        pId.prcNm := BOX(".ctor");
        pId.SetMode(mod);
        pTp := Ty.newPrcTp();
        pTp.idnt := pId;
        pId.type := pTp;
        spc.AddFormals(pTp, mth);
        rcv := mth.GetParent()(Per.Type);
        pTp.retType := cpTypeFromCTS(rcv, spc);
        res := rec.symTb.enter(Glb.initBkt, pId);
        Sy.AppendIdnt(rec.statics, pId);
        Glb.ListTy(pTp);

      ELSE                                (* instance method *)
        mId := Id.newMthId();
        mId.SetKind(Id.conMth);
        mId.hash := hsh;
        mId.SetMode(mod);

        pMd := Sy.val;
        rcv := mth.GetParent()(Per.Type);
        IF isValClass(rcv) THEN pMd := Sy.var END;

        mId.rcvFrm := mkParam("this", pMd, cpTypeFromCTS(rcv, spc), TRUE);
        pTp := Ty.newPrcTp();
        pTp.idnt := mId;
        mId.type := pTp;
        pTp.receiver := rec;
        spc.AddFormals(pTp, mth);

        IF    abMth IN bts    THEN 
          mId.mthAtt := Id.isAbs;
        ELSIF (vrMth IN bts) & ~(fnMth IN bts) THEN 
          mId.mthAtt := Id.extns;
        END;
        IF ~(vrMth IN bts) OR (nwMth IN bts) THEN 
          INCL(mId.mthAtt, Id.newBit);
        END;

(* FIXME -- boxRcv flag needs to be set ... *)

        res := rec.symTb.enter(hsh, mId);
        Sy.AppendIdnt(rec.methods, mId);
      END;
    END;
  END AddRecMth;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)AddRecEvt(rec : Ty.Record; 
                                          evt : Per.Event), NEW;
    VAR   eTp : Per.Type;
          nam : RTS.NativeString;
          hsh : INTEGER;
          fId : Id.FldId;
          res : BOOLEAN;
  BEGIN
    eTp := evt.GetEventType();
    nam := evt.Name();
    hsh := Nh.enterStr(nam);
    fId := Id.newFldId();
    fId.hash := hsh;
    fId.SetMode(Sy.pubMode);
    fId.recTyp := rec;
    fId.type := cpTypeFromCTS(eTp, spc);
    res := rec.symTb.enter(hsh, fId);
    Sy.AppendIdnt(rec.fields, fId);
  END AddRecEvt;

 (* ------------------------------------------------------------ *)

  PROCEDURE MakeRefCls(cls : Per.ClassDef; 
                       spc : DefNamespace;
                       att : Per.TypeAttr;
                   OUT tId : Id.TypId);
    VAR ptr  : Ty.Pointer;
   (* ------------------------------------------------- *)
    PROCEDURE mkRecord(cls : Per.ClassDef; 
                       spc : DefNamespace;
                       att : Per.TypeAttr) : Ty.Record;
      VAR rec : Ty.Record;
          spr : Per.Class;
          knd : INTEGER;
          bts : SET;
          idx : INTEGER;
          ifE : Per.Class;
          ifA : POINTER TO ARRAY OF Per.Class;
    BEGIN
      bts := BITS(att);
      rec := Ty.newRecTp();
      spr := cls.get_SuperType();

      ifA := cls.GetInterfaces();
      IF ifA # NIL THEN
        FOR idx := 0 TO LEN(ifA) - 1 DO
          ifE := ifA[idx];
          IF ~(ifE IS Per.ClassSpec) & isPublicClass(ifE) THEN
            Sy.AppendType(rec.interfaces, cpTypeFromCTS(ifE, spc));
          ELSIF Glb.verbose THEN 
            SayWhy(ifE);
          END;
        END;
      END;

      IF spr = NIL THEN knd := objTyp ELSE knd := getKind(spr) END;
      IF knd # objTyp THEN rec.baseTp := cpTypeFromCTS(spr, spc) END;
     (*
      *  The INTERFACE test must come first, since
      *  these have the ABSTRACT bit set as well.
      *)
      IF    intTp IN bts THEN rec.recAtt := Ty.iFace;
     (*
      *  Now the ABSTRACT but not interface case.
      *)
      ELSIF absTp IN bts THEN rec.recAtt := Ty.isAbs;
     (*
      *  If class is sealed, then default for CP.
      *)
      ELSIF sldTp IN bts THEN rec.recAtt := Ty.noAtt;
     (*
      *  Else CP default is EXTENSIBLE.
      *)
      ELSE                    rec.recAtt := Ty.extns;
      END;
     (*
      *  This is effectively the "no __copy__" flag.
      *)
      IF ~Glb.cpCmpld THEN INCL(rec.xAttr, Sy.isFn) END;
      Glb.ListTy(rec);
      RETURN rec;
    END mkRecord;
   (* ------------------------------------------------- *)
  BEGIN
   (*
    *  Create the descriptors.
    *)
    ptr := Ty.newPtrTp();
    tId := Id.newTypId(ptr);
    ptr.idnt := tId;
    ptr.boundTp := mkRecord(cls, spc, att);
    ptr.boundTp(Ty.Record).bindTp := ptr;
    tId.hash := Nh.enterStr(gpName(cls));
    Glb.ListTy(ptr);
  END MakeRefCls;

 (* ------------------------------------------------------------ *)

  PROCEDURE MakeEnumTp(cls : Per.ClassDef; 
                   OUT tId : Id.TypId);
    VAR enu : Ty.Enum;
  BEGIN
   (*
    *  Create the descriptors.
    *)
    enu := Ty.newEnuTp();
    tId := Id.newTypId(enu);
    tId.hash := Nh.enterStr(gpName(cls));
    enu.idnt := tId;
    Glb.ListTy(enu);
  END MakeEnumTp;

 (* ------------------------------------------------ *)

  PROCEDURE MakeValCls(cls : Per.ClassDef; 
                   OUT tId : Id.TypId);
    VAR rec  : Ty.Record;
  BEGIN
   (*
    *  Create the descriptors.
    *)
    rec := Ty.newRecTp();
    tId := Id.newTypId(rec);
    rec.idnt := tId;
    tId.hash := Nh.enterStr(gpName(cls));
    IF ~Glb.cpCmpld THEN INCL(rec.xAttr, Sy.isFn) END;
    Glb.ListTy(rec);
  END MakeValCls;

 (* ------------------------------------------------ *)

  PROCEDURE MakePrcCls(cls : Per.ClassDef; 
                   OUT tId : Id.TypId);
    VAR prc  : Ty.Procedure;
  BEGIN
   (*
    *  Create the descriptors.
    *)
(*                          (* We have no way of distinguishing between *)
 *   prc := Ty.newPrcTp();  (* CP EVENT and CP PROCEDURE types from the *)
 *)                         (* PE-file.  So, default to EVENT meantime. *)
    prc := Ty.newEvtTp();
    tId := Id.newTypId(prc);
    prc.idnt := tId;
    tId.hash := Nh.enterStr(gpName(cls));
    Glb.ListTy(prc);
  END MakePrcCls;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)DefineRec(cls : Per.ClassDef; 
                                          rec : Ty.Record), NEW;
    VAR indx : INTEGER;
        flds : POINTER TO ARRAY OF Per.FieldDef;
        evts : POINTER TO ARRAY OF Per.Event;
        mths : POINTER TO ARRAY OF Per.MethodDef;
  BEGIN
   (*
    *  Now fill in record fields ...
    *)
    flds := cls.GetFields();
    FOR indx := 0 TO LEN(flds) - 1 DO
      spc.AddRecFld(rec, flds[indx]);
    END;
   (*
    *  Now fill in record events ...
    *)
    evts := cls.GetEvents();
    FOR indx := 0 TO LEN(evts) - 1 DO
      spc.AddRecEvt(rec, evts[indx]);
    END;
   (*
    *  Now fill in record methods ...
    *)
    mths := cls.GetMethods();
    FOR indx := 0 TO LEN(mths) - 1 DO
      spc.AddRecMth(rec, mths[indx]);
    END;
  END DefineRec;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)DefineEnu(cls : Per.ClassDef; 
                                          enu : Ty.Enum), NEW;
    CONST litB = 6; (* 40H *)
    VAR   indx : INTEGER;
          valu : LONGINT;
          flds : POINTER TO ARRAY OF Per.FieldDef;
          thsF : Per.FieldDef;
          thsC : Id.ConId;
          mode : INTEGER;
          bits : SET;
          sCon : Per.SimpleConstant;
  BEGIN
   (*
    *  Now fill in record details ...
    *)
    flds := cls.GetFields();
    FOR indx := 0 TO LEN(flds) - 1 DO
      thsF := flds[indx];
      bits := BITS(thsF.GetFieldAttr());
      mode := modeFromMbrAtt(bits);
      IF (mode > Sy.prvMode) & (litB IN bits) THEN
        sCon := thsF.GetValue()(Per.SimpleConstant);
        WITH sCon : Per.IntConst DO valu := sCon.GetLong();
           | sCon : Per.UIntConst DO valu := sCon.GetULongAsLong();
        END;
        thsC := Id.newConId();
        thsC.SetMode(mode);
        thsC.hash := Nh.enterStr(thsF.Name());
        thsC.conExp := Xp.mkNumLt(valu);
        thsC.type := Bi.intTp;
        Sy.AppendIdnt(enu.statics, thsC);
      END;
    END;
  END DefineEnu;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)DefinePrc(cls : Per.ClassDef; 
                                          prc : Ty.Procedure), NEW;
    VAR indx : INTEGER;
        valu : INTEGER;
        invk : Per.MethodDef;
  BEGIN
   (*
    *  Now fill in parameter details ...
    *)
    invk := cls.GetMethod(MKSTR("Invoke"));
    spc.AddFormals(prc, invk);
    RETURN;
  END DefinePrc;

 (* ------------------------------------------------------------ *)

  PROCEDURE MakeTypIds*(thsN : DefNamespace);
    VAR indx : INTEGER;
        thsC : Per.ClassDef;
        attr : Per.TypeAttr;
        tEnu : INTEGER;
        tpId : Id.TypId;
        clsh : Sy.Idnt;
  BEGIN
   (*
    *  For every namespace, define gpcp descriptors 
    *  for each class, method, field and constant.
    *)
    Glb.CondMsg(" CP Module name - " + Nh.charOpenOfHash(thsN.bloc.hash)^);
    Glb.CondMsg(' Alternative import name - "' + thsN.bloc.scopeNm^ + '"');
    FOR indx := 0 TO LEN(thsN.clss) - 1 DO
      thsC := thsN.clss[indx];
      attr := thsC.GetAttributes();
      tEnu := getKind(thsC);

      IF Glb.Verbose THEN
        Console.WriteString("        ");
        Console.WriteString(kindStr(tEnu)); Console.Write(ASCII.HT);
        Console.WriteString(gpName(thsC));
        Console.WriteLn;
      END;
      
      CASE tEnu OF
      | refCls  : MakeRefCls(thsC, thsN, attr, tpId);
      | valCls  : MakeValCls(thsC, tpId);
      | enuCls  : MakeEnumTp(thsC, tpId);
(*
 *    | evtCls  : MakeEvtCls(thsC, tpId);
 *)
      | dlgCls  : MakePrcCls(thsC, tpId);
      ELSE tpId := NIL;
      END;
(* ---- temporary ---- *)
IF tpId # NIL THEN
(* ---- temporary ---- *)
      IF isProtectedType(attr) THEN
        tpId.SetMode(Sy.protect);
      ELSE
        tpId.SetMode(Sy.pubMode);
      END;
      tpId.dfScp := thsN.bloc;
      IF ~thsN.bloc.symTb.enter(tpId.hash, tpId) THEN
       (*
        *  Just a sanity check!
        *)
        clsh := thsN.bloc.symTb.lookup(tpId.hash);
        ASSERT((clsh IS Id.TypId) & (clsh.type IS Ty.Opaque));

        thsN.bloc.symTb.Overwrite(tpId.hash, tpId);
      END;
(* ---- temporary ---- *)
END;
(* ---- temporary ---- *)
      APPEND(thsN.tIds, tpId);
    END;
  END MakeTypIds;

 (* ------------------------------------------------ *)
 (* ------------------------------------------------ * 

  PROCEDURE MakeRefIds(thsN : RefNamespace);
    VAR indx : INTEGER;
        thsC : Per.ClassRef;
        tEnu : INTEGER;
        tpId : Id.TypId;
  BEGIN
   (*
    *  For every namespace, define gpcp TypId descriptors for each class
    *)
    IF Glb.verbose THEN
      Glb.Message(" GPCP-Module name - " + Nh.charOpenOfHash(thsN.bloc.hash)^);
    END;
    FOR indx := 0 TO LEN(thsN.clss) - 1 DO
      thsC := thsN.clss[indx];
      IF Glb.Verbose THEN
        Console.WriteString("        class rfrnce ");
        Console.WriteString(gpName(thsC));
        Console.WriteLn;
      END;
      tpId := makeNameType(thsN.bloc, Nh.enterStr(gpName(thsC)));
      APPEND(thsN.tIds, tpId);
    END;
  END MakeRefIds;

  * ------------------------------------------------ *)
 (* ------------------------------------------------ *)

  PROCEDURE MakeBlkId*(spc : Namespace; aNm : Glb.CharOpen);
  BEGIN
    NEW(spc.bloc);
    INCL(spc.bloc.xAttr, Sy.need);
    Glb.BlkIdInit(spc.bloc, aNm, Nh.charOpenOfHash(spc.hash));
    IF Glb.superVb THEN Glb.Message("Creating blk - " +
                                     Nh.charOpenOfHash(spc.bloc.hash)^) END;
  END MakeBlkId;

 (* ------------------------------------------------ *)

  PROCEDURE DefineClss*(thsN : DefNamespace);
    VAR indx : INTEGER;
        tEnu : INTEGER;
        thsT : Sy.Type;
        thsI : Id.TypId;
        thsC : Per.ClassDef;
  BEGIN
   (*
    *  For every namespace, define gpcp descriptors 
    *  for each class, method, field and constant.
    *)
    FOR indx := 0 TO LEN(thsN.clss) - 1 DO
      thsC := thsN.clss[indx];
      thsI := thsN.tIds[indx];
      tEnu := getKind(thsC);

      CASE tEnu OF
      | valCls  : thsN.DefineRec(thsC, thsI.type(Ty.Record));
      | enuCls  : thsN.DefineEnu(thsC, thsI.type(Ty.Enum));
      | dlgCls  : thsN.DefinePrc(thsC, thsI.type(Ty.Procedure));
      | refCls  : thsT := thsI.type(Ty.Pointer).boundTp;
                  thsN.DefineRec(thsC, thsT(Ty.Record));
(*
 *    | evtCls  : thsN.MakeEvtCls(thsC, ); (* Can't distinguish from dlgCls! *)
 *)
      ELSE (* skip *)
      END;
    END;
  END DefineClss;

 (* ------------------------------------------------------------ *)
 (*    Separate flat class-list into lists for each namespace    *)
 (* ------------------------------------------------------------ *)

  PROCEDURE Classify*(IN  clss : ARRAY OF Per.ClassDef;
                      OUT nVec : VECTOR OF DefNamespace);
    VAR indx : INTEGER;
        thsC : Per.ClassDef;
        attr : Per.TypeAttr;
   (* ======================================= *)
    PROCEDURE Insert(nVec : VECTOR OF DefNamespace; 
                     thsC : Per.ClassDef);
      VAR thsH : INTEGER;
          jndx : INTEGER;
          nSpc : RTS.NativeString;
          cNam : RTS.NativeString;
          newN : DefNamespace;
    BEGIN
      nSpc := gpSpce(thsC);
      cNam := gpName(thsC);
      IF nSpc = "" THEN thsH := anon ELSE thsH := Nh.enterStr(nSpc) END;
     (*
      *  See if already a Namespace for this hash bucket
      *)
      FOR jndx := 0 TO LEN(nVec) - 1 DO
        IF nVec[jndx].hash = thsH THEN
          APPEND(nVec[jndx].clss, thsC); RETURN;    (* FORCED EXIT! *)
        END;
      END;
     (*
      *  Else insert in a new Namespace
      *)
      NEW(newN);                  (* Create new DefNamespace object    *)
      NEW(newN.clss, 8);          (* Create new vector of ClassDef     *)
      NEW(newN.tIds, 8);          (* Create new vector of Id.TypId     *)
      newN.hash := thsH;
      APPEND(newN.clss, thsC);    (* Append class to new class vector  *)
      APPEND(nVec, newN);         (* Append new DefNamespace to result *)
    END Insert;
   (* ======================================= *)
  BEGIN
    NEW(nVec, 8);
    FOR indx := 0 TO LEN(clss) - 1 DO
      thsC := clss[indx];
      IF isPublicClass(thsC) THEN 
        Insert(nVec, thsC);
      ELSIF Glb.verbose THEN
        SayWhy(thsC);
      END;
(* ------------------------------------- *
 *    attr := thsC.GetAttributes();
 *    IF isExportedType(attr) THEN 
 *      IF ~isGenericClass(thsC) THEN (* SPECIAL FOR PRE 1.4 VERSION *)
 *        Insert(nVec, thsC);
 *      ELSIF Glb.verbose THEN
 *        Glb.Message(" Hiding generic class -- " + 
 *                         gpSpce(thsC) + "." + gpName(thsC));
 *      END;
 *    END;
 * ------------------------------------- *)
    END;
    IF Glb.verbose THEN
      IF LEN(nVec) = 1 THEN
        Glb.Message(" Found one def namespace");
      ELSE
        Glb.Message(" Found "+Ltv.intToCharOpen(LEN(nVec))^+" def namespaces");
      END;
    END;
  END Classify;

(* ------------------------------------------------------------- *)
(* ------------------------------------------------------------- *)

  PROCEDURE InitCorLibTypes*();
  BEGIN
   (*
    *  Create import descriptor for [mscorlib]System
    *)
    Bi.MkDummyImport("mscorlib_System", "[mscorlib]System", corLib);
   (*
    *  Create various classes.
    *)
    ntvObj := makeNameType(corLib, Nh.enterStr("Object")).type;
    ntvStr := makeNameType(corLib, Nh.enterStr("String")).type;
    ntvExc := makeNameType(corLib, Nh.enterStr("Exception")).type;
    ntvTyp := makeNameType(corLib, Nh.enterStr("Type")).type;
    ntvEvt := makeNameType(corLib, Nh.enterStr("MulticastDelegate")).type;
    ntvVal := makeNameType(corLib, Nh.enterStr("ValueType")).type;
    ntvEnu := makeNameType(corLib, Nh.enterStr("Enum")).type;
   (*
    *  Do the unsigned types with no CP equivalent.
    *)
    sysU16 := makeNameType(corLib, Nh.enterStr("UInt16")).type;
    sysU32 := makeNameType(corLib, Nh.enterStr("UInt32")).type;
    sysU64 := makeNameType(corLib, Nh.enterStr("UInt64")).type;
    voidSt := makeNameType(corLib, Nh.enterStr("VoidStar")).type;
    intPtr := makeNameType(corLib, Nh.enterStr("IntPtr")).type;
    uIntPt := makeNameType(corLib, Nh.enterStr("UIntPtr")).type;
    tpdRef := makeNameType(corLib, Nh.enterStr("TypedReference")).type;
  END InitCorLibTypes;

(* ------------------------------------------------------------- *)
(*
  PROCEDURE ImportCorlib*();
  BEGIN
    Glb.InsertImport(corLib);
    INCL(corLib.xAttr, Sy.need);
  END ImportCorlib;
 *)
(* ------------------------------------------------------------- *)

  PROCEDURE ImportCorlib*(spc : DefNamespace);
  BEGIN
    IF (spc # NIL) & (spc.bloc # corLib) THEN
      IF ~Sy.refused(corLib, spc.bloc) THEN
        IF Glb.superVb THEN
          Console.WriteString("Inserting import <");
          Console.WriteString(Nh.charOpenOfHash(corLib.hash));
          Console.WriteString("> in Namespace ");
          Console.WriteString(Nh.charOpenOfHash(spc.bloc.hash));
          Console.WriteLn;
        END;
      END; 
    END;
    INCL(corLib.xAttr, Sy.need);
  END ImportCorlib;

(* ------------------------------------------------------------- *)

  PROCEDURE BindSystemTypes*();
    VAR blk : Id.BlkId;       (* The Blk descriptor *)
        tId : Sy.Idnt;
   (* -------------------------- *)
    PROCEDURE MakeAbstract(blk : Id.BlkId; hsh : INTEGER);
    BEGIN
      blk.symTb.lookup(hsh).type(Ty.Record).recAtt := Ty.isAbs;
    END MakeAbstract;
   (* -------------------------- *)
  BEGIN
   (*
    *  Load import descriptor for [mscorlib]System
    *)
    corLib := Glb.thisMod.symTb.lookup(
                        Nh.enterStr("mscorlib_System"))(Id.BlkId);
    blk := corLib;

   (* 
    *  THIS IS ONLY EXPERIMENTAL 
    *  We make the record types that correspond to the
    *  primitive types abstract to prevent the declaration
    *  of variables of these types.
    *
    *  The static methods can still be called, of course.
    *)
    MakeAbstract(blk, Nh.enterStr("Boolean"));
    MakeAbstract(blk, Nh.enterStr("Byte"));
    MakeAbstract(blk, Nh.enterStr("Char"));
    MakeAbstract(blk, Nh.enterStr("SByte"));
    MakeAbstract(blk, Nh.enterStr("Int16"));
    MakeAbstract(blk, Nh.enterStr("Int32"));
    MakeAbstract(blk, Nh.enterStr("Int64"));
    MakeAbstract(blk, Nh.enterStr("UInt16"));
    MakeAbstract(blk, Nh.enterStr("UInt32"));
    MakeAbstract(blk, Nh.enterStr("UInt64"));
   (*
    *  Create various classes.
    *)
    tId := blk.symTb.lookup(Nh.enterStr("Object"));
    ntvObj := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("String"));
    ntvStr := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("Exception"));
    ntvExc := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("Type"));
    ntvTyp := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("MulticastDelegate"));
    ntvEvt := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("ValueType"));
    ntvVal := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("Enum"));
    ntvEnu := tId.type;
   (*
    *  Do the unsigned types with no CP equivalent.
    *)
    tId := blk.symTb.lookup(Nh.enterStr("UInt16"));
    sysU16 := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("UInt32"));
    sysU32 := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("UInt64"));
    sysU64 := tId.type;
   (*
    *  Do the miscellaneous values
    *)
    tId := blk.symTb.lookup(Nh.enterStr("IntPtr"));
    voidSt := tId.type;
    intPtr := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("UIntPtr"));
    uIntPt := tId.type;

    tId := blk.symTb.lookup(Nh.enterStr("TypedReference"));
    tpdRef := tId.type;

  END BindSystemTypes;

(* ------------------------------------------------------------- *)
BEGIN
  Bi.InitBuiltins;
END ClsToType.
(* ------------------------------------------------------------- *)
