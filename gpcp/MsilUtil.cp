(* ============================================================ *)
(*  MsilUtil is the module which writes ILASM file structures   *)
(*  Copyright (c) John Gough 1999, 2000.                        *)
(* ============================================================ *)

MODULE MsilUtil;

  IMPORT 
        GPCPcopyright,
        RTS, 
        Console,
        MsilBase,
        NameHash,
        Scn := CPascalS,
        CSt := CompState,
        Lv  := LitValue,
        Sy  := Symbols,
        Bi  := Builtin,
        Id  := IdDesc,
        Ty  := TypeDesc,
        Asm := IlasmCodes;

(* ============================================================ *)

   CONST
        (* various ILASM-specific runtime name strings *)
        initString  = ".ctor";

   CONST
       (* Conversions from System.String to char[]    *)
        vStr2ChO*  = 1;
        vStr2ChF*  = 2;
       (* Runtime support for CP's MOD,DIV operations *)
        sysExit*    = 3;
        toUpper*    = 4;
        dFloor*     = 5;
        dAbs*       = 6;
        fAbs*       = 7;
        iAbs*       = 8;
        lAbs*       = 9;
        getTpM*     = 10;
        CpModI*     = 11;
        CpDivI*     = 12;
        CpModL*     = 13;
        CpDivL*     = 14;
       (* various ILASM-specific runtime name strings *)
        aStrLen*    = 15;
        aStrChk*    = 16;
        aStrLp1*    = 17;
        aaStrCmp*   = 18;
        aaStrCopy*  = 19;
       (* Error reporting facilities ................ *)
        caseMesg*   = 20;
        withMesg*   = 21;
        mkExcept*   = 22;
       (* Conversions from char[] to System.String    *)
        chs2Str*    =  23;
       (* various CPJ-specific concatenation helpers  *)
        CPJstrCatAA*   = 24;
        CPJstrCatSA*   = 25;
        CPJstrCatAS*   = 26;
        CPJstrCatSS*   = 27;
        rtsLen*        = 28;

(* ============================================================ *)
(* ============================================================ *)

  TYPE Label*     = POINTER TO ABSTRACT RECORD END;
       LbArr*     = POINTER TO ARRAY OF Label;

  TYPE ProcInfo*  = POINTER TO (* EXTENSIBLE *) RECORD
                      prId- : Sy.Scope; (* mth., prc. or mod.   *)
                      rtLc* : INTEGER;  (* return value local # *)
                      (* ---- depth tracking ------ *)
                      dNum- : INTEGER;  (* current stack depth  *)
                      dMax- : INTEGER;  (* maximum stack depth  *)
                      (* ---- temp-var manager ---- *)
                      lNum- : INTEGER;         (* prog vars     *)
                      tLst- : Sy.TypeSeq;      (* type list     *)
                      fLst- : Sy.TypeSeq;      (* free list     *)
                      (* ---- end temp manager ---- *)
                      exLb* : Label;    (* exception exit label *)
                    END;

(* ============================================================ *)

  TYPE MsilFile* =  POINTER TO ABSTRACT RECORD
                      srcS* : Lv.CharOpen;(* source file name   *)
                      outN* : Lv.CharOpen;
                      proc* : ProcInfo;
                    END;

(* ============================================================ *)

  VAR   nmArray   : Lv.CharOpenSeq;


  VAR   lPar, rPar, lBrk,               (* ( ) {    *)
        rBrk, dotS, rfMk,               (* } . &    *)
        atSg, cmma,                     (* @ ,      *)
        vFld,                           (* "v$"     *)
        brks,                           (* "[]"     *)
        rtsS,                           (* "RTS"    *)
        prev,                           (* "prev"   *)
        body,                           (* ".body"  *)
        ouMk : Lv.CharOpen;             (* "[out]"  *)

        evtAdd, evtRem : Lv.CharOpen;
        pVarSuffix     : Lv.CharOpen;
        xhrMk          : Lv.CharOpen;
        xhrDl          : Lv.CharOpen;
        vecPrefix      : Lv.CharOpen;

  VAR   boxedObj  : Lv.CharOpen;
        corlibAsm : Lv.CharOpen;
        xhrIx     : INTEGER;

(* ============================================================ *)

  VAR   vecBlkId  : Id.BlkId;
        vecBase   : Id.TypId;
        vecTypes  : ARRAY Ty.anyPtr+1 OF Id.TypId; (* pointers *)
        vecTide   : Id.FldId;
        vecElms   : ARRAY Ty.anyPtr+1 OF Id.FldId;
        vecExpnd  : ARRAY Ty.anyPtr+1 OF Id.MthId;

(* ============================================================ *)

  VAR   typeGetE  : ARRAY 16 OF INTEGER;
        typePutE  : ARRAY 16 OF INTEGER;
        typeStInd : ARRAY 16 OF INTEGER;
        typeLdInd : ARRAY 16 OF INTEGER;

(* ============================================================ *)

  PROCEDURE (t : MsilFile)fileOk*() : BOOLEAN,NEW,ABSTRACT;
  (* Test if file was opened successfully *)

(* ============================================================ *)
(*   EMPTY text format Procedures only overidden in IlasmUtil   *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)MkNewProcInfo*(s : Sy.Scope),NEW,ABSTRACT;
  PROCEDURE (os : MsilFile)Comment*(IN s : ARRAY OF CHAR),NEW,EMPTY;
  PROCEDURE (os : MsilFile)CommentT*(IN s : ARRAY OF CHAR),NEW,EMPTY;
  PROCEDURE (os : MsilFile)OpenBrace*(i : INTEGER),NEW,EMPTY;
  PROCEDURE (os : MsilFile)CloseBrace*(i : INTEGER),NEW,EMPTY;
  PROCEDURE (os : MsilFile)Blank*(),NEW,EMPTY;

(* ============================================================ *)
(*       ABSTRACT Procedures overidden in both subclasses       *)
(* ============================================================ *)
(*       Various code emission methods                          *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)Code*(code : INTEGER),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CodeI*(code,int : INTEGER),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CodeT*(code : INTEGER; type : Sy.Type),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CodeTn*(code : INTEGER; type : Sy.Type),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CodeL*(code : INTEGER; long : LONGINT),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CodeR*(code : INTEGER; real : REAL),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CodeLb*(code : INTEGER; i2 : Label),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CodeS*(code : INTEGER; 
                                  str  : INTEGER),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)MkNewRecord*(typ : Ty.Record),NEW,ABSTRACT;
  (* emit constructor call ... *)

  PROCEDURE (os : MsilFile)LoadType*(id : Sy.Idnt),NEW,ABSTRACT;
  (* load runtime type descriptor *)

  PROCEDURE (os : MsilFile)PushStr*(IN str : ARRAY OF CHAR),NEW,ABSTRACT;
  (* load a literal string *)

  PROCEDURE (os : MsilFile)NumberParams*(pId : Id.Procs; 
                                         pTp : Ty.Procedure),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)Finish*(),NEW,ABSTRACT;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)MkBodyClass*(mod : Id.BlkId),NEW,ABSTRACT;
  PROCEDURE (os : MsilFile)ClassHead*(attSet : SET;
                                      thisRc : Ty.Record;
                                      superT : Ty.Record),NEW,ABSTRACT;
  PROCEDURE (os : MsilFile)StartBoxClass*(rec : Ty.Record;
                                          att : SET;
                                          blk : Id.BlkId),NEW,ABSTRACT;
  PROCEDURE (os : MsilFile)ClassTail*(),NEW,EMPTY;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)StartNamespace*(nm : Lv.CharOpen),NEW,ABSTRACT;
  PROCEDURE (os : MsilFile)RefRTS*(),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)MkBasX*(t : Ty.Base),NEW,EMPTY;
  PROCEDURE (os : MsilFile)MkArrX*(t : Ty.Array),NEW,EMPTY;
  PROCEDURE (os : MsilFile)MkPtrX*(t : Ty.Pointer),NEW,EMPTY;
  PROCEDURE (os : MsilFile)MkVecX*(t : Sy.Type; s : Id.BlkId),NEW,EMPTY;
  PROCEDURE (os : MsilFile)MkEnuX*(t : Ty.Enum; s : Sy.Scope),NEW,EMPTY;
  PROCEDURE (os : MsilFile)MkRecX*(t : Ty.Record; s : Sy.Scope),NEW,EMPTY;
  PROCEDURE (os : MsilFile)AsmDef*(IN pkNm : ARRAY OF CHAR),NEW,EMPTY;
  PROCEDURE (os : MsilFile)SubSys*(xAtt : SET),NEW,ABSTRACT;

(* ============================================================ *)
(*   Calling a static (usually runtime helper) method           *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)StaticCall*(s : INTEGER;
                                       d : INTEGER),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CopyCall*(typ : Ty.Record),NEW,ABSTRACT;

(* ============================================================ *)
(*   Calling a user defined method, constructor or delegate     *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)CallIT*(code : INTEGER; 
                                   proc : Id.Procs; 
                                   type : Ty.Procedure),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CallCT*(proc : Id.Procs; 
                                   type : Ty.Procedure),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CallDelegate*(typ : Ty.Procedure),NEW,ABSTRACT;


(* ============================================================ *)
(*   Various element access abstractions                        *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)PutGetS*(code : INTEGER;
                                    blk  : Id.BlkId;
                                    fld  : Id.VarId),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)PutGetF*(code : INTEGER;
                                    fld  : Id.FldId),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)GetValObj*(code : INTEGER; 
                                    ptrT : Ty.Pointer),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)PutGetXhr*(code : INTEGER;
                                      proc : Id.Procs;
                                      locD : Id.LocId),NEW,ABSTRACT;

(* ============================================================ *)
(*   Line and Label handling                                    *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)Line*(nm : INTEGER),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)LinePlus*(l,w : INTEGER),NEW,EMPTY;
  
  PROCEDURE (os : MsilFile)LineSpan*(span : Scn.Span),NEW,EMPTY; 

  PROCEDURE (os : MsilFile)LstLab*(l : Label),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)DefLab*(l : Label),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)DefLabC*(l : Label; 
                                 IN c : ARRAY OF CHAR),NEW,ABSTRACT;

(* ============================================================ *)
(*   Declaration utilities                                      *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)EmitField*(id : Id.AbVar; att : SET),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)ExternList*(),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)MarkInterfaces*(IN seq : Sy.TypeSeq),NEW,ABSTRACT;

(* ============================================================ *)
(*    Start and finish various structures                       *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)SwitchHead*(num : INTEGER),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)SwitchTail*(),NEW,ABSTRACT;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)Try*(),NEW,ABSTRACT;
  PROCEDURE (os : MsilFile)Catch*(proc : Id.Procs),NEW,ABSTRACT;
  PROCEDURE (os : MsilFile)CloseCatch*(),NEW,ABSTRACT;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)MkNewProcVal*(p : Sy.Idnt; t : Sy.Type),NEW,ABSTRACT;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)InitHead*(typ : Ty.Record;
                                     prc : Id.PrcId),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CallSuper*(typ : Ty.Record;
                                      prc : Id.PrcId),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)InitTail*(typ : Ty.Record),NEW,ABSTRACT;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)CopyHead*(typ : Ty.Record),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)CopyTail*(),NEW,ABSTRACT;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)MainHead*(xAtt : SET),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)MainTail*(),NEW,ABSTRACT;
  
(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)ClinitHead*(),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)ClinitTail*(),NEW,ABSTRACT;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)MethodDecl*(attr : SET; 
                                       proc : Id.Procs),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)MethodTail*(id : Id.Procs),NEW,ABSTRACT;

(* ============================================================ *)
(*       Start of Procedure Variable and Event Stuff            *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)EmitEventMethods*(id : Id.AbVar),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)EmitPTypeBody*(tId : Id.TypId),NEW,ABSTRACT;

  PROCEDURE (os : MsilFile)MkAndLinkDelegate*(dl : Sy.Idnt;
                                              id : Sy.Idnt;
                                              ty : Sy.Type;
                                             add : BOOLEAN),NEW,ABSTRACT;

(* ============================================================ *)
(*            End of Procedure Variable and Event Stuff         *)
(* ============================================================ *)

