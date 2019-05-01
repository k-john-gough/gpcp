
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
   (* ################ *)
   (* Util := PeToCpsUtils_, only needed while bootstrapping v1.4.05 *)
   (* ################ *)

    Rfl := "[mscorlib]System.Reflection",
    Sys := "[mscorlib]System",
    FNm := FileNames,
    Mng := ForeignName,
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
        sysEnuT* = 12; sysDelT* = 13; sysExcT* = 14; 
        genTyp*  = 15; voidStar* = 16;

  CONST (* type attribute enumeration bits   *)
        absTp = 7; intTp = 5; sldTp = 8;

        (* field attribute enumeration bits  *)
        stFld = 4; ltFld = 6; 

        (* method attribute enumeration bits *)
        stMth = 4; fnMth = 5; vrMth = 6; nwMth = 8; abMth = 10;

 CONST  (* Binding Flags *)
        staticBF = 
		  Rfl.BindingFlags.Static + 
		  Rfl.BindingFlags.DeclaredOnly +
		  Rfl.BindingFlags.NonPublic +
		  Rfl.BindingFlags.Public;

        instanceBF = 
		  Rfl.BindingFlags.Instance + 
		  Rfl.BindingFlags.DeclaredOnly +
		  Rfl.BindingFlags.NonPublic +
		  Rfl.BindingFlags.Public;

		bothBF =
		  Rfl.BindingFlags.Instance + 
		  Rfl.BindingFlags.Static + 
		  Rfl.BindingFlags.DeclaredOnly +
		  Rfl.BindingFlags.NonPublic +
		  Rfl.BindingFlags.Public;

 (* ------------------------------------------------------------ *)

  TYPE  Namespace*    = POINTER TO ABSTRACT RECORD
                          nStr  : RTS.NativeString;
                          hash  : INTEGER;
                          bloc* : Id.BlkId;
                          tIds  : VECTOR OF Id.TypId;
                        END;

        DefNamespace* = POINTER TO RECORD (Namespace)
                          clss  : VECTOR OF Sys.Type;
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

  PROCEDURE^ cpTypeFromCTS(peT : Sys.Type; spc : DefNamespace) : Sy.Type;
  PROCEDURE^ AddTypeToId(thsC : Sys.Type; thsN : DefNamespace); 

 (* ------------------------------------------------ *)

  PROCEDURE isExportedType(attr : Rfl.TypeAttributes) : BOOLEAN;
    VAR bits : SET;
  BEGIN
    bits := BITS(attr) * {0..2};
    CASE ORD(bits) OF
    | 1, 2, 4, 7 : RETURN TRUE;
    ELSE RETURN FALSE;
    END;
  END isExportedType;

 (* ------------------------------------------------ *)

  PROCEDURE isProtectedType(attr : Rfl.TypeAttributes) : BOOLEAN;
    VAR bits : SET;
  BEGIN
    bits := BITS(attr) * {0..2};
    CASE ORD(bits) OF
    | 4, 7 : RETURN TRUE;
    ELSE RETURN FALSE;
    END;
  END isProtectedType;

 (* ------------------------------------------------ *)

  PROCEDURE isGenericClass(cls : Sys.Type) : BOOLEAN;
  BEGIN
    RETURN cls.get_IsGenericType();
  END isGenericClass;

 (* ------------------------------------------------ *)
 (* This method detects generic types that appear as *)
 (* byRef values that point to generic types. These  *)
 (* types have isGenericType false!                  *)
 (* ------------------------------------------------ *)
  PROCEDURE isGenericType(typ : Sys.Type) : BOOLEAN;
   (* --------------------------------- *)
    PROCEDURE hasBackTick(n : RTS.NativeString) : BOOLEAN;
	BEGIN
	  RETURN n.Contains(MKSTR("`"));
	END hasBackTick;
   (* --------------------------------- *)
  BEGIN 
    IF typ.get_IsArray() THEN 
      RETURN isGenericType(typ.GetElementType());
    ELSIF typ.get_HasElementType() & hasBackTick(typ.get_Name()) THEN
	  RETURN TRUE;
    ELSE
      RETURN typ.get_IsGenericType();
    END;
  END isGenericType;

 (* ------------------------------------------------ *)

  PROCEDURE isPublicClass(cls : Sys.Type) : BOOLEAN;
  BEGIN
    IF isGenericType(cls) THEN 
      RETURN FALSE;
    ELSIF cls.get_IsNested() THEN
      RETURN isExportedType(cls.get_Attributes())
           & isPublicClass(cls.get_DeclaringType());
    ELSE 
      RETURN TRUE;
    END;
  END isPublicClass;

   (* ------------------------------------------------ *)

  PROCEDURE MthReturnType(mth : Rfl.MethodBase) : Sys.Type;
  BEGIN
    WITH mth : Rfl.MethodInfo DO 
        RETURN mth.get_ReturnType();
    | mth : Rfl.ConstructorInfo DO 
        RETURN mth.get_DeclaringType();
    END;
  END MthReturnType;

 (* ------------------------------------------------ *)

  PROCEDURE hasGenericArg(mth : Rfl.MethodBase) : BOOLEAN;
    VAR idx : INTEGER;
        par : Sys.Type;
        prs : POINTER TO ARRAY OF Rfl.ParameterInfo;
  BEGIN
    prs := mth.GetParameters();
    FOR idx := 0 TO LEN(prs) - 1 DO
      par := prs[idx].get_ParameterType();
      IF isGenericType(par) THEN RETURN TRUE END;
    END;
    RETURN FALSE;
  END hasGenericArg;

 (* ------------------------------------------------ *)

  PROCEDURE isGenericMethod(mth : Rfl.MethodBase) : BOOLEAN;
  BEGIN
    RETURN mth.get_IsGenericMethod() OR 
           hasGenericArg(mth) OR 
           isGenericType(MthReturnType(mth));
  END isGenericMethod;

 (* ------------------------------------------------ *)

  PROCEDURE isVarargMethod(mth : Rfl.MethodBase) : BOOLEAN;
  BEGIN
    RETURN mth.get_CallingConvention() = Rfl.CallingConventions.VarArgs;
  END isVarargMethod;

 (* ------------------------------------------------ *)

  PROCEDURE gpInt(num : INTEGER) : RTS.NativeString;
  BEGIN
    RETURN Sys.Convert.ToString(num);
  END gpInt;

  PROCEDURE typName(typ : Sys.Type) : RTS.NativeString;
  BEGIN
    RETURN typ.get_Name();
(*
    RETURN Util.Utils.typName(typ);
 *)
  END typName;

 (* ------------------------------------------------ *)

  PROCEDURE gpName(typ : Sys.Type) : RTS.NativeString;
    VAR name : RTS.NativeString;
   (* --------------------------------- *)
   (* ----- Trim the trailing '&' ----- *)
    PROCEDURE trimAmp(n : RTS.NativeString) : RTS.NativeString;
    BEGIN
      RETURN n.Substring(0, n.get_Length()-1);
    END trimAmp;
   (* --------------------------------- *)
  BEGIN
    name := typName(typ);
    IF typ.get_IsByRef() THEN name := trimAmp(name) END;
    IF typ.get_IsNested() THEN
      RETURN gpName(typ.get_DeclaringType()) + "$" + name;
    ELSE
      RETURN name;
    END;
  END gpName;

(* ------------------------------------------------ *)

  PROCEDURE gpSpce(typ : Sys.Type) : RTS.NativeString;
  BEGIN
    RETURN typ.get_Namespace();
  END gpSpce;

 (* ------------------------------------------------ *)

  PROCEDURE isCorLibRef(res : Rfl.Assembly) : BOOLEAN;
    VAR str : RTS.NativeString;
  BEGIN

    IF res = NIL THEN
      RETURN FALSE;
    ELSE
      str := res.GetName().get_Name();
      RETURN ((str = "mscorlib") OR (str = "CommonLanguageRuntimeLibrary"));
    END;
  END isCorLibRef;

 (* ------------------------------------------------ *)

  PROCEDURE SayWhy(cls : Sys.Type);
    VAR str : Glb.CharOpen;
  BEGIN
    IF isGenericClass(cls) THEN str := BOX(" Hiding generic class -- "); 
    ELSIF cls.get_IsNested() THEN
      IF ~isExportedType(cls.get_Attributes()) THEN RETURN; (* just private! *)
      ELSIF ~isPublicClass(cls.get_DeclaringType()) THEN
        str := BOX(" Hiding public child of private class -- ");
      ELSE RETURN; 
      END;
    ELSE RETURN;
    END;
    Glb.Message(str^ + gpSpce(cls) + "." + gpName(cls));
  END SayWhy;

 (* ------------------------------------------------ *)

  PROCEDURE SayWhyBase(bas : Sys.Type; cls : Sys.Type);
    VAR str : Glb.CharOpen;
  BEGIN
    str := BOX(" Hiding class with generic base type -- ");
    Glb.Message(str^ + gpSpce(cls) + "." + gpName(cls));
	Glb.Message(" ... Base type is == " + gpSpce(bas) + "." + gpName(bas));
  END SayWhyBase;

 (* ------------------------------------------------ *)

  PROCEDURE getBaseKind(typ : Sys.Type) : INTEGER;
    VAR name : RTS.NativeString;
        rScp : Rfl.Assembly;
  BEGIN
    name := gpName(typ);
    IF isCorLibRef(typ.get_Assembly()) THEN
      IF    name = "Object"                  THEN RETURN objTyp;
      ELSIF name = "ValueType"               THEN RETURN sysValT;
      ELSIF name = "Enum"                    THEN RETURN sysEnuT;
      ELSIF name = "MulticastDelegate"       THEN RETURN sysDelT;
      ELSIF name = "Exception"               THEN RETURN sysExcT;
      END;
    END;
    (* -------------------------------------- *) RETURN refCls;      
  END getBaseKind;

 (* ------------------------------------------------ *)
  PROCEDURE getSysKind(name : RTS.NativeString) : INTEGER;
  BEGIN
    (* ASSERT: isCorLibRef(asm) is true *)
    IF    name = "Object"                  THEN RETURN objTyp;
    ELSIF name = "String"                  THEN RETURN strTyp;
    ELSIF name = "ValueType"               THEN RETURN sysValT;
    ELSIF name = "Enum"                    THEN RETURN sysEnuT;
    ELSIF name = "MulticastDelegate"       THEN RETURN sysDelT;
    ELSIF name = "Exception"               THEN RETURN sysExcT;
    ELSE (* -------------------------------- *) RETURN refCls;
    END;      
  END getSysKind;

 (* ------------------------------------------------ *)

  PROCEDURE getKind(typ : Sys.Type) : INTEGER;
    VAR pEnu : INTEGER;
        pTyp : Sys.Type;
        name : RTS.NativeString;
  BEGIN
    name := gpName(typ);
    IF typ.get_HasElementType() THEN
      IF typ.get_IsArray()      THEN RETURN arrTyp;
      ELSIF typ.get_IsPointer() THEN RETURN voidStar; (* ???? *)
      ELSIF typ.get_IsClass()   THEN RETURN refCls;
      END;
    END;
    IF    typ.get_IsGenericType()  THEN RETURN genTyp;
    ELSIF typ.get_IsPrimitive() THEN RETURN primTyp;
    ELSIF typ.get_IsEnum()      THEN RETURN enuCls;
    ELSIF typ.get_IsClass() OR 
          typ.get_IsInterface()THEN
      pTyp := typ.get_BaseType();
      IF pTyp # NIL THEN 
        pEnu := getBaseKind(pTyp);
        IF    pEnu = refCls     THEN RETURN refCls;
        ELSIF pEnu = sysDelT    THEN RETURN dlgCls;
        ELSIF isCorLibRef(typ.get_Assembly()) 
                                THEN RETURN getSysKind(name);
        ELSE (* ----------------- *) RETURN refCls;
        END;
      ELSIF name = "Object"     THEN RETURN objTyp;
      ELSE  (* ------------------ *) RETURN refCls;
      END;
    ELSIF typ.get_IsValueType() THEN
      IF name = "Void"          THEN RETURN voidTyp;
      ELSE (* ------------------- *) RETURN valCls;
      END;
    ELSE (* --------------------- *) RETURN default; 
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
    | genTyp   : RETURN BOX("<genericTp>          ");
    | voidStar : RETURN BOX("Sys.Void*            ");
    ELSE         RETURN BOX("unknown              ");
    END;
  END kindStr;

 (* ------------------------------------------------ *)

  PROCEDURE mapPrimitive(peT : Sys.Type) : Sy.Type;
    VAR name : RTS.NativeString;
  BEGIN
    name := typName(peT);
    IF    name = "Char"    THEN RETURN Bi.charTp;
    ELSIF name = "Int32"   THEN RETURN Bi.intTp;
    ELSIF name = "Int16"   THEN RETURN Bi.sIntTp;
    ELSIF name = "Int64"   THEN RETURN Bi.lIntTp;
    ELSIF name = "SByte"   THEN RETURN Bi.byteTp;
    ELSIF name = "UInt8"   THEN RETURN Bi.uBytTp;
    ELSIF name = "Byte"   THEN RETURN Bi.uBytTp;
    ELSIF name = "Single"  THEN RETURN Bi.sReaTp;
    ELSIF name = "Double"  THEN RETURN Bi.realTp;
    ELSIF name = "Boolean" THEN RETURN Bi.boolTp;

    ELSIF name = "UInt16"  THEN RETURN sysU16;
    ELSIF name = "UInt32"  THEN RETURN sysU32;
    ELSIF name = "UInt64"  THEN RETURN sysU64;
    ELSIF name = "IntPtr"  THEN RETURN intPtr;
    ELSIF name = "UIntPtr" THEN RETURN uIntPt;
    ELSIF name = "TypeRef" THEN RETURN tpdRef;

    ELSE RTS.Throw("Unimplemented Method Branch " + name); RETURN NIL;
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
    IF Sy.refused(tId, blk) THEN 
      Glb.AbortMsg("bad TypId insert");
    ELSIF tId.namStr = NIL THEN 
      tId.SetNameFromHash(hsh);
    END;
    RETURN tId;
  END makeNameType;

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

  PROCEDURE lookup(peT : Sys.Type; nSp : DefNamespace) : Sy.Type;
    VAR asm : Glb.CharOpen;  (* assembly file name *)
        spc : Glb.CharOpen;  (* namespace name str *)
        mNm : Glb.CharOpen;  (* CP module name     *)
        blk : Sy.Idnt;       (* The Blk descriptor *)
        bId : Id.BlkId;      (* The Blk descriptor *)
        tId : Sy.Idnt;       (* TypId descriptor   *)
        hsh : INTEGER;       (* Class name hash    *)
        cNm : RTS.NativeString;  (* PE file class name *)
        byRefArray : BOOLEAN;    (* byRef AND an array *)
   (* -------------------------------------------- *)
    PROCEDURE NoteImport(spc : DefNamespace; imp : Id.BlkId);
    BEGIN
      IF (spc # NIL) & (spc.bloc # imp) THEN
        (* IF ~Sy.trackedRefused(imp, spc.bloc) THEN *)
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
   (* -------------------------------------------- *
   // Here is an ugly kludge.
   // We ignore generic types as found in the original
   // list of exported types in PeToCps::Process.
   // However, at least one formal param in the API
   // is of this type and the param type is NOT 
   // marked as generic. Thus before we insert this
   // wrongly classified type into our type list we
   // must check if it is already known as generic.
   * -------------------------------------------- *)
    PROCEDURE OnIgnoreList(nSp : Glb.CharOpen; tNm : Glb.CharOpen) : BOOLEAN;
      VAR qualHash : INTEGER;
          foundId  : Sy.Idnt;
    BEGIN
      qualHash := Nh.enterStr(nSp^ + "." + tNm^);
      foundId := Glb.ignoreBlk.symTb.lookup(qualHash);
      RETURN foundId # NIL;
    END OnIgnoreList;
  (* -------------------------------------------- *)
  BEGIN
    bId := NIL;
    byRefArray := FALSE;
   (*
    *  First we establish the (mangled) name of the defining scope.
    *)
    asm := BOX(peT.get_Assembly().GetName().get_Name());
    spc := BOX(gpSpce(peT));
    mNm := Mng.MangledName(asm, spc);
   (*
    *  Check if this name is already known to PeToCps
    *)
    blk := Glb.thisMod.symTb.lookup(Nh.enterStr(mNm));
    cNm := gpName(peT);
    IF cNm.EndsWith("[]") THEN
      cNm := cNm.Substring(0, cNm.get_Length()-2);
      byRefArray := TRUE;
    ELSIF cNm.EndsWith("*") THEN 
      RETURN voidSt;
    END;
    hsh := Nh.enterStr(cNm);
    WITH blk : Id.BlkId DO
        tId := blk.symTb.lookup(hsh);
       (*
        *  The module name is known to PeToCps.
        *  However, it may not have been listed as an import
        *  into the current namespace, in the case of multiple
        *  namespaces defined in the same source PEFile.
        *)
        NoteImport(nSp, blk);
        IF (tId # NIL) & (tId IS Id.TypId) THEN
          IF byRefArray THEN
            RETURN ptrToArrayOf(tId.type);
          ELSE
            RETURN tId.type;
          END;
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
	  bId.SetNameFromString(spc);
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
    IF OnIgnoreList(spc, BOX(cNm)) THEN
      RETURN NIL;
    ELSE
      tId := makeNameType(bId, hsh);
    END;
    RETURN tId.type;
  END lookup;

 (* ------------------------------------------------ *)

  PROCEDURE cpTypeFromCTS(peT : Sys.Type; spc : DefNamespace) : Sy.Type;
    VAR kind : INTEGER;
        rslt : Sy.Type;
  BEGIN
    kind := getKind(peT);
    CASE kind OF
    | voidTyp  : rslt := NIL;
    | arrTyp   : rslt := ptrToArrayOf(
                             cpTypeFromCTS(peT.GetElementType(), spc));
    | primTyp  : rslt := mapPrimitive(peT);
    | strTyp   : rslt := ntvStr;
    | objTyp   : rslt := ntvObj;
    | sysValT  : rslt := ntvVal;
    | sysEnuT  : rslt := ntvEnu;
    | sysDelT  : rslt := ntvEvt;
    (* There is no CP type corresonding to unmanaged pointers (voidSt;) *)
    | voidStar : rslt := voidSt; 
    ELSE (* default, refCls, valCls, enuCls, evtCls, dlgCls *)  
      IF peT.get_IsClass() OR peT.get_IsInterface() OR peT.get_IsValueType() THEN        
        rslt := lookup(peT, spc);
      ELSE
        IF peT # NIL THEN
          Console.WriteString("Not a class -- ");
          Console.WriteLn;
        END;
        rslt := NIL;
      END;
    END;
    IF (rslt IS Ty.Opaque) THEN
      ASSERT((rslt.idnt # NIL) & (rslt.idnt.dfScp # NIL));
    END;
    RETURN rslt;
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

  PROCEDURE modeFromFldAtt(att : SET) : INTEGER;
  BEGIN
    CASE ORD(att * {0,1,2,5}) OF
    | 4, 5 : RETURN Sy.protect;
    | 6    : RETURN Sy.pubMode;
	| 26H  : RETURN Sy.rdoMode; (* Actually InitOnly for fields *)
    ELSE     RETURN Sy.prvMode;
    END;
  END modeFromFldAtt;

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
    par.SetNameFromHash(par.hash);
    par.isRcv  := rcv;
    RETURN par;
  END mkParam;

 (* ------------------------------------------------------------ *)

  PROCEDURE isValClass(cls : Sys.Type) : BOOLEAN;
  BEGIN
    RETURN getKind(cls) = valCls;
  END isValClass;

 (* ------------------------------------------------------------ *)
 (*                   Main processing code                       *)
 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)AddRecFld(rec : Ty.Record; 
                                          fld : Rfl.FieldInfo), NEW;
    VAR   mod : INTEGER;
          hsh : INTEGER;
          bts : SET;
          res : BOOLEAN;
          typ : Sys.Type;
          fId : Id.FldId;
          vId : Id.VarId;
          cId : Id.ConId;
		  raw : Sys.Object;
   (* ------------------------------------ *)
   (* ------------------------------------ *)

    PROCEDURE conExp(val : Sys.Object) : Sy.Expr;
      VAR byts : POINTER TO ARRAY OF UBYTE;
          chrs : POINTER TO ARRAY OF CHAR;
          indx : INTEGER;
          type : RTS.NativeType;
          tNam : RTS.NativeString;
    BEGIN [UNCHECKED_ARITHMETIC]
      type := val.GetType();
      tNam := typName(type);
      IF (tNam = "Double") OR (tNam = "UInt64") THEN
        RETURN Xp.mkRealLt(Sys.Convert.ToDouble(val));
      ELSIF tNam = "Single" THEN
        RETURN Xp.mkRealLt(Sys.Convert.ToDouble(val));
      ELSIF tNam = "Char" THEN
        RETURN Xp.mkCharLt(Sys.Convert.ToChar(val));
      ELSIF (tNam = "Int16") OR (tNam = "Int32") OR(tNam = "Int64") OR
            (* (tNam = "UInt64") OR*) (tNam = "UInt32") OR (tNam = "UInt16") OR
            (tNam = "SByte") OR (tNam = "Byte") THEN
        RETURN Xp.mkNumLt(Sys.Convert.ToInt64(val));
      ELSIF tNam = "String" THEN
        RETURN Xp.mkStrLt(Sys.Convert.ToString(val));
      ELSE
        RTS.Throw("Unimplemented Method conExp " + tNam);
        RETURN NIL;
      END;
    END conExp;
   (* ------------------------------------ *)
  BEGIN
    bts := BITS(fld.get_Attributes());
    mod := modeFromFldAtt(bts);
    typ := fld.get_FieldType();
    IF isGenericType(typ) THEN RETURN END;
    IF mod > Sy.prvMode THEN
      hsh := Nh.enterStr(fld.get_Name());
      IF ltFld IN bts THEN                 (* literal field  *)
        cId := Id.newConId();
        cId.hash := hsh;
        cId.SetNameFromHash(hsh);
        cId.SetMode(mod);
        cId.recTyp := rec;
        cId.type := cpTypeFromCTS(typ, spc);

		raw := fld.GetRawConstantValue();

        cId.conExp := conExp(raw);
        res := rec.symTb.enter(hsh, cId);
        Sy.AppendIdnt(rec.statics, cId);
      ELSIF stFld IN bts THEN              (* static field   *)
        vId := Id.newVarId();
        vId.hash := hsh;
        vId.SetNameFromHash(hsh);
        vId.SetMode(mod);
        vId.recTyp := rec;
        vId.type := cpTypeFromCTS(typ, spc);
        res := rec.symTb.enter(hsh, vId);
        Sy.AppendIdnt(rec.statics, vId);
      ELSE                                 (* instance field *)
        fId := Id.newFldId();
        fId.hash := hsh;
        fId.SetNameFromHash(hsh);
        fId.SetMode(mod);
        fId.recTyp := rec;
        fId.type := cpTypeFromCTS(typ, spc);
        res := rec.symTb.enter(hsh, fId);
        Sy.AppendIdnt(rec.fields, fId);
      END;
    END;
  END AddRecFld;

 (* ------------------------------------------------------------ *
  PROCEDURE (spc : DefNamespace)AddFormals(typ : Ty.Procedure; 
                                           mth : Rfl.MethodBase), NEW;
    VAR indx : INTEGER;
        pMod : INTEGER;
        thsP : Rfl.ParameterInfo;
        thsT : Sys.Type;
        cpTp : Sy.Type;
        pPar : Id.ParId;
        pars : POINTER TO ARRAY OF Rfl.ParameterInfo;

  BEGIN
    typ.retType := cpTypeFromCTS(MthReturnType(mth), spc);  
    pars := mth.GetParameters();
    FOR indx := 0 TO LEN(pars) - 1 DO
      pMod := Sy.val;
      thsP := pars[indx];
      thsT := thsP.get_ParameterType();
      IF thsT.get_IsByRef() THEN pMod := Sy.var END;
      cpTp := cpTypeFromCTS(thsT, spc);

      pPar := mkParam(thsP.get_Name(), pMod, cpTp, FALSE);
      (* pPar := mkParam(thsP.get_Name(), pMod, cpTypeFromCTS(thsT, spc), FALSE); *)
      Id.AppendParam(typ.formals, pPar);
    END;
  END AddFormals;
  *)

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)AddFormalsOK(typ : Ty.Procedure; 
                                             mth : Rfl.MethodBase) : BOOLEAN, NEW;
    VAR indx : INTEGER;
        pMod : INTEGER;
        thsP : Rfl.ParameterInfo;
        thsT : Sys.Type;
        cpTp : Sy.Type;
        pPar : Id.ParId;
        pars : POINTER TO ARRAY OF Rfl.ParameterInfo;

  BEGIN
    typ.retType := cpTypeFromCTS(MthReturnType(mth), spc);  
    pars := mth.GetParameters();
    FOR indx := 0 TO LEN(pars) - 1 DO
      pMod := Sy.val;
      thsP := pars[indx];
      thsT := thsP.get_ParameterType();
      IF thsT.get_IsByRef() THEN
        IF thsP.get_IsOut() THEN
          pMod := Sy.out;
        ELSE
          pMod := Sy.var;
        END;
      END;
      cpTp := cpTypeFromCTS(thsT, spc);
      IF cpTp = NIL THEN RETURN FALSE END;
      pPar := mkParam(thsP.get_Name(), pMod, cpTp, FALSE);
      Id.AppendParam(typ.formals, pPar);
    END;
    RETURN TRUE;
  END AddFormalsOK;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)AddRecMth(rec : Ty.Record; 
                                          mth : Rfl.MethodBase), NEW;
    VAR   mod : INTEGER;
          hsh : INTEGER;
          pMd : INTEGER;
          bts : SET;
          res : BOOLEAN;
          pId : Id.PrcId;
          mId : Id.MthId;
          dnR : Sys.Type;        (* .NET Receiver type *)
          cpR : Sy.Type;         (*   CP Receiver type *)  
          pTp : Ty.Procedure;
  BEGIN
   (* SPECIAL FOR PRE 1.4 VERSION *)
    IF isGenericMethod(mth) THEN
      Glb.CondMsg(" Hiding generic method -- " + mth.ToString());
      RETURN;
    ELSIF isVarargMethod(mth) THEN
      Glb.CondMsg(" Hiding Vararg call method -- " + mth.ToString());
      RETURN;
    END;
    bts := BITS(mth.get_Attributes());
    mod := modeFromMbrAtt(bts);
    IF mod > Sy.prvMode THEN
      hsh := Nh.enterStr(mth.get_Name());

      IF stMth IN bts THEN                (* static method *)
        pId := Id.newPrcId(); 
        pId.SetKind(Id.conPrc);
        pId.hash := hsh;
        pId.SetMode(mod);
        pId.SetNameFromHash(hsh);
        pTp := Ty.newPrcTp();
        pTp.idnt := pId;
        pId.type := pTp;
        IF ~spc.AddFormalsOK(pTp, mth) THEN RETURN END;
        res := rec.symTb.enter(hsh, pId);
        Sy.AppendIdnt(rec.statics, pId);
        Glb.ListTy(pTp);

      ELSIF hsh = Glb.ctorBkt THEN        (* constructor method *)
        pId := Id.newPrcId(); 
        pId.SetKind(Id.ctorP);
        pId.hash := Glb.initBkt;
        pId.prcNm := BOX(".ctor");
        pId.SetMode(mod);
        pId.SetNameFromHash(Glb.initBkt);
        pTp := Ty.newPrcTp();
        pTp.idnt := pId;
        pId.type := pTp;
        IF ~spc.AddFormalsOK(pTp, mth) THEN RETURN END;
        res := rec.symTb.enter(Glb.initBkt, pId);
        Sy.AppendIdnt(rec.statics, pId);
        Glb.ListTy(pTp);

      ELSE                                (* instance method *)
        mId := Id.newMthId();
        mId.SetKind(Id.conMth);
        mId.hash := hsh;
        mId.SetMode(mod);
        mId.SetNameFromHash(hsh);

        pMd := Sy.val;
        dnR := mth.get_DeclaringType();
        cpR := cpTypeFromCTS(dnR, spc);
        IF cpR = NIL THEN RETURN END;

        IF isValClass(dnR) THEN pMd := Sy.var END;
        mId.rcvFrm := mkParam("this", pMd, cpR, TRUE);
        pTp := Ty.newPrcTp();
        pTp.idnt := mId;
        mId.type := pTp;
        pTp.receiver := rec;
        IF ~spc.AddFormalsOK(pTp, mth) THEN RETURN END;
        IF    abMth IN bts    THEN 
          mId.mthAtt := Id.isAbs;
        ELSIF (vrMth IN bts) & ~(fnMth IN bts) THEN 
          mId.mthAtt := Id.extns;
        END;
        IF ~(vrMth IN bts) OR (nwMth IN bts) THEN 
          INCL(mId.mthAtt, Id.newBit);
        END;
        res := rec.symTb.enter(hsh, mId);
        Sy.AppendIdnt(rec.methods, mId);
      END;
    END;
  END AddRecMth;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)AddRecEvt(rec : Ty.Record; 
                                          evt : Rfl.EventInfo), NEW;
    VAR   eTp : Sys.Type;
          nam : RTS.NativeString;
          hsh : INTEGER;
          fId : Id.FldId;
          res : BOOLEAN;
  BEGIN
    eTp := evt.get_EventHandlerType();
    IF isGenericType(eTp) THEN RETURN END;
    nam := evt.get_Name();
    hsh := Nh.enterStr(nam);
    fId := Id.newFldId();
    fId.hash := hsh;
    fId.SetNameFromHash(hsh);
    fId.SetMode(Sy.pubMode);
    fId.recTyp := rec;
    fId.type := cpTypeFromCTS(eTp, spc);
    res := rec.symTb.enter(hsh, fId);
    Sy.AppendIdnt(rec.fields, fId);
  END AddRecEvt;

 (* ------------------------------------------------------------ *)

  PROCEDURE MakeRefCls(cls : Sys.Type; 
                       spc : DefNamespace;
                       att : Rfl.TypeAttributes;
                       tId : Id.TypId);
    VAR ptr  : Ty.Pointer;
   (* ------------------------------------------------- *)
    PROCEDURE mkRecord(cls : Sys.Type;
                       spc : DefNamespace;
                       att : Rfl.TypeAttributes) : Ty.Record;
      VAR rec : Ty.Record;
          bts : SET;
    BEGIN
      bts := BITS(att);
      rec := Ty.newRecTp();
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
    tId.type := ptr;
    ptr.idnt := tId;
    ptr.boundTp := mkRecord(cls, spc, att);
    ptr.boundTp(Ty.Record).bindTp := ptr;
    Glb.ListTy(ptr);
  END MakeRefCls;

 (* ------------------------------------------------------------ *)

  PROCEDURE MakeEnumTp(cls : Sys.Type; 
                       tId : Id.TypId);
    VAR enu : Ty.Enum;
  BEGIN
   (*
    *  Create the descriptors.
    *)
    enu := Ty.newEnuTp();
    tId.type := enu;
    enu.idnt := tId;
    Glb.ListTy(enu);
  END MakeEnumTp;

 (* ------------------------------------------------ *)

  PROCEDURE MakeValCls(cls : Sys.Type; 
                       tId : Id.TypId);
    VAR rec  : Ty.Record;
  BEGIN
   (*
    *  Create the descriptors.
    *)
    rec := Ty.newRecTp();
    tId.type := rec;
    rec.idnt := tId;
    IF ~Glb.cpCmpld THEN INCL(rec.xAttr, Sy.isFn) END;
    Glb.ListTy(rec);
  END MakeValCls;

  PROCEDURE MakePrimitiveHost(cls : Sys.Type; tId : Id.TypId);
  BEGIN
    MakeValCls(cls, tId);
    tId.type(Ty.Record).recAtt := Ty.isAbs;
  END MakePrimitiveHost;

 (* ------------------------------------------------ *)

  PROCEDURE MakePrcCls(cls : Sys.Type; 
                       tId : Id.TypId);
    VAR prc  : Ty.Procedure;
  BEGIN
   (*
   (*  Create the descriptor.                 *)
   (* We have no way of distinguishing between *)
   (* CP EVENT and CP PROCEDURE types from the *)
   (* PE-file.  So, default to EVENT meantime. *)
    *)
    prc := Ty.newEvtTp();
    prc.idnt := tId;
    tId.type := prc;
    Glb.ListTy(prc);
  END MakePrcCls;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)DefinePrimary(cls : Sys.Type;
                                              rec : Ty.Record), NEW;
    VAR indx : INTEGER;
        flds : POINTER TO ARRAY OF Rfl.FieldInfo;
        mths : POINTER TO ARRAY OF Rfl.MethodInfo;
        mthI : Rfl.MethodInfo;
        fldI : Rfl.FieldInfo;
		btsI : SET;
   (* ----------------------------------------------- *)
  BEGIN
   (*
    *  Now fill in record fields ...
    *)
    flds := cls.GetFields();
    FOR indx := 0 TO LEN(flds) - 1 DO
      fldI := flds[indx];
	  btsI := BITS(fldI.get_Attributes());
     (*
      *  Don't emit inherited or instance fields.
      *)
      IF (fldI.get_DeclaringType() = cls) & (stFld IN btsI) THEN 
        spc.AddRecFld(rec, fldI);
      END;
    END;
   (*
    *  Now fill in record methods ...
    *)
    mths := cls.GetMethods();
    FOR indx := 0 TO LEN(mths) - 1 DO
      mthI := mths[indx];
	  btsI := BITS(mthI.get_Attributes());
     (*
      *  Don't emit inherited or instance methods.
      *)
      IF (mthI.get_DeclaringType() = cls) & (stMth IN btsI) THEN 
        spc.AddRecMth(rec, mths[indx]);
      END;
    END;
  END DefinePrimary;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)DefineRec(cls : Sys.Type;
                                          rec : Ty.Record), NEW;
    VAR indx : INTEGER;
        flds : POINTER TO ARRAY OF Rfl.FieldInfo;
        evts : POINTER TO ARRAY OF Rfl.EventInfo;
        mths : POINTER TO ARRAY OF Rfl.MethodInfo;
        cons : POINTER TO ARRAY OF Rfl.ConstructorInfo;
        nTps : POINTER TO ARRAY OF Sys.Type;
        mthI : Rfl.MethodInfo;
        fldI : Rfl.FieldInfo;
   (* ----------------------------------------------- *)
    PROCEDURE FixBaseAndInterfaces(spc : DefNamespace; 
                                   cls : Sys.Type; 
                                   rec : Ty.Record);
      VAR index : INTEGER;
          super : Sys.Type;
          ifElm : Sys.Type;
          cpTyp : Sy.Type;
          ifArr : POINTER TO ARRAY OF Sys.Type;
    BEGIN
      super := cls.get_BaseType();
      ifArr := cls.GetInterfaces();
      IF super # NIL THEN 
	    IF ~super.get_IsGenericType() & isPublicClass(super) THEN
		  rec.baseTp := cpTypeFromCTS(super, spc);
        ELSIF Glb.verbose THEN
		 (* 
		  * We exclude non-generic types that are 
		  * instantiated from a generic base type.
		  * In principal this could be allowed, but
		  * requires many code changes. (kjg 2019)
		  *)
          SayWhyBase(super, cls);
        END;
      END;
      IF ifArr # NIL THEN
        FOR index := 0 TO LEN(ifArr) - 1 DO
          ifElm := ifArr[index];
          IF ~ifElm.get_IsGenericType() & isPublicClass(ifElm) THEN
            cpTyp := cpTypeFromCTS(ifElm, spc);
            Sy.AppendType(rec.interfaces, cpTyp);
          ELSIF Glb.verbose THEN 
            SayWhy(ifElm);
          END;
        END;
      END;
    END FixBaseAndInterfaces;
   (* ----------------------------------------------- *)
  BEGIN
   (*
    *  First we must add resolved base and interface types.
 	* name := cls.get_Name();
   *)
    FixBaseAndInterfaces(spc, cls, rec);
	IF cls.get_IsValueType() THEN INCL(rec.xAttr, Sy.valTp);
	ELSIF cls.get_IsClass() THEN INCL(rec.xAttr, Sy.clsTp);
	END;
   (*
    *  Now fill in record fields ...
    *)
	flds := cls.GetFields(bothBF);
    FOR indx := 0 TO LEN(flds) - 1 DO
      fldI := flds[indx];
      spc.AddRecFld(rec, fldI);
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
    mths := cls.GetMethods(bothBF);
    FOR indx := 0 TO LEN(mths) - 1 DO
      mthI := mths[indx];
      spc.AddRecMth(rec, mths[indx]);
    END;
   (*
    *  Now fill in constructors ...
    *  even if there are no instance members.
    *)
    cons := cls.GetConstructors(instanceBF);
    FOR indx := 0 TO LEN(cons) - 1 DO 
      spc.AddRecMth(rec, cons[indx]);
    END;
    Glb.VerbMsg(gpName(cls) + " " + gpInt(LEN(mths)) + " methods");
  END DefineRec;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)DefineEnu(cls : Sys.Type; 
                                          enu : Ty.Enum), NEW;
    CONST litB = 6; (* 40H *)
    VAR   indx : INTEGER;
          valu : LONGINT;
          flds : POINTER TO ARRAY OF Rfl.FieldInfo;
          thsF : Rfl.FieldInfo;
          thsC : Id.ConId;
          mode : INTEGER;
          bits : SET;
          sCon : Sys.Object;
   (* ------------------------------------ *)
    PROCEDURE conExp(val : Sys.Object) : LONGINT;
      VAR type : RTS.NativeType;
          tNam : RTS.NativeString;
    BEGIN
      type := val.GetType();
      tNam := typName(type);
      IF (tNam = "Int16") OR 
         (tNam = "Int32") OR
         (tNam = "Int64") OR 
         (tNam = "Byte") THEN
        RETURN Sys.Convert.ToInt64(val);
      ELSE
        RTS.Throw("Unimplemented Method conExp type " + tNam);
        RETURN 0;
      END;
    END conExp;
   (* ------------------------------------ *)
  BEGIN
   (*
    *  Now fill in record details ...
    *)
    valu := 0;
    flds := cls.GetFields();
    FOR indx := 0 TO LEN(flds) - 1 DO
      thsF := flds[indx];
      bits := BITS(thsF.get_Attributes());
      mode := modeFromMbrAtt(bits);
      IF (mode > Sy.prvMode) & (litB IN bits) THEN
        sCon := thsF.GetRawConstantValue();
        valu := conExp(sCon);
        thsC := Id.newConId();
        thsC.SetMode(mode);
        thsC.hash := Nh.enterStr(thsF.get_Name());
        thsC.SetNameFromHash(thsC.hash);
        thsC.conExp := Xp.mkNumLt(valu);
        thsC.type := Bi.intTp;
        Sy.AppendIdnt(enu.statics, thsC);
      END;
    END;
  END DefineEnu;

 (* ------------------------------------------------------------ *)

  PROCEDURE (spc : DefNamespace)DefinePrc(cls : Sys.Type; 
                                          prc : Ty.Procedure), NEW;
    VAR invk : Rfl.MethodInfo;
        junk : BOOLEAN;
  BEGIN
   (*
    *  Now fill in parameter details ...
    *)
    invk := cls.GetMethod(MKSTR("Invoke"));
    junk := spc.AddFormalsOK(prc, invk);
  END DefinePrc;

 (* ------------------------------------------------------------ *)
 (*                         Not exported                         *)
 (* ------------------------------------------------------------ *)
  PROCEDURE AddTypeToId(thsC : Sys.Type; thsN : DefNamespace); 
    VAR tEnu : INTEGER;
        hash : INTEGER;
        tpId : Id.TypId;
        clsh : Sy.Idnt;
        attr : Rfl.TypeAttributes;
  BEGIN
    hash := Nh.enterStr(gpName(thsC));
    tpId := thsN.bloc.symTb.lookup(hash)(Id.TypId);
    tEnu := getKind(thsC);
    attr := thsC.get_Attributes();

    Glb.VerbMsg(kindStr(tEnu)^ + " " + gpName(thsC)); 

    CASE tEnu OF
    | primTyp, voidTyp : 
         MakePrimitiveHost(thsC, tpId);
    | refCls, objTyp, strTyp, 
     (* 
      * All of the follwing types are reference types
      * even although all of their derived types are not.
      *)
      sysDelT, sysExcT, sysEnuT, sysValT  : 
         MakeRefCls(thsC, thsN, attr, tpId);
    | valCls  : MakeValCls(thsC, tpId);
    | enuCls  : MakeEnumTp(thsC, tpId);
    | dlgCls  : MakePrcCls(thsC, tpId);
    ELSE
      RTS.Throw("Unknown sub-type");
      RETURN; 
    END;
   (* 
    *  Set the access mode for tpId 
    *)
    IF isProtectedType(attr) THEN
      tpId.SetMode(Sy.protect);
    ELSE
      tpId.SetMode(Sy.pubMode);
    END;
  END AddTypeToId;

 (* ------------------------------------------------------------ *)

  PROCEDURE AddTypesToIds*(thsN : DefNamespace);
    VAR indx : INTEGER;
  BEGIN
   (*
    *  For every namespace, define gpcp descriptors for
    *  each class on this namespace's class vector.
	*  All the descriptors are added to the tIds vector.
    *)
    Glb.CondMsg(" CP Module name - " + Nh.charOpenOfHash(thsN.bloc.hash)^);
    Glb.CondMsg(' Alternative import name - "' + thsN.bloc.scopeNm^ + '"');
    FOR indx := 0 TO LEN(thsN.clss) - 1 DO
      AddTypeToId(thsN.clss[indx], thsN);
    END;
  END AddTypesToIds;

 (* ------------------------------------------------ *)
 (* ------------------------------------------------ *)

  PROCEDURE MakeBlkId*(spc : Namespace; aNm : Glb.CharOpen);
    VAR name : Glb.CharOpen;
  BEGIN
    NEW(spc.bloc);
    INCL(spc.bloc.xAttr, Sy.need);
    name := Nh.charOpenOfHash(spc.hash);
    spc.bloc.SetNameFromHash(spc.hash);
    Glb.BlkIdInit(spc.bloc, aNm, name);
    IF Glb.superVb & (name # NIL) THEN Glb.Message("Creating blk - " + name^) END;
  END MakeBlkId;

 (* ------------------------------------------------ *)

  PROCEDURE DefineClss*(thsN : DefNamespace);
    VAR indx : INTEGER;
        tEnu : INTEGER;
        thsT : Sy.Type;
        thsI : Id.TypId;
        thsC : Sys.Type;
  BEGIN
   (*
    *  For every namespace, define gpcp descriptors 
    *  for each class, method, field and constant.
    *)
    indx := 0;
    FOR indx := 0 TO LEN(thsN.clss) - 1 DO
      thsC := thsN.clss[indx];
      thsI := thsN.tIds[indx];
      tEnu := getKind(thsC);
      CASE tEnu OF
      | valCls  : thsN.DefineRec(thsC, thsI.type(Ty.Record));
      | enuCls  : thsN.DefineEnu(thsC, thsI.type(Ty.Enum));
   (* | evtCls  : (* Can't distinguish from dlgCls! *) *)
      | dlgCls  : thsN.DefinePrc(thsC, thsI.type(Ty.Procedure));
      | primTyp : thsN.DefinePrimary(thsC, thsI.type(Ty.Record));
      | refCls, objTyp, strTyp, 
      (* 
       * All of the follwing types are reference types
       * even although all of their derived types are not.
       *)
        sysDelT, sysExcT, sysEnuT, sysValT  : 
                  thsT := thsI.type(Ty.Pointer).boundTp;
                  thsN.DefineRec(thsC, thsT(Ty.Record));
      ELSE (* skip *)
      END;
    END;
  END DefineClss;

 (* ------------------------------------------------------------ *)
 (*    Separate flat class-list into lists for each namespace    *)
 (* ------------------------------------------------------------ *)
  PROCEDURE Classify*(IN  tVec : ARRAY OF Sys.Type;
                      OUT nVec : VECTOR OF DefNamespace);
    VAR indx : INTEGER;
        thsC : Sys.Type;
        (* attr : Rfl.TypeAttributes; *)
   (* ======================================= *)
    PROCEDURE Insert(nVec : VECTOR OF DefNamespace; 
                     thsC : Sys.Type);
      VAR thsH : INTEGER;
          jndx : INTEGER;
          nSpc : RTS.NativeString;
          cNam : RTS.NativeString;
          newN : DefNamespace;
          idxN : DefNamespace;
          tpId : Id.TypId;
    BEGIN
      nSpc := gpSpce(thsC);
      cNam := gpName(thsC);
      tpId := Id.newTypId(NIL);
      tpId.hash := Nh.enterStr(cNam);
      tpId.SetNameFromHash(tpId.hash);

      IF (nSpc = NIL) OR 
         (nSpc = "") THEN thsH := anon ELSE thsH := Nh.enterStr(nSpc) END;
     (*
      *  See if there is already a Namespace for this hash bucket
      *  Linear search is slow, but LEN(nVec) is usually short
      *)
      FOR jndx := 0 TO LEN(nVec) - 1 DO
        idxN := nVec[jndx];
        IF idxN.hash = thsH THEN
          APPEND(idxN.clss, thsC); 
          APPEND(idxN.tIds, tpId);
          ASSERT(idxN.bloc.symTb.enter(tpId.hash, tpId));
          tpId.dfScp := idxN.bloc; 
          RETURN;    (* FORCED EXIT! *)
        END;
      END;
     (*
      *  Else insert in a new Namespace
      *)
      NEW(newN);                  (* Create new DefNamespace object    *)
      NEW(newN.clss, 8);          (* Create new vector of ClassDef     *)
      NEW(newN.tIds, 8);          (* Create new vector of Id.TypId     *)
      IF thsH = anon THEN
        newN.nStr := MKSTR("<UnNamed>");
      ELSE 
        newN.nStr := nSpc;
        newN.hash := thsH;
      END;      
      MakeBlkId(newN, Glb.basNam);
      APPEND(nVec, newN);         (* Append new DefNamespace to result *)
      APPEND(newN.clss, thsC);    (* Append class to new clss vector  *)
      APPEND(newN.tIds, tpId);    (* Append typId to new tIds vector  *)
      ASSERT(newN.bloc.symTb.enter(tpId.hash, tpId));
      tpId.dfScp := newN.bloc;
    END Insert;
   (* ======================================= *)
    PROCEDURE AddToIgnoreTypes(thsC : Sys.Type);
      VAR typeId : Id.TypId;
          qualNm : Glb.CharOpen;
          hashIx : INTEGER;
    BEGIN
      typeId := Id.newTypId(Ty.newNamTp());

      qualNm := BOX(BOX(gpSpce(thsC))^ + "." + BOX(gpName(thsC))^);
      typeId.hash := Nh.enterStr(qualNm^);
      (*
      // ignoreBlk has a table of types known to be 
      // generic so we will not mistakenly add them 
      // to the typelist if a used occurrence is found.
      *)
      ASSERT(Glb.ignoreBlk.symTb.enter(typeId.hash, typeId));
    END AddToIgnoreTypes;
   (* ======================================= *)
  BEGIN
    NEW(nVec, 8);
    FOR indx := 0 TO LEN(tVec) - 1 DO
      thsC := tVec[indx];
      IF isPublicClass(thsC) THEN
        Insert(nVec, thsC);
      ELSE
        AddToIgnoreTypes(thsC);
      END;
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
    EXCL(corLib.xAttr, Sy.weak);
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
