
(* ============================================================ *)
(*  JavaUtil is the module which writes java classs file        *)
(*  structures                                                  *)
(*  Copyright (c) John Gough 1999, 2000.                        *)
(*  Modified DWC September, 2000.                               *)
(* ============================================================ *)

MODULE JavaUtil;

  IMPORT 
	GPCPcopyright,
	RTS,
	Console,
	JavaBase,
	Hsh := NameHash,
	Cst := CompState,
	Psr := CPascalP,
	Jvm := JVMcodes,
	Sym := Symbols,
	Blt := Builtin,
	Id  := IdDesc,
	Xp  := ExprDesc,
	Ty  := TypeDesc,
	L   := LitValue;

(* ============================================================ *)

  CONST
       initStr* = "<init>";
       classPrefix* = "CP";
       retMarker* = -1;   (* ==> out param is func-return *)
       StrCmp* = 1;       (* indexes for rts procs *)
       StrToChrOpen* = 2;
       StrToChrs* = 3;
       ChrsToStr* = 4;
       StrCheck* = 5;
       StrLen* = 6;
       ToUpper* = 7;
       DFloor* = 8;
       ModI* = 9;
       ModL* = 10;
       DivI* = 11;
       DivL* = 12;
       StrCatAA* = 13; 
       StrCatSA* = 14; 
       StrCatAS* = 15; 
       StrCatSS* = 16; 
       StrLP1* = 17; 
       StrVal* = 18;
       SysExit* = 19;
       LoadTp1* = 20;	(* getClassByOrd  *)
       LoadTp2* = 21;	(* getClassByName *)
       GetTpM* = 22;

(* ============================================================ *)

  TYPE JavaFile* = POINTER TO ABSTRACT RECORD
		     theP* : Id.Procs;
                   END;

(* ============================================================ *)

  TYPE Label* = POINTER TO RECORD
                  defIx* : INTEGER;
                END;

(* ============================================================ *)

  VAR
	typeRetn-  : ARRAY 16 OF INTEGER;
	typeLoad-  : ARRAY 16 OF INTEGER;
	typeStore- : ARRAY 16 OF INTEGER;
	typePutE-  : ARRAY 16 OF INTEGER;
	typeGetE-  : ARRAY 16 OF INTEGER;

  VAR	nmArray*   : L.CharOpenSeq;
        fmArray*   : L.CharOpenSeq;

  VAR   semi-,comma-,colon-,lPar-,rPar-,rParV-,
        brac-,lCap-, void-,lowL-,dlar-,slsh-,prfx- : L.CharOpen;

(* ============================================================ *)

  VAR   xhrIx : INTEGER;
        xhrDl : L.CharOpen;
        xhrMk : L.CharOpen;

  VAR   invokeHash : INTEGER;
		ptvIx      : INTEGER;    (* Index number for procedure type literals *)
        procLitPrefix : L.CharOpen;

(* ============================================================ *)

  VAR   vecBlkId  : Id.BlkId;
        vecBase   : Id.TypId;
        vecTypes  : ARRAY Ty.anyPtr+1 OF Id.TypId;
        vecTide   : Id.FldId;
        vecElms   : ARRAY Ty.anyPtr+1 OF Id.FldId;
        vecExpnd  : ARRAY Ty.anyPtr+1 OF Id.MthId;

(* ============================================================ *)

  PROCEDURE (jf : JavaFile)StartModClass*(mod : Id.BlkId),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)StartRecClass*(rec : Ty.Record),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)StartProc*(proc : Id.Procs),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)EndProc*(),NEW,EMPTY;
  PROCEDURE (jf : JavaFile)isAbstract*():BOOLEAN,NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)getScope*():Sym.Scope,NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile) EmitField*(field : Id.AbVar),NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)MkNewRecord*(typ : Ty.Record),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)MkNewFixedArray*(topE : Sym.Type;
                                            len0 : INTEGER),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)MkNewOpenArray*(arrT : Ty.Array;
                                           dims : INTEGER),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)MkArrayCopy*(arrT : Ty.Array),NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)newLocal*() : INTEGER,NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)ReleaseLocal*(i : INTEGER),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)ReleaseAll*(m : INTEGER),NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)markTop*() : INTEGER,NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)getDepth*() : INTEGER,NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)setDepth*(i : INTEGER),NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)newLabel*() : Label,NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)getLabelRange*(VAR labs:ARRAY OF Label),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)AddSwitchLab*(lab : Label; 
                                         pos : INTEGER),NEW,ABSTRACT;


  PROCEDURE (jf : JavaFile)Comment*(IN msg : ARRAY OF CHAR),NEW,EMPTY;
  PROCEDURE (jf : JavaFile)Header*(IN str : ARRAY OF CHAR),NEW,EMPTY;

  PROCEDURE (jf : JavaFile)Code*(code : INTEGER),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CodeI*(code,val : INTEGER),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CodeL*(code : INTEGER; num : LONGINT),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CodeC*(code : INTEGER; 
                                  IN str : ARRAY OF CHAR),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CodeR*(code : INTEGER; 
                                  num : REAL; short : BOOLEAN),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CodeLb*(code : INTEGER; lab : Label),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)LstDef*(l : Label),NEW,EMPTY;
  PROCEDURE (jf : JavaFile)DefLab*(lab : Label),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)DefLabC*(lab : Label; 
                                    IN c : ARRAY OF CHAR),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CodeInc*(localIx,incVal : INTEGER),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CodeT*(code : INTEGER; ty : Sym.Type),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CodeSwitch*(low,high : INTEGER; 
                                       defLab : Label),NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)PushStr*(IN str : L.CharOpen),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)LoadConst*(num : INTEGER),NEW,ABSTRACT;


  PROCEDURE (jf : JavaFile)CallGetClass*(),NEW,ABSTRACT; 
  PROCEDURE (jf : JavaFile)CallRTS*(ix,args,ret : INTEGER),NEW,ABSTRACT; 
  PROCEDURE (jf : JavaFile)CallIT*(code : INTEGER; 
				   proc : Id.Procs; 
				   type : Ty.Procedure),NEW,ABSTRACT;


  PROCEDURE (jf : JavaFile)ClinitHead*(),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)MainHead*(),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)VoidTail*(),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)ModNoArgInit*(),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)RecMakeInit*(rec : Ty.Record;
					prc : Id.PrcId),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CallSuperCtor*(rec : Ty.Record;
                                          pTy : Ty.Procedure),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CopyProcHead*(rec : Ty.Record),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)ValRecCopy*(typ : Ty.Record),NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)InitFields*(num : INTEGER),NEW,EMPTY;
  PROCEDURE (jf : JavaFile)InitMethods*(num : INTEGER),NEW,EMPTY;

  PROCEDURE (jf : JavaFile)Try*(),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)Catch*(prc : Id.Procs),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)MkNewException*(),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)InitException*(),NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)Dump*(),NEW,ABSTRACT;

(* ============================================================ *)

  PROCEDURE (jf : JavaFile)PutGetS*(code : INTEGER; (* static field *)
				    blk  : Id.BlkId;
				    fld  : Id.VarId),NEW,ABSTRACT;

  PROCEDURE (jf : JavaFile)PutGetF*(code : INTEGER; (* instance field *)
				    rec  : Ty.Record;
				    fld  : Id.AbVar),NEW,ABSTRACT;

(* ============================================================ *)

  PROCEDURE (jf : JavaFile)Alloc1d*(elTp : Sym.Type),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)VarInit*(var : Sym.Idnt),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)Trap*(IN str : ARRAY OF CHAR),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)CaseTrap*(i : INTEGER),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)WithTrap*(id : Sym.Idnt),NEW,ABSTRACT;
  PROCEDURE (jf : JavaFile)Line*(nm : INTEGER),NEW,ABSTRACT;

(* ============================================================ *)
(*                      Some XHR utilities                      *)
(* ============================================================ *)

  PROCEDURE^ (jf : JavaFile)PutUplevel*(var : Id.LocId),NEW;
  PROCEDURE^ (jf : JavaFile)GetUplevel*(var : Id.LocId),NEW;
  PROCEDURE^ (jf : JavaFile)PushInt*(num : INTEGER),NEW;
  PROCEDURE^ (jf : JavaFile)PutElement*(typ : Sym.Type),NEW;
  PROCEDURE^ (jf : JavaFile)GetElement*(typ : Sym.Type),NEW;
  PROCEDURE^ (jf : JavaFile)ConvertDn*(inT, outT : Sym.Type),NEW;

  PROCEDURE^ cat2*(i,j : L.CharOpen) : L.CharOpen;
  PROCEDURE^ MkRecName*(typ : Ty.Record);
  PROCEDURE^ MkProcName*(proc : Id.Procs);
  PROCEDURE^ NumberParams(pIdn : Id.Procs; pTyp : Ty.Procedure);
  PROCEDURE^ typeToChOpen(typ : Sym.Type) : L.CharOpen;


(* ============================================================ *)

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

  PROCEDURE newXHR() : L.CharOpen;
  BEGIN
    INC(xhrIx);
    RETURN cat2(xhrDl, L.intToCharOpen(xhrIx));
  END newXHR;

  PROCEDURE MkXHR(scp : Id.Procs);
    VAR typId : Id.TypId;
	recTp : Ty.Record;
	index : INTEGER;
	locVr : Id.LocId;
	fldVr : Id.FldId;
  BEGIN
    Blt.MkDummyClass(newXHR(), Cst.thisMod, Ty.noAtt, typId);
    typId.SetMode(Sym.prvMode);
    scp.xhrType := typId.type;
    recTp := typId.type.boundRecTp()(Ty.Record);
    recTp.baseTp := Cst.rtsXHR.boundRecTp();
    INCL(recTp.xAttr, Sym.noCpy);

    FOR index := 0 TO scp.locals.tide-1 DO
      locVr := scp.locals.a[index](Id.LocId);
      IF Id.uplevA IN locVr.locAtt THEN
        fldVr := Id.newFldId();
        fldVr.hash := locVr.hash;
        fldVr.type := locVr.type;
        fldVr.recTyp := recTp;
	Sym.AppendIdnt(recTp.fields, fldVr);
      END;
    END;
  END MkXHR;

(* ============================================================ *)
(*			Some vector utilities			*)
(* ============================================================ *)

  PROCEDURE mapVecElTp(typ : Sym.Type) : INTEGER;
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

  PROCEDURE mapOrdRepT(ord : INTEGER) : Sym.Type;
  BEGIN
    CASE ord OF
    | Ty.charN  : RETURN Blt.charTp;
    | Ty.intN   : RETURN Blt.intTp;
    | Ty.lIntN  : RETURN Blt.lIntTp;
    | Ty.sReaN  : RETURN Blt.sReaTp;
    | Ty.realN  : RETURN Blt.realTp;
    | Ty.anyPtr : RETURN Blt.anyPtr;
    END;
  END mapOrdRepT;

