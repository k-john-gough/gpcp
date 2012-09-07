(* ==================================================================== *)
(*									*)
(*  Builtin Symbols for the Gardens Point Component Pascal Compiler.	*)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*									*)
(* ==================================================================== *)

MODULE Builtin;

  IMPORT 
	GPCPcopyright,
	Console,
 	NameHash,
	CompState,
	Symbols,
	IdDesc,
    LitValue,
	Typ := TypeDesc;

(* ============================================================ *)
  CONST	(* Here are global ordinals for builtin objects	procs	*)
	(* Builtin Functions					*)
      absP*  =  1; ashP*  =  2; bitsP* =  3; capP*  =  4;
      chrP*  =  5; entP*  =  6; lenP*  =  7; longP* =  8; 
      maxP*  =  9; minP*  = 10; oddP*  = 11; ordP*  = 12; 
      shrtP* = 13; sizeP* = 14; mStrP* = 15; tpOfP* = 16;
      boxP*  = 17; uBytP* = 18; 
	(* Builtin Proper Procedures				*)
      asrtP* = 21; decP*  = 22; incP*  = 23; inclP* = 24; 
      exclP* = 25; haltP* = 26; newP*  = 27; throwP*= 28;
      subsP* = 29; unsbP* = 30; apndP* = 31; cutP*  = 32;
	(* Builtin SYSTEM Functions      			*)
      adrP*  = 33; getP*  = 34; putP*  = 35;

  CONST builtinTypeNum* = 16;

(* ============================================================ *)

  VAR	(* Here are the global descriptors for builtin objects.	*)
	(* Builtin Types					*)
      boolTp- : Symbols.Type;	(* type descriptor of BOOLEAN	*)
      byteTp- : Symbols.Type;	(* type descriptor of BYTE	*)
      uBytTp- : Symbols.Type;	(* type descriptor of UBYTE	*)
      charTp- : Symbols.Type;	(* type descriptor of CHAR	*)
      sChrTp- : Symbols.Type;	(* type descriptor of SHORTCHAR	*)
      intTp-  : Symbols.Type;	(* type descriptor of INTEGER	*)
      sIntTp- : Symbols.Type;	(* type descriptor of SHORTINT	*)
      lIntTp- : Symbols.Type;	(* type descriptor of LONGINT	*)
      realTp- : Symbols.Type;	(* type descriptor of REAL	*)
      sReaTp- : Symbols.Type;	(* type descriptor of SHORTREAL	*)
      anyRec- : Symbols.Type;	(* type descriptor of ANYREC	*)
      anyPtr- : Symbols.Type;	(* type descriptor of ANYPTR	*)
      setTp-  : Symbols.Type;	(* type descriptor of SET	*)
      strTp-  : Symbols.Type;	(* type descriptor of <strings>	*)
      sStrTp- : Symbols.Type;	(* type descriptor of <shortSt>	*)
      metaTp- : Symbols.Type;	(* type descriptor of META	*)

      chrArr- : Symbols.Type;	(* open value array of CHAR     *)

      anyTpId- : IdDesc.TypId;

  VAR baseTypeArray- : ARRAY builtinTypeNum+1 OF Symbols.Type;

  VAR sysBkt- : INTEGER;
      frnBkt- : INTEGER;
      noChkB- : INTEGER;
      constB- : INTEGER;
      basBkt- : INTEGER;
      selfBk- : INTEGER;
      xpndBk- : INTEGER;