(* ==================================================================== *
 *             A word on naming for the ILASM version.                  *
 * ==================================================================== *
 *  Part one: module-level declarations, in Module Mmm.                 *
 *              TYPE Ttt = POINTER TO RECORD ... END;                   *
 *        has ilasm class name                                          *
 *              .class <attr> Mmm.Ttt { ... }                           *
 *        Similarly the static procedure                                *
 *              PROCEDURE Ppp(); END Ppp;                               *
 *        has ilasm method name (inside static class Mmm)               *
 *              .method <attr> void Ppp() {...}                         *
 *        which is referenced as                                        *
 *              Ppp(...)                within the static class, &      *
 *              Mmm::Ppp(...)           elswhere inside the module, &   *
 *              [Mmm]Mmm::Ppp(...)      from outside the module.        *
 *        Likewise, methods bound to Ttt will be referenced as          *
 *              Ppp(...)                inside the dynamic class, &     *
 *              Mmm.Ttt::Ppp(...)       elsewhere inside the module, &  *
 *              [Mmm]Mmm.Ttt::Ppp(...)  from outside the module.        *
 *                                                                      *
 * ==================================================================== *
 *  Part two: declarations nested inside procedure Outer (say).         *
 *              PROCEDURE Ppp(); END Ppp;                               *
 *        will have ilasm name (inside Mmm)                             *
 *              .method <attr> void Outer@Ppp() {...}                   *
 *        which is referenced as                                        *
 *              Outer@Ppp(...)                                          *
 *        Nested type Ttt will have name                                *
 *              .struct(?) <attr> Mmm.Outer@Ttt {...}                   *
 *        and cannot have type bound procedures, or be exported.        *
 *                                                                      *
 * ==================================================================== *
 *  Where are these names stored?                                       *
 *  The principle is: every identifier has its class name stored in     *
 *  in d.scopeNm, and its simple name is stored in d.xName.             *
 *  Thus, for names defined in this module:                             *
 * ==================================================================== *
 *        The name for BlkId Mmm is stored in desc.xName, as            *
 *              desc.xName   = "Mmm"                                    *
 *              desc.scopeNm = "Mmm"                                    *
 *        The names for PrcId Ppp are stored as                         *
 *              desc.xName   = "Ppp"                                    *
 *              desc.scopeNm = "Mmm"                                    *
 *        or in the nested case...                                      *
 *              desc.xName   = "Outer@Ppp"                              *
 *              desc.scopeNm = "Mmm"                                    *
 *        The names for (non-nested) MthId Ppp are stored as            *
 *              desc.xName   = "Ppp"                                    *
 *              desc.scopeNm = "Mmm.Ttt"                                *
 *                                                                      *
 * For types, the names are stored thuswise.                            *
 *        The name for Record descriptor Ttt will be                    *
 *              recT.xName   = "Mmm_Ttt"                                *
 *              recT.scopeNm = "Mmm_Ttt"                                *
 *        or in the nested case ...                                     *
 *              recT.xName   = "Mmm_Ppp@Ttt"                            *
 *              recT.scopeNm = "Mmm_Ppp@Ttt"                            *
 *                                                                      *
 * ==================================================================== *
 *  Where are these names stored?  For external names:                  *
 * ==================================================================== *
 *        The name for BlkId Mmm is stored in desc.xName, as            *
 *              desc.xName   = "Mmm"                                    *
 *              desc.scopeNm = "[Mmm]Mmm"                               *
 *        The names for PrcId Ppp are stored as                         *
 *              desc.xName   = "Ppp"                                    *
 *              desc.scopeNm = "[Mmm]Mmm"                               *
 *        The names for (non-nested) MthId Ppp are stored as            *
 *              desc.xName   = "Ppp"                                    *
 *              desc.scopeNm = "[Mmm]Mmm_Ttt"                           *
 *                                                                      *
 * For types, the names are stored thuswise.                            *
 *        The name for Record descriptor Ttt will be                    *
 *              recT.xName   = "Mmm_Ttt"                                *
 *              recT.scopeNm = "[Mmm]Mmm_Ttt"                           *
 * ==================================================================== *
 * ==================================================================== *)


(* ============================================================ *)
(*                    Some static utilities                     *)
(* ============================================================ *)

  PROCEDURE cat2*(i,j : Lv.CharOpen) : Lv.CharOpen;
  BEGIN
    Lv.ResetCharOpenSeq(nmArray);
    Lv.AppendCharOpen(nmArray, i);
    Lv.AppendCharOpen(nmArray, j);
    RETURN Lv.arrayCat(nmArray); 
  END cat2;

(* ============================================================ *)

  PROCEDURE cat3*(i,j,k : Lv.CharOpen) : Lv.CharOpen;
  BEGIN
    Lv.ResetCharOpenSeq(nmArray);
    Lv.AppendCharOpen(nmArray, i);
    Lv.AppendCharOpen(nmArray, j);
    Lv.AppendCharOpen(nmArray, k);
    RETURN Lv.arrayCat(nmArray); 
  END cat3;

(* ============================================================ *)

  PROCEDURE cat4*(i,j,k,l : Lv.CharOpen) : Lv.CharOpen;
  BEGIN
    Lv.ResetCharOpenSeq(nmArray);
    Lv.AppendCharOpen(nmArray, i);
    Lv.AppendCharOpen(nmArray, j);
    Lv.AppendCharOpen(nmArray, k);
    Lv.AppendCharOpen(nmArray, l);
    RETURN Lv.arrayCat(nmArray); 
  END cat4;

(* ============================================================ *)

  PROCEDURE mapVecElTp(typ : Sy.Type) : INTEGER;
  BEGIN
    WITH typ : Ty.Base DO
      CASE typ.tpOrd OF
      | Ty.sChrN : RETURN Ty.charN;
      | Ty.boolN, Ty.byteN, Ty.sIntN, Ty.setN, Ty.uBytN : RETURN Ty.intN;
      | Ty.charN, Ty.intN, Ty.lIntN, Ty.sReaN, Ty.realN : RETURN typ.tpOrd;
      ELSE RETURN Ty.anyPtr;
      END;
    ELSE RETURN Ty.anyPtr;
    END;
  END mapVecElTp;


  PROCEDURE mapOrdRepT(ord : INTEGER) : Sy.Type;
  BEGIN
    CASE ord OF
    | Ty.charN  : RETURN Bi.charTp;
    | Ty.intN   : RETURN Bi.intTp;
    | Ty.lIntN  : RETURN Bi.lIntTp;
    | Ty.sReaN  : RETURN Bi.sReaTp;
    | Ty.realN  : RETURN Bi.realTp;
    | Ty.anyPtr : RETURN Bi.anyPtr;
    END;
  END mapOrdRepT;

(* ============================================================ *)

  PROCEDURE^ MkProcName*(proc : Id.Procs; os : MsilFile);
  PROCEDURE^ MkAliasName*(typ : Ty.Opaque; os : MsilFile);
  PROCEDURE^ MkEnumName*(typ : Ty.Enum; os : MsilFile);
  PROCEDURE^ MkTypeName*(typ : Sy.Type; fil : MsilFile);
  PROCEDURE^ MkRecName*(typ : Ty.Record; os : MsilFile);
  PROCEDURE^ MkPtrName*(typ : Ty.Pointer; os : MsilFile);
  PROCEDURE^ MkPTypeName*(typ : Ty.Procedure; os : MsilFile);
  PROCEDURE^ MkIdName*(id : Sy.Idnt; os : MsilFile);
  PROCEDURE^ MkBasName(typ : Ty.Base; os : MsilFile);
  PROCEDURE^ MkArrName(typ : Ty.Array; os : MsilFile);
  PROCEDURE^ MkVecName(typ : Ty.Vector; os : MsilFile);

  PROCEDURE^ (os : MsilFile)PutUplevel*(var : Id.LocId),NEW;
  PROCEDURE^ (os : MsilFile)PushInt*(num : INTEGER),NEW;
  PROCEDURE^ (os : MsilFile)GetVar*(id : Sy.Idnt),NEW;
  PROCEDURE^ (os : MsilFile)GetVarA*(id : Sy.Idnt),NEW;
  PROCEDURE^ (os : MsilFile)PushLocal*(ord : INTEGER),NEW;
  PROCEDURE^ (os : MsilFile)StoreLocal*(ord : INTEGER),NEW;
  PROCEDURE^ (os : MsilFile)FixCopies(prId : Sy.Idnt),NEW;
  PROCEDURE^ (os : MsilFile)DecTemp(ord : INTEGER),NEW;
  PROCEDURE^ (os : MsilFile)PutElem*(typ : Sy.Type),NEW;
  PROCEDURE^ (os : MsilFile)GetElem*(typ : Sy.Type),NEW;

(* ------------------------------------------------------------ *)

  PROCEDURE takeAdrs*(i : Id.ParId) : BOOLEAN;
  (*  A parameter needs to have its address taken iff      *)
  (*  * Param Mode is VAL & FALSE                          *)
  (*  * Param Mode is VAR & type is value class or scalar  *)
  (*  * Param Mode is OUT & type is value class or scalar  *)
  (*  * Param Mode is IN  & type is value class            *)
  (*    (IN Scalars get treated as VAL on the caller side) *)
    VAR type : Sy.Type;
  BEGIN
    IF i.parMod = Sy.val THEN RETURN FALSE END;

    IF i.type IS Ty.Opaque THEN i.type := i.type(Ty.Opaque).resolved END;

    type := i.type;
    WITH type : Ty.Vector DO RETURN i.parMod # Sy.in;
    |    type : Ty.Array  DO RETURN FALSE;
    |    type : Ty.Record DO RETURN ~(Sy.clsTp IN type.xAttr);
    ELSE (* scalar type *)   RETURN i.parMod # Sy.in;
    END;
  END takeAdrs;

(* ------------------------------------------------------------ *)

  PROCEDURE needsInit*(type : Sy.Type) : BOOLEAN;
  BEGIN
    WITH type : Ty.Vector DO RETURN FALSE;
    |    type : Ty.Array  DO RETURN type.length # 0;
    |    type : Ty.Record DO RETURN Sy.clsTp IN type.xAttr;
    ELSE (* scalar type *)   RETURN FALSE;
    END;
  END needsInit;

(* ------------------------------------------------------------ *)

  PROCEDURE isRefSurrogate*(type : Sy.Type) : BOOLEAN;
  BEGIN
    WITH type : Ty.Array  DO RETURN type.kind # Ty.vecTp;
    |    type : Ty.Record DO RETURN Sy.clsTp IN type.xAttr;
    ELSE (* scalar type *)   RETURN FALSE;
    END;
  END isRefSurrogate;

(* ------------------------------------------------------------ *)

  PROCEDURE hasValueRep*(type : Sy.Type) : BOOLEAN;
  BEGIN
    WITH type : Ty.Array  DO RETURN type.kind = Ty.vecTp;
    |    type : Ty.Record DO RETURN ~(Sy.clsTp IN type.xAttr);
    ELSE (* scalar type *)   RETURN TRUE;
    END;
  END hasValueRep;

(* ------------------------------------------------------------ *)

  PROCEDURE isValRecord*(type : Sy.Type) : BOOLEAN;
  BEGIN
    WITH type : Ty.Array  DO RETURN FALSE;
    |    type : Ty.Record DO RETURN ~(Sy.clsTp IN type.xAttr);
    ELSE (* scalar type *)   RETURN FALSE;
    END;
  END isValRecord;

(* ------------------------------------------------------------ *)

  PROCEDURE vecMod() : Id.BlkId;
  BEGIN
    IF vecBlkId = NIL THEN
      Bi.MkDummyImport("RTS_Vectors", "[RTS]Vectors", vecBlkId);
      Bi.MkDummyClass("VecBase", vecBlkId, Ty.noAtt, vecBase);
    END;
    RETURN vecBlkId;
  END vecMod;

  PROCEDURE vecClass(ord : INTEGER) : Id.TypId;
    VAR str : ARRAY 8 OF CHAR;
        tId : Id.TypId;
        rcT : Ty.Record;
  BEGIN
    IF vecTypes[ord] = NIL THEN
      CASE ord OF
      | Ty.charN  : str := "VecChr";
      | Ty.intN   : str := "VecI32";
      | Ty.lIntN  : str := "VecI64";
      | Ty.sReaN  : str := "VecR32";
      | Ty.realN  : str := "VecR64";
      | Ty.anyPtr : str := "VecRef";
      END;
      Bi.MkDummyClass(str, vecMod(), Ty.noAtt, tId);
      rcT := tId.type.boundRecTp()(Ty.Record);
      rcT.baseTp := vecBase.type.boundRecTp();
      vecTypes[ord] := tId;
    END;
    RETURN vecTypes[ord];
  END vecClass;

  PROCEDURE vecRecord(ord : INTEGER) : Ty.Record;
  BEGIN
    RETURN vecClass(ord).type.boundRecTp()(Ty.Record);
  END vecRecord;

  PROCEDURE vecArray(ord : INTEGER) : Id.FldId;
    VAR fld : Id.FldId;
  BEGIN
    IF vecElms[ord] = NIL THEN
      fld := Id.newFldId();
      fld.hash := NameHash.enterStr("elms");
      fld.dfScp := vecMod();
      fld.recTyp := vecRecord(ord);
      fld.type := Ty.mkArrayOf(mapOrdRepT(ord));
      vecElms[ord] := fld;
    END;
    RETURN vecElms[ord];
  END vecArray;