(* ------------------------------------------------------------ *)

  PROCEDURE InitVecDescriptors;
    VAR i : INTEGER;
  BEGIN
    vecBlkId := NIL;
    vecBase  := NIL;
    vecTide  := NIL;
    FOR i := 0 TO Ty.anyPtr DO
      vecTypes[i] := NIL;
      vecElms[i]  := NIL;
      vecExpnd[i] := NIL;
    END;
  END InitVecDescriptors;

  PROCEDURE vecModId() : Id.BlkId;
  BEGIN
    IF vecBlkId = NIL THEN
      Blt.MkDummyImport("$CPJvec$", "CP.CPJvec", vecBlkId);
      Blt.MkDummyClass("VecBase", vecBlkId, Ty.noAtt, vecBase);
     (*
      *  Initialize vecTide while we are at it ...
      *)
      vecTide := Id.newFldId();
      vecTide.hash := Hsh.enterStr("tide");
      vecTide.dfScp := vecBlkId;
      vecTide.recTyp := vecBase.type.boundRecTp();
      vecTide.type := Blt.intTp;
      MkRecName(vecTide.recTyp(Ty.Record)); 
    END;
    RETURN vecBlkId;
  END vecModId;

  PROCEDURE vecClsTyId(ord : INTEGER) : Id.TypId;
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
      Blt.MkDummyClass(str, vecModId(), Ty.noAtt, tId);
      rcT := tId.type.boundRecTp()(Ty.Record);
      rcT.baseTp := vecTide.recTyp;
      vecTypes[ord] := tId;
    END;
    RETURN vecTypes[ord];
  END vecClsTyId;

  PROCEDURE vecRecTyp(ord : INTEGER) : Ty.Record;
  BEGIN
    RETURN vecClsTyId(ord).type.boundRecTp()(Ty.Record);
  END vecRecTyp;

  PROCEDURE vecArrFlId(ord : INTEGER) : Id.FldId;
    VAR fld : Id.FldId;
  BEGIN
    IF vecElms[ord] = NIL THEN
      fld := Id.newFldId();
      fld.hash := Hsh.enterStr("elms");
      fld.dfScp := vecModId();
      fld.recTyp := vecRecTyp(ord);
      fld.type := Ty.mkArrayOf(mapOrdRepT(ord));
      vecElms[ord] := fld;
    END;
    RETURN vecElms[ord];
  END vecArrFlId;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)MkVecRec*(eTp : Sym.Type),NEW;
    VAR ord : INTEGER;
  BEGIN
    ord := mapVecElTp(eTp);
    jf.MkNewRecord(vecRecTyp(ord));
  END MkVecRec;

(* ------------------------------- *)

  PROCEDURE (jf : JavaFile)MkVecArr*(eTp : Sym.Type),NEW;
    VAR ord : INTEGER;
        vTp : Sym.Type;
  BEGIN
    ord := mapVecElTp(eTp);
    jf.Alloc1d(mapOrdRepT(ord));
    jf.PutGetF(Jvm.opc_putfield, vecRecTyp(ord), vecArrFlId(ord));
  END MkVecArr;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)GetVecArr*(eTp : Sym.Type),NEW;
    VAR ord : INTEGER;
        fId : Id.FldId;
  BEGIN
    ord := mapVecElTp(eTp);
    fId := vecArrFlId(ord);
    jf.PutGetF(Jvm.opc_getfield, fId.recTyp(Ty.Record), fId);
  END GetVecArr;

(* ------------------------------- *)

  PROCEDURE (jf : JavaFile)GetVecLen*(),NEW;
  BEGIN
    jf.PutGetF(Jvm.opc_getfield, vecTide.recTyp(Ty.Record), vecTide);
  END GetVecLen;

(* ------------------------------- *)

  PROCEDURE (jf : JavaFile)PutVecLen*(),NEW;
  BEGIN
    jf.PutGetF(Jvm.opc_putfield, vecTide.recTyp(Ty.Record), vecTide);
  END PutVecLen;

(* ------------------------------- *)

  PROCEDURE (jf : JavaFile)InvokeExpand*(eTp : Sym.Type),NEW;
    VAR ord : INTEGER;
        mth : Id.MthId;
        typ : Ty.Procedure;
  BEGIN
    ord := mapVecElTp(eTp);
    IF vecExpnd[ord] = NIL THEN
      mth := Id.newMthId();
      mth.hash := Blt.xpndBk;
      mth.dfScp := vecModId();
      typ := Ty.newPrcTp();
      typ.idnt := mth;
      typ.receiver := vecClsTyId(ord).type;
      mth.bndType  := typ.receiver.boundRecTp();
      MkProcName(mth);
      NumberParams(mth, typ);
      mth.type := typ;
      vecExpnd[ord] := mth;
    ELSE
      mth := vecExpnd[ord];
      typ := mth.type(Ty.Procedure);
    END;
    jf.CallIT(Jvm.opc_invokevirtual, mth, typ);
  END InvokeExpand;

(* ------------------------------- *)

  PROCEDURE (jf : JavaFile)PutVecElement*(eTp : Sym.Type),NEW;
  BEGIN
    jf.PutElement(mapOrdRepT(mapVecElTp(eTp)));
  END PutVecElement;

(* ------------------------------- *)

  PROCEDURE (jf : JavaFile)GetVecElement*(eTp : Sym.Type),NEW;
    VAR rTp : Sym.Type; (* representation type *)
  BEGIN
    rTp := mapOrdRepT(mapVecElTp(eTp));
   (*
    *  If rTp and eTp are not equal, then must restore erased type
    *)
    jf.GetElement(rTp);
    IF rTp # eTp THEN
      IF rTp = Blt.anyPtr THEN
        jf.CodeT(Jvm.opc_checkcast, eTp);
      ELSE
        jf.ConvertDn(rTp, eTp);
      END;
    END;
  END GetVecElement;

(* ============================================================ *)
(*                     Some static utilities                    *)
(* ============================================================ *)

  PROCEDURE jvmSize*(t : Sym.Type) : INTEGER;
  BEGIN
    IF t.isLongType() THEN RETURN 2 ELSE RETURN 1 END;
  END jvmSize;

(* ------------------------------------------------------------ *)

  PROCEDURE newAnonLit() : L.CharOpen;
  BEGIN
    INC(ptvIx);
    RETURN cat2(procLitPrefix, L.intToCharOpen(ptvIx));
  END newAnonLit;

(* ------------------------------------------------------------ *)

  PROCEDURE needsBox*(i : Id.ParId) : BOOLEAN;
  (*   A parameter needs to be boxed if it has non-reference 	*)
  (*   representation in the JVM, and is OUT or VAR mode.	*)
  BEGIN
    RETURN ((i.parMod = Sym.var) OR (i.parMod = Sym.out)) & 
	   i.type.isScalarType();
  END needsBox;

(* ============================================================ *)

  PROCEDURE cat2*(i,j : L.CharOpen) : L.CharOpen;
  BEGIN
    L.ResetCharOpenSeq(nmArray);
    L.AppendCharOpen(nmArray, i);
    L.AppendCharOpen(nmArray, j);
    RETURN L.arrayCat(nmArray); 
  END cat2;

  PROCEDURE cat3*(i,j,k : L.CharOpen) : L.CharOpen;
  BEGIN
    L.ResetCharOpenSeq(nmArray);
    L.AppendCharOpen(nmArray, i);
    L.AppendCharOpen(nmArray, j);
    L.AppendCharOpen(nmArray, k);
    RETURN L.arrayCat(nmArray);
  END cat3;

(* ------------------------------------------------------------ *)

  PROCEDURE MkBlkName*(mod : Id.BlkId);
    VAR mNm : L.CharOpen;
  (* -------------------------------------------------- *)
    PROCEDURE dotToSlash(arr : L.CharOpen) : L.CharOpen;
      VAR ix : INTEGER;
    BEGIN
      FOR ix := 0 TO LEN(arr)-1 DO
	IF arr[ix] = "." THEN arr[ix] := "/" END;
      END;
      RETURN arr;
    END dotToSlash;
  (* -------------------------------------------------- *)
  BEGIN
    IF mod.xName # NIL THEN RETURN END;
    mNm := Sym.getName.ChPtr(mod);
    IF mod.scopeNm # NIL THEN 
      mod.scopeNm := dotToSlash(mod.scopeNm);
    ELSE
      mod.scopeNm := cat3(prfx, slsh, mNm); (* "CP/<modname>" *)
    END;
    IF ~Cst.doCode         (* Only doing Jasmin output       *)
        OR Cst.doJsmn      (* Forcing assembly via Jasmin    *)
        OR (mod.scopeNm[0] = 0X) (* Explicitly forcing no package! *) THEN
      mod.xName   := mNm;
    ELSE (* default case *)
      mod.xName := cat3(mod.scopeNm, slsh, mNm);
    END;
  END MkBlkName;

(* ------------------------------------------------------------ *)

  PROCEDURE scopeName(scp : Sym.Scope) : L.CharOpen;
  BEGIN
    WITH scp : Id.BlkId DO
        IF scp.xName = NIL THEN MkBlkName(scp) END;
        IF Cst.doCode & ~Cst.doJsmn THEN
          RETURN Sym.getName.ChPtr(scp);
        ELSE
          RETURN scp.xName; 
        END;
    | scp : Id.Procs DO
        IF scp.prcNm = NIL THEN MkProcName(scp) END;
	RETURN scp.prcNm;
    END;
  END scopeName;

(* ------------------------------------------------------------ *)

  PROCEDURE qualScopeName(scp : Sym.Scope) : L.CharOpen;
  BEGIN
    WITH scp : Id.BlkId DO
      IF scp.xName = NIL THEN MkBlkName(scp) END;
      RETURN scp.scopeNm;
    | scp : Id.Procs DO
      IF scp.prcNm = NIL THEN MkProcName(scp) END;
      RETURN scp.scopeNm;
    END;
  END qualScopeName;

(* ------------------------------------------------------------ *)
  PROCEDURE newMthId*(IN name : ARRAY OF CHAR; dfScp : Id.BlkId; bndTp : Sym.Type) : Id.MthId;
    VAR rslt : Id.MthId;
  BEGIN
    rslt := Id.newMthId();
	rslt.SetKind(Id.conMth);
	rslt.hash := Hsh.enterStr(name);
	rslt.dfScp := dfScp;
	rslt.bndType := bndTp;
	rslt.rcvFrm := Id.newParId();
	rslt.rcvFrm.type := bndTp;
	IF bndTp IS Ty.Record THEN rslt.rcvFrm.parMod := Sym.var END; 
    RETURN rslt;
  END newMthId;