(* ============================================================ *)

  VAR	(* Here are more global descriptors for builtin objects	*)
	(* Builtin Functions					*)
      absPd-  : Symbols.Idnt;	(* ident descriptor of ABS	*)
      ashPd-  : Symbols.Idnt;	(* ident descriptor of ASH	*)
      bitsPd- : Symbols.Idnt;	(* ident descriptor of BITS	*)
      capPd-  : Symbols.Idnt;	(* ident descriptor of CAP	*)
      chrPd-  : Symbols.Idnt;	(* ident descriptor of CHR	*)
      entPd-  : Symbols.Idnt;	(* ident descriptor of ENTIER	*)
      lenPd-  : Symbols.Idnt;	(* ident descriptor of LEN	*)
      longPd- : Symbols.Idnt;	(* ident descriptor of LONG	*)
      maxPd-  : Symbols.Idnt;	(* ident descriptor of MAX	*)
      minPd-  : Symbols.Idnt;	(* ident descriptor of MIN	*)
      oddPd-  : Symbols.Idnt;	(* ident descriptor of ODD	*)
      ordPd-  : Symbols.Idnt;	(* ident descriptor of ORD	*)
      uBytPd- : Symbols.Idnt;	(* ident descriptor of USHORT	*)
      shrtPd- : Symbols.Idnt;	(* ident descriptor of SHORT	*)
      sizePd- : Symbols.Idnt;	(* ident descriptor of SIZE	*)
      mStrPd- : Symbols.Idnt;	(* ident descriptor of MKSTR	*)
      tpOfPd- : Symbols.Idnt;	(* ident descriptor of TYPEOF	*)
      boxPd-  : Symbols.Idnt;	(* ident descriptor of BOX      *)
	(* SYSTEM functions                                     *)
      adrPd-  : Symbols.Idnt;	(* ident descriptor of ADR      *)
      getPd-  : Symbols.Idnt;	(* ident descriptor of GET      *)
      putPd-  : Symbols.Idnt;	(* ident descriptor of PUT      *)
	(* Builtin Proper Procedures				*)
      asrtPd- : Symbols.Idnt;	(* ident descriptor of ASSERT	*)
      decPd-  : Symbols.Idnt;	(* ident descriptor of DEC	*)
      incPd-  : Symbols.Idnt;	(* ident descriptor of INC	*)
      inclPd- : Symbols.Idnt;	(* ident descriptor of INCL	*)
      exclPd- : Symbols.Idnt;	(* ident descriptor of EXCL	*)
      haltPd- : Symbols.Idnt;	(* ident descriptor of HALT	*)
      throwPd-: Symbols.Idnt;	(* ident descriptor of THROW	*)
      newPd-  : Symbols.Idnt;	(* ident descriptor of NEW	*)
      subsPd- : Symbols.Idnt;	(* ident desc of REGISTER	*)
      unsbPd- : Symbols.Idnt;	(* ident desc of DEREGISTER	*)
      apndPd- : Symbols.Idnt;	(* ident descriptor of APPEND   *)
      cutPd-  : Symbols.Idnt;	(* ident descriptor of CUT      *)

(* ============================================================ *)

  VAR	(* Here are more global descriptors for builtin objects	*)
	(* Builtin Constants					*)
      trueC- : Symbols.Idnt;	(* ident descriptor of TRUE	*)
      falsC- : Symbols.Idnt;	(* ident descriptor of FALSE	*)
      infC-  : Symbols.Idnt;	(* ident descriptor of INF	*)
      nInfC- : Symbols.Idnt;	(* ident descriptor of NEGINF	*)
      nilC-  : Symbols.Idnt;	(* ident descriptor of NIL	*)

(* ============================================================ *)

  VAR (* some private stuff *)
      dummyProcType : Typ.Procedure;
      dummyFuncType : Typ.Procedure;

(* ============================================================ *)

  PROCEDURE MkDummyImport*(IN  nam : ARRAY OF CHAR;
			   IN  xNm : ARRAY OF CHAR;
			   OUT blk : IdDesc.BlkId);
    VAR jnk : BOOLEAN;
  BEGIN
    blk := IdDesc.newImpId();
    blk.dfScp   := blk;
    blk.hash    := NameHash.enterStr(nam);
    IF LEN(xNm) > 1 THEN blk.scopeNm := LitValue.strToCharOpen(xNm) END;
    jnk := CompState.thisMod.symTb.enter(blk.hash, blk);
    INCL(blk.xAttr, Symbols.isFn);
  END MkDummyImport;

(* ------------------------------------------------------------	*)

  PROCEDURE MkDummyClass*(IN  nam : ARRAY OF CHAR;
			      blk : IdDesc.BlkId;
			      att : INTEGER;
			  OUT tId : IdDesc.TypId);
    VAR ptr : TypeDesc.Pointer;
	rec : TypeDesc.Record;
	jnk : BOOLEAN;
  BEGIN
    ptr := TypeDesc.newPtrTp(); 
    rec := TypeDesc.newRecTp(); 
    tId := IdDesc.newTypId(ptr);
    ptr.idnt    := tId;
    ptr.boundTp := rec;
    rec.bindTp  := ptr;
    rec.extrnNm := blk.scopeNm;
    rec.recAtt  := att;
    INCL(rec.xAttr, Symbols.clsTp);		(* new 04.jun.01 *)
    tId.SetMode(Symbols.pubMode);
    tId.dfScp := blk;
    tId.hash  := NameHash.enterStr(nam);
	tId.SetNameFromHash(tId.hash);
    jnk := blk.symTb.enter(tId.hash, tId);
  END MkDummyClass;