(* ------------------------------------------------------------ *)

  PROCEDURE vecArrFld*(typ : Ty.Vector; os : MsilFile) : Id.FldId;
    VAR fld : Id.FldId;
  BEGIN
    fld := vecArray(mapVecElTp(typ.elemTp));
    IF fld.recTyp.xName = NIL THEN MkRecName(fld.recTyp(Ty.Record), os) END;
    RETURN fld;
  END vecArrFld;

  PROCEDURE vecRepTyp*(typ : Ty.Vector) : Sy.Type;
  BEGIN
    RETURN vecClass(mapVecElTp(typ.elemTp)).type;
  END vecRepTyp;

  PROCEDURE vecRepElTp*(typ : Ty.Vector) : Sy.Type;
  BEGIN
    RETURN mapOrdRepT(mapVecElTp(typ.elemTp));
  END vecRepElTp;

  PROCEDURE vecLeng*(os : MsilFile) : Id.FldId;
  BEGIN
    IF vecTide = NIL THEN
      vecTide := Id.newFldId();
      vecTide.hash := NameHash.enterStr("tide");
      vecTide.dfScp := vecMod();
      vecTide.recTyp := vecBase.type.boundRecTp();
      vecTide.type  := Bi.intTp;
      MkRecName(vecTide.recTyp(Ty.Record), os);
    END;
    RETURN vecTide;
  END vecLeng;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)InvokeExpand*(typ : Ty.Vector),NEW;
  (*  Assert: vector ref is on stack *)
    VAR ord : INTEGER;
        xpd : Id.MthId;
        xpT : Ty.Procedure;
  BEGIN
    ord := mapVecElTp(typ.elemTp);
    xpd := vecExpnd[ord];
    IF xpd = NIL THEN
      xpd := Id.newMthId();
      xpd.hash := Bi.xpndBk;
      xpd.dfScp := vecMod();
      xpT := Ty.newPrcTp();
      xpT.idnt := xpd;
      xpT.receiver := vecClass(ord).type;
      xpd.bndType  := xpT.receiver.boundRecTp();
      MkProcName(xpd, os);
      os.NumberParams(xpd, xpT);
      xpd.type := xpT;
      vecExpnd[ord] := xpd;
    END;
    os.CallIT(Asm.opc_callvirt, xpd, xpd.type(Ty.Procedure));
  END InvokeExpand;

(* ------------------------------------------------------------ *)
(* ------------------------------------------------------------ *)

  PROCEDURE xhrCount(tgt, ths : Id.Procs) : INTEGER;
    VAR count : INTEGER;
  BEGIN
    IF ths.lxDepth = 0 THEN RETURN 0 END;
   (*
    *  "ths" is the calling procedure.
    *  "tgt" is the procedure with the uplevel data.
    *)
    count := 0;
    REPEAT
      ths := ths.dfScp(Id.Procs);
      IF Id.hasXHR IN ths.pAttr THEN INC(count) END;
    UNTIL (ths.lxDepth = 0) OR
        ((ths.lxDepth <= tgt.lxDepth) & (Id.hasXHR  IN ths.pAttr));
    RETURN count;
  END xhrCount;

  PROCEDURE newXHR() : Lv.CharOpen;
  BEGIN
    INC(xhrIx);
    RETURN cat2(xhrDl, Lv.intToCharOpen(xhrIx));
  END newXHR;

  PROCEDURE MkXHR(scp : Id.Procs);
    VAR typId : Id.TypId;
      recTp : Ty.Record;
      index : INTEGER;
      locVr : Id.LocId;
      fldVr : Id.FldId;
  BEGIN
   (*
    *  Create a type descriptor for the eXplicit 
    *  Heap-allocated activation Record.  This is
    *  an extension of the [RTS]XHR system type.
    *)
    Bi.MkDummyClass(newXHR(), CSt.thisMod, Ty.noAtt, typId);
    typId.SetMode(Sy.prvMode);
    scp.xhrType := typId.type;
    recTp := typId.type.boundRecTp()(Ty.Record);
    recTp.baseTp := CSt.rtsXHR.boundRecTp();
    INCL(recTp.xAttr, Sy.noCpy);

    FOR index := 0 TO scp.locals.tide-1 DO
      locVr := scp.locals.a[index](Id.LocId);
      IF Id.uplevA IN locVr.locAtt THEN
        fldVr := Id.newFldId();
        fldVr.hash := locVr.hash;
        fldVr.type := locVr.type;
        fldVr.recTyp := recTp;
        Sy.AppendIdnt(recTp.fields, fldVr);
      END;
    END;
  END MkXHR;

(* ============================================================ *)
(*                    ProcInfo Methods                          *)
(* ============================================================ *)

  PROCEDURE InitProcInfo*(info : ProcInfo; proc : Sy.Scope);
    VAR i : INTEGER;
  BEGIN
   (*
    *  Assert: the locals have already been numbered
    *               by a call to NumberLocals(), and 
    *               rtsFram has been set accordingly.
    *)
    info.prId := proc;
    WITH proc : Id.Procs DO
      info.lNum := proc.rtsFram;
      IF info.lNum > 0 THEN
        Sy.InitTypeSeq(info.tLst, info.lNum * 2);    (* the (t)ypeList *)
        Sy.InitTypeSeq(info.fLst, info.lNum * 2);    (* the (f)reeList *)
        FOR i := 0 TO info.lNum-1 DO 
          Sy.AppendType(info.tLst, NIL);
          Sy.AppendType(info.fLst, NIL);
        END;
      END;
    ELSE (* Id.BlkId *)
      info.lNum := 0;
    END;
    info.dNum := 0;
    info.dMax := 0;
    info.rtLc := -1;   (* maybe different for IlasmUtil and PeUtil? *)
  END InitProcInfo;

(* ------------------------------------------------------------ *)

  PROCEDURE (info : ProcInfo)newLocal*(typ : Sy.Type) : INTEGER,NEW;
    VAR ord : INTEGER;
  BEGIN
   (*
    *   We try to find a previously allocated, but
    *   currently free slot of the identical type.
    *)
    FOR ord := info.lNum TO info.tLst.tide-1 DO
      IF typ.equalType(info.fLst.a[ord]) THEN
        info.fLst.a[ord] := NIL;      (* mark ord as used *)
        RETURN ord;
      END;
    END;
   (* Free slot of correct type not found *)
    ord := info.tLst.tide;
    Sy.AppendType(info.tLst, typ);
    Sy.AppendType(info.fLst, NIL);
    RETURN ord;
  END newLocal;

(* ------------------------------------------------------------ *)

  PROCEDURE (info : ProcInfo)ReleaseLocal*(ord : INTEGER),NEW;
  BEGIN
    info.fLst.a[ord] := info.tLst.a[ord];
  END ReleaseLocal;

(* ------------------------------------------------------------ *)

  PROCEDURE (info : ProcInfo)numLocals*() : INTEGER,NEW;
  BEGIN
    RETURN info.tLst.tide;
  END numLocals;

(* ------------------------------------------------------------ *)

  PROCEDURE (info : ProcInfo)SetDepth*(d : INTEGER),NEW;
  BEGIN
    info.dNum := d;
  END SetDepth;

(* ------------------------------------------------------------ *)

  PROCEDURE (info : ProcInfo)getDepth*() : INTEGER,NEW;
  BEGIN
    RETURN info.dNum;
  END getDepth;

(* ============================================================ *)
(*                    Private Methods                           *)
(* ============================================================ *)


  PROCEDURE typeName*(typ : Sy.Type; os : MsilFile) : Lv.CharOpen;
  BEGIN
    IF typ.xName = NIL THEN MkTypeName(typ, os) END;
    WITH typ : Ty.Base DO
        RETURN typ.xName;
    | typ : Ty.Array DO
        RETURN typ.xName;
    | typ : Ty.Record DO
        RETURN typ.scopeNm;
    | typ : Ty.Pointer DO
        RETURN typ.xName;
    | typ : Ty.Opaque DO
        RETURN typ.xName;
    | typ : Ty.Enum DO
        RETURN typ.xName;
    | typ : Ty.Procedure DO
        RETURN typ.tName;
    END;
  END typeName;

(* ============================================================ *)

  PROCEDURE boxedName*(typ : Ty.Record; os : MsilFile) : Lv.CharOpen;
  BEGIN
    IF typ.xName = NIL THEN MkRecName(typ, os) END;
    RETURN cat3(typ.idnt.dfScp.scopeNm, boxedObj, typ.xName);
  END boxedName;

(* ============================================================ *)

  PROCEDURE MkTypeName*(typ : Sy.Type; fil : MsilFile);
  BEGIN
    WITH typ : Ty.Vector  DO MkVecName(typ, fil);
      |  typ : Ty.Array   DO MkArrName(typ, fil);
      |  typ : Ty.Base    DO MkBasName(typ, fil);
      |  typ : Ty.Record  DO MkRecName(typ, fil);
      |  typ : Ty.Pointer DO MkPtrName(typ, fil);
      |  typ : Ty.Opaque  DO MkAliasName(typ, fil);
      |  typ : Ty.Enum    DO MkEnumName(typ, fil);
      |  typ : Ty.Procedure DO MkPTypeName(typ, fil);
    END;
  END MkTypeName;

