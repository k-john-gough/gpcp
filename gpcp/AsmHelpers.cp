
(* ============================================================ *)
(*  AsmHelpers is the module which provides static helper       *)
(*  methods for module AsmUtil                                  *)
(*  Copyright (c) John Gough 2016.                              *)
(* ============================================================ *)

MODULE AsmHelpers;
  IMPORT 
    RTS,
    Jvm := JVMcodes,
    Sym := Symbols,
    Blt := Builtin,
    CSt := CompState,
    Id  := IdDesc,
    Ty  := TypeDesc,
    Ju  := JavaUtil,
    Lv  := LitValue,

    JL  := java_lang,

    Def := AsmDefinitions,
    Acs := AsmCodeSets,
    ASM := org_objectweb_asm;

(* ================================================ *)

(*
 *  Helper type for generating ident and
 *  type names in style required by ASM5
 *
 *)
  TYPE IdntTgPtr = 
      POINTER TO RECORD
        name,      (* plain name of ident referent *)
        owner,     (* _classname_ of owning class  *)
        signature  (* signature of identifier type *)
                  : RTS.NativeString;
      END;

  TYPE TypeTgPtr =
      POINTER TO RECORD
        signature, (* signature of type- all cases *)
        classname  (* class of type- all cases     *)
            : RTS.NativeString;
        auxField (* CASE OF type                   *)
                 (*   Procedure: impl-ret-type     *)
                 (*   Others: arr-typ (on demand)  *)
            : Sym.Type;
      END;

(* ================================================ *)

  VAR noName- : RTS.NativeString;
      emptyMs- : RTS.NativeString;
      invalid- : RTS.NativeString;

(* ================================================ *)
(* ================================================ *)
(*          Forward Procedure Declarations          *)
(* ================================================ *)
(* ================================================ *)

(*  -- currently unused diagnostic helpers
 *
 *PROCEDURE^ TyXtnMsg*( ty : Sym.Type );
 *PROCEDURE^ IdXtnMsg*( id : Sym.Idnt );
 *PROCEDURE^ IdXtnMsg2*( code : INTEGER; id : Sym.Idnt );
 * --------------------------------- *)

  PROCEDURE^ EnsurePrcName*( prc : Id.Procs );
  PROCEDURE^ EnsureBlkName*( blk : Id.BlkId );
  PROCEDURE^ EnsureRecName*( typ : Ty.Record );
  PROCEDURE^ EnsureVecName*( vec : Ty.Vector );
  PROCEDURE^ EnsurePTpName*( pTp : Ty.Procedure );
  PROCEDURE^ EnsureTypName*( typ : Sym.Type );
  PROCEDURE^ GetBinaryTypeName*(typ : Sym.Type) : Lv.CharOpen;
  PROCEDURE^ tyXtn( this : Sym.Type ) : TypeTgPtr;
  PROCEDURE^ tyCls*( this : Sym.Type ) : RTS.NativeString; 
  PROCEDURE^ tySig*( this : Sym.Type ) : RTS.NativeString; 
 
(* ================================================ *)
(*      Notes on the usage of the tgXtn fields      *)
(* ================================================ *)

  PROCEDURE idXtn( this : Sym.Idnt ) : IdntTgPtr;
    VAR xtn : IdntTgPtr;
  BEGIN
    IF this.tgXtn = NIL THEN 
      NEW( xtn );
      this.tgXtn := xtn;
(* ... *)
      WITH this : Id.BlkId DO
          EnsureBlkName( this );
          xtn.name := MKSTR( this.xName^ );
          RETURN xtn;
      | this : Id.PrcId DO
          EnsurePrcName( this );
          xtn.owner := MKSTR( this.clsNm^ );
          xtn.name := MKSTR( this.prcNm^ );
          xtn.signature := tySig( this.type );
      | this : Id.MthId DO
          EnsurePrcName( this );
          xtn.owner := tyCls( this.bndType );
          xtn.name := MKSTR( this.prcNm^ );
          xtn.signature := tySig( this.type );
      | this : Id.VarId DO (* A static variable *)
          xtn.name := MKSTR( Sym.getName.ChPtr(this)^ );
          IF this.recTyp = NIL THEN (* module var *)
            xtn.owner := idXtn( this.dfScp ).name;
          ELSE
            xtn.owner := tyCls( this.recTyp );
          END;
          xtn.signature := MKSTR( GetBinaryTypeName( this.type )^ ); 