(* ------------------------------------------------------------	*)

  PROCEDURE MkDummyMethodAndInsert*(IN namStr : ARRAY OF CHAR;
                                       prcTyp : TypeDesc.Procedure;
                                       hostTp : Symbols.Type;
                                       scope  : IdDesc.BlkId;
                                       access : INTEGER;
                                       rcvFrm : INTEGER;
                                       mthAtt : SET);
    VAR mthD : IdDesc.MthId;
        recT : TypeDesc.Record;
        rcvD : IdDesc.ParId;
       	oldD : IdDesc.OvlId;
        junk : BOOLEAN;
  BEGIN
    recT := hostTp.boundRecTp()(TypeDesc.Record);
    prcTyp.receiver := hostTp;

    mthD := IdDesc.newMthId();
    mthD.SetMode(access);
    mthD.setPrcKind(IdDesc.conMth);
    mthD.hash := NameHash.enterStr(namStr);
    mthD.dfScp := scope;
    mthD.type := prcTyp;
    mthD.bndType := hostTp;
    mthD.mthAtt := mthAtt;
	mthD.SetNameFromString(BOX(namStr));

    rcvD := IdDesc.newParId();
    rcvD.varOrd := 0;
    rcvD.parMod := rcvFrm;
    rcvD.type := hostTp;
	rcvD.hash := NameHash.enterStr("this");
	rcvD.dfScp := mthD;

    mthD.rcvFrm := rcvD;
    TypeDesc.InsertInRec(mthD, recT, TRUE, oldD, junk);
    Symbols.AppendIdnt(recT.methods, mthD);
  END MkDummyMethodAndInsert;

(* ------------------------------------------------------------	*)

  PROCEDURE MkDummyVar*(IN  nam : ARRAY OF CHAR;
                            blk : IdDesc.BlkId;
                            typ : Symbols.Type;
                        OUT vId : IdDesc.VarId);
    VAR jnk : BOOLEAN;
  BEGIN
    vId := IdDesc.newVarId();
    vId.SetMode(Symbols.pubMode);
    vId.type  := typ;
    vId.dfScp := blk;
    vId.hash  := NameHash.enterStr(nam);
    jnk := blk.symTb.enter(vId.hash, vId);
  END MkDummyVar;

(* ------------------------------------------------------------	*)

  PROCEDURE MkDummyAlias*(IN  nam : ARRAY OF CHAR;
			      blk : IdDesc.BlkId;
			      typ : Symbols.Type;
			  OUT tId : Symbols.Idnt);
    VAR (* tId : IdDesc.TypId; *)
	jnk : BOOLEAN;
  BEGIN
    tId := IdDesc.newTypId(typ);
    tId.SetMode(Symbols.pubMode);
    tId.dfScp := blk;
    tId.hash  := NameHash.enterStr(nam);
    jnk := blk.symTb.enter(tId.hash, tId);
  END MkDummyAlias;

(* ------------------------------------------------------------	*)

  PROCEDURE SetPtrBase*(cls, bas : IdDesc.TypId);
    VAR ptrC : TypeDesc.Pointer;
	recC : TypeDesc.Record;
    VAR ptrB : TypeDesc.Pointer;
	recB : TypeDesc.Record;
  BEGIN
    ptrC := cls.type(TypeDesc.Pointer); 
    recC := ptrC.boundTp(TypeDesc.Record);
    ptrB := bas.type(TypeDesc.Pointer); 
    recB := ptrB.boundTp(TypeDesc.Record);
    recC.baseTp := recB;
  END SetPtrBase;

(* ============================================================ *)

  PROCEDURE InitAnyRec(ord : INTEGER);
    VAR base : TypeDesc.Base;
        tpId : IdDesc.TypId;
  BEGIN
    base := TypeDesc.anyRecTp;
    tpId := IdDesc.newTypId(base);
    anyRec := base;
    anyTpId := tpId;
    base.idnt  := tpId;
    base.tpOrd := ord;
    base.dump  := ord;
    baseTypeArray[ord] := base;
  END InitAnyRec;

  PROCEDURE InitAnyPtr(ord : INTEGER);
    VAR base : TypeDesc.Base;
	tpId : IdDesc.TypId;
  BEGIN
    base := TypeDesc.anyPtrTp;
    tpId := IdDesc.newTypId(base);
    anyPtr := base;
    base.idnt  := tpId;
    base.tpOrd := ord;
    base.dump  := ord;
    baseTypeArray[ord] := base;
  END InitAnyPtr;