(* ------------------------------------------------------------ *)
(* Generate all naming strings for this record type, and put    *)
(* a corresponding emitter record on the work list.             *)
(* ------------------------------------------------------------ *)
  PROCEDURE MkRecName*(typ : Ty.Record);
    VAR mNm : L.CharOpen;
	qNm : L.CharOpen;
	rNm : L.CharOpen;
	tId : Sym.Idnt;
  BEGIN
   (* ###################################### *)
    IF typ.xName # NIL THEN RETURN END;
   (* ###################################### *)
    IF typ.bindTp # NIL THEN		(* Synthetically named rec'd *)
      tId := typ.bindTp.idnt;
    ELSE				(* Normal, named record type *)
      IF typ.idnt = NIL THEN		(* Anonymous record type     *)
        typ.idnt := Id.newAnonId(typ.serial);
      END;
      tId := typ.idnt;
    END;
    IF tId.dfScp = NIL THEN tId.dfScp := Cst.thisMod END;
    rNm := Sym.getName.ChPtr(tId);
    mNm := scopeName(tId.dfScp);
    qNm := qualScopeName(tId.dfScp);
   (*
    *  At this point:
    *	    rNm holds the simple record name
    *	    mNm holds the qualifying module name
    *	    qNm holds the qualifying scope name
    *	    If extrnNm = NIL, the default mangling is used.
    *  At exit we want:
    *	    xName to hold the fully qualified name
    *	    extrnNm to hold the simple name
    *	    scopeNm to hold the "L<qualid>;" name
    *)
    IF typ.extrnNm # NIL THEN
      typ.extrnNm := rNm;
    ELSE
      typ.extrnNm := cat3(mNm, lowL, rNm);
    END;
    IF qNm[0] # 0X THEN
      typ.xName := cat3(qNm, slsh, typ.extrnNm);
    ELSE
      typ.xName := typ.extrnNm;
    END;
    typ.scopeNm := cat3(lCap, typ.xName, semi);
   (*
    *   It is at this point that we link records into the
    *   class-emission worklist.
    *)
    IF tId.dfScp.kind # Id.impId THEN
      JavaBase.worklist.AddNewRecEmitter(typ);
    END;
  END MkRecName;

(* ============================================================ *)
(*               Some Procedure Variable utilities              *)
(* ============================================================ *)

  PROCEDURE getProcWrapperInvoke*(typ : Ty.Record) : Id.MthId;
    VAR idnt : Sym.Idnt;
  BEGIN 
   (*  We could get the method descriptor more cheaply by
    *  indexing into the symbol table, but this would be
	*  very fragile against future code changes.
	*)
    idnt := typ.symTb.lookup(invokeHash);
	RETURN idnt(Id.MthId);
  END getProcWrapperInvoke;

  PROCEDURE getProcVarInvoke*(typ : Ty.Procedure) : Id.MthId;
  BEGIN
    IF (typ = NIL) OR (typ.hostClass = NIL) THEN RETURN NIL;
	ELSE RETURN getProcWrapperInvoke(typ.hostClass);
	END;
  END getProcVarInvoke;

(* ------------------------------------------------------------ *)

 (*
  *  Copy the formals from the template procedure type descriptor
  *  to the type descriptor for the method 'scp'.  Change the 
  *  dfScp of the params (and receiver) to be local to scp.
  *  Also, in the case of methods imported without parameter
  *  names, generate synthetic names for the formals.
  *)
  PROCEDURE RescopeFormals(template : Ty.Procedure; scp : Id.MthId);
	VAR param : Id.ParId;
	    index : INTEGER;
		synthH : INTEGER;
		newTyp : Ty.Procedure;
  BEGIN
	newTyp := scp.type(Ty.Procedure);
	newTyp.retType := template.retType;
	FOR index := 0 TO template.formals.tide -1 DO
	  param := Id.cloneParInScope(template.formals.a[index], scp);
	  IF param.hash = 0 THEN 
	    synthH := Hsh.enterStr("p" + L.intToCharOpen(index)^);
		template.formals.a[index].hash := synthH;
		param.hash := synthH;
	  END;
      IF ~Sym.refused(param, scp) THEN
	    Id.AppendParam(newTyp.formals, param);
	    Sym.AppendIdnt(scp.locals, param);
      END;
	END;
  END RescopeFormals;

(* ------------------------------------------------------------ *)
(* Generate all naming strings for this procedure type, and     *)
(* put a corresponding emitter record on the work list.         *)
(* ------------------------------------------------------------ *)
  PROCEDURE MkProcTypeName*(typ : Ty.Procedure);
    VAR tIdent : Sym.Idnt;
	    hostTp : Ty.Record;
		(*invoke : Id.MthId;*)
	    rNm, mNm, qNm : L.CharOpen;
  BEGIN
   (* ###################################### *)
    IF typ.xName # NIL THEN RETURN END;
   (* ###################################### *)
    IF typ.idnt = NIL THEN              (* Anonymous procedure type *)
      typ.idnt := Id.newAnonId(typ.serial);
      typ.idnt.type := typ;
    END;
	tIdent := typ.idnt;
    IF tIdent.dfScp = NIL THEN tIdent.dfScp := Cst.thisMod END;
	NEW(hostTp);
	rNm := Sym.getName.ChPtr(tIdent);
	mNm := scopeName(tIdent.dfScp);
	qNm := qualScopeName(tIdent.dfScp);
   (*
    *  At this point:
    *	    rNm holds the simple record name
    *	    mNm holds the qualifying module name
    *	    qNm holds the qualifying scope name
    *  At exit we want:
    *	    xName to hold the fully qualified name
    *)
	hostTp.extrnNm := cat3(mNm, lowL, rNm);
    hostTp.xName := cat3(qNm, slsh, hostTp.extrnNm);
    hostTp.scopeNm := cat3(lCap, hostTp.xName, semi);
	typ.hostClass := hostTp;
	Blt.MkDummyMethodAndInsert("Invoke", Ty.newPrcTp(), hostTp, Cst.thisMod, Sym.pubMode, Sym.var, Id.isAbs);
	RescopeFormals(typ, getProcVarInvoke(typ));
	typ.xName := hostTp.xName;
   (*
    *   It is at this point that we link records into the
    *   class-emission worklist.
    *)
	IF tIdent.dfScp.kind # Id.impId THEN
	  JavaBase.worklist.AddNewProcTypeEmitter(typ);
	END;
  END MkProcTypeName;

(* ------------------------------------------------------------ *)
(*  Generate the body statement sequence for the proc-type      *)
(*  wrapper class to invoke the encapsulated procedure literal. *) 
(* ------------------------------------------------------------ *)
  PROCEDURE procLitBodyStatement(targetId : Sym.Idnt; thisMth : Id.MthId) : Sym.Stmt;
    VAR text  : L.CharOpenSeq;
	    mthTp : Ty.Procedure;
	    param : Id.ParId;
	    index : INTEGER;
   (* ###################################### *)
	PROCEDURE textName(trgt : Sym.Idnt) : L.CharOpen;
	  VAR simple : L.CharOpen;
	BEGIN
	  simple := trgt.name();
	  IF trgt.dfScp = Cst.thisMod THEN
	    RETURN simple;
	  ELSE
	    RETURN BOX(trgt.dfScp.name()^ + '.' + simple^);
	  END;
	END textName;
   (* ###################################### *)
 BEGIN
    mthTp := thisMth.type(Ty.Procedure);
    IF mthTp.retType # NIL THEN L.AppendCharOpen(text, BOX("RETURN ")) END;
	L.AppendCharOpen(text, textName(targetId));
	L.AppendCharOpen(text, lPar);
    FOR index := 0 TO mthTp.formals.tide - 1 DO
	  IF index # 0 THEN L.AppendCharOpen(text, comma) END;
	  param := mthTp.formals.a[index];
	  L.AppendCharOpen(text, param.name());
	END;
	L.AppendCharOpen(text, rPar);
	L.AppendCharOpen(text, BOX("END"));
	RETURN Psr.parseTextAsStatement(text.a, thisMth);
  END procLitBodyStatement;

(* ------------------------------------------------------------ *)
(*  Every value of procedure type is represented by a singleton *) 
(*  class derived from the abstract host type of the proc-type. *)
(* ------------------------------------------------------------ *)
  PROCEDURE newProcLitWrapperClass(exp : Sym.Expr; typ : Ty.Procedure) : Ty.Record;
    VAR singleton : Id.TypId;
	    hostClass : Ty.Record;
		newInvoke : Id.MthId;
  BEGIN
    ASSERT(exp IS Xp.IdLeaf);
	Blt.MkDummyClass(newAnonLit(), Cst.thisMod, Ty.noAtt, singleton);
	hostClass := singleton.type.boundRecTp()(Ty.Record);
	Blt.MkDummyMethodAndInsert("Invoke", Ty.newPrcTp(), hostClass, Cst.thisMod, Sym.pubMode, Sym.var, {});
	MkRecName(hostClass); (* Add this class to the emission work-list *)
	newInvoke := getProcWrapperInvoke(hostClass);
	RescopeFormals(typ, newInvoke);
	newInvoke.body := procLitBodyStatement(exp(Xp.IdLeaf).ident, newInvoke);
	RETURN hostClass;
  END newProcLitWrapperClass;

(* ------------------------------------------------------------ *)
(* ------------------------------------------------------------ *)

  PROCEDURE MkVecName*(typ : Ty.Vector);
    VAR ord : INTEGER;
        rTp : Ty.Record;
  BEGIN
    ord := mapVecElTp(typ.elemTp);
    rTp := vecRecTyp(ord);
    IF rTp.xName = NIL THEN MkRecName(rTp) END;
    typ.xName := rTp.scopeNm;
  END MkVecName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkProcName*(proc : Id.Procs);
    VAR pNm : L.CharOpen;
	res : Id.Procs;
	scp : Sym.Scope;
	bTp : Ty.Record;
  (* -------------------------------------------------- *)
    PROCEDURE clsNmFromRec(typ : Sym.Type) : L.CharOpen;
    BEGIN
      IF Cst.doCode & ~Cst.doJsmn THEN
        RETURN typ(Ty.Record).xName;
      ELSE
        RETURN typ(Ty.Record).extrnNm;
      END;
    END clsNmFromRec;
  (* -------------------------------------------------- *)
    PROCEDURE className(p : Id.Procs) : L.CharOpen;
    BEGIN
      WITH p : Id.PrcId DO RETURN p.clsNm;
      |    p : Id.MthId DO RETURN clsNmFromRec(p.bndType);
      END;
    END className;
  (* -------------------------------------------------- *)
    PROCEDURE GetClassName(pr : Id.PrcId; bl : Id.BlkId);
      VAR nm : L.CharOpen;
    BEGIN
      nm := Sym.getName.ChPtr(pr);
      IF pr.bndType = NIL THEN	(* normal case  *)
        pr.clsNm := bl.xName;
        IF pr.prcNm = NIL THEN pr.prcNm := nm END;
      ELSE			(* static method *)
	IF pr.bndType.xName = NIL THEN MkRecName(pr.bndType(Ty.Record)) END;
	pr.clsNm := clsNmFromRec(pr.bndType);
        IF pr.prcNm = NIL THEN
          pr.prcNm := nm;
        ELSIF pr.prcNm^ = initStr THEN
          pr.SetKind(Id.ctorP);
        END;
      END;
    END GetClassName;
  (* -------------------------------------------------- *)
    PROCEDURE MkPrcNm(prc : Id.PrcId);
      VAR res : Id.PrcId;
	  scp : Sym.Scope;
	  blk : Id.BlkId;
	  rTp : Ty.Record;
    BEGIN
      IF prc.scopeNm # NIL THEN RETURN;
      ELSIF prc.kind = Id.fwdPrc THEN
        res := prc.resolve(Id.PrcId); MkPrcNm(res);
        prc.prcNm := res.prcNm;
        prc.clsNm := res.clsNm;
        prc.scopeNm := res.scopeNm;
      ELSIF prc.kind = Id.conPrc THEN
        scp := prc.dfScp;
        WITH scp : Id.BlkId DO
            IF scp.xName = NIL THEN MkBlkName(scp) END;
            IF  Sym.isFn IN scp.xAttr THEN
              GetClassName(prc, scp);
            ELSE
              prc.clsNm := scp.xName;
              IF prc.prcNm = NIL THEN prc.prcNm := Sym.getName.ChPtr(prc) END;
            END;
        | scp : Id.Procs DO
            MkProcName(scp);
            prc.clsNm := className(scp);
            prc.prcNm   := cat3(Sym.getName.ChPtr(prc), dlar, scp.prcNm);
        END;
        prc.scopeNm := scp.scopeNm;
      ELSE (* prc.kind = Id.ctorP *)
        blk := prc.dfScp(Id.BlkId);
	rTp := prc.type.returnType().boundRecTp()(Ty.Record);
        IF blk.xName = NIL THEN MkBlkName(blk) END;
        IF rTp.xName = NIL THEN MkRecName(rTp) END;
        prc.clsNm := clsNmFromRec(rTp);
        prc.prcNm := L.strToCharOpen(initStr);
        prc.scopeNm := blk.scopeNm;
      END;
    END MkPrcNm;
  (* -------------------------------------------------- *)
    PROCEDURE MkMthNm(mth : Id.MthId);
      VAR res : Id.MthId;
          scp : Id.BlkId;
          typ : Sym.Type;
    BEGIN
      IF mth.scopeNm # NIL THEN RETURN;
      ELSIF mth.kind = Id.fwdMth THEN
        res := mth.resolve(Id.MthId); MkMthNm(res);
        mth.prcNm := res.prcNm; mth.scopeNm := res.scopeNm;
      ELSE
        scp := mth.dfScp(Id.BlkId);
        typ := mth.bndType;
        IF typ.xName = NIL THEN MkRecName(typ(Ty.Record)) END;
        IF scp.xName = NIL THEN MkBlkName(scp) END;

        mth.scopeNm := scp.scopeNm;
        IF mth.prcNm = NIL THEN mth.prcNm := Sym.getName.ChPtr(mth) END;
      END;
    END MkMthNm;
  (* -------------------------------------------------- *)
  BEGIN (* MkProcName *)
    WITH proc : Id.MthId DO MkMthNm(proc);
    |    proc : Id.PrcId DO MkPrcNm(proc);
    END;
  END MkProcName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkAliasName*(typ : Ty.Opaque);
    VAR mNm : L.CharOpen;
	rNm : L.CharOpen;
	sNm : L.CharOpen;
  BEGIN
   (* 
    * This was almost certainly broken,  
    * at least for foreign explicit names 
    *)
    IF typ.xName # NIL THEN RETURN END;
    rNm := Sym.getName.ChPtr(typ.idnt);
   (* 
    * old code --
    *  mNm := scopeName(typ.idnt.dfScp);
    *  sNm := cat3(mNm, lowL, rNm);
    *  typ.xName   := cat3(qualScopeName(typ.idnt.dfScp), slsh, sNm);
    *
    * replaced by ...
    *)
    typ.xName   := cat3(qualScopeName(typ.idnt.dfScp), slsh, rNm);
   (* end *)
    typ.scopeNm := cat3(lCap, typ.xName, semi);
  END MkAliasName;

(* ------------------------------------------------------------ *)

  PROCEDURE MkVarName*(var : Id.VarId);
    VAR mod : Id.BlkId;
  BEGIN
    IF var.varNm # NIL THEN RETURN END;
    mod := var.dfScp(Id.BlkId);
    var.varNm := Sym.getName.ChPtr(var);
    IF var.recTyp = NIL THEN	(* normal case *)
      var.clsNm := mod.xName;
    ELSE			(* static field *)
      IF var.recTyp.xName = NIL THEN MkRecName(var.recTyp(Ty.Record)) END;
      var.clsNm := var.recTyp(Ty.Record).extrnNm;
    END;
  END MkVarName;

(* ------------------------------------------------------------ *)

  PROCEDURE NumberParams(pIdn : Id.Procs; pTyp : Ty.Procedure);
    VAR parId : Id.ParId;
	index : INTEGER;
	count : INTEGER;
	retTp : Sym.Type;
   (* ----------------------------------------- *)
    PROCEDURE AppendTypeName(VAR lst : L.CharOpenSeq; typ : Sym.Type);
    BEGIN
      WITH typ : Ty.Base DO
	  L.AppendCharOpen(lst, typ.xName);
      | typ : Ty.Vector DO
	  IF typ.xName = NIL THEN MkVecName(typ) END;
	  L.AppendCharOpen(lst, typ.xName);
      | typ : Ty.Array DO
	  L.AppendCharOpen(lst, brac);
	  AppendTypeName(lst, typ.elemTp);
      | typ : Ty.Record DO
	  IF typ.xName = NIL THEN MkRecName(typ) END;
	  L.AppendCharOpen(lst, typ.scopeNm);
      | typ : Ty.Enum DO
	  AppendTypeName(lst, Blt.intTp);
      | typ : Ty.Pointer DO
	  AppendTypeName(lst, typ.boundTp);
      | typ : Ty.Opaque DO
	  IF typ.xName = NIL THEN MkAliasName(typ) END;
	  L.AppendCharOpen(lst, typ.scopeNm);
      END;
    END AppendTypeName;
   (* ----------------------------------------- *)
  BEGIN
   (*
    *  The parameter numbering scheme tries to use the return
    *  value for the first OUT or VAR parameter.  The variable
    *  'hasRt' notes whether this possiblity has been used up. If
    *  this is a value returning function hasRt is true at entry.
    *)
    count := pIdn.rtsFram;
    retTp := pTyp.retType;
    IF pIdn.kind = Id.ctorP THEN 
      INC(count);
    ELSIF retTp # NIL THEN	(* and not a constructor... *)
      pTyp.retN := jvmSize(pTyp.retType);
    END;
    L.ResetCharOpenSeq(fmArray);
    L.AppendCharOpen(fmArray, lPar);
    IF pIdn.lxDepth > 0 THEN
      L.AppendCharOpen(fmArray, xhrMk); INC(count);
    END;
    FOR index := 0 TO pTyp.formals.tide-1 DO
      parId := pTyp.formals.a[index];
      IF needsBox(parId) THEN
	IF parId.parMod = Sym.var THEN (* pass value as well *)
	  parId.varOrd := count;
	  INC(count, jvmSize(parId.type));
	  AppendTypeName(fmArray, parId.type);
	END;
	IF retTp = NIL THEN
	 (*
	  *  Return slot is not already used, use it now.
	  *)
	  parId.boxOrd := retMarker;
	  pTyp.retN := jvmSize(parId.type);
	  retTp := parId.type;
	ELSE
	 (*
	  *  Return slot is already used, use a boxed variable.
	  *)
	  parId.boxOrd := count;
	  INC(count);
	  L.AppendCharOpen(fmArray, brac);
	  AppendTypeName(fmArray, parId.type);
	END;
      ELSE  (* could be two slots ... *)
	parId.varOrd := count;
	INC(count, jvmSize(parId.type));
	AppendTypeName(fmArray, parId.type);
      END;
    END;
    L.AppendCharOpen(fmArray, rPar);
    IF (retTp = NIL) OR (pIdn.kind = Id.ctorP) THEN 
      L.AppendCharOpen(fmArray, void);
    ELSIF (pIdn IS Id.MthId) & (Id.covar IN pIdn(Id.MthId).mthAtt) THEN
     (*
      *  This is a method with a covariant return type. We must
      *  erase the declared type, substituting the non-covariant
      *  upper-bound. Calls will cast the result to the real type.
      *)
      AppendTypeName(fmArray, pIdn.retTypBound());
    ELSE
      AppendTypeName(fmArray, retTp);
    END;
    pTyp.xName := L.arrayCat(fmArray);
   (*
    *   We must now set the argsize and retsize.
    *   The current info.lNum (before the locals
    *   have been added) is the argsize.
    *)
    pTyp.argN := count;
    pIdn.rtsFram := count;
  END NumberParams;

(* ------------------------------------------------------------ *)
(* Proxies are the local variables corresponding to boxed       *)
(* arguments that are not also passed by value i.e. OUT mode.   *)
(* ------------------------------------------------------------ *)
  PROCEDURE NumberProxies(pIdn : Id.Procs; IN pars : Id.ParSeq);
    VAR parId : Id.ParId;
	index : INTEGER;
  BEGIN
   (* ------------------ *
    *  Allocate an activation record slot for the XHR,
    *  if this is needed.  The XHR reference will be local
    *  number pIdn.type.argN.
    * ------------------ *)
    IF Id.hasXHR IN pIdn.pAttr THEN MkXHR(pIdn); INC(pIdn.rtsFram) END;
    FOR index := 0 TO pars.tide-1 DO
      parId := pars.a[index];
      IF parId.parMod # Sym.var THEN
        IF needsBox(parId) THEN
          parId.varOrd := pIdn.rtsFram;
          INC(pIdn.rtsFram, jvmSize(parId.type));
        END;
      END;
    END;
  END NumberProxies;

(* ------------------------------------------------------------ *)

  PROCEDURE NumberLocals(pIdn : Id.Procs; IN locs : Sym.IdSeq);
    VAR ident : Sym.Idnt;
	index : INTEGER;
	count : INTEGER;
  BEGIN
    count := pIdn.rtsFram;
    FOR index := 0 TO locs.tide-1 DO
      ident := locs.a[index];
      WITH ident : Id.ParId DO (* skip *)
      | ident : Id.LocId DO
	  ident.varOrd := count;
	  INC(count, jvmSize(ident.type));
      END;
    END;
    pIdn.rtsFram := count;
  END NumberLocals;

(* ------------------------------------------------------------ *)

  PROCEDURE MkCallAttr*(pIdn : Sym.Idnt; pTyp : Ty.Procedure);
  BEGIN
    WITH pIdn : Id.MthId DO
      IF ~needsBox(pIdn.rcvFrm) THEN 
        pIdn.rtsFram := 1;	(* count one for "this" *)
      ELSE
        pIdn.rtsFram := 2;	(* this plus the retbox *)
      END;
      MkProcName(pIdn);
      NumberParams(pIdn, pTyp);
    | pIdn : Id.PrcId DO 
        pIdn.rtsFram := 0;
        MkProcName(pIdn);
        NumberParams(pIdn, pTyp);
    END;
  END MkCallAttr;

(* ------------------------------------------------------------ *)

  PROCEDURE RenumberLocals*(prcId : Id.Procs);
    VAR parId : Id.ParId;
	frmTp : Ty.Procedure;
	funcT : BOOLEAN;
  BEGIN
   (*
    * Rules:
    * 	(i)   The receiver (if any) must be #0
    * 	(ii)  Params are #1 .. #N, or #0 .. for statics
    * 	(iii) Locals are #(N+1) ...
    * 	(iv)  doubles and longs take two slots.
    * 
    *   This procedure computes the number of local slots. It
    *   renumbers the varOrd fields, and initializes rtsFram.
    *   The procedure also computes the formal name for the JVM.
    *) 
    prcId.rtsFram := 0;
    frmTp := prcId.type(Ty.Procedure);
    funcT := (frmTp.retType # NIL);
    WITH prcId : Id.MthId DO
      parId := prcId.rcvFrm;
      parId.varOrd := 0;   prcId.rtsFram := 1;	(* count one for "this" *)
      ASSERT(~needsBox(parId));
(*
 *  Receivers are never boxed in Component Pascal
 *
 *    IF needsBox(parId) THEN 
 *      parId.boxOrd := 1; prcId.rtsFram := 2;	(* count one for retbox *)
 *    END;
 *)
    ELSE (* skip static procedures *)
    END;
   (*
    *   Assert: params do not appear in the local array.
    *   Count params (and boxes if needed).
    *)
    NumberParams(prcId, frmTp);
    IF prcId.body # NIL THEN
      NumberProxies(prcId, frmTp.formals);
      NumberLocals(prcId, prcId.locals);
    END;
  END RenumberLocals;

(* ------------------------------------------------------------ *)
(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)MakeAndPushProcLitValue*(exp : Sym.Expr; typ : Ty.Procedure),NEW;
    VAR singleton : Id.TypId;
	    hostClass : Ty.Record;
  BEGIN
	MkProcTypeName(typ);
	hostClass := newProcLitWrapperClass(exp, typ);
	hostClass.baseTp := typ.hostClass;
	jf.MkNewRecord(hostClass);
  END MakeAndPushProcLitValue;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)LoadLocal*(ord : INTEGER; typ : Sym.Type),NEW;
    VAR code : INTEGER;
  BEGIN
    IF (typ # NIL) & (typ IS Ty.Base) THEN 
      code := typeLoad[typ(Ty.Base).tpOrd];
    ELSE
      code := Jvm.opc_aload;
    END;
    IF ord < 4 THEN
      CASE code OF
      | Jvm.opc_iload : code := Jvm.opc_iload_0 + ord;
      | Jvm.opc_lload : code := Jvm.opc_lload_0 + ord;
      | Jvm.opc_fload : code := Jvm.opc_fload_0 + ord;
      | Jvm.opc_dload : code := Jvm.opc_dload_0 + ord;
      | Jvm.opc_aload : code := Jvm.opc_aload_0 + ord;
      END;
      jf.Code(code);
    ELSE
      jf.CodeI(code, ord);
    END;
  END LoadLocal;

(* ---------------------------------------------------- *)

  PROCEDURE (jf : JavaFile)GetLocal*(var : Id.LocId),NEW;
  BEGIN
    IF Id.uplevA IN var.locAtt THEN jf.GetUplevel(var);
    ELSE jf.LoadLocal(var.varOrd, var.type);
    END;
  END GetLocal;

(* ---------------------------------------------------- *)

  PROCEDURE typeToChOpen(typ : Sym.Type) : L.CharOpen;
   (* --------------------------------------------- *)
    PROCEDURE slashToDot(a : L.CharOpen) : L.CharOpen;
      VAR nw : L.CharOpen; ix : INTEGER; ch : CHAR;
    BEGIN
      NEW(nw, LEN(a));
      FOR ix := 0 TO LEN(a)-1 DO
	ch := a[ix]; IF ch = "/" THEN nw[ix] := "." ELSE nw[ix] := ch  END;
      END;
      RETURN nw;
    END slashToDot;
   (* --------------------------------------------- *)
    PROCEDURE typeTag(typ : Sym.Type) : L.CharOpen;
    BEGIN
      WITH typ : Ty.Base DO
          RETURN typ.xName;
      | typ : Ty.Array DO
          RETURN cat2(brac, typeTag(typ.elemTp));
      | typ : Ty.Record DO
          IF typ.xName = NIL THEN MkRecName(typ) END;
          RETURN slashToDot(typ.scopeNm);
      | typ : Ty.Enum DO
          RETURN Blt.intTp.xName;
      | typ : Ty.Pointer DO
          RETURN typeTag(typ.boundTp);
      | typ : Ty.Opaque DO
	  IF typ.xName = NIL THEN MkAliasName(typ) END;
          RETURN slashToDot(typ.scopeNm);
      END;
    END typeTag;
   (* --------------------------------------------- *)
  BEGIN 
    WITH typ : Ty.Base DO
	    RETURN typeTag(typ);
	| typ : Ty.Array DO
        RETURN cat2(brac, typeTag(typ.elemTp));
    | typ : Ty.Record DO
	IF typ.xName = NIL THEN MkRecName(typ) END;
        RETURN slashToDot(typ.xName);
    | typ : Ty.Pointer DO
        RETURN typeToChOpen(typ.boundTp);
    | typ : Ty.Opaque DO
	IF typ.xName = NIL THEN MkAliasName(typ) END;
        RETURN slashToDot(typ.xName);
    END;
  END typeToChOpen;

(* ---------------------------------------------------- *)

  PROCEDURE (jf : JavaFile)LoadType*(id : Sym.Idnt),NEW;
    VAR tp : Sym.Type;
  BEGIN
    ASSERT(id IS Id.TypId);
    tp := id.type;
    WITH tp : Ty.Base DO
      jf.PushInt(tp.tpOrd);
      jf.CallRTS(LoadTp1, 1, 1);
    ELSE
     (*
      *  First we get the string-name of the 
      *  type, and then we push the string.
      *)
      jf.PushStr(typeToChOpen(id.type));
     (*
      *  Then we call getClassByName
      *)
      jf.CallRTS(LoadTp2, 1, 1);
    END;
  END LoadType;

(* ---------------------------------------------------- *)

  PROCEDURE (jf : JavaFile)GetVar*(id : Sym.Idnt),NEW;
    VAR var : Id.AbVar;
        scp : Sym.Scope;
  BEGIN
    var := id(Id.AbVar);
    IF var.kind = Id.conId THEN
      jf.GetLocal(var(Id.LocId));
    ELSE
      scp := var.dfScp;
      WITH scp : Id.BlkId DO
        jf.PutGetS(Jvm.opc_getstatic, scp, var(Id.VarId));
      ELSE (* must be local *)
        jf.GetLocal(var(Id.LocId));
      END;
    END;
  END GetVar;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)StoreLocal*(ord : INTEGER; typ : Sym.Type),NEW;
    VAR code : INTEGER;
  BEGIN
    IF (typ # NIL) & (typ IS Ty.Base) THEN 
      code := typeStore[typ(Ty.Base).tpOrd];
    ELSE
      code := Jvm.opc_astore;
    END;
    IF ord < 4 THEN
      CASE code OF
      | Jvm.opc_istore : code := Jvm.opc_istore_0 + ord;
      | Jvm.opc_lstore : code := Jvm.opc_lstore_0 + ord;
      | Jvm.opc_fstore : code := Jvm.opc_fstore_0 + ord;
      | Jvm.opc_dstore : code := Jvm.opc_dstore_0 + ord;
      | Jvm.opc_astore : code := Jvm.opc_astore_0 + ord;
      END;
      jf.Code(code);
    ELSE
      jf.CodeI(code, ord);
    END;
  END StoreLocal;

(* ---------------------------------------------------- *)

  PROCEDURE (jf : JavaFile)PutLocal*(var : Id.LocId),NEW;
  BEGIN
    IF Id.uplevA IN var.locAtt THEN jf.PutUplevel(var);
    ELSE jf.StoreLocal(var.varOrd, var.type);
    END;
  END PutLocal;

(* ---------------------------------------------------- *)

  PROCEDURE (jf : JavaFile)PutVar*(id : Sym.Idnt),NEW;
    VAR var : Id.AbVar;
	scp : Sym.Scope;
  BEGIN
    var := id(Id.AbVar);
    scp := var.dfScp;
    WITH scp : Id.BlkId DO
      jf.PutGetS(Jvm.opc_putstatic, scp, var(Id.VarId));
    ELSE (* could be in an XHR *)
      jf.PutLocal(var(Id.LocId));
    END;
  END PutVar;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PutElement*(typ : Sym.Type),NEW;
    VAR code : INTEGER;
  BEGIN
    IF (typ # NIL) & (typ IS Ty.Base) THEN
      code := typePutE[typ(Ty.Base).tpOrd];
    ELSE
      code := Jvm.opc_aastore;
    END;
    jf.Code(code);
  END PutElement;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)GetElement*(typ : Sym.Type),NEW;
    VAR code : INTEGER;
  BEGIN
    IF (typ # NIL) & (typ IS Ty.Base) THEN
      code := typeGetE[typ(Ty.Base).tpOrd];
    ELSE
      code := Jvm.opc_aaload;
    END;
    jf.Code(code);
  END GetElement;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PushInt*(num : INTEGER),NEW;
  VAR
    conIx : INTEGER;
  BEGIN
    IF (num >= MIN(BYTE)) & (num <= MAX(BYTE)) THEN
      CASE num OF
      | -1 : jf.Code(Jvm.opc_iconst_m1);
      |  0 : jf.Code(Jvm.opc_iconst_0);
      |  1 : jf.Code(Jvm.opc_iconst_1);
      |  2 : jf.Code(Jvm.opc_iconst_2);
      |  3 : jf.Code(Jvm.opc_iconst_3);
      |  4 : jf.Code(Jvm.opc_iconst_4);
      |  5 : jf.Code(Jvm.opc_iconst_5);
      ELSE
        jf.CodeI(Jvm.opc_bipush, num);
      END;
    ELSE
      jf.LoadConst(num);
    END;
  END PushInt;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PushLong*(num : LONGINT),NEW;
  BEGIN
    IF num = 0 THEN
      jf.Code(Jvm.opc_lconst_0);
    ELSIF num = 1 THEN
      jf.Code(Jvm.opc_lconst_1);
    ELSIF (num >= MIN(INTEGER)) & (num <= MAX(INTEGER)) THEN
      jf.PushInt(SHORT(num));
      jf.Code(Jvm.opc_i2l);
    ELSE
      jf.CodeL(Jvm.opc_ldc2_w, num);
    END;
  END PushLong;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PushReal*(num : REAL),NEW;
  BEGIN
    IF num = 0.0 THEN
      jf.Code(Jvm.opc_dconst_0);
    ELSIF num = 1.0 THEN
      jf.Code(Jvm.opc_dconst_1);
    ELSE
      jf.CodeR(Jvm.opc_ldc2_w, num, FALSE);
    END;
  END PushReal;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PushSReal*(num : REAL),NEW;
  VAR
    conIx : INTEGER;
  BEGIN
    IF num = 0.0 THEN
      jf.Code(Jvm.opc_fconst_0);
    ELSIF num = 1.0 THEN
      jf.Code(Jvm.opc_fconst_1);
    ELSIF num = 2.0 THEN
      jf.Code(Jvm.opc_fconst_2);
    ELSE
      jf.CodeR(Jvm.opc_ldc, num, TRUE);
    END;
  END PushSReal;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PushStaticLink*(tgt : Id.Procs),NEW;
    VAR lxDel : INTEGER;
	clr   : Id.Procs;
	pTp   : Ty.Procedure;
  BEGIN
    clr   := jf.theP;
    lxDel := tgt.lxDepth - clr.lxDepth;
    pTp   := clr.type(Ty.Procedure);

    CASE lxDel OF
    | 0 : jf.Code(Jvm.opc_aload_0);
    | 1 : IF Id.hasXHR IN clr.pAttr THEN
            jf.LoadLocal(pTp.argN, NIL);
          ELSIF clr.lxDepth = 0 THEN
            jf.Code(Jvm.opc_aconst_null);
          ELSE
            jf.Code(Jvm.opc_aload_0);
          END;
    ELSE
      jf.Code(Jvm.opc_aload_0);
      REPEAT
        clr := clr.dfScp(Id.Procs);
        IF Id.hasXHR IN clr.pAttr THEN 
	  jf.PutGetF(Jvm.opc_getfield, 
		Cst.rtsXHR.boundRecTp()(Ty.Record), Cst.xhrId);
        END;
      UNTIL clr.lxDepth = tgt.lxDepth;
    END;
  END PushStaticLink;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)GetXHR(var : Id.LocId),NEW;
    VAR scp : Id.Procs; (* the scope holding the datum *)
	clr : Id.Procs; (* the scope making the call   *)
        pTp : Ty.Procedure;
	del : INTEGER;
  BEGIN
    scp := var.dfScp(Id.Procs);
    clr := jf.theP;
    pTp := clr.type(Ty.Procedure);
   (*
    *  Check if this is an own local
    *)
    IF scp = clr THEN
      jf.LoadLocal(pTp.argN, NIL);
    ELSE
      del := xhrCount(scp, clr);
     (*
      *  First, load the static link
      *)
      jf.Code(Jvm.opc_aload_0);
     (*
      *  Next, load the XHR pointer.
      *)
      WHILE del > 1 DO
	jf.PutGetF(Jvm.opc_getfield, 
		Cst.rtsXHR.boundRecTp()(Ty.Record), Cst.xhrId);
        DEC(del);
      END;
     (*
      *  Finally, cast to concrete type
      *)
      jf.CodeT(Jvm.opc_checkcast, scp.xhrType);
    END;
  END GetXHR;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PutGetX*(cde : INTEGER; var : Id.LocId),NEW;
    VAR pTyp : Sym.Type;
  BEGIN
    pTyp := var.dfScp(Id.Procs).xhrType;
    jf.PutGetF(cde, pTyp.boundRecTp()(Ty.Record), var);
  END PutGetX;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)XhrHandle*(var : Id.LocId),NEW;
  BEGIN
    jf.GetXHR(var);
  END XhrHandle;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)GetUplevel*(var : Id.LocId),NEW;
  BEGIN
    jf.GetXHR(var);
    jf.PutGetX(Jvm.opc_getfield, var);
  END GetUplevel;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PutUplevel*(var : Id.LocId),NEW;
  BEGIN
    jf.PutGetX(Jvm.opc_putfield, var);
  END PutUplevel;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)ConvertUp*(inT, outT : Sym.Type),NEW;
   (* Conversion "up" is always safe at runtime. Many are nop.	*)
    VAR inB, outB, code : INTEGER;
  BEGIN
    inB  := inT(Ty.Base).tpOrd;
    outB := outT(Ty.Base).tpOrd;
    IF inB = outB THEN RETURN END;		(* PREMATURE RETURN! *)
    CASE outB OF
    | Ty.realN :
	IF    inB = Ty.sReaN THEN code := Jvm.opc_f2d;
	ELSIF inB = Ty.lIntN THEN code := Jvm.opc_l2d;
	ELSE 			  code := Jvm.opc_i2d;
        END;
    | Ty.sReaN :
	IF    inB = Ty.lIntN THEN code := Jvm.opc_l2f;
	ELSE 			  code := Jvm.opc_i2f;
        END;
    | Ty.lIntN :
	code := Jvm.opc_i2l;
    ELSE RETURN;				(* PREMATURE RETURN! *)
    END;
    jf.Code(code);
  END ConvertUp;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)ConvertDn*(inT, outT : Sym.Type),NEW;
   (* Conversion "down" often needs a runtime check. *)
    VAR inB, outB, code : INTEGER;
  BEGIN
    inB  := inT(Ty.Base).tpOrd;
    outB := outT(Ty.Base).tpOrd;
    IF inB = outB THEN RETURN END;		(* PREMATURE RETURN! *)
    CASE outB OF
    | Ty.realN : RETURN;			(* PREMATURE RETURN! *)
    | Ty.sReaN : 
	code := Jvm.opc_d2f;
    | Ty.lIntN :
	IF    inB = Ty.realN THEN code := Jvm.opc_d2l;
	ELSIF inB = Ty.sReaN THEN code := Jvm.opc_f2l;
	ELSE  RETURN;				(* PREMATURE RETURN! *)
        END;
    | Ty.intN  :
	IF    inB = Ty.realN THEN code := Jvm.opc_d2i;
	ELSIF inB = Ty.sReaN THEN code := Jvm.opc_f2i;
	ELSIF inB = Ty.lIntN THEN 
	  (* jf.RangeCheck(...); STILL TO DO *)
	  code := Jvm.opc_l2i;
	ELSE  RETURN;				(* PREMATURE RETURN! *)
        END;
    | Ty.sIntN :
        jf.ConvertDn(inT, Blt.intTp);
        (* jf.RangeCheck(...); STILL TO DO *)
        code := Jvm.opc_i2s;
    | Ty.uBytN :
         jf.ConvertDn(inT, Blt.intTp);
        (* jf.RangeCheck(...); STILL TO DO *)
        jf.PushInt(255);
        code := Jvm.opc_iand;
    | Ty.byteN :
	jf.ConvertDn(inT, Blt.intTp);
	(* jf.RangeCheck(...); STILL TO DO *)
	code := Jvm.opc_i2b;
    | Ty.setN  : 
	jf.ConvertDn(inT, Blt.intTp); RETURN;	(* PREMATURE RETURN! *)
    | Ty.charN  : 
        jf.ConvertDn(inT, Blt.intTp);
        (* jf.RangeCheck(...); STILL TO DO *)
        code := Jvm.opc_i2c;
    | Ty.sChrN  : 
        jf.ConvertDn(inT, Blt.intTp);
        (* jf.RangeCheck(...); STILL TO DO *)
        jf.PushInt(255);
        code := Jvm.opc_iand;
    END;
    jf.Code(code);
  END ConvertDn;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)EmitOneRange*
	       (var : INTEGER;		(* local variable index *)
		loC : INTEGER;		(* low-value of range   *)
		hiC : INTEGER;		(* high-value of range  *)
		min : INTEGER;		(* minimun selector val *)
		max : INTEGER;		(* maximum selector val *)
		def : Label;          (* default code label   *)
                target : Label),NEW;	
  (* ---------------------------------------------------------- *
   *  The selector value is known to be in the range min .. max *
   *  and we wish to send values between loC and hiC to the     *
   *  target code label. All otherwise go to def.               *
   *  A range is "compact" if it is hard against min/max limits *
   * ---------------------------------------------------------- *)
  BEGIN
   (*
    *    Deal with several special cases...
    *)
    IF (min = loC) & (max = hiC) THEN	(* fully compact: just GOTO *)
      jf.CodeLb(Jvm.opc_goto, target);
    ELSE
      jf.LoadLocal(var, Blt.intTp);
      IF loC = hiC THEN (* a singleton *)
	jf.PushInt(loC);
	jf.CodeLb(Jvm.opc_if_icmpeq, target);
      ELSIF min = loC THEN		(* compact at low end only  *)
	jf.PushInt(hiC);
	jf.CodeLb(Jvm.opc_if_icmple, target);
      ELSIF max = hiC THEN		(* compact at high end only *)
	jf.PushInt(loC);
	jf.CodeLb(Jvm.opc_if_icmpge, target);
      ELSE				(* Shucks! The general case *)
	jf.PushInt(loC);
	jf.CodeLb(Jvm.opc_if_icmplt, def);
	jf.LoadLocal(var, Blt.intTp);
	jf.PushInt(hiC);
	jf.CodeLb(Jvm.opc_if_icmple, target);
      END;
      jf.CodeLb(Jvm.opc_goto, def);
    END;
  END EmitOneRange;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)Return*(ret : Sym.Type),NEW;
  BEGIN
    IF ret = NIL THEN
      jf.Code(Jvm.opc_return);
    ELSIF ret IS Ty.Base THEN
      jf.Code(typeRetn[ret(Ty.Base).tpOrd]);
    ELSE
      jf.Code(Jvm.opc_areturn);
    END;
  END Return;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)FixPar(par : Id.ParId),NEW;
  BEGIN
   (*
    *   Load up the actual into boxVar[0];
    *)
    jf.LoadLocal(par.boxOrd, NIL);
    jf.Code(Jvm.opc_iconst_0);
   (* 
    *   The param might be an XHR field, so 
    *   jf.LoadLocal(par.varOrd, par.type) breaks.
    *)
    jf.GetLocal(par);
    jf.PutElement(par.type);
  END FixPar;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)FixOutPars*(pId : Id.Procs; OUT ret : Sym.Type),NEW;
    VAR frm : Ty.Procedure;
	par : Id.ParId;
	idx : INTEGER;
  BEGIN
    ret := NIL;