ASSERT( xtn.name # NIL );
ASSERT( xtn.owner # NIL );
ASSERT( xtn.signature # NIL );

      | this : Id.FldId DO
          xtn.name := MKSTR( this.fldNm^ );
          xtn.owner := tyCls( this.recTyp );
          xtn.signature := MKSTR( GetBinaryTypeName( this.type )^ );

ASSERT( this.fldNm # NIL );
ASSERT( xtn.owner # NIL );
ASSERT( xtn.signature # NIL );

      | this : Id.LocId DO

          ASSERT( Id.uplevA IN this.locAtt );
          xtn.name := MKSTR( Sym.getName.ChPtr( this )^ );
          xtn.owner := tyCls( this.dfScp(Id.Procs).xhrType.boundRecTp() );
          xtn.signature := MKSTR( GetBinaryTypeName( this.type )^ ); 

ASSERT( xtn.name # NIL );
ASSERT( xtn.owner # NIL );
ASSERT( xtn.signature # NIL );
      END;
(* ... *)
      RETURN xtn;
    ELSE
      RETURN this.tgXtn(IdntTgPtr);
    END;
  END idXtn;

  PROCEDURE idNam*( this : Sym.Idnt ) : RTS.NativeString; 
  BEGIN
    RETURN idXtn( this ).name;
  END idNam;

  PROCEDURE idCls*( this : Sym.Idnt ) : RTS.NativeString; 
  BEGIN
    RETURN idXtn( this ).owner;
  END idCls;

  PROCEDURE idSig*( this : Sym.Idnt ) : RTS.NativeString; 
  BEGIN
    RETURN idXtn( this ).signature;
  END idSig;

 (* --------------------------------------------- *)

  PROCEDURE tyXtn( this : Sym.Type ) : TypeTgPtr;
    VAR xtn : TypeTgPtr;
        bnd : TypeTgPtr;
        sig : Lv.CharOpen;
  BEGIN
    IF this.tgXtn = NIL THEN 
      NEW( xtn );
      this.tgXtn := xtn;
      WITH this : Ty.Base DO
         (* xName is loaded in JavaUtil's mod body *)
          sig := this.xName;
          xtn.auxField := Ty.mkArrayOf( this );
          xtn.signature := MKSTR( sig^ );
      | this : Ty.Vector DO
          EnsureVecName( this );
          xtn.signature := MKSTR( this.xName^ );
          xtn.classname := tyCls( Ju.getHostRecTp( this ) )
      | this : Ty.Record DO
          EnsureRecName( this );
          xtn.classname := MKSTR( this.xName^ );
          xtn.signature := MKSTR( this.scopeNm^ );
      | this : Ty.Array DO
          sig := GetBinaryTypeName( this ); (* FIXME : Refactor later! *)
          xtn.signature := MKSTR( sig^ );
      | this : Ty.Procedure DO
          EnsurePTpName( this );
         (* FIXME: do we need classname? *)
          xtn.signature := MKSTR( this.xName^ );
      | this : Ty.Pointer DO
          xtn^ := tyXtn( this.boundTp )^;
      END;
      RETURN xtn;
    ELSE
      ASSERT( this.tgXtn IS TypeTgPtr );
      RETURN this.tgXtn(TypeTgPtr);
    END;
  END tyXtn;

  (* Returns signature of this type *)
  PROCEDURE tySig*( this : Sym.Type ) : RTS.NativeString; 
  BEGIN
    RETURN tyXtn(this).signature;
  END tySig;

  (* Returns classname of this type *)
  PROCEDURE tyCls*( this : Sym.Type ) : RTS.NativeString; 
  BEGIN
    RETURN tyXtn(this).classname;
  END tyCls;

  PROCEDURE tyNam*( this : Sym.Type ) : RTS.NativeString;
    VAR rslt : RTS.NativeString;
        tXtn : TypeTgPtr;
  BEGIN
    tXtn := tyXtn( this );
    rslt := tXtn.classname;
    IF rslt # NIL THEN 
      RETURN rslt;
    ELSE
      RETURN tXtn.signature;
    END;
  END tyNam;

  (* Returns type descriptor of an array of this type *)
  PROCEDURE tyArrTp*( this : Sym.Type ) : Sym.Type;
    VAR xtn : TypeTgPtr;
  BEGIN
    xtn := tyXtn( this ); 
    IF xtn.auxField = NIL THEN 
      xtn.auxField := Ty.mkArrayOf( this );
    END;
    RETURN xtn.auxField;
  END tyArrTp;

  (* Returns signature of an array of this type *)
  PROCEDURE tyArrSig*( this : Sym.Type ) : RTS.NativeString; 
    VAR xtn : TypeTgPtr;
  BEGIN
    RETURN tySig( tyArrTp( this ) );
  END tyArrSig;

  PROCEDURE tyRetTyp*( this : Sym.Type ) : Sym.Type;
    VAR xtn : TypeTgPtr;
   (* ----------------------------- *)
    PROCEDURE GetImplRetType( this : Ty.Procedure ) : Sym.Type;
      VAR ix : INTEGER;
          px : Id.ParId;
    BEGIN
      IF this.retType # NIL THEN
        RETURN this.retType;
      ELSE
        FOR ix := 0 TO this.formals.tide-1 DO
          px := this.formals.a[ix];
          IF px.boxOrd = Ju.retMarker THEN RETURN px.type END;
        END;
        RETURN NIL;
      END;
    END GetImplRetType;
   (* ----------------------------- *)
  BEGIN
    WITH this : Ty.Procedure DO
      xtn := tyXtn( this );
      IF xtn.auxField = NIL THEN
        xtn.auxField := GetImplRetType( this );
      END;
      RETURN xtn.auxField;
    (* else take the WITH-trap *)
    END;
  END tyRetTyp;

(* ================================================ *)
(* ================================================ *)
  PROCEDURE objToStr*( obj : RTS.NativeObject ) : RTS.NativeString;
    VAR dst : ARRAY 128 OF CHAR;
  BEGIN
    IF obj = NIL THEN RETURN noName END;

    WITH obj : RTS.NativeString DO
      RETURN obj;
    | obj : JL.Integer DO
        IF obj = ASM.Opcodes.NULL THEN RETURN MKSTR( "NIL" );
        ELSIF obj = ASM.Opcodes.TOP THEN RETURN MKSTR( "TOP" );
        ELSIF obj = ASM.Opcodes.INTEGER THEN RETURN MKSTR( "INT" );
        ELSIF obj = ASM.Opcodes.LONG THEN RETURN MKSTR( "LNG" );
        ELSIF obj = ASM.Opcodes.FLOAT THEN RETURN MKSTR( "FLT" );
        ELSIF obj = ASM.Opcodes.DOUBLE THEN RETURN MKSTR( "DBL" );
        ELSE RETURN MKSTR( "unknown" );
        END;
    ELSE
      IF obj = NIL THEN dst := "NIL" ELSE RTS.ObjToStr( obj, dst ) END;
      THROW( "objToStr arg is " + BOX(dst$)^ );
    END;
  END objToStr;
 
(* ================================================ *)
(*       Message Utilities for ASM modules          *)
(* ================================================ *)

  PROCEDURE Msg*( IN s : ARRAY OF CHAR );
  BEGIN
    CSt.Message( "ASM: " + s );
  END Msg;

 (* --------------------------------------------- *)

  PROCEDURE MsgArr*( IN sep : ARRAY OF CHAR; 
                          a : Lv.CharOpenArr );
    VAR str : Lv.CharOpen;
        idx : INTEGER;
  BEGIN
    str := BOX("ASM: " + a[0]^);
    FOR idx := 1 TO LEN(a) - 1 DO
      str := BOX( str^ + sep + a[idx]^ );
    END;
    CSt.Message( str );
  END MsgArr;

 (* --------------------------------------------- *)

  PROCEDURE CatStrArr( a : Def.JlsArr ) : RTS.NativeString;
    VAR str : RTS.NativeString;
        idx : INTEGER;
  BEGIN
    IF a = NIL THEN RETURN emptyMs;
    ELSE
      str := a[0];
      FOR idx := 1 TO LEN(a) - 1 DO
        str := str + ", " + a[idx];
      END;
    END;
    RETURN str;
  END CatStrArr;

 (* --------------------------------------------- *)

  PROCEDURE ObjArrToStr*( arr : Def.JloArr ) : RTS.NativeString;
    VAR strArr : Def.JlsArr;
        ix : INTEGER;
  BEGIN
    IF arr = NIL THEN RETURN emptyMs END;
    NEW( strArr, LEN( arr ) );
    FOR ix := 0 TO LEN(strArr) - 1 DO
      strArr[ix] := objToStr( arr[ix] );
    END;
    RETURN CatStrArr( strArr );
  END ObjArrToStr;

 (* ----------------------------------------------- *)
 (* ----------------------------------------------- *)
  PROCEDURE TypeToObj*( type : Sym.Type ) : RTS.NativeObject;
    VAR rslt : RTS.NativeObject;
  BEGIN
    ASSERT( type # NIL );
    IF (type.kind = Ty.basTp) OR (type.kind = Ty.enuTp) THEN
      CASE (type(Ty.Base).tpOrd) OF
      | Ty.boolN .. Ty.intN, Ty.setN, Ty.uBytN :
          rslt := ASM.Opcodes.INTEGER;
      | Ty.sReaN :
          rslt := ASM.Opcodes.FLOAT;
      | Ty.anyRec .. Ty.sStrN :
          rslt := ASM.Opcodes.TOP; 
      | Ty.lIntN :
          rslt := ASM.Opcodes.LONG;
      | Ty.realN :
          rslt := ASM.Opcodes.DOUBLE;
      END;
    ELSE
      EnsureTypName(type);
      rslt := MKSTR( type.xName^ );
    END;
    RETURN rslt;
  END TypeToObj;
 (* ----------------------------------------------- *)

 (* ----------------------------------------------- *)
  PROCEDURE SigToObj*( sig : RTS.NativeString ) : RTS.NativeObject;
  BEGIN
    ASSERT( sig # NIL );
    CASE sig.charAt(0) OF
      | 'I','B','C','S','Z' : RETURN ASM.Opcodes.INTEGER;
      | 'J' :                 RETURN ASM.Opcodes.LONG;
      | 'F' :                 RETURN ASM.Opcodes.FLOAT;
      | 'D' :                 RETURN ASM.Opcodes.DOUBLE;
    ELSE
      RETURN sig;
    END;
  END SigToObj;

(* ================================================ *)
(* ================================================ *)
(*     Methods to ensure descriptor names exist     *)
(* ================================================ *)
(* ================================================ *)

 (* ---------------------------------------------- *)
 (*  Ensure that this record desc. has valid names *)
 (* ---------------------------------------------- *)
  PROCEDURE EnsureRecName*( typ : Ty.Record );
  BEGIN
    IF typ.xName = NIL THEN 
      Ju.MkRecName(typ);
    END;
  END EnsureRecName;

 (* ---------------------------------------------- *)
 (*   Ensure that this proc desc. has valid names  *)
 (* ---------------------------------------------- *)
  PROCEDURE EnsurePrcName*( prc : Id.Procs );
  BEGIN
    IF prc.scopeNm = NIL THEN Ju.MkProcName( prc ) END;
  END EnsurePrcName;

 (* ---------------------------------------------- *)
 (*   Ensure that this field desc. has valid names *)
 (* ---------------------------------------------- *)
  PROCEDURE EnsureFldName*( fld : Id.VarId );
  BEGIN
    IF fld.varNm = NIL THEN 
      Ju.MkVarName(fld);
    END;
  END EnsureFldName;

 (* ----------------------------------------------- *)
 (*  Ensure that this block desc. has valid names   *)
 (* ----------------------------------------------- *)
  PROCEDURE EnsureBlkName*( blk : Id.BlkId );
  BEGIN
    IF blk.xName = NIL THEN Ju.MkBlkName(blk) END;
  END EnsureBlkName;

 (* ----------------------------------------------- *)
 (*  Ensure that this vector desc. has valid names  *)
 (* ----------------------------------------------- *)
  PROCEDURE EnsureVecName*( vec : Ty.Vector );
  BEGIN
    IF vec.xName = NIL THEN Ju.MkVecName(vec) END;
  END EnsureVecName;

 (* ----------------------------------------------- *)
 (*  Ensure that this block desc. has valid names   *)
 (* ----------------------------------------------- *)
  PROCEDURE EnsurePTpName*( pTp : Ty.Procedure );
  BEGIN
    IF pTp.xName = NIL THEN Ju.MkProcTypeName(pTp) END;
  END EnsurePTpName;

 (* ------------------------------------------------ *)
 (* Ensure that this type descriptor has valid xName *)
 (* ------------------------------------------------ *)
  PROCEDURE EnsureTypName*( typ : Sym.Type );
  BEGIN
    IF typ.xName # NIL THEN RETURN END;
    WITH typ : Ty.Record DO
        Ju.MkRecName( typ );
    | typ : Ty.Vector DO
        Ju.MkVecName( typ );
    | typ : Ty.Array DO
        typ.xName := Ju.cat2( Ju.brac, GetBinaryTypeName( typ.elemTp ) );
    | typ : Ty.Pointer DO
        EnsureTypName( typ.boundTp );
        typ.xName := typ.boundTp.xName;
    | typ : Ty.Procedure DO
        Ju.MkProcTypeName( typ );
    ELSE
      THROW( "Can't make TypName" );
    END;
  END EnsureTypName;

 (* ------------------------------------------------- *)
 (*    Compute the binary type name, and persist it   *)
 (* ------------------------------------------------- *)
  PROCEDURE GetBinaryTypeName*(typ : Sym.Type) : Lv.CharOpen;
  VAR
    arrayName : Lv.CharOpenSeq;
    arrayTy : Sym.Type;
  BEGIN
    WITH typ : Ty.Base DO
        RETURN typ.xName;
    | typ : Ty.Vector DO
        EnsureVecName( typ );
        RETURN typ.xName;
    | typ : Ty.Procedure DO
        EnsurePTpName( typ );
        RETURN typ.hostClass.scopeNm;
    | typ : Ty.Array DO
        IF typ.xName = NIL THEN
          Lv.InitCharOpenSeq(arrayName,3);
          arrayTy := typ;
          WHILE arrayTy IS Ty.Array DO 
            Lv.AppendCharOpen(arrayName,Ju.brac); 
            arrayTy := arrayTy(Ty.Array).elemTp;
          END;
          Lv.AppendCharOpen(arrayName, GetBinaryTypeName(arrayTy)); 
          typ.xName := Lv.arrayCat(arrayName);
        END;
        ASSERT(typ.xName # NIL);
        RETURN typ.xName;
    | typ : Ty.Record DO
        EnsureRecName( typ );
        RETURN typ.scopeNm;
    | typ : Ty.Enum DO
        RETURN Blt.intTp.xName;
    | typ : Ty.Pointer DO
        RETURN GetBinaryTypeName(typ.boundTp)
    | typ : Ty.Opaque DO
        IF typ.xName = NIL THEN 
          Ju.MkAliasName(typ);
        END;
        RETURN typ.scopeNm;
    END;
  END GetBinaryTypeName;

(* ================================================ *)
(*  Factories for the creation of boxed basic types *) 
(* ================================================ *)
  PROCEDURE MkInteger*( val : INTEGER ) : JL.Object;
  BEGIN
    RETURN JL.Integer.Init( val );
  END MkInteger;

  PROCEDURE MkLong*( val : LONGINT ) : JL.Object;
  BEGIN
    RETURN JL.Long.Init( val );
  END MkLong;

  PROCEDURE MkFloat*( val : SHORTREAL ) : JL.Object;
  BEGIN
    RETURN JL.Float.Init( val );
  END MkFloat;

  PROCEDURE MkDouble*( val : REAL ) : JL.Object;
  BEGIN
    RETURN JL.Double.Init( val );
  END MkDouble;

(* ================================================ *)
BEGIN
  noName := MKSTR( "?" );
  emptyMs := MKSTR( "empty" );
  invalid := MKSTR( "invalid" );
END AsmHelpers.
(* ================================================ *)