(* -------------------------------------------- *)

  PROCEDURE StdType(ord : INTEGER; OUT var : Symbols.Type);
    VAR base : TypeDesc.Base;
	tpId : IdDesc.TypId;
  BEGIN
    base := TypeDesc.newBasTp();
    tpId := IdDesc.newTypId(base);
    base.idnt  := tpId;
    base.tpOrd := ord;
    base.dump  := ord;
    var := base;
    baseTypeArray[ord] := base;
  END StdType;

(* -------------------------------------------- *)

  PROCEDURE StdConst(typ : Symbols.Type; OUT var : Symbols.Idnt);
    VAR conD : IdDesc.ConId;
  BEGIN
    conD := IdDesc.newConId();
    conD.SetStd();
    conD.type  := typ;
    var := conD;
  END StdConst;

(* -------------------------------------------- *)

  PROCEDURE StdFunc(ord : INTEGER; OUT var : Symbols.Idnt);
    VAR proc : IdDesc.PrcId;
  BEGIN
    proc := IdDesc.newPrcId();
    proc.SetKind(IdDesc.conPrc);
    proc.SetOrd(ord);
    proc.type := dummyFuncType;
    var := proc;
  END StdFunc;

(* -------------------------------------------- *)

  PROCEDURE StdProc(ord : INTEGER; OUT var : Symbols.Idnt);
    VAR proc : IdDesc.PrcId;
  BEGIN
    proc := IdDesc.newPrcId();
    proc.SetKind(IdDesc.conPrc);
    proc.SetOrd(ord);
    proc.type := dummyProcType;
    var := proc;
  END StdProc;

(* -------------------------------------------- *)

  PROCEDURE BindName(var : Symbols.Idnt; IN str : ARRAY OF CHAR);
    VAR hash : INTEGER;
	temp : IdDesc.BlkId;
  BEGIN
    hash := NameHash.enterStr(str);
    var.hash := hash;
    var.dfScp := NIL;
    var.SetNameFromString(BOX(str$));
    ASSERT(CompState.thisMod.symTb.enter(hash, var));
  END BindName;

(* -------------------------------------------- *)

  PROCEDURE BindSysName(var : Symbols.Idnt; IN str : ARRAY OF CHAR);
    VAR hash : INTEGER;
	temp : IdDesc.BlkId;
  BEGIN
    hash := NameHash.enterStr(str);
    var.hash := hash;
    var.dfScp := NIL;
    ASSERT(CompState.sysMod.symTb.enter(hash, var));
  END BindSysName;

(* -------------------------------------------- *)

  PROCEDURE RebindBuiltins*;
  BEGIN
    selfBk := NameHash.enterStr("SELF");
    basBkt := NameHash.enterStr("BASE");
    sysBkt := NameHash.enterStr("SYSTEM");
    xpndBk := NameHash.enterStr("expand");
    frnBkt := NameHash.enterStr("FOREIGN");
    constB := NameHash.enterStr("CONSTRUCTOR");
    noChkB := NameHash.enterStr("UNCHECKED_ARITHMETIC");
    BindName(boolTp.idnt, "BOOLEAN");
    BindName(byteTp.idnt, "BYTE");
    BindName(uBytTp.idnt, "UBYTE");
    BindName(charTp.idnt, "CHAR");
    BindName(sChrTp.idnt, "SHORTCHAR");
    BindName(intTp.idnt,  "INTEGER");
    BindName(sIntTp.idnt, "SHORTINT");
    BindName(lIntTp.idnt, "LONGINT");
    BindName(realTp.idnt, "REAL");
    BindName(sReaTp.idnt, "SHORTREAL");
    BindName(anyRec.idnt, "ANYREC");
    BindName(anyPtr.idnt, "ANYPTR");
    BindName(setTp.idnt,  "SET");
    BindName(strTp.idnt,  "<string>");
    BindName(sStrTp.idnt, "<shortString>");
    BindName(metaTp.idnt, "<META-TYPE>");

    BindName(absPd,  "ABS");
    BindName(ashPd,  "ASH");
    BindName(bitsPd, "BITS");
    BindName(capPd,  "CAP");
    BindName(chrPd,  "CHR");
    BindName(entPd,  "ENTIER");
    BindName(lenPd,  "LEN");
    BindName(longPd, "LONG");
    BindName(maxPd,  "MAX");
    BindName(minPd,  "MIN");
    BindName(oddPd,  "ODD");
    BindName(ordPd,  "ORD");
    BindName(uBytPd, "USHORT");
    BindName(shrtPd, "SHORT");
    BindName(sizePd, "SIZE");
    BindName(mStrPd, "MKSTR");
    BindName(boxPd,  "BOX");
    BindName(tpOfPd, "TYPEOF");

    BindSysName(adrPd, "ADR");
    BindSysName(getPd, "GET");
    BindSysName(putPd, "PUT");

    BindName(asrtPd, "ASSERT");
    BindName(decPd,  "DEC");
    BindName(incPd,  "INC");
    BindName(inclPd, "INCL");
    BindName(exclPd, "EXCL");
    BindName(haltPd, "HALT");
    BindName(throwPd,"THROW");
    BindName(newPd,  "NEW");
    BindName(subsPd, "REGISTER");
    BindName(unsbPd, "DEREGISTER");
    BindName(apndPd, "APPEND");
    BindName(cutPd,  "CUT");

    BindName(trueC, "TRUE");
    BindName(falsC, "FALSE");
    BindName(infC,  "INF");
    BindName(nInfC, "NEGINF");
    BindName(nilC,  "NIL");

    CompState.sysMod.hash := sysBkt;
  END RebindBuiltins;