(* ============================================================ *)
(*                    Exported Methods                          *)
(* ============================================================ *)

  PROCEDURE (os : MsilFile)Adjust*(delta : INTEGER),NEW;
  BEGIN
    INC(os.proc.dNum, delta); 
    IF os.proc.dNum > os.proc.dMax THEN os.proc.dMax := os.proc.dNum END;
  END Adjust;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)newLabel*() : Label,NEW,ABSTRACT;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)getLabelRange*(num : INTEGER) : LbArr,NEW;
    VAR arr : LbArr;
        idx : INTEGER;
  BEGIN
    NEW(arr, num);
    FOR idx := 0 TO num-1 DO arr[idx] := os.newLabel() END;
    RETURN arr;
  END getLabelRange;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)EndCatch*(),NEW,EXTENSIBLE;
  BEGIN
    os.CloseCatch(); 
    os.DefLab(os.proc.exLb);
    IF os.proc.rtLc # -1 THEN os.PushLocal(os.proc.rtLc) END;
    os.FixCopies(os.proc.prId);
    os.Code(Asm.opc_ret);
  END EndCatch;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)DoReturn*(),NEW;
    VAR pTyp : Sy.Type;
  BEGIN
    IF os.proc.exLb = NIL THEN
      os.FixCopies(os.proc.prId);
      os.Code(Asm.opc_ret);
      pTyp := os.proc.prId.type;
      IF (pTyp # NIL) & (pTyp.returnType() # NIL) THEN DEC(os.proc.dNum) END;
    ELSE
      IF os.proc.rtLc # -1 THEN os.StoreLocal(os.proc.rtLc) END;
      os.CodeLb(Asm.opc_leave, os.proc.exLb);
    END;
  END DoReturn;


(* ============================================================ *)

  PROCEDURE (os : MsilFile)MkFixedArray*(arTp : Ty.Array),NEW;
    VAR cTmp : INTEGER;                    (* card'ty of this dim. *)
        aTmp : INTEGER;                  (* array reference temp *)
        labl : Label;
        elTp : Sy.Type;
        aLen : INTEGER;
  BEGIN
    ASSERT(arTp.length # 0);
    elTp := arTp.elemTp;
    aLen := arTp.length;
    os.PushInt(aLen);
   (* os.CodeTn(Asm.opc_newarr, elTp); *)
    os.CodeT(Asm.opc_newarr, elTp);
   (*
    *   Do we need an initialization loop?
    *)
    IF ~hasValueRep(elTp) THEN
      labl := os.newLabel();
      cTmp := os.proc.newLocal(Bi.intTp);
      aTmp := os.proc.newLocal(arTp);
      os.StoreLocal(aTmp);              (* (top)...             *)
      os.PushInt(aLen);
      os.StoreLocal(cTmp);
     (*
      *  Now the allocation loop
      *)
      os.DefLab(labl);
      os.DecTemp(cTmp);
      os.PushLocal(aTmp);
      os.PushLocal(cTmp);
      WITH elTp : Ty.Array DO
          os.MkFixedArray(elTp);
      | elTp : Ty.Record DO
          os.MkNewRecord(elTp);
      END;                              (* (top)elem,ix,ref,... *)
      os.PutElem(elTp);
     (*
      *  Now the termination test
      *)
      os.PushLocal(cTmp);
      os.CodeLb(Asm.opc_brtrue, labl);
      os.PushLocal(aTmp);
      os.proc.ReleaseLocal(cTmp);
      os.proc.ReleaseLocal(aTmp);
    END;
  END MkFixedArray;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)MkVecRec*(eTp : Sy.Type),NEW;
    VAR ord : INTEGER;
  BEGIN
    ord := mapVecElTp(eTp);
    os.MkNewRecord(vecRecord(ord));
  END MkVecRec;

  PROCEDURE (os : MsilFile)MkVecArr*(eTp : Sy.Type),NEW;
    VAR ord : INTEGER;
        vTp : Sy.Type;
  BEGIN
    ord := mapVecElTp(eTp);
    (*os.CodeTn(Asm.opc_newarr, mapOrdRepT(ord)); *)
    os.CodeT(Asm.opc_newarr, mapOrdRepT(ord));
    os.PutGetF(Asm.opc_stfld, vecArray(ord));
  END MkVecArr;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)MkOpenArray*(arTp : Ty.Array),NEW;
    VAR lens : ARRAY 32 OF INTEGER;
        elTp : Sy.Type;
   (* ----------------------------------------- *)
    PROCEDURE GetLengths(os  : MsilFile;
                     dim : INTEGER;
                     typ : Sy.Type;
                   VAR lAr : ARRAY OF INTEGER);
      VAR tmp : INTEGER;
    BEGIN
      ASSERT(dim < 31);
      WITH typ : Ty.Array DO
        IF typ.length = 0 THEN      (* another open dimension *)
          tmp := os.proc.newLocal(Bi.intTp);
          lAr[dim] := tmp;
          os.StoreLocal(tmp);
          GetLengths(os, dim+1, typ.elemTp, lAr);
        END;
      ELSE
      END;
    END GetLengths;
   (* ----------------------------------------- *)
    PROCEDURE InitLoop(os  : MsilFile;
                     dim : INTEGER;
                     typ : Ty.Array;
                  IN lAr : ARRAY OF INTEGER);
      VAR aEl : INTEGER;
          lab : Label;
          elT : Sy.Type;
    BEGIN
     (*
      *  Pre  : the uninitialized array is on the stack
      *)
      elT := typ.elemTp;
      IF ~hasValueRep(elT) THEN
        aEl := os.proc.newLocal(typ);
        os.StoreLocal(aEl);
        lab := os.newLabel();
       (*
        *  Start of initialization loop
        *)
        os.DefLab(lab);
       (*
        *  Decrement the loop counter
        *)
        os.DecTemp(lAr[dim]);
       (*
        *  Assign the array element
        *)
        os.PushLocal(aEl);
        os.PushLocal(lAr[dim]);
        WITH elT : Ty.Record DO
            os.MkNewRecord(elT);
        | elT : Ty.Array DO
            IF elT.length > 0 THEN
              os.MkFixedArray(elT);
            ELSE
              os.PushLocal(lAr[dim+1]);
              (*os.CodeTn(Asm.opc_newarr, elT.elemTp); *)
              os.CodeT(Asm.opc_newarr, elT.elemTp);
              InitLoop(os, dim+1, elT, lAr);
            END;
        END;
        os.PutElem(elT);
       (*
        *  Test and branch to loop header
        *)
        os.PushLocal(lAr[dim]);
        os.CodeLb(Asm.opc_brtrue, lab);
       (*
        *  Reload the original array
        *)
        os.PushLocal(aEl);
        os.proc.ReleaseLocal(aEl);
        os.proc.ReleaseLocal(lAr[dim]);
      END;
     (*
      *  Post : the initialized array is on the stack
      *)
    END InitLoop;
   (* ----------------------------------------- *)
  BEGIN
    elTp := arTp.elemTp;
    IF (elTp IS Ty.Array) OR (elTp IS Ty.Record) THEN
      GetLengths(os, 0, arTp, lens);
      os.PushLocal(lens[0]);
      (*os.CodeTn(Asm.opc_newarr, elTp); *)
      os.CodeT(Asm.opc_newarr, elTp);
      InitLoop(os, 0, arTp, lens);
    ELSE
      (*os.CodeTn(Asm.opc_newarr, elTp); *)
      os.CodeT(Asm.opc_newarr, elTp);
    END; 
  END MkOpenArray;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)MkArrayCopy*(arrT : Ty.Array),NEW;
    VAR dims : INTEGER;
        elTp : Sy.Type;
 (* ----------------------------------- *)
    PROCEDURE PushLengths(os : MsilFile; aT : Ty.Array);
    BEGIN
      IF aT.elemTp IS Ty.Array THEN
        os.Code(Asm.opc_dup);
        os.Code(Asm.opc_ldc_i4_0);
        os.GetElem(aT.elemTp);
        PushLengths(os, aT.elemTp(Ty.Array));
      END;
      os.Code(Asm.opc_ldlen);
    END PushLengths;
 (* ----------------------------------- *)
  BEGIN
   (*
    *        Assert: we must find the lengths from the runtime 
    *   descriptors. The array to copy is on the top of 
    *   stack, which reads -         (top) aRef, ...
    *)
    PushLengths(os, arrT);
    os.MkOpenArray(arrT);
  END MkArrayCopy;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)StructInit*(var : Sy.Idnt),NEW;
    VAR typ : Sy.Type;
        fld : Sy.Idnt;
        idx : INTEGER;
        lnk : BOOLEAN;
   (* ------------------------------------------------- *)
    PROCEDURE Assign(os : MsilFile; id : Sy.Idnt);
      VAR md : Id.BlkId;
    BEGIN
      WITH id : Id.LocId DO
          IF id.varOrd # Id.xMark THEN
            os.StoreLocal(id.varOrd);
          ELSE
            os.PutUplevel(id);
          END;
      | id : Id.FldId DO
          os.PutGetF(Asm.opc_stfld, id);
      | id : Id.VarId DO
          md := id.dfScp(Id.BlkId);
          os.PutGetS(Asm.opc_stsfld, md, id);
      END;
    END Assign;
   (* ------------------------------------------------- *)
  BEGIN
    os.Comment("initialize " + Sy.getName.ChPtr(var)^);
   (*
    *  Precondition: var is of a type that needs initialization,
    *)
    typ := var.type;
    lnk := (var IS Id.LocId) & (var(Id.LocId).varOrd = Id.xMark);
    WITH typ : Ty.Array DO
        IF lnk THEN os.Code(Asm.opc_ldloc_0) END;
        os.MkFixedArray(typ);
        Assign(os, var);
    | typ : Ty.Record DO
        IF Sy.clsTp IN typ.xAttr THEN
         (*
          *  Reference record type
          *)
          IF lnk THEN os.Code(Asm.opc_ldloc_0) END;
          os.MkNewRecord(typ);
          Assign(os, var);
        ELSE
         (*
          *  Value record type
          *)
          os.GetVarA(var);
          os.CodeTn(Asm.opc_initobj, typ);
        END;
    ELSE
      IF lnk THEN os.Code(Asm.opc_ldloc_0) END;
      os.Code(Asm.opc_ldnull);
      Assign(os, var);
    END;
  END StructInit;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)PushZero(typ : Sy.Type),NEW;
    VAR cde : INTEGER;
  BEGIN
    WITH typ : Ty.Base DO
      CASE typ.tpOrd OF
      | Ty.sReaN : os.CodeR(Asm.opc_ldc_r4, 0.0);
      | Ty.realN : os.CodeR(Asm.opc_ldc_r8, 0.0);
      | Ty.lIntN : os.CodeL(Asm.opc_ldc_i8, 0);
      | Ty.charN,
        Ty.sChrN : os.Code(Asm.opc_ldc_i4_0);
      ELSE           os.Code(Asm.opc_ldc_i4_0);
      END;
    ELSE
      os.Code(Asm.opc_ldnull);
    END;
  END PushZero;

 (* ----------------------------------- *)

  PROCEDURE (os : MsilFile)ScalarInit*(var : Sy.Idnt),NEW;
    VAR typ : Sy.Type;
        cde : INTEGER;
  BEGIN
    os.Comment("initialize " + Sy.getName.ChPtr(var)^);
    typ := var.type;
   (*
    *  Precondition: var is of a scalar type that is referenced
    *)
    os.PushZero(typ);
  END ScalarInit;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)Throw*(),NEW;
  BEGIN
    os.CodeS(Asm.opc_newobj, mkExcept);
    os.Code(Asm.opc_throw);
  END Throw;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)Trap*(IN str : ARRAY OF CHAR),NEW;
  BEGIN
    os.PushStr('"' + str + '"');
    os.Throw();
  END Trap;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)IndexTrap*(),NEW;
  BEGIN
    os.Comment("IndexTrap");
    os.Trap("Vector index out of bounds");
  END IndexTrap;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)CaseTrap*(i : INTEGER),NEW;
  BEGIN
    os.Comment("CaseTrap");
    os.PushLocal(i);
    os.CodeS(Asm.opc_call, caseMesg);
    os.CodeS(Asm.opc_newobj, mkExcept);
    os.Code(Asm.opc_throw);
  END CaseTrap;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)WithTrap*(id : Sy.Idnt),NEW;
  BEGIN
    os.Comment("WithTrap " + Sy.getName.ChPtr(id)^);
    os.GetVar(id);
    os.CodeS(Asm.opc_call, withMesg);
    os.CodeS(Asm.opc_newobj, mkExcept);
    os.Code(Asm.opc_throw);
  END WithTrap;

(* ============================================================ *)

  PROCEDURE EliminatePathFromSrcName(str : Lv.CharOpen): Lv.CharOpen;
  VAR
    i, idx, len: INTEGER;
    rslt: Lv.CharOpen;
  BEGIN
    FOR idx := LEN(str)-1 TO 0 BY - 1 DO
      IF str[idx] = '\' THEN
        len := LEN(str) - idx - 1;
	NEW (rslt, len);
	FOR i := 0 TO len - 2 DO rslt[i] := str[idx+i+1]; END;
	rslt[len-1] := 0X;
        RETURN rslt;
      END;
    END; (* FOR *)
    RETURN str;
  END EliminatePathFromSrcName;

  PROCEDURE (os : MsilFile)Header*(IN str : ARRAY OF CHAR),NEW;
    VAR date : ARRAY 64 OF CHAR;
  BEGIN
    os.srcS := Lv.strToCharOpen(
          "'" + EliminatePathFromSrcName(Lv.strToCharOpen(str))^ + "'");
    RTS.GetDateString(date);
    os.Comment("ILASM output produced by GPCP compiler (" +
                                RTS.defaultTarget + " version)");
    os.Comment("at date: " + date);
    os.Comment("from source file <" + str + '>');
  END Header;


(* ============================================================ *)
(*                    Namehandling Methods                      *)
(* ============================================================ *)

  PROCEDURE MkBlkName*(mod : Id.BlkId);
    VAR mNm : Lv.CharOpen;
  (* -------------------------------------------------- *)
    PROCEDURE scpMangle(mod : Id.BlkId) : Lv.CharOpen;
      VAR outS : Lv.CharOpen;
    BEGIN
      IF mod.kind = Id.impId THEN
        outS := cat4(lBrk,mod.pkgNm,rBrk,mod.xName);
      ELSE
        outS := mod.xName;
      END;
      IF LEN(mod.xName$) > 0 THEN outS := cat2(outS, dotS) END;
      RETURN outS;
    END scpMangle;
  (* -------------------------------------------------- *)
    PROCEDURE nmSpaceOf(mod : Id.BlkId) : Lv.CharOpen;
      VAR ix : INTEGER;
          ln : INTEGER;
          ch : CHAR;
          inS : Lv.CharOpen;
    BEGIN
      inS := mod.scopeNm;
      IF inS[0] # '[' THEN 
        RETURN inS;
      ELSE
        ln := LEN(inS);
        ix := 0;
        REPEAT
          ch := inS[ix]; 
          INC(ix);
        UNTIL (ix = LEN(inS)) OR (ch = ']');
        RETURN Lv.subChOToChO(inS, ix, ln-ix);
      END;
    END nmSpaceOf;
  (* -------------------------------------------------- *)
    PROCEDURE pkgNameOf(mod : Id.BlkId) : Lv.CharOpen;
      VAR ix : INTEGER;
          ln : INTEGER;
          ch : CHAR;
          inS : Lv.CharOpen;
    BEGIN
      inS := mod.scopeNm;
      IF inS[0] # '[' THEN 
        RETURN mod.clsNm;
      ELSE
        INCL(mod.xAttr, Sy.isFn); (* make sure this is marked foreign *)
        ln := LEN(inS);
        ix := 0;
        REPEAT
          ch := inS[ix]; 
          INC(ix);
        UNTIL (ix = LEN(inS)) OR (ch = ']');
        RETURN Lv.subChOToChO(inS, 1, ix-2);
      END;
    END pkgNameOf;
  (* -------------------------------------------------- *)
  BEGIN
    IF mod.xName # NIL THEN RETURN END;
    mNm := Sy.getName.ChPtr(mod);
    IF mod.scopeNm # NIL THEN 
      IF mod.clsNm = NIL THEN
        mod.clsNm   := mNm;            (* dummy class name  *)
      END;
      mod.pkgNm   := pkgNameOf(mod);   (* assembly filename *)
      mod.xName   := nmSpaceOf(mod);   (* namespace name    *)
      mod.scopeNm := scpMangle(mod);   (* class prefix name *)
    ELSE
      mod.clsNm   := mNm;              (* dummy class name  *)
      mod.pkgNm   := mNm;              (* assembly filename *)
      mod.xName   := mNm;              (* namespace name    *)
     (*
      *  In the normal case, the assembly name is the
      *  same as the module name.  However, system 
      *  modules always have the assembly name "RTS".
      *)
      IF Sy.rtsMd IN mod.xAttr THEN 
        mod.scopeNm := cat3(lBrk, rtsS, rBrk);
      ELSE
        mod.scopeNm := scpMangle(mod); (* class prefix name *)
      END;
    END;
  END MkBlkName;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)CheckNestedClass*(typ : Ty.Record;
                                             scp : Sy.Scope;
                                             rNm : Lv.CharOpen),NEW,ABSTRACT;