(*
 *  Receivers are never boxed in Component Pascal.
 *
 *  WITH pId : Id.MthId DO
 *    par := pId.rcvFrm;
 *    IF par.boxOrd # 0 THEN jf.FixPar(par) END;
 *  ELSE (* nothing *)
 *  END;
 *)
    frm := pId.type(Ty.Procedure);
    FOR idx := 0 TO frm.formals.tide-1 DO
      par := frm.formals.a[idx];
      IF par.boxOrd = retMarker THEN 
	ret := par.type;
       (* 
        *   The param might be an XHR field, so 
        *   jf.LoadLocal(par.varOrd, ret) breaks.
        *)
        jf.GetLocal(par);
      ELSIF needsBox(par) THEN 
	jf.FixPar(par);
      END;
    END;
   (*
    *  If ret is still NIL, then either there is an explicit
    *  return type, or there was no OUT or VAR parameters here. 
    *  So...
    *)
    IF (ret = NIL) & (pId.kind # Id.ctorP) THEN ret := frm.retType END;
  END FixOutPars;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)PushJunkAndReturn*(),NEW;
    VAR frm : Ty.Procedure;
        ret : Sym.Type;
        idx : INTEGER;
        par : Id.ParId;
  BEGIN
   (*
    *  This procedure pushes a dummy return value 
    *  if that is necessary, and calls return.
    *)
    ret := NIL;
    IF jf.theP = NIL THEN RETURN END;         (* PREMATURE EXIT FOR MOD BODY *)
    frm := jf.theP.type(Ty.Procedure);
   (*
    *  First, we must find the (jvm) return type.
    *  It would have been nice to store this in out.info!
    *)
    FOR idx := 0 TO frm.formals.tide-1 DO
      par := frm.formals.a[idx];
      IF par.boxOrd = retMarker THEN ret := par.type END;
    END;
    IF ret = NIL THEN ret := frm.retType END;
   (*
    *  Now push a "zero" if necessary, then return.
    *  If this is a void function in the JVM, then we
    *  may safely leave things to the fall through return.
    *)
    IF ret # NIL THEN
      WITH ret : Ty.Base DO
        CASE ret.tpOrd OF
        |  Ty.boolN .. Ty.intN : jf.Code(Jvm.opc_iconst_0);
        |  Ty.lIntN            : jf.Code(Jvm.opc_lconst_0);
        |  Ty.sReaN            : jf.Code(Jvm.opc_fconst_0);
        |  Ty.realN            : jf.Code(Jvm.opc_dconst_0);
        ELSE                     jf.Code(Jvm.opc_aconst_null);
        END;
      ELSE
        jf.Code(Jvm.opc_aconst_null);
      END;
      jf.Return(ret);
    END;
  END PushJunkAndReturn;