(* -------------------------------------------- *)

  PROCEDURE InitBuiltins*;
  BEGIN
    InitAnyRec(Typ.anyRec);
    InitAnyPtr(Typ.anyPtr);
    StdType(Typ.boolN, boolTp);
    StdType(Typ.byteN, byteTp);
    StdType(Typ.uBytN, uBytTp);
    StdType(Typ.charN, charTp); chrArr := Typ.mkArrayOf(charTp);
    StdType(Typ.sChrN, sChrTp);
    StdType(Typ.intN,  intTp);  Typ.integerT := intTp;
    StdType(Typ.sIntN, sIntTp);
    StdType(Typ.lIntN, lIntTp);
    StdType(Typ.realN, realTp);
    StdType(Typ.sReaN, sReaTp);
(*
    StdType(Typ.anyPtr,anyPtr);
 *)
    StdType(Typ.setN,  setTp);
    StdType(Typ.strN,  strTp);
    StdType(Typ.sStrN, sStrTp);
    StdType(Typ.metaN, metaTp);

    dummyProcType := Typ.newPrcTp();
    dummyFuncType := Typ.newPrcTp(); 
    dummyFuncType.retType := anyPtr;

    StdFunc(absP,  absPd);
    StdFunc(ashP,  ashPd);
    StdFunc(bitsP, bitsPd);
    StdFunc(capP,  capPd);
    StdFunc(chrP,  chrPd);
    StdFunc(entP,  entPd);
    StdFunc(lenP,  lenPd);
    StdFunc(longP, longPd);
    StdFunc(maxP,  maxPd);
    StdFunc(minP,  minPd);
    StdFunc(oddP,  oddPd);
    StdFunc(ordP,  ordPd);
    StdFunc(uBytP, uBytPd);
    StdFunc(shrtP, shrtPd);
    StdFunc(sizeP, sizePd);
    StdFunc(mStrP, mStrPd);
    StdFunc(boxP,  boxPd);
    StdFunc(tpOfP, tpOfPd);

    StdFunc(adrP,  adrPd);
    StdProc(getP,  getPd);
    StdProc(putP,  putPd);

    StdProc(asrtP, asrtPd);
    StdProc(decP,  decPd);
    StdProc(incP,  incPd);
    StdProc(inclP, inclPd);
    StdProc(exclP, exclPd);
    StdProc(haltP, haltPd);
    StdProc(throwP,throwPd);
    StdProc(newP,  newPd);
    StdProc(subsP, subsPd);
    StdProc(unsbP, unsbPd);
    StdProc(apndP, apndPd);
    StdProc(cutP,  cutPd);

    StdConst(boolTp, trueC);
    StdConst(boolTp, falsC);
    StdConst(sReaTp, infC);
    StdConst(sReaTp, nInfC);
    StdConst(anyPtr, nilC);
  END InitBuiltins;

(* ============================================================ *)
END Builtin.  (* ============================================== *)
(* ============================================================ *)