(* ------------------------------------------------------------ *
 *
 *PROCEDURE StrSubChr(str: Lv.CharOpen; 
 *                    ch1, ch2: CHAR): Lv.CharOpen;
 *  VAR i, len: INTEGER;
 *      rslt: Lv.CharOpen;
 *BEGIN
 * (* 
 *  *  copy str to rslt with all occurences of 
 *  *  ch1 replaced by ch2, except at index 0  
 *  *)
 *  len := LEN(str); NEW(rslt, len);
 *  rslt[0] := str[0];
 *  FOR i := 1 TO len-1 DO
 *    IF str[i] # ch1 THEN rslt[i] := str[i] ELSE rslt[i] := ch2 END;
 *  END; (* FOR *)
 *  RETURN rslt;
 *END StrSubChr;
 *
 * ------------------------------------------------------------ *)

  PROCEDURE MkRecName*(typ : Ty.Record; os : MsilFile);
    VAR mNm : Lv.CharOpen;      (* prefix scope name    *)
        rNm : Lv.CharOpen;      (* simple name of type  *)
        tId : Sy.Idnt;
        scp : Sy.Scope;
  (* ---------------------------------- *
   * The choice below may need revision *
   * depending on any decison about the *
   * format of foreign type-names       *
   * extracted from the metadata.       *
   * ---------------------------------- *)
    PROCEDURE unmangle(arr : Lv.CharOpen) : Lv.CharOpen;
    BEGIN
      RETURN arr;
    END unmangle;
  (* ---------------------------------------------------------- *)
  BEGIN
    IF typ.xName # NIL THEN RETURN END;

    IF (typ.baseTp IS Ty.Record) &
       (typ.baseTp.xName = NIL) THEN MkRecName(typ.baseTp(Ty.Record), os) END;

    IF typ.bindTp # NIL THEN              (* Synthetically named rec'd *)
      tId := typ.bindTp.idnt;
      rNm := Sy.getName.ChPtr(tId);
    ELSE                                  (* Normal, named record type *)
      IF typ.idnt = NIL THEN              (* Anonymous record type     *)
        typ.idnt := Id.newAnonId(typ.serial);
        typ.idnt.type := typ;
      END;
      tId := typ.idnt;
      rNm := Sy.getName.ChPtr(tId);
    END;

    IF tId.dfScp = NIL THEN tId.dfScp := CSt.thisMod END;
    scp := tId.dfScp;

    IF typ.extrnNm # NIL THEN
      typ.scopeNm := unmangle(typ.extrnNm);
     (*
      * This is an external class, so it might be a nested class!
      *)
      os.CheckNestedClass(typ, scp, rNm);