(* ------------------------------------------------------------ *)

  PROCEDURE (jf : JavaFile)Init1dArray*(elTp : Sym.Type; leng : INTEGER),NEW;
    CONST inlineLimit = 4;
    VAR indx : INTEGER;
	labl : Label;
	arrT : Ty.Array;
  BEGIN
   (*
    *   Precondition: elTp is either a record or fixed array.
    *	At entry stack is (top) arrayRef, unchanged at exit.
    *	(len == 0) ==> take length from runtime descriptor.
    *)
    IF (leng < 4) & (leng # 0) & (elTp.kind = Ty.recTp) THEN
     (*
      *    Do a compile-time loop ...
      *)
      FOR indx := 0 TO leng-1 DO
	jf.Code(Jvm.opc_dup);
	jf.PushInt(indx);
	jf.MkNewRecord(elTp(Ty.Record));
	jf.Code(Jvm.opc_aastore);
      END;
    ELSE
     (* ------------------------------------------------------ *
      *    Do a runtime loop ...
      *
      *		push-len>	; (top) len, ref,...
      *	 loop: 
      *		iconst_1	; (top) 1, len, ref,...
      *		isub		; (top) len*, ref,...
      *		dup2		; (top) len*, ref, len*, ref,...
      *		<newElem>	; (top) new, len*, ref, len*, ref,...
      *		aastore		; (top) len*, ref,...
      *		dup		; (top) len*, len*, ref,...
      *		ifne loop	; (top) len*, ref,...
      *		pop		; (top) ref, ...
      * ------------------------------------------------------ *)
      IF leng = 0 THEN (* find the length from the descriptor  *)
	jf.Code(Jvm.opc_dup);
	jf.Code(Jvm.opc_arraylength);
      ELSE
	jf.PushInt(leng);
      END;
      labl := jf.newLabel();
      jf.DefLabC(labl, "1-d init loop");
      jf.Code(Jvm.opc_iconst_1);
      jf.Code(Jvm.opc_isub);
      jf.Code(Jvm.opc_dup2);
      IF elTp.kind = Ty.recTp THEN
        jf.MkNewRecord(elTp(Ty.Record));
      ELSE
        arrT := elTp(Ty.Array); 
        jf.MkNewFixedArray(arrT.elemTp, arrT.length);
      END;
      jf.Code(Jvm.opc_aastore);
      jf.Code(Jvm.opc_dup);
      jf.CodeLb(Jvm.opc_ifne, labl);
      jf.CodeC(Jvm.opc_pop, "	; end 1-d loop");
    END;
  END Init1dArray;

(* ============================================================ *)
	
  PROCEDURE (jf : JavaFile)InitNdArray*(desc : Sym.Type; elTp : Sym.Type),NEW;
    VAR labl : Label;
  BEGIN
   (* ------------------------------------------------------ *
    *	Initialize multi-dimensional array, using 
    *	the runtime array descriptors to generate lengths.
    *   Here, desc is the outer element type; elTp 
    *   most nested type.
    *
    *	At entry stack is (top) arrayRef, unchanged at exit.
    *
    *		dup		; (top) ref,ref...
    *		arraylength	; (top) len,ref...
    *	  loop:      	
    *		iconst_1	; (top) 1,len,ref...
    *		isub		; (top) len',ref...
    *		dup2 		; (top) hi,ref,hi,ref...
    *		if (desc == elTp)
    *		   <eleminit>	; (top) rec,ref[i],hi,ref...
    *		   aastore	; (top) hi,ref...
    *		else
    *		   aaload	; (top) ref[i],hi,ref...
    *		   <recurse>	; (top) ref[i],hi,ref...
    *		   pop		; (top) hi,ref...
    *		endif
    *		dup		; (top) hi,hi,ref...
    *		ifne loop	; (top) hi,ref...
    *		pop		; (top) ref...
    * ------------------------------------------------------ *)
    labl := jf.newLabel();
    jf.Code(Jvm.opc_dup);
    jf.Code(Jvm.opc_arraylength);
    jf.DefLabC(labl, "Element init loop");
    jf.Code(Jvm.opc_iconst_1);
    jf.Code(Jvm.opc_isub);
    jf.Code(Jvm.opc_dup2);
    IF desc = elTp THEN
     (*
      *    This is the innermost loop!
      *)
      WITH elTp : Ty.Array DO
         (*
	  *  Must be switching from open to fixed arrays...
	  *)
	  jf.MkNewFixedArray(elTp.elemTp, elTp.length);
      | elTp : Ty.Record DO
         (*
	  *  Element type is some record type.
	  *)
	  jf.MkNewRecord(elTp);
      END;
      jf.Code(Jvm.opc_aastore);
    ELSE
     (*
      *    There are more dimensions to go ... so recurse down.
      *)
      jf.Code(Jvm.opc_aaload);
      jf.InitNdArray(desc(Ty.Array).elemTp, elTp);
      jf.Code(Jvm.opc_pop);
    END;
    jf.Code(Jvm.opc_dup);
    jf.CodeLb(Jvm.opc_ifne, labl);
    jf.CodeC(Jvm.opc_pop, "	; end loop");
  END InitNdArray;

(* ============================================================ *)

  PROCEDURE (jf : JavaFile)ValArrCopy*(typ : Ty.Array),NEW;
    VAR	local : INTEGER;
	sTemp : INTEGER;
	label : Label;
	elTyp : Sym.Type;
  BEGIN
   (*
    *    Stack at entry is (top) srcRef, dstRef...
    *)
    label := jf.newLabel();
    local := jf.newLocal();
    IF typ.length = 0 THEN (* open array, get length from source desc *)
      jf.Code(Jvm.opc_dup);
      jf.Code(Jvm.opc_arraylength);
    ELSE 
      jf.PushInt(typ.length);
    END;
    jf.StoreLocal(local, Blt.intTp);
   (*
    *      <get length>	; (top) n,rr,lr...
    *      store(n)	; (top) rr,lr...
    * lab:
    *      dup2		; (top) rr,lr,rr,lr...
    *      iinc n -1	; (top) rr,lr...
    *      load(n)	; (top) n,rr,lr,rr,lr...
    *      dup_x1	; (top) n,rr,n,lr,rr,lr...
    *      <doassign>   ; (top) rr,lr
    *      load(n)	; (top) n,rr,lr...
    *      ifne lab     ; (top) rr,lr...
    *      pop2		; (top) ...
    *)
    jf.DefLab(label);
    jf.Code(Jvm.opc_dup2);
    jf.CodeInc(local, -1);
    jf.LoadLocal(local, Blt.intTp);
    jf.Code(Jvm.opc_dup_x1);
   (*
    *    Assign the element
    *)
    elTyp := typ.elemTp;
    jf.GetElement(elTyp);		(* (top) r[n],n,lr,rr,lr...	*)
    IF  (elTyp.kind = Ty.arrTp) OR
	(elTyp.kind = Ty.recTp) THEN
      sTemp := jf.newLocal();	(* must recurse in copy code	*)
      jf.StoreLocal(sTemp, elTyp);	(* (top) n,lr,rr,lr...		*)
      jf.GetElement(elTyp);		(* (top) l{n],rr,lr...		*)
      jf.LoadLocal(sTemp, elTyp);	(* (top) r[n],l[n],rr,lr...     *)
      jf.ReleaseLocal(sTemp);
      WITH elTyp : Ty.Record DO
	  jf.ValRecCopy(elTyp);
      | elTyp : Ty.Array DO
	  jf.ValArrCopy(elTyp);
      END;
    ELSE
      jf.PutElement(elTyp);
    END;
   (*
    *    stack is (top) rr,lr...
    *)
    jf.LoadLocal(local, Blt.intTp);
    jf.CodeLb(Jvm.opc_ifne, label);
    jf.Code(Jvm.opc_pop2);
    jf.ReleaseLocal(local);
  END ValArrCopy;

(* ============================================================ *)

  PROCEDURE (jf : JavaFile)InitVars*(scp : Sym.Scope),NEW;
    VAR index : INTEGER;
        xhrNo : INTEGER;
	ident : Sym.Idnt;
        scalr : BOOLEAN;
  BEGIN
    xhrNo := 0;
   (* 
    *  Create the explicit activation record, if needed.
    *)
    WITH scp : Id.Procs DO
      IF Id.hasXHR IN scp.pAttr THEN 
        xhrNo := scp.type(Ty.Procedure).argN;
        jf.Comment("create XHR record");
	jf.MkNewRecord(scp.xhrType.boundRecTp()(Ty.Record));
	IF scp.lxDepth > 0 THEN
          jf.Code(Jvm.opc_dup);
          jf.Code(Jvm.opc_aload_0);
          jf.PutGetF(Jvm.opc_putfield, 
		Cst.rtsXHR.boundRecTp()(Ty.Record), Cst.xhrId);
	END;
        jf.StoreLocal(xhrNo, NIL);
      END;
    ELSE (* skip *)
    END;
   (* 
    *  Initialize local fields, if needed
    *)
    FOR index := 0 TO scp.locals.tide-1 DO
      ident := scp.locals.a[index];
      scalr := ident.type.isScalarType();
      WITH ident : Id.ParId DO
         (*
          *   If any args are uplevel addressed, they must
          *   be copied to the correct field of the XHR.
          *   The test "varOrd < xhrNo" excludes out params.
          *)
          IF (Id.uplevA IN ident.locAtt) & (ident.varOrd < xhrNo) THEN 
            jf.LoadLocal(xhrNo, NIL);
            jf.LoadLocal(ident.varOrd, ident.type);
            jf.PutGetX(Jvm.opc_putfield, ident);
	  END;
      | ident : Id.LocId DO
          IF ~scalr THEN
            IF Id.uplevA IN ident.locAtt THEN jf.LoadLocal(xhrNo, NIL) END;
            jf.VarInit(ident);
            jf.PutLocal(ident);
	  END;
      | ident : Id.VarId DO
          IF ~scalr THEN
            jf.VarInit(ident);
	    jf.PutGetS(Jvm.opc_putstatic, scp(Id.BlkId), ident);
	  END;
      END;
    END;
  END InitVars;

(* ============================================================ *)

  PROCEDURE Init*();
  BEGIN
    xhrIx := 0;
    InitVecDescriptors();
  END Init;

(* ============================================================ *)
(* ============================================================ *)

BEGIN
  invokeHash := Hsh.enterStr("Invoke");

  L.InitCharOpenSeq(fmArray, 8);
  L.InitCharOpenSeq(nmArray, 8);

  typeRetn[ Ty.boolN] := Jvm.opc_ireturn;
  typeRetn[ Ty.sChrN] := Jvm.opc_ireturn;
  typeRetn[ Ty.charN] := Jvm.opc_ireturn;
  typeRetn[ Ty.byteN] := Jvm.opc_ireturn;
  typeRetn[ Ty.sIntN] := Jvm.opc_ireturn;
  typeRetn[  Ty.intN] := Jvm.opc_ireturn;
  typeRetn[ Ty.lIntN] := Jvm.opc_lreturn;
  typeRetn[ Ty.sReaN] := Jvm.opc_freturn;
  typeRetn[ Ty.realN] := Jvm.opc_dreturn;
  typeRetn[  Ty.setN] := Jvm.opc_ireturn;
  typeRetn[Ty.anyPtr] := Jvm.opc_areturn;
  typeRetn[ Ty.uBytN] := Jvm.opc_ireturn;

  typeLoad[ Ty.boolN] := Jvm.opc_iload;
  typeLoad[ Ty.sChrN] := Jvm.opc_iload;
  typeLoad[ Ty.charN] := Jvm.opc_iload;
  typeLoad[ Ty.byteN] := Jvm.opc_iload;
  typeLoad[ Ty.sIntN] := Jvm.opc_iload;
  typeLoad[  Ty.intN] := Jvm.opc_iload;
  typeLoad[ Ty.lIntN] := Jvm.opc_lload;
  typeLoad[ Ty.sReaN] := Jvm.opc_fload;
  typeLoad[ Ty.realN] := Jvm.opc_dload;
  typeLoad[  Ty.setN] := Jvm.opc_iload;
  typeLoad[Ty.anyPtr] := Jvm.opc_aload;
  typeLoad[Ty.anyRec] := Jvm.opc_aload;
  typeLoad[ Ty.uBytN] := Jvm.opc_iload;

  typeStore[ Ty.boolN] := Jvm.opc_istore;
  typeStore[ Ty.sChrN] := Jvm.opc_istore;
  typeStore[ Ty.charN] := Jvm.opc_istore;
  typeStore[ Ty.byteN] := Jvm.opc_istore;
  typeStore[ Ty.sIntN] := Jvm.opc_istore;
  typeStore[  Ty.intN] := Jvm.opc_istore;
  typeStore[ Ty.lIntN] := Jvm.opc_lstore;
  typeStore[ Ty.sReaN] := Jvm.opc_fstore;
  typeStore[ Ty.realN] := Jvm.opc_dstore;
  typeStore[  Ty.setN] := Jvm.opc_istore;
  typeStore[Ty.anyPtr] := Jvm.opc_astore;
  typeStore[Ty.anyRec] := Jvm.opc_astore;
  typeStore[ Ty.uBytN] := Jvm.opc_istore;

  typePutE[ Ty.boolN] := Jvm.opc_bastore;
  typePutE[ Ty.sChrN] := Jvm.opc_castore;
  typePutE[ Ty.charN] := Jvm.opc_castore;
  typePutE[ Ty.byteN] := Jvm.opc_bastore;
  typePutE[ Ty.sIntN] := Jvm.opc_sastore;
  typePutE[  Ty.intN] := Jvm.opc_iastore;
  typePutE[ Ty.lIntN] := Jvm.opc_lastore;
  typePutE[ Ty.sReaN] := Jvm.opc_fastore;
  typePutE[ Ty.realN] := Jvm.opc_dastore;
  typePutE[  Ty.setN] := Jvm.opc_iastore;
  typePutE[Ty.anyPtr] := Jvm.opc_aastore;
  typePutE[Ty.anyRec] := Jvm.opc_aastore;
  typePutE[ Ty.uBytN] := Jvm.opc_bastore;

  typeGetE[ Ty.boolN] := Jvm.opc_baload;
  typeGetE[ Ty.sChrN] := Jvm.opc_caload;
  typeGetE[ Ty.charN] := Jvm.opc_caload;
  typeGetE[ Ty.byteN] := Jvm.opc_baload;
  typeGetE[ Ty.sIntN] := Jvm.opc_saload;
  typeGetE[  Ty.intN] := Jvm.opc_iaload;
  typeGetE[ Ty.lIntN] := Jvm.opc_laload;
  typeGetE[ Ty.sReaN] := Jvm.opc_faload;
  typeGetE[ Ty.realN] := Jvm.opc_daload;
  typeGetE[  Ty.setN] := Jvm.opc_iaload;
  typeGetE[Ty.anyPtr] := Jvm.opc_aaload;
  typeGetE[Ty.anyRec] := Jvm.opc_aaload;
  typeGetE[ Ty.uBytN] := Jvm.opc_baload;

  semi := L.strToCharOpen(";"); 
  comma := L.strToCharOpen(","); 
  colon := L.strToCharOpen(":"); 
  lPar := L.strToCharOpen("("); 
  rPar := L.strToCharOpen(")"); 
  brac := L.strToCharOpen("["); 
  lCap := L.strToCharOpen("L"); 
  void := L.strToCharOpen("V"); 
  rParV:= L.strToCharOpen(")V");
  lowL := L.strToCharOpen("_"); 
  slsh := L.strToCharOpen("/"); 
  dlar := L.strToCharOpen("$"); 
  prfx := L.strToCharOpen(classPrefix); 
  xhrDl := L.strToCharOpen("XHR$");
  xhrMk := L.strToCharOpen("LCP/CPJrts/XHR;");
  procLitPrefix := L.strToCharOpen("Proc$Lit$");

  Blt.setTp.xName := L.strToCharOpen("I");
  Blt.intTp.xName := L.strToCharOpen("I");
  Blt.boolTp.xName := L.strToCharOpen("Z");
  Blt.byteTp.xName := L.strToCharOpen("B");
  Blt.uBytTp.xName := L.strToCharOpen("B");  (* same as BYTE *)
  Blt.charTp.xName := L.strToCharOpen("C");
  Blt.sChrTp.xName := L.strToCharOpen("C");
  Blt.sIntTp.xName := L.strToCharOpen("S");
  Blt.lIntTp.xName := L.strToCharOpen("J");
  Blt.realTp.xName := L.strToCharOpen("D");
  Blt.sReaTp.xName := L.strToCharOpen("F");
  Blt.anyRec.xName := L.strToCharOpen("Ljava/lang/Object;");
  Blt.anyPtr.xName := Blt.anyRec.xName;
END JavaUtil.
(* ============================================================ *)
(* ============================================================ *)