(*
 *    Console.WriteString(typ.name());
 *    Console.WriteLn;
 *
 *    rNm := StrSubChr(rNm,'$','/');
 *)
    END;

   (*
    *  At this program point the situation is as follows:
    *  rNm holds the simple name of the record. The scope
    *  in which the record is defined is scp.
    *)
    WITH scp : Id.Procs DO
        IF scp.prcNm = NIL THEN MkProcName(scp, os) END;
        rNm         := cat3(scp.prcNm, atSg, rNm);
        typ.xName   := rNm;
        typ.scopeNm := cat2(scp.scopeNm, rNm);
    | scp : Id.BlkId DO
        IF scp.xName = NIL THEN MkBlkName(scp) END;
        typ.xName   := rNm;
        typ.scopeNm := cat2(scp.scopeNm, rNm);
    END;
   (*
    *   It is at this point that we link records into the
    *   class-emission worklist.
    *)
    IF typ.tgXtn = NIL THEN os.MkRecX(typ, scp) END;
    IF tId.dfScp.kind # Id.impId THEN
      MsilBase.emitter.AddNewRecEmitter(typ);
    END;
  END MkRecName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkEnumName*(typ : Ty.Enum; os : MsilFile);
    VAR mNm : Lv.CharOpen;      (* prefix scope name    *)
        rNm : Lv.CharOpen;      (* simple name of type  *)
        tId : Sy.Idnt;
        scp : Sy.Scope;
  (* ---------------------------------------------------------- *)
  BEGIN
   (* Assert: Enums are always imported ... *)
    IF typ.xName # NIL THEN RETURN END;

    tId := typ.idnt;
    rNm := Sy.getName.ChPtr(tId);
    scp := tId.dfScp;
   (*
    *  At this program point the situation is at follows:
    *  rNm holds the simple name of the type. The scope
    *  in which the record is defined is scp.
    *)
    WITH  scp : Id.BlkId DO
      IF scp.xName = NIL THEN MkBlkName(scp) END;
      typ.xName := cat2(scp.scopeNm, rNm);
    END;
    os.MkEnuX(typ, scp);
  END MkEnumName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkBasName(typ : Ty.Base; os : MsilFile);
  BEGIN
    ASSERT(typ.xName # NIL);
    os.MkBasX(typ);
  END MkBasName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkArrName(typ : Ty.Array; os : MsilFile);
  BEGIN
    typ.xName := cat2(typeName(typ.elemTp, os), brks);
    os.MkArrX(typ);
  END MkArrName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkVecName(typ : Ty.Vector; os : MsilFile);
    VAR ord : INTEGER;
        cls : Id.TypId;
  BEGIN
    ord := mapVecElTp(typ.elemTp);
    CASE ord OF
    | Ty.charN  : typ.xName := cat2(vecPrefix, BOX("VecChr"));
    | Ty.intN   : typ.xName := cat2(vecPrefix, BOX("VecI32"));
    | Ty.lIntN  : typ.xName := cat2(vecPrefix, BOX("VecI64"));
    | Ty.sReaN  : typ.xName := cat2(vecPrefix, BOX("VecR32"));
    | Ty.realN  : typ.xName := cat2(vecPrefix, BOX("VecR64"));
    | Ty.anyPtr : typ.xName := cat2(vecPrefix, BOX("VecRef"));
    END;
    cls := vecClass(ord);
    IF cls.type.tgXtn = NIL THEN os.MkVecX(cls.type, vecMod()) END;
    typ.tgXtn := cls.type.tgXtn;
  END MkVecName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkPtrName*(typ : Ty.Pointer; os : MsilFile);
    VAR bndTp : Sy.Type;
        bndNm : Lv.CharOpen;
  BEGIN
    bndTp := typ.boundTp;
    bndNm := typeName(bndTp, os); (* recurse with MkTypeName *)
    IF isValRecord(bndTp) THEN
      typ.xName := boxedName(bndTp(Ty.Record), os);
    ELSE
      typ.xName := bndNm;
    END;
    os.MkPtrX(typ);
  END MkPtrName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkPTypeName*(typ : Ty.Procedure; os : MsilFile);
    VAR tNm : Lv.CharOpen;
        sNm : Lv.CharOpen;
  BEGIN
    IF typ.xName # NIL THEN RETURN END;
   (*
    *  Set the eName field
    *)
    IF typ.idnt = NIL THEN              (* Anonymous procedure type *)
      typ.idnt := Id.newAnonId(typ.serial);
      typ.idnt.type := typ;
    END;
    IF typ.idnt.dfScp = NIL THEN typ.idnt.dfScp := CSt.thisMod END;

    MkIdName(typ.idnt.dfScp, os);
    os.NumberParams(NIL, typ); 

    sNm := typ.idnt.dfScp.scopeNm;
    tNm := Sy.getName.ChPtr(typ.idnt);
    typ.tName := cat2(sNm, tNm);

    WITH typ : Ty.Event DO 
      typ.bndRec.xName := tNm;
      typ.bndRec.scopeNm := typ.tName 
    ELSE (* skip *) 
    END;
   (*
    *   os.MkTyXtn(...); // called from inside NumberParams().
    *
    *   It is at this point that we link events into the
    *   class-emission worklist.
    *)
    IF typ.idnt.dfScp.kind # Id.impId THEN
      MsilBase.emitter.AddNewRecEmitter(typ);
    END;
  END MkPTypeName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkProcName*(proc : Id.Procs; os : MsilFile);
    VAR pNm : Lv.CharOpen;
        res : Id.Procs;
        scp : Id.BlkId;
        bTp : Ty.Record;
  (* -------------------------------------------------- *)
    PROCEDURE MkMthNm(mth : Id.MthId; os : MsilFile);
      VAR res : Id.MthId;
          scp : Id.BlkId;
          typ : Sy.Type;
    BEGIN
      IF mth.scopeNm # NIL THEN RETURN;
      ELSIF mth.kind = Id.fwdMth THEN
        res := mth.resolve(Id.MthId); MkMthNm(res, os);
        mth.prcNm := res.prcNm; mth.scopeNm := res.scopeNm;
      ELSE
        scp := mth.dfScp(Id.BlkId);
        typ := mth.bndType;
        IF typ.xName = NIL THEN MkRecName(typ(Ty.Record), os) END;
        IF scp.xName = NIL THEN MkBlkName(scp) END;
        mth.scopeNm := scp.scopeNm;
        IF mth.prcNm = NIL THEN mth.prcNm := Sy.getName.ChPtr(mth) END;
        IF ~(Sy.clsTp IN typ(Ty.Record).xAttr) &
           (mth.rcvFrm.type IS Ty.Pointer) THEN INCL(mth.mthAtt, Id.boxRcv) END;
      END;
    END MkMthNm;
  (* -------------------------------------------------- *)
    PROCEDURE className(p : Id.Procs) : Lv.CharOpen;
    BEGIN
      WITH p : Id.PrcId DO RETURN p.clsNm;
      |    p : Id.MthId DO RETURN p.bndType.xName;
      END;
    END className;
  (* -------------------------------------------------- *)
    PROCEDURE GetClassName(pr : Id.PrcId; bl : Id.BlkId; os : MsilFile);
      VAR nm : Lv.CharOpen;
    BEGIN
      nm := Sy.getName.ChPtr(pr);
      IF pr.bndType = NIL THEN   (* normal procedure *)
        pr.clsNm := bl.clsNm;
        IF pr.prcNm = NIL THEN pr.prcNm := nm END;
      ELSE                       (* static method    *)
        IF pr.bndType.xName = NIL THEN MkRecName(pr.bndType(Ty.Record), os) END;
        pr.clsNm := pr.bndType.xName; 
        IF pr.prcNm = NIL THEN 
          pr.prcNm := nm;
        ELSIF pr.prcNm^ = initString THEN 
          pr.SetKind(Id.ctorP);
        END;
      END;
    END GetClassName;
  (* -------------------------------------------------- *)
    PROCEDURE MkPrcNm(prc : Id.PrcId; os : MsilFile);
      VAR scp : Sy.Scope;
          res : Id.PrcId;
          blk : Id.BlkId;
          rTp : Sy.Type;
    BEGIN
      IF prc.scopeNm # NIL THEN RETURN;
      ELSIF prc.kind = Id.fwdPrc THEN
        res := prc.resolve(Id.PrcId); MkPrcNm(res, os);
        prc.prcNm := res.prcNm; 
        prc.clsNm := res.clsNm;
        prc.scopeNm := res.scopeNm;
      ELSIF prc.kind = Id.conPrc THEN
        scp := prc.dfScp;
        WITH scp : Id.BlkId DO
            IF scp.xName = NIL THEN MkBlkName(scp) END;
            IF  Sy.isFn IN scp.xAttr THEN
              GetClassName(prc, scp, os);
            ELSE
              prc.clsNm := scp.clsNm;
              IF prc.prcNm = NIL THEN prc.prcNm := Sy.getName.ChPtr(prc) END;
            END;
        | scp : Id.Procs DO
            MkProcName(scp, os);
            prc.clsNm := className(scp);
            prc.prcNm   := cat3(Sy.getName.ChPtr(prc), atSg, scp.prcNm);
        END;
        prc.scopeNm := scp.scopeNm;
      ELSE (* prc.kind = Id.ctorP *)
        blk := prc.dfScp(Id.BlkId);
        rTp := prc.type.returnType();
        IF blk.xName = NIL THEN MkBlkName(blk) END;
        IF rTp.xName = NIL THEN MkTypeName(rTp, os) END;
        prc.clsNm := rTp.boundRecTp().xName;
        prc.prcNm := Lv.strToCharOpen(initString); 
        prc.scopeNm := blk.scopeNm;

        prc.bndType := rTp.boundRecTp();
        prc.type(Ty.Procedure).retType := NIL;

      END;
    END MkPrcNm;
  (* -------------------------------------------------- *)
  BEGIN
    WITH proc : Id.MthId DO MkMthNm(proc, os);
    |    proc : Id.PrcId DO MkPrcNm(proc, os);
    END;
   (*
    *    In this case proc.tgXtn is set in NumberParams 
    *)
  END MkProcName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkAliasName*(typ : Ty.Opaque; os : MsilFile);
    VAR tNm : Lv.CharOpen;
        sNm : Lv.CharOpen;
  BEGIN
    IF typ.xName # NIL THEN RETURN END;
    MkBlkName(typ.idnt.dfScp(Id.BlkId));
    tNm := Sy.getName.ChPtr(typ.idnt);
    sNm := typ.idnt.dfScp.scopeNm;
    typ.xName   := cat2(sNm, tNm);
    typ.scopeNm := sNm;
  END MkAliasName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkVarName*(var : Id.VarId; os : MsilFile);
  BEGIN
    var.varNm := Sy.getName.ChPtr(var);
    IF var.recTyp = NIL THEN     (* normal case  *)
      var.clsNm := var.dfScp(Id.BlkId).clsNm;
    ELSE                         (* static field *)
      IF var.recTyp.xName = NIL THEN MkTypeName(var.recTyp, os) END;
      var.clsNm := var.recTyp.xName; 
    END;
  END MkVarName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkFldName*(id : Id.FldId; os : MsilFile);
  BEGIN 
    id.fldNm := Sy.getName.ChPtr(id);
  END MkFldName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkIdName*(id : Sy.Idnt; os : MsilFile);
  BEGIN
    WITH id : Id.Procs DO IF id.scopeNm = NIL THEN MkProcName(id, os) END;
    |    id : Id.BlkId DO IF id.scopeNm = NIL THEN MkBlkName(id)      END;
    |    id : Id.VarId DO IF id.varNm   = NIL THEN MkVarName(id, os)  END;
    |    id : Id.FldId DO IF id.fldNm   = NIL THEN MkFldName(id, os)  END;
    |    id : Id.LocId DO (* skip *)
    END;
  END MkIdName;
 
(* ------------------------------------------------------------ *)

  PROCEDURE NumberLocals(pIdn : Id.Procs; IN locs : Sy.IdSeq);
    VAR ident : Sy.Idnt;
        index : INTEGER;
        count : INTEGER;
  BEGIN
    count := 0;
    (* ------------------ *)
    IF Id.hasXHR IN pIdn.pAttr THEN MkXHR(pIdn); INC(count) END;
    (* ------------------ *)
    FOR index := 0 TO locs.tide-1 DO
      ident := locs.a[index];
      WITH ident : Id.ParId DO (* skip *)
      | ident : Id.LocId DO
          IF Id.uplevA IN ident.locAtt THEN
            ident.varOrd := Id.xMark;
          ELSE
            ident.varOrd := count;
            INC(count);
          END;
      END;
    END;
    pIdn.rtsFram := count;
  END NumberLocals;

(* ------------------------------------------------------------ *)

  PROCEDURE MkCallAttr*(pIdn : Sy.Idnt; os : MsilFile); 
    VAR pTyp : Ty.Procedure;
        rcvP : Id.ParId;
  BEGIN
   (*  
    *  This is only called for imported methods.  
    *  All local methods have been already fixed 
    *  by the call from RenumberLocals() 
    *)
    pTyp := pIdn.type(Ty.Procedure);
    WITH pIdn : Id.MthId DO
        pTyp.argN := 1;            (* count one for "this" *)
        rcvP := pIdn.rcvFrm;
        MkProcName(pIdn, os);
        IF takeAdrs(rcvP) THEN rcvP.boxOrd := rcvP.parMod END;
        os.NumberParams(pIdn, pTyp);
    | pIdn : Id.PrcId DO 
        pTyp.argN := 0;
        MkProcName(pIdn, os);
        os.NumberParams(pIdn, pTyp);
    END;
  END MkCallAttr;

(* ------------------------------------------------------------ *)

  PROCEDURE RenumberLocals*(prcId : Id.Procs; os : MsilFile);
    VAR parId : Id.ParId;
        frmTp : Ty.Procedure;
        funcT : BOOLEAN;
  BEGIN
   (*  
    *  This is only called for local methods.
    *  Imported methods do not have visible locals,
    *  and get their signatures computed by the 
    *  call of NumberParams() in MkCallAttr()
    *
    *  Numbering Rules:
    *         (i)   The receiver (if any) must be #0
    *         (ii)  Params are #0 .. #N for statics, 
    *               or #1 .. #N for methods.
    *         (iii) Incoming static link is #0 if this is
    *               a nested procedure (methods are not nested)
    *         (iv)  Locals separately number from zero.
    *) 
    frmTp := prcId.type(Ty.Procedure);
    funcT := (frmTp.retType # NIL);
    WITH prcId : Id.MthId DO
      parId := prcId.rcvFrm;
      parId.varOrd := 0;   
      IF takeAdrs(parId) THEN parId.boxOrd := parId.parMod END;
      frmTp.argN := 1;              (* count one for "this" *)
    ELSE (* static procedures *)
      IF (prcId.kind = Id.ctorP) OR
         (prcId.lxDepth > 0) THEN frmTp.argN := 1 ELSE frmTp.argN := 0 END;
    END;
   (*
    *   Assert: params do not appear in the local array.
    *   Count params.
    *)
    os.NumberParams(prcId, frmTp); (* Make signature method defined here *)
(*
 *  If NumberLocals is NOT called on a procedure that
 *  has locals but no body, then PeUtil pulls an index
 *  exception. Such a program may be silly, but is legal. (kjg)
 *
 *  IF prcId.body # NIL THEN
 *    NumberLocals(prcId, prcId.locals);
 *  END;
 *)
    NumberLocals(prcId, prcId.locals);
  END RenumberLocals;

(* ------------------------------------------------------------ *)
(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)LoadIndirect*(typ : Sy.Type),NEW;
    VAR code : INTEGER;
  BEGIN
    IF (typ # NIL) & (typ IS Ty.Base) THEN
      os.Code(typeLdInd[typ(Ty.Base).tpOrd]);
    ELSIF isValRecord(typ) THEN
      os.CodeT(Asm.opc_ldobj, typ);
    ELSE
      os.Code(Asm.opc_ldind_ref);
    END;
  END LoadIndirect;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)StoreIndirect*(typ : Sy.Type),NEW;
    VAR code : INTEGER;
  BEGIN
    IF (typ # NIL) & (typ IS Ty.Base) THEN
      os.Code(typeStInd[typ(Ty.Base).tpOrd]);
    ELSIF isValRecord(typ) THEN
      os.CodeT(Asm.opc_stobj, typ);
    ELSE
      os.Code(Asm.opc_stind_ref);
    END;
  END StoreIndirect;

(* ------------------------------------------------------------ *)
(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PushArg*(ord : INTEGER),NEW;
  BEGIN
    IF ord < 256 THEN
      CASE ord OF
      | 0 : os.Code(Asm.opc_ldarg_0);
      | 1 : os.Code(Asm.opc_ldarg_1);
      | 2 : os.Code(Asm.opc_ldarg_2);
      | 3 : os.Code(Asm.opc_ldarg_3);
      ELSE
        os.CodeI(Asm.opc_ldarg_s, ord);
      END;
    ELSE
      os.CodeI(Asm.opc_ldarg, ord);
    END;
  END PushArg;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PushStaticLink*(tgt : Id.Procs),NEW;
    VAR lxDel : INTEGER;
        clr   : Id.Procs;
  BEGIN
    clr   := os.proc.prId(Id.Procs);
    lxDel := tgt.lxDepth - clr.lxDepth;

    CASE lxDel OF
    | 0 : os.Code(Asm.opc_ldarg_0);
    | 1 : IF Id.hasXHR IN clr.pAttr THEN
            os.Code(Asm.opc_ldloc_0);
          ELSIF clr.lxDepth = 0 THEN
            os.Code(Asm.opc_ldnull);
          ELSE
            os.Code(Asm.opc_ldarg_0);
          END;
    ELSE
      os.Code(Asm.opc_ldarg_0);
      REPEAT
        clr := clr.dfScp(Id.Procs);
        IF Id.hasXHR IN clr.pAttr THEN 
          os.PutGetF(Asm.opc_ldfld, CSt.xhrId);
        END;
      UNTIL clr.lxDepth = tgt.lxDepth;
    END;
  END PushStaticLink;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)GetXHR(var : Id.LocId),NEW;
    VAR scp : Id.Procs; (* the scope holding the datum *)
        clr : Id.Procs; (* the scope making the call   *)
        del : INTEGER;
  BEGIN
    scp := var.dfScp(Id.Procs);
    clr := os.proc.prId(Id.Procs);
   (*
    *  Check if this is an own local
    *)
    IF scp = clr THEN
      os.Code(Asm.opc_ldloc_0);
    ELSE
      del := xhrCount(scp, clr);
     (*
      *  First, load the static link
      *)
      os.Code(Asm.opc_ldarg_0);
     (*
      *  Next, load the XHR pointer.
      *)
      WHILE del > 1 DO
        os.PutGetF(Asm.opc_ldfld, CSt.xhrId);
        DEC(del);
      END;
     (*
      *  Finally, cast to concrete type
      *)
      os.CodeT(Asm.opc_castclass, scp.xhrType);
    END;
  END GetXHR;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PushLocal*(ord : INTEGER),NEW;
  BEGIN
    IF ord < 256 THEN
      CASE ord OF
      | 0 : os.Code(Asm.opc_ldloc_0);
      | 1 : os.Code(Asm.opc_ldloc_1);
      | 2 : os.Code(Asm.opc_ldloc_2);
      | 3 : os.Code(Asm.opc_ldloc_3);
      ELSE
        os.CodeI(Asm.opc_ldloc_s, ord);
      END;
    ELSE
      os.CodeI(Asm.opc_ldloc, ord);
    END;
  END PushLocal;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)PushLocalA*(ord : INTEGER),NEW;
  BEGIN
    IF ord < 256 THEN
      os.CodeI(Asm.opc_ldloca_s, ord);
    ELSE
      os.CodeI(Asm.opc_ldloca, ord);
    END;
  END PushLocalA;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)PushArgA*(ord : INTEGER),NEW;
  BEGIN
    IF ord < 256 THEN
      os.CodeI(Asm.opc_ldarga_s, ord);
    ELSE
      os.CodeI(Asm.opc_ldarga, ord);
    END;
  END PushArgA;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)GetXhrField(cde : INTEGER; var : Id.LocId),NEW;
    VAR proc : Id.Procs;
  BEGIN
    proc := var.dfScp(Id.Procs);
    os.PutGetXhr(cde, proc, var);
  END GetXhrField;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)XhrHandle*(var : Id.LocId),NEW;
  BEGIN
    os.GetXHR(var);
    IF var.boxOrd # Sy.val THEN os.GetXhrField(Asm.opc_ldfld, var) END; 
  END XhrHandle;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)GetUplevel(var : Id.LocId),NEW;
  BEGIN
    os.GetXHR(var);
   (*
    *  If var is a LocId do "ldfld FT XT::'vname'"
    *  If var is a ParId then
    *    if not a byref then "ldfld FT XT::'vname'"
    *    elsif is a byref then "ldfld FT& XT::'vname'; ldind.TT"
    *)
    os.GetXhrField(Asm.opc_ldfld, var); 
    IF var.boxOrd # Sy.val THEN os.LoadIndirect(var.type) END;
  END GetUplevel;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)GetUplevelA(var : Id.LocId),NEW;
  BEGIN
    os.GetXHR(var);
   (*
    *  If var is a LocId do "ldflda FT XT::'vname'"
    *  If var is a ParId then
    *    if not a byref then "ldflda FT XT::'vname'"
    *    elsif is a byref then "ldfld FT& XT::'vname'"
    *)
    IF var.boxOrd # Sy.val THEN         (* byref case ... *)
      os.GetXhrField(Asm.opc_ldfld, var); 
    ELSE                               (* value case ... *)
      os.GetXhrField(Asm.opc_ldflda, var); 
    END;
  END GetUplevelA;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)PutUplevel*(var : Id.LocId),NEW;
  BEGIN
   (*
    *  If var is a LocId do "stfld FT XT::'vname'"
    *  If var is a ParId then
    *    if not a byref then "stfld FT XT::'vname'"
    *    elsif is a byref then "ldfld FT& XT::'vname'; stind.TT"
    *)
    IF var.boxOrd # Sy.val THEN         (* byref case ... *)
      os.StoreIndirect(var.type);
    ELSE                               (* value case ... *)
      os.GetXhrField(Asm.opc_stfld, var); 
    END;
  END PutUplevel;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)GetLocal*(var : Id.LocId),NEW;
  BEGIN
    IF Id.uplevA IN var.locAtt THEN os.GetUplevel(var); RETURN END;
    WITH var : Id.ParId DO
      os.PushArg(var.varOrd);
      IF var.boxOrd # Sy.val THEN os.LoadIndirect(var.type) END;
    ELSE
      os.PushLocal(var.varOrd);
    END;
  END GetLocal;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)DecTemp*(ord : INTEGER),NEW;
  BEGIN
    os.PushLocal(ord);
    os.Code(Asm.opc_ldc_i4_1);
    os.Code(Asm.opc_sub);
    os.StoreLocal(ord);
  END DecTemp;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)GetVar*(id : Sy.Idnt),NEW;
    VAR scp : Sy.Scope;
  BEGIN
    WITH id : Id.AbVar DO
      IF id.kind = Id.conId THEN
        os.GetLocal(id(Id.LocId));
      ELSE
        scp := id.dfScp;
        WITH scp : Id.BlkId DO
          os.PutGetS(Asm.opc_ldsfld, scp, id(Id.VarId));
        ELSE
          os.GetLocal(id(Id.LocId));
        END;
      END;
    END;
  END GetVar;

(* ------------------------------------------------------------ *)
(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)GetLocalA(var : Id.LocId),NEW;
    VAR ord : INTEGER;
  BEGIN
    ord := var.varOrd;
    IF Id.uplevA IN var.locAtt THEN os.GetUplevelA(var); RETURN END;
    IF ~(var IS Id.ParId) THEN              (* local var *)
      os.PushLocalA(ord);
    ELSIF var.boxOrd # Sy.val THEN        (* ref param *)
      os.PushArg(ord);
    ELSE                          (* val param *)
      os.PushArgA(ord);
    END;
  END GetLocalA;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)GetVarA*(id : Sy.Idnt),NEW;
    VAR var : Id.AbVar;
        scp : Sy.Scope;
  BEGIN
   (*
    *  Assert:  the handle is NOT pushed on the tos yet.
    *)
    var := id(Id.AbVar);
    scp := var.dfScp;
    WITH scp : Id.BlkId DO
      os.PutGetS(Asm.opc_ldsflda, scp, var(Id.VarId));
    ELSE
      os.GetLocalA(var(Id.LocId));
    END;
  END GetVarA;

(* ------------------------------------------------------------ *)
(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)StoreArg*(ord : INTEGER),NEW;
  BEGIN
    IF ord < 256 THEN
      os.CodeI(Asm.opc_starg_s, ord);
    ELSE
      os.CodeI(Asm.opc_starg, ord);
    END;
  END StoreArg;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)StoreLocal*(ord : INTEGER),NEW;
  BEGIN
    IF ord < 256 THEN
      CASE ord OF
      | 0 : os.Code(Asm.opc_stloc_0);
      | 1 : os.Code(Asm.opc_stloc_1);
      | 2 : os.Code(Asm.opc_stloc_2);
      | 3 : os.Code(Asm.opc_stloc_3);
      ELSE
        os.CodeI(Asm.opc_stloc_s, ord);
      END;
    ELSE
      os.CodeI(Asm.opc_stloc, ord);
    END;
  END StoreLocal;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)PutLocal*(var : Id.LocId),NEW;
  BEGIN
    IF Id.uplevA IN var.locAtt THEN os.PutUplevel(var); RETURN END;
    WITH var : Id.ParId DO
      IF var.boxOrd = Sy.val THEN 
        os.StoreArg(var.varOrd);
      ELSE
       (*
        *  stack goes (top) value, reference, ... so
        *  os.PushArg(var.varOrd);
        *)
        os.StoreIndirect(var.type);
      END;
    ELSE
      os.StoreLocal(var.varOrd);
    END;
  END PutLocal;

(* ---------------------------------------------------- *)

  PROCEDURE (os : MsilFile)PutVar*(id : Sy.Idnt),NEW;
    VAR var : Id.AbVar;
        scp : Sy.Scope;
  BEGIN
    var := id(Id.AbVar);
    scp := var.dfScp;
    WITH scp : Id.BlkId DO
      os.PutGetS(Asm.opc_stsfld, scp, var(Id.VarId));
    ELSE (* must be local *)
      os.PutLocal(var(Id.LocId));
    END;
  END PutVar;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PutElem*(typ : Sy.Type),NEW;
  (* typ is element type *)
  BEGIN
    IF (typ # NIL) & (typ IS Ty.Base) THEN
      os.Code(typePutE[typ(Ty.Base).tpOrd]);
    ELSIF isValRecord(typ) THEN
      os.CodeT(Asm.opc_stobj, typ);
    ELSIF typ IS Ty.Enum THEN
      os.Code(typePutE[Ty.intN]);  (* assume enum <==> int32 *)
    ELSE
      os.Code(Asm.opc_stelem_ref);
    END;
  END PutElem;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)GetElem*(typ : Sy.Type),NEW;
  BEGIN
    IF (typ # NIL) & (typ IS Ty.Base) THEN
      os.Code(typeGetE[typ(Ty.Base).tpOrd]);
    ELSIF isValRecord(typ) THEN
      os.CodeT(Asm.opc_ldobj, typ);
    ELSIF typ IS Ty.Enum THEN
      os.Code(typeGetE[Ty.intN]);  (* assume enum <==> int32 *)
    ELSE
      os.Code(Asm.opc_ldelem_ref);
    END;
  END GetElem;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)GetField*(fld : Id.FldId),NEW;
  BEGIN
    os.PutGetF(Asm.opc_ldfld, fld);
  END GetField;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)GetFieldAdr*(fld : Id.FldId),NEW;
  BEGIN
    os.PutGetF(Asm.opc_ldflda, fld);
  END GetFieldAdr;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PutField*(fld : Id.FldId),NEW;
  BEGIN
    os.PutGetF(Asm.opc_stfld, fld);
  END PutField;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)GetElemA*(typ : Sy.Type),NEW;
  BEGIN
    os.CodeTn(Asm.opc_ldelema, typ);
  END GetElemA;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)GetVal*(typ : Ty.Pointer),NEW;
  BEGIN
    os.GetValObj(Asm.opc_ldfld, typ);
  END GetVal;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)GetValA*(typ : Ty.Pointer),NEW;
  BEGIN
    os.GetValObj(Asm.opc_ldflda, typ);
  END GetValA;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PushInt*(num : INTEGER),NEW;
  BEGIN
    IF (-128 <= num) & (num <= 127) THEN
      CASE num OF
      | -1 : os.Code(Asm.opc_ldc_i4_M1);
      |  0 : os.Code(Asm.opc_ldc_i4_0);
      |  1 : os.Code(Asm.opc_ldc_i4_1);
      |  2 : os.Code(Asm.opc_ldc_i4_2);
      |  3 : os.Code(Asm.opc_ldc_i4_3);
      |  4 : os.Code(Asm.opc_ldc_i4_4);
      |  5 : os.Code(Asm.opc_ldc_i4_5);
      |  6 : os.Code(Asm.opc_ldc_i4_6);
      |  7 : os.Code(Asm.opc_ldc_i4_7);
      |  8 : os.Code(Asm.opc_ldc_i4_8);
      ELSE
        os.CodeI(Asm.opc_ldc_i4_s, num);
      END;
    ELSE
      os.CodeI(Asm.opc_ldc_i4, num);
    END;
  END PushInt;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PushLong*(num : LONGINT),NEW;
  BEGIN
   (*
    *  IF num is short we could do PushInt, then i2l!
    *)
    os.CodeL(Asm.opc_ldc_i8, num);
  END PushLong;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PushReal*(num : REAL),NEW;
  BEGIN
    os.CodeR(Asm.opc_ldc_r8, num);
  END PushReal;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PushSReal*(num : REAL),NEW;
  BEGIN
    os.CodeR(Asm.opc_ldc_r4, num);
  END PushSReal;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)PushJunkAndQuit*(prc : Sy.Scope),NEW;
    VAR pTyp : Ty.Procedure;
  BEGIN
    IF (prc # NIL) & (prc.type # NIL) THEN
      pTyp := prc.type(Ty.Procedure);
      IF pTyp.retType # NIL THEN os.PushZero(pTyp.retType) END;
    END;
    os.DoReturn();
  END PushJunkAndQuit;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)ConvertUp*(inT, outT : Sy.Type),NEW;
   (* Conversion "up" is always safe at runtime. Many are nop. *)
    VAR inB, outB, code : INTEGER;
  BEGIN
    inB  := inT(Ty.Base).tpOrd;
    outB := outT(Ty.Base).tpOrd;
    IF inB = outB THEN RETURN END;                     (* PREMATURE RETURN! *)
    CASE outB OF
    | Ty.realN : code := Asm.opc_conv_r8;
    | Ty.sReaN : code := Asm.opc_conv_r4;
    | Ty.lIntN : code := Asm.opc_conv_i8;
    ELSE               RETURN;                         (* PREMATURE RETURN! *)
    END;
    os.Code(code);
  END ConvertUp;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)ConvertDn*(inT, outT : Sy.Type; check : BOOLEAN),NEW;
   (* Conversion "down" often needs a runtime check. *)
    VAR inB, outB, code : INTEGER;
  BEGIN
    inB  := inT(Ty.Base).tpOrd;
    outB := outT(Ty.Base).tpOrd;
    IF inB = Ty.setN THEN inB := Ty.intN END;
    IF inB = outB THEN RETURN END;                     (* PREMATURE RETURN! *)
    (* IF os.proc.prId.ovfChk THEN *)
	IF check THEN
      CASE outB OF
      | Ty.realN : RETURN;                             (* PREMATURE RETURN! *)
      | Ty.sReaN : code := Asm.opc_conv_r4; (* No check possible *)
      | Ty.lIntN : code := Asm.opc_conv_ovf_i8;
      | Ty.intN  : code := Asm.opc_conv_ovf_i4;
      | Ty.sIntN : code := Asm.opc_conv_ovf_i2;
      | Ty.uBytN : code := Asm.opc_conv_ovf_u1;
      | Ty.byteN : code := Asm.opc_conv_ovf_i1;
      | Ty.setN  : code := Asm.opc_conv_u4; (* no check here! *)
      | Ty.charN : code := Asm.opc_conv_ovf_u2;
      | Ty.sChrN : code := Asm.opc_conv_ovf_u1;
      END;
    ELSE
      CASE outB OF
      | Ty.realN : RETURN;                             (* PREMATURE RETURN! *)
      | Ty.sReaN : code := Asm.opc_conv_r4; (* No check possible *)
      | Ty.lIntN : code := Asm.opc_conv_i8;
      | Ty.intN  : code := Asm.opc_conv_i4;
      | Ty.sIntN : code := Asm.opc_conv_i2;
      | Ty.byteN : code := Asm.opc_conv_i1;
      | Ty.uBytN : code := Asm.opc_conv_u1;
      | Ty.setN  : code := Asm.opc_conv_u4; (* no check here! *)
      | Ty.charN : code := Asm.opc_conv_u2;
      | Ty.sChrN : code := Asm.opc_conv_u1;
      END;
    END;
    os.Code(code);
  END ConvertDn;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)EmitOneRange*
             (var : INTEGER;            (* local variable index *)
              loC : INTEGER;            (* low-value of range   *)
              hiC : INTEGER;            (* high-value of range  *)
              ord : INTEGER;            (* case-index of range  *)
              min : INTEGER;            (* minimun selector val *)
              max : INTEGER;            (* maximum selector val *)
              def : LbArr),NEW;         (* default code label   *)
  (* ---------------------------------------------------------- *
   *  The selector value is known to be in the range min .. max *
   *  and we wish to send values between loC and hiC to the     *
   *  code label associated with ord. All otherwise go to def.  *
   *  A range is "compact" if it is hard against min/max limits *
   * ---------------------------------------------------------- *)
    VAR target : INTEGER;
  BEGIN
   (*
    *    Deal with several special cases...
    *)
    target := ord + 1;
    IF (min = loC) & (max = hiC) THEN      (* fully compact: just GOTO *)
      os.CodeLb(Asm.opc_br, def[target]);
    ELSE
      os.PushLocal(var);
      IF loC = hiC THEN (* a singleton *)
        os.PushInt(loC);
        os.CodeLb(Asm.opc_beq, def[target]);
      ELSIF min = loC THEN                 (* compact at low end only  *)
        os.PushInt(hiC);
        os.CodeLb(Asm.opc_ble, def[target]);
      ELSIF max = hiC THEN                 (* compact at high end only *)
        os.PushInt(loC);
        os.CodeLb(Asm.opc_bge, def[target]);
      ELSE                                 (* Shucks! The general case *)
        IF loC # 0 THEN
          os.PushInt(loC);
          os.Code(Asm.opc_sub);
        END;
        os.PushInt(hiC-loC);
        os.CodeLb(Asm.opc_ble_un, def[target]);
      END;
      os.CodeLb(Asm.opc_br, def[0]);
    END;
  END EmitOneRange;

(* ------------------------------------------------------------ *)
(* ------------------------------------------------------------ *)

  PROCEDURE (os : MsilFile)InitVars*(scp : Sy.Scope),NEW;
    VAR index : INTEGER;
        ident : Sy.Idnt;
  BEGIN
   (* 
    *  Create the explicit activation record, if needed.
    *)
    WITH scp : Id.Procs DO
      IF Id.hasXHR IN scp.pAttr THEN 
        os.Comment("create XHR record");
        os.MkNewRecord(scp.xhrType.boundRecTp()(Ty.Record));
        IF scp.lxDepth > 0 THEN
          os.Code(Asm.opc_dup);
          os.Code(Asm.opc_ldarg_0);
          os.PutGetF(Asm.opc_stfld, CSt.xhrId);
        END;
        os.Code(Asm.opc_stloc_0);
      END;
    ELSE (* skip *)
    END;
   (* 
    *  Initialize local fields, if needed
    *)
    FOR index := 0 TO scp.locals.tide-1 DO
      ident := scp.locals.a[index];
      WITH ident : Id.ParId DO
        IF Id.uplevA IN ident.locAtt THEN  (* copy to XHR *)
          os.GetXHR(ident);
          os.PushArg(ident.varOrd);
          IF Id.cpVarP IN ident.locAtt THEN os.LoadIndirect(ident.type) END;
          os.GetXhrField(Asm.opc_stfld, ident);
        END; (* else skip *)
      ELSE
        IF ~ident.type.isScalarType() THEN 
          os.StructInit(ident);
        ELSE
          WITH ident : Id.LocId DO
           (*
            *  Special code to step around deficiency in the the
            *  verifier. Verifier does not understand OUT semantics.
            *
            *  IF Id.addrsd IN ident.locAtt THEN
            *)
            IF (Id.addrsd IN ident.locAtt) & ~(Id.uplevA IN ident.locAtt) THEN
              ASSERT(~(scp IS Id.BlkId));
              os.ScalarInit(ident); 
              os.StoreLocal(ident.varOrd);
            END;
          ELSE
          END;
        END;
      END;
    END;
  END InitVars;

(* ============================================================ *)

  PROCEDURE (os : MsilFile)FixCopies(prId : Sy.Idnt),NEW;
    VAR index : INTEGER;
        pType : Ty.Procedure;
        formP : Id.ParId;
  BEGIN
    IF prId # NIL THEN 
      WITH prId : Id.Procs DO
        pType := prId.type(Ty.Procedure);
        FOR index := 0 TO pType.formals.tide - 1 DO
          formP := pType.formals.a[index];
          IF Id.cpVarP IN formP.locAtt THEN
            os.PushArg(formP.varOrd);
            os.GetXHR(formP);
            os.GetXhrField(Asm.opc_ldfld, formP);
            os.StoreIndirect(formP.type);
          END;
        END;
      ELSE (* skip *)
      END; (* with *)
    END;
  END FixCopies;

(* ============================================================ *)

  PROCEDURE InitVectorDescriptors();
    VAR idx : INTEGER;
  BEGIN
    vecBlkId := NIL;
    vecBase  := NIL;
    vecTide  := NIL;
    FOR idx := 0 TO Ty.anyPtr DO
      vecElms[idx] := NIL;
      vecTypes[idx] := NIL;
      vecExpnd[idx] := NIL;
    END;
  END InitVectorDescriptors;

(* ============================================================ *)

  PROCEDURE SetNativeNames*();
    VAR sRec, oRec : Ty.Record;
  BEGIN
    xhrIx := 0;
    oRec := CSt.ntvObj.boundRecTp()(Ty.Record);
    sRec := CSt.ntvStr.boundRecTp()(Ty.Record);

    InitVectorDescriptors();
(*
 *  From release 1.2, only the RTM version is supported
 *)
    INCL(oRec.xAttr, Sy.spshl);
    INCL(sRec.xAttr, Sy.spshl);
    oRec.xName := Lv.strToCharOpen("object");
    sRec.xName := Lv.strToCharOpen("string");
    oRec.scopeNm := oRec.xName;
    sRec.scopeNm := sRec.xName;
    pVarSuffix   := Lv.strToCharOpen(".ctor($O, native int) ");

    CSt.ntvObj.xName := oRec.scopeNm;
    CSt.ntvStr.xName := sRec.scopeNm;

  END SetNativeNames;

(* ============================================================ *)
(* ============================================================ *)
BEGIN
  Lv.InitCharOpenSeq(nmArray, 8);

  rtsS := Lv.strToCharOpen("RTS"); 
  brks := Lv.strToCharOpen("[]"); 
  dotS := Lv.strToCharOpen("."); 
  cmma := Lv.strToCharOpen(","); 
  lPar := Lv.strToCharOpen("("); 
  rPar := Lv.strToCharOpen(")"); 
  lBrk := Lv.strToCharOpen("["); 
  rBrk := Lv.strToCharOpen("]"); 
  atSg := Lv.strToCharOpen("@"); 
  rfMk := Lv.strToCharOpen("&"); 
  vFld := Lv.strToCharOpen("v$"); 
  ouMk := Lv.strToCharOpen("[out] ");
  prev := Lv.strToCharOpen("prev"); 
  body := Lv.strToCharOpen("$static"); 
  xhrDl := Lv.strToCharOpen("XHR@"); 
  xhrMk := Lv.strToCharOpen("class [RTS]XHR"); 
  boxedObj := Lv.strToCharOpen("Boxed_"); 
  corlibAsm := Lv.strToCharOpen("[mscorlib]System."); 

  vecPrefix := Lv.strToCharOpen("[RTS]Vectors."); 
  evtAdd := Lv.strToCharOpen("add_"); 
  evtRem := Lv.strToCharOpen("remove_"); 

  Bi.setTp.xName  := Lv.strToCharOpen("int32");
  Bi.boolTp.xName := Lv.strToCharOpen("bool");
  Bi.byteTp.xName := Lv.strToCharOpen("int8");
  Bi.uBytTp.xName := Lv.strToCharOpen("unsigned int8");
  Bi.charTp.xName := Lv.strToCharOpen("wchar");
  Bi.sChrTp.xName := Lv.strToCharOpen("char");
  Bi.sIntTp.xName := Lv.strToCharOpen("int16");
  Bi.lIntTp.xName := Lv.strToCharOpen("int64");
  Bi.realTp.xName := Lv.strToCharOpen("float64");
  Bi.sReaTp.xName := Lv.strToCharOpen("float32");
  Bi.intTp.xName  := Bi.setTp.xName;
  Bi.anyRec.xName := Lv.strToCharOpen("class System.Object");
  Bi.anyPtr.xName := Bi.anyRec.xName;

  typeGetE[ Ty.boolN] := Asm.opc_ldelem_i1;
(*
 * typeGetE[ Ty.sChrN] := Asm.opc_ldelem_u1;
 *)
  typeGetE[ Ty.sChrN] := Asm.opc_ldelem_u2;
  typeGetE[ Ty.charN] := Asm.opc_ldelem_u2;
  typeGetE[ Ty.byteN] := Asm.opc_ldelem_i1;
  typeGetE[ Ty.uBytN] := Asm.opc_ldelem_u1;
  typeGetE[ Ty.sIntN] := Asm.opc_ldelem_i2;
  typeGetE[  Ty.intN] := Asm.opc_ldelem_i4;
  typeGetE[ Ty.lIntN] := Asm.opc_ldelem_i8;
  typeGetE[ Ty.sReaN] := Asm.opc_ldelem_r4;
  typeGetE[ Ty.realN] := Asm.opc_ldelem_r8;
  typeGetE[  Ty.setN] := Asm.opc_ldelem_i4;
  typeGetE[Ty.anyPtr] := Asm.opc_ldelem_ref;
  typeGetE[Ty.anyRec] := Asm.opc_ldelem_ref;

  typePutE[ Ty.boolN] := Asm.opc_stelem_i1;
(*
 * typePutE[ Ty.sChrN] := Asm.opc_stelem_i1;
 *)
  typePutE[ Ty.sChrN] := Asm.opc_stelem_i2;
  typePutE[ Ty.charN] := Asm.opc_stelem_i2;
  typePutE[ Ty.byteN] := Asm.opc_stelem_i1;
  typePutE[ Ty.uBytN] := Asm.opc_stelem_i1;
  typePutE[ Ty.sIntN] := Asm.opc_stelem_i2;
  typePutE[  Ty.intN] := Asm.opc_stelem_i4;
  typePutE[ Ty.lIntN] := Asm.opc_stelem_i8;
  typePutE[ Ty.sReaN] := Asm.opc_stelem_r4;
  typePutE[ Ty.realN] := Asm.opc_stelem_r8;
  typePutE[  Ty.setN] := Asm.opc_stelem_i4;
  typePutE[Ty.anyPtr] := Asm.opc_stelem_ref;
  typePutE[Ty.anyRec] := Asm.opc_stelem_ref;

  typeLdInd[ Ty.boolN] := Asm.opc_ldind_u1;
  typeLdInd[ Ty.sChrN] := Asm.opc_ldind_u2;
  typeLdInd[ Ty.charN] := Asm.opc_ldind_u2;
  typeLdInd[ Ty.byteN] := Asm.opc_ldind_i1;
  typeLdInd[ Ty.uBytN] := Asm.opc_ldind_u1;
  typeLdInd[ Ty.sIntN] := Asm.opc_ldind_i2;
  typeLdInd[  Ty.intN] := Asm.opc_ldind_i4;
  typeLdInd[ Ty.lIntN] := Asm.opc_ldind_i8;
  typeLdInd[ Ty.sReaN] := Asm.opc_ldind_r4;
  typeLdInd[ Ty.realN] := Asm.opc_ldind_r8;
  typeLdInd[  Ty.setN] := Asm.opc_ldind_i4;
  typeLdInd[Ty.anyPtr] := Asm.opc_ldind_ref;
  typeLdInd[Ty.anyRec] := Asm.opc_ldind_ref;

  typeStInd[ Ty.boolN] := Asm.opc_stind_i1;
  typeStInd[ Ty.sChrN] := Asm.opc_stind_i2;
  typeStInd[ Ty.charN] := Asm.opc_stind_i2;
  typeStInd[ Ty.byteN] := Asm.opc_stind_i1;
  typeStInd[ Ty.uBytN] := Asm.opc_stind_i1;
  typeStInd[ Ty.sIntN] := Asm.opc_stind_i2;
  typeStInd[  Ty.intN] := Asm.opc_stind_i4;
  typeStInd[ Ty.lIntN] := Asm.opc_stind_i8;
  typeStInd[ Ty.sReaN] := Asm.opc_stind_r4;
  typeStInd[ Ty.realN] := Asm.opc_stind_r8;
  typeStInd[  Ty.setN] := Asm.opc_stind_i4;
  typeStInd[Ty.anyPtr] := Asm.opc_stind_ref;
  typeStInd[Ty.anyRec] := Asm.opc_stind_ref;

(* ============================================================ *)
END MsilUtil.
(* ============================================================ *)
