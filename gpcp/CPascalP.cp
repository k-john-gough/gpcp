(* ==================================================================== *)
(*                                                                      *)
(*  Parser Module for the Gardens Point Component Pascal Compiler.      *)
(*      Copyright (c) John Gough 1999, 2000.                            *)
(*      This module was extensively modified from the parser            *)
(*      automatically produced by the M2 version of COCO/R, using       *)
(*      the CPascal.atg grammar used for the JVM version of GPCP.       *)
(*                                                                      *)
(* ==================================================================== *)

MODULE CPascalP;

  IMPORT
        GPCPcopyright,
        RTS,
        FileNames,
        ForeignName,
        LitValue,
        C := Console,
        T := CPascalG,
        S := CPascalS,
        G := CompState,
        Sy := Symbols,
        Id := IdDesc,
        Ty := TypeDesc,
        Xp := ExprDesc,
        Bi := Builtin,
        StatDesc,
        Visitor,
        OldSymFileRW,
        NewSymFileRW,
        NameHash;

(* ==================================================================== *)

CONST
  maxT       = 85;
  minErrDist =  2;  (* minimal distance (good tokens) between two errors *)
  setsize    = 32;
  noError    = -1;

TYPE
  SymbolSet = ARRAY (maxT DIV setsize + 1) OF SET; (* 0 .. 2 *)

VAR
  symSet  : ARRAY 13 OF SymbolSet; (*symSet[0] = allSyncSyms*)
  errDist : INTEGER;   (* number of symbols recognized since last error *)
  token   : S.Token;   (* current input symbol   *)
  nextT   : S.Token;   (* lookahead input symbol *)
  comma   : LitValue.CharOpen;

(* ==================================================================== *)
(*                         Utilities                                    *)
(* ==================================================================== *)

  PROCEDURE  Error (errNo: INTEGER);
  BEGIN
    IF errDist >= minErrDist THEN
      S.ParseErr.Report(errNo, nextT.lin, nextT.col);
    END;
    IF errNo < 300 THEN errDist := 0 END;
  END Error;

(* ==================================================================== *)

  PROCEDURE  SemError(errNo: INTEGER);
  BEGIN
    IF errDist >= minErrDist THEN
      S.SemError.Report(errNo, token.lin, token.col);
    END;
    IF errNo < 300 THEN errDist := 0 END;
  END SemError;

(* ==================================================================== *)

  PROCEDURE  SemErrorS1(errNo: INTEGER; IN str : ARRAY OF CHAR);
  BEGIN
    IF errDist >= minErrDist THEN
      S.SemError.RepSt1(errNo, str, token.lin, token.col);
    END;
    IF errNo < 300 THEN errDist := 0 END;
  END SemErrorS1;

(* ==================================================================== *)

  PROCEDURE  SemErrorT(errNo: INTEGER; tok : S.Token);
  BEGIN
    IF errDist >= minErrDist THEN
      S.SemError.Report(errNo, tok.lin, tok.col);
    END;
    IF errNo < 300 THEN errDist := 0 END;
  END SemErrorT;

(* ==================================================================== *)

  PROCEDURE TypeResolve(scp : Sy.Scope);
  BEGIN
   (*
    *  This visitor marks all reachable types with depth=REACHED;
    *)
    scp.symTb.Apply(Visitor.newResolver());
  END TypeResolve;

(* ==================================================================== *)

  PROCEDURE bindToken(scp : Sy.Scope) : Sy.Idnt;
    VAR hash : INTEGER;
  BEGIN
    hash  := NameHash.enterSubStr(token.pos, token.len);
    RETURN Sy.bind(hash, scp);
  END bindToken;

(* ==================================================================== *)

  PROCEDURE bindFieldToken(typ : Sy.Type; tok : S.Token) : Sy.Idnt;
    VAR hash : INTEGER;
        recT : Ty.Record;
        idnt : Sy.Idnt;
  BEGIN
   (*
    *  We must do a full bind here, rather than just a bind-local
    *  since we must look for inherited methods from the supertypes
    *)
    hash := NameHash.enterSubStr(tok.pos, tok.len);
    WITH typ : Ty.Record DO
        RETURN typ.bindField(hash);
    | typ : Ty.Enum DO
        RETURN typ.symTb.lookup(hash);
    END;
  END bindFieldToken;

(* ==================================================================== *)

  PROCEDURE bindTokenLocal(scp : Sy.Scope) : Sy.Idnt;
    VAR hash : INTEGER;
  BEGIN
    hash  := NameHash.enterSubStr(token.pos, token.len);
    RETURN Sy.bindLocal(hash, scp);
  END bindTokenLocal;

(* ==================================================================== *)

  PROCEDURE  Get;
  BEGIN
    REPEAT
      token := nextT;
      nextT := S.get();
      IF nextT.sym <= maxT THEN INC(errDist) ELSE Error(91) END;
    UNTIL nextT.sym <= maxT;
    S.prevTok := token;
  END Get;

(* ==================================================================== *)

  PROCEDURE  in (VAR s: SymbolSet; x: INTEGER): BOOLEAN;
  BEGIN
    RETURN x MOD setsize IN s[x DIV setsize];
  END in;

(* ==================================================================== *)

  PROCEDURE  Expect (n: INTEGER);
  BEGIN
    IF nextT.sym = n THEN Get ELSE Error(n) END;
  END Expect;

(* ==================================================================== *)

  PROCEDURE  weakSeparator (n, syFol, repFol: INTEGER): BOOLEAN;
  VAR
    s: SymbolSet;
    i: INTEGER;
  BEGIN
    IF nextT.sym = n THEN Get; RETURN TRUE
    ELSIF in(symSet[repFol], nextT.sym) THEN RETURN FALSE
    ELSE
      FOR i := 0 TO maxT DIV setsize DO
        s[i] := symSet[0, i] + symSet[syFol, i] + symSet[repFol, i];
      END;
      Error(n); WHILE ~ in(s, nextT.sym) DO Get END;
      RETURN in(symSet[syFol], nextT.sym)
    END
  END weakSeparator;

(* ==================================================================== *)
(*                   Forward Procedure Declarations                     *)
(* ==================================================================== *)

  PROCEDURE^ ActualParameters(VAR rslt : Sy.ExprSeq; inhScp : Sy.Scope);
  PROCEDURE^ ImportList (modScope : Id.BlkId);
  PROCEDURE^ DeclarationSequence (defScp : Sy.Scope);
  PROCEDURE^ ConstantDeclaration (defScp : Sy.Scope);
  PROCEDURE^ TypeDeclaration(defScp : Sy.Scope);
  PROCEDURE^ VariableDeclaration(defScp : Sy.Scope);
  PROCEDURE^ IdentDefList(OUT iSeq : Sy.IdSeq; scp : Sy.Scope; kind : INTEGER);
  PROCEDURE^ typeQualid(defScp : Sy.Scope) : Id.TypId;
  PROCEDURE^ qualident(defScp : Sy.Scope) : Sy.Idnt;
  PROCEDURE^ FPSection(VAR pars : Id.ParSeq; thisP, defScp : Sy.Scope);
  PROCEDURE^ type(defScp : Sy.Scope; vMod : INTEGER) : Sy.Type;
  PROCEDURE^ identDef(inhScp : Sy.Scope; tag : INTEGER) : Sy.Idnt;
  PROCEDURE^ constExpression(defScp : Sy.Scope) : Xp.LeafX;
  PROCEDURE^ expression(scope : Sy.Scope) : Sy.Expr;
  PROCEDURE^ designator(inhScp : Sy.Scope) : Sy.Expr;
  PROCEDURE^ OptExprList(VAR xList : Sy.ExprSeq; inhScp : Sy.Scope);
  PROCEDURE^ ExprList(VAR xList : Sy.ExprSeq; inhScp : Sy.Scope);
  PROCEDURE^ statementSequence(inhLp : Sy.Stmt; inhSc : Sy.Scope) : Sy.Stmt;
  PROCEDURE^ ProcedureStuff(scope : Sy.Scope);
  PROCEDURE^ CheckVisibility(seq : Sy.IdSeq; in : INTEGER; OUT out : INTEGER);
  PROCEDURE^ FormalParameters(thsP : Ty.Procedure;
                              proc : Id.Procs;
                              scpe : Sy.Scope);

(* ==================================================================== *)

  PROCEDURE  CPmodule();
    VAR name : LitValue.CharOpen;
  BEGIN
    Expect(T.MODULESym);
    Expect(T.identSym);
    G.thisMod.hash := NameHash.enterSubStr(token.pos, token.len);
    G.thisMod.token := token;
    Sy.getName.Of(G.thisMod, G.modNam);
   (* Manual addition 15-June-2000. *)
    IF nextT.sym = T.lbrackSym THEN
      Get;
      IF G.strict THEN SemError(221); END;
      Expect(T.stringSym);
      name := LitValue.subStrToCharOpen(token.pos+1, token.len-2);
      G.thisMod.scopeNm := name;
      Expect(T.rbrackSym);
      IF G.verbose THEN G.Message('external modName "' + name^ + '"') END;
(*
 *    SemError(144);
 *    Get;
 *    Expect(T.stringSym);
 *    Expect(T.rbrackSym);
 *)
    END;
   (* End addition 15-June-2000 kjg *)
    Expect(T.semicolonSym);
    IF (nextT.sym = T.IMPORTSym) THEN
      ImportList(G.thisMod);
    ELSE
      Sy.ResetScpSeq(G.impSeq);
    END;
    DeclarationSequence(G.thisMod);
    IF (nextT.sym = T.BEGINSym) THEN
      G.thisMod.begTok := nextT;
      Get;
     (*
      *     "BEGIN" [ '[' "UNCHECKED_ARITHMETIC" ']' ] ...
      *)
      IF nextT.sym = T.lbrackSym THEN
        Get;
        IF G.strict THEN SemError(221); END;
        Expect(T.identSym);
        IF NameHash.enterSubStr(token.pos, token.len) = Bi.noChkB THEN
          G.thisMod.ovfChk := FALSE;
        ELSE
          SemError(194);
        END;
        Expect(T.rbrackSym);
      END;
      G.thisMod.modBody := statementSequence(NIL, G.thisMod);
      IF (nextT.sym = T.CLOSESym) THEN
        Get;
        G.thisMod.modClose := statementSequence(NIL, G.thisMod);
      END;
    END;
  END CPmodule;

(* ==================================================================== *)

  PROCEDURE  ForeignMod();
    VAR name : LitValue.CharOpen;
  BEGIN
    Expect(T.MODULESym);
    Expect(T.identSym);
    G.thisMod.hash := NameHash.enterSubStr(token.pos, token.len);
    G.thisMod.token := token;
    Sy.getName.Of(G.thisMod, G.modNam);
    IF nextT.sym = T.lbrackSym THEN
      Get;
      Expect(T.stringSym);
      name := LitValue.subStrToCharOpen(token.pos+1, token.len-2);
      G.thisMod.scopeNm := name;
      Expect(T.rbrackSym);
      IF G.verbose THEN G.Message('external modName "' + name^ + '"') END;
    END;
    Expect(T.semicolonSym);
    IF (nextT.sym = T.IMPORTSym) THEN
      ImportList(G.thisMod);
    ELSE
      Sy.ResetScpSeq(G.impSeq);
    END;
    DeclarationSequence(G.thisMod);
  END ForeignMod;

(* ==================================================================== *)

  PROCEDURE  Import (modScope : Id.BlkId; VAR impSeq : Sy.ScpSeq);
    VAR ident : Id.BlkId;  (* The imported module name descriptor *)
        alias : Id.BlkId;  (* The declared alias name (optional)  *)
        clash : Sy.Idnt;
        dummy : BOOLEAN;
        idHsh : INTEGER;
        strng : LitValue.CharOpen;
        impNm : LitValue.CharOpen;
  BEGIN
    alias := NIL;
    ident := Id.newImpId();
    Expect(T.identSym);
    IF (nextT.sym = T.colonequalSym) THEN
      alias := Id.newAlias();
      alias.hash  := NameHash.enterSubStr(token.pos, token.len);
      IF Sy.refused(alias, modScope) THEN SemError(4) END;
      Get; (* Read past colon symbol *)
     (*
      *          the place to put
      *  Here is ^ the experimental processing for the option
      *  of using a literal string at this position.  If there
      *  is a literal string, it should be mapped to the default
      *  name here.
      *)
      IF nextT.sym = T.stringSym THEN
        Get;
        strng := LitValue.subStrToCharOpen(token.pos+1, token.len-2);
        ForeignName.ParseModuleString(strng, impNm);
        alias.token := token;  (* fake the name for err-msg use *)
        idHsh := NameHash.enterStr(impNm);
        IF G.strict THEN Error(221) END;
      ELSE
        Expect(T.identSym);
        alias.token := token;  (* fake the name for err-msg use *)
        idHsh := NameHash.enterSubStr(token.pos, token.len);
      END;
    ELSE
      idHsh := NameHash.enterSubStr(token.pos, token.len);
    END;
    ident.token := token;
    ident.dfScp := ident;
    ident.hash  := idHsh;

	IF G.verbose THEN ident.SetNameFromHash(idHsh) END;

    IF ident.hash = Bi.sysBkt THEN
      dummy := CompState.thisMod.symTb.enter(Bi.sysBkt, CompState.sysMod);
      IF G.verbose THEN G.Message("imports unsafe SYSTEM module") END;
      IF ~CompState.unsafe THEN SemError(227);
      ELSIF ~CompState.targetIsNET() THEN SemError(228);
      END;
    ELSE
      INCL(ident.xAttr, Sy.weak);
    END;

    clash := Sy.bind(ident.hash, modScope);
    IF clash = NIL THEN
      dummy := Sy.refused(ident, modScope);
    ELSIF clash.kind = Id.impId THEN
     (*
      *  This import might already be known as a result of an
      *  indirect import.  If that is the case, then we must
      *  substitute the old descriptor for "ident" in case there
      *  there are already references to it in the structure.
      *)
      clash.token := ident.token;   (* to help error reports  *)
	  IF G.verbose THEN clash.SetNameFromHash(clash.hash) END;
      ident := clash(Id.BlkId);
      IF ~ident.isWeak() & 
         (ident.hash # Bi.sysBkt) THEN SemError(170) END; (* imported twice  *)
    ELSE
      SemError(4);
    END;

    IF ident.hash = NameHash.mainBkt THEN
      modScope.main := TRUE;      (* the import is "CPmain" *)
      IF G.verbose THEN G.Message("contains CPmain entry point") END;
      INCL(modScope.xAttr, Sy.cMain); (* Console Main *)
    ELSIF ident.hash = NameHash.winMain THEN
      modScope.main := TRUE;      (* the import is "CPmain" *)
      INCL(modScope.xAttr, Sy.wMain); (* Windows Main *)
      IF G.verbose THEN G.Message("contains WinMain entry point") END;
    ELSIF ident.hash = NameHash.staBkt THEN
      INCL(modScope.xAttr, Sy.sta);
      IF G.verbose THEN G.Message("sets Single Thread Apartment") END;
    END;

    IF Sy.weak IN ident.xAttr THEN
     (*
      *  List the file, for importation later  ...
      *)
      Sy.AppendScope(impSeq, ident);
     (*
      *  Alias (if any) must appear after ImpId
      *)
      IF alias # NIL THEN
        alias.dfScp  := ident;    (* AFTER clash resolved. *)
        Sy.AppendScope(impSeq, alias);
      END;
      
      EXCL(ident.xAttr, Sy.weak); (* ==> directly imported *)
      INCL(ident.xAttr, Sy.need); (* ==> needed in symfile *)
    END;
  END Import;
  
  PROCEDURE ImportThreading(modScope : Id.BlkId; VAR impSeq : Sy.ScpSeq);
    VAR hash : INTEGER;
        idnt : Id.BlkId;
  BEGIN
    hash := NameHash.enterStr("mscorlib_System_Threading");
    idnt := Id.newImpId();
    idnt.dfScp := idnt;
    idnt.hash := hash;
    IF ~Sy.refused(idnt, modScope) THEN
      EXCL(idnt.xAttr, Sy.weak);
      INCL(idnt.xAttr, Sy.need);
      Sy.AppendScope(impSeq, idnt);
    END;
  END ImportThreading;

(* ==================================================================== *)

  PROCEDURE  ImportList (modScope : Id.BlkId);
    VAR index  : INTEGER;
  BEGIN
    Get;
    Sy.ResetScpSeq(G.impSeq);
    Import(modScope, G.impSeq);
    WHILE (nextT.sym = T.commaSym) DO
      Get;
      Import(modScope, G.impSeq);
    END;
    Expect(T.semicolonSym);
	(*
	 * Now some STA-specific tests.
	 *)
	IF Sy.sta IN modScope.xAttr THEN
      IF Sy.trgtNET THEN
        ImportThreading(modScope, G.impSeq);
       ELSE
         SemError(238);
      END;
      IF ~modScope.main THEN 
        SemError(319); 
        EXCL(modScope.xAttr, Sy.sta);
      END;	  
    END;
    
    G.import1 := RTS.GetMillis();
IF G.legacy THEN
    OldSymFileRW.WalkImports(G.impSeq, modScope);
ELSE
    NewSymFileRW.WalkImports(G.impSeq, modScope);
END;
    G.import2 := RTS.GetMillis();
  END ImportList;

(* ==================================================================== *)

  PROCEDURE  FPSection(VAR pars : Id.ParSeq; thisP, defScp : Sy.Scope);
  (* sequence is passed in from the caller *)
    VAR mode : INTEGER;
        indx : INTEGER;
        parD : Id.ParId;
        tpDx : Sy.Type;
        tokn : S.Token;
        pTst : BOOLEAN; (* test if formal type is private *)
   (* --------------------------- *)
    PROCEDURE isPrivate(t : Sy.Type) : BOOLEAN;
    BEGIN
      RETURN ~(t IS Ty.Base) & (t.idnt.vMod = Sy.prvMode);
    END isPrivate;
   (* --------------------------- *)
    PROCEDURE CheckFormalType(tst : BOOLEAN; tok : S.Token; typ : Sy.Type);
    BEGIN (*
      * There are two separate kinds of tests here:
      * * formals must not be less visible than the procedure
      * * anonymous formals are only useful in two cases:
      *   - open arrays, provided the element type is visible enough;
      *   - pointer types, provided the bound type is visible enough.
      *)
      IF typ = NIL THEN RETURN;
      ELSIF typ.idnt # NIL THEN
        IF tst & isPrivate(typ) THEN SemErrorT(150, tok) END;
      ELSE
        WITH typ : Ty.Record  DO
            SemErrorT(314, tok);
        | typ : Ty.Pointer DO
            CheckFormalType(tst, tok, typ.boundTp);
        | typ : Ty.Array   DO
            CheckFormalType(tst, tok, typ.elemTp);
            (* Open arrays and vectors have length = 0 *)
            IF typ.length # 0 THEN SemErrorT(315, tok) END;
        ELSE (* skip procs? *)
        END;
      END;
    END CheckFormalType;
   (* --------------------------- *)
  BEGIN
    Id.ResetParSeq(pars);     (* make sequence empty  *)
    IF nextT.sym = T.INSym THEN
      Get;
      mode := Sy.in;
    ELSIF nextT.sym = T.OUTSym THEN
      Get;
      mode := Sy.out;
    ELSIF nextT.sym = T.VARSym THEN
      Get;
      mode := Sy.var;
    ELSE
      mode := Sy.val;
    END;
    parD := identDef(defScp, Id.parId)(Id.ParId);
    Id.AppendParam(pars, parD);
    WHILE weakSeparator(12, 1, 2) DO
      parD := identDef(defScp, Id.parId)(Id.ParId);
      Id.AppendParam(pars, parD);
    END;
    Expect(T.colonSym);
    tokn := nextT;
    tpDx := type(defScp, Sy.prvMode);
    IF tpDx # NIL THEN
      pTst := ~G.special & (thisP # NIL) & (thisP.vMod = Sy.pubMode);

      CheckFormalType(pTst, tokn, tpDx);

      FOR indx := 0 TO pars.tide-1 DO
        pars.a[indx].parMod := mode;
        pars.a[indx].type := tpDx;
      END;
    END;
  END FPSection;

(* ==================================================================== *)

  PROCEDURE  DeclarationSequence (defScp : Sy.Scope);
  BEGIN
    WHILE (nextT.sym = T.CONSTSym) OR
          (nextT.sym = T.TYPESym) OR
          (nextT.sym = T.VARSym) DO
      IF (nextT.sym = T.CONSTSym) THEN
        Get;
        WHILE (nextT.sym = T.identSym) DO
          ConstantDeclaration(defScp);
          Expect(T.semicolonSym);
        END;
      ELSIF (nextT.sym = T.TYPESym) THEN
        Get;
        WHILE (nextT.sym = T.identSym) DO
          TypeDeclaration(defScp);
          Expect(T.semicolonSym);
        END;
      ELSE
        Get;
        WHILE (nextT.sym = T.identSym) DO
          VariableDeclaration(defScp);
          Expect(T.semicolonSym);
        END;
      END;
    END;
    (* Last chance to resolve forward types in this block *)
    defScp.endDecl := TRUE;
    TypeResolve(defScp);
    (* Now the local procedures *)
    WHILE (nextT.sym = T.PROCEDURESym) DO
      ProcedureStuff(defScp);
      Expect(T.semicolonSym);
    END;
  END DeclarationSequence;

(* ==================================================================== *)

  PROCEDURE otherAtts(in : SET) : SET;
  BEGIN
    IF nextT.sym = T.ABSTRACTSym THEN
      Get;
      RETURN in + Id.isAbs;
    ELSIF nextT.sym = T.EMPTYSym THEN
      Get;
      RETURN in + Id.empty;
    ELSIF nextT.sym = T.EXTENSIBLESym THEN
      Get;
      RETURN in + Id.extns;
    ELSE
      Error(77);
      RETURN in;
    END;
  END otherAtts;

(* ==================================================================== *)

  PROCEDURE^ MethAttributes(pDesc : Id.Procs);

(* ==================================================================== *)

  PROCEDURE receiver(scope : Sy.Scope) : Id.ParId;
    VAR mode : INTEGER;
        parD : Id.ParId;
        rcvD : Sy.Idnt;
  BEGIN
    Get;  (* read past lparenSym *)
    IF nextT.sym = T.INSym THEN
      Get;
      mode := Sy.in;
    ELSIF nextT.sym = T.VARSym THEN
      Get;
      mode := Sy.var;
    ELSE
      mode := Sy.val;
    END;
    parD := identDef(scope, Id.parId)(Id.ParId);
    parD.isRcv := TRUE;
    parD.parMod := mode;
    Expect(T.colonSym);
    Expect(T.identSym);
    rcvD := bindToken(scope);
    IF rcvD = NIL THEN
      SemError(2);
    ELSIF ~(rcvD IS Id.TypId) THEN
      SemError(5);
    ELSE
      parD.type := rcvD.type;
    END;
    Expect(T.rparenSym);
    RETURN parD;
  END receiver;

(* ==================================================================== *)

  PROCEDURE ExceptBody(pInhr : Id.Procs);
    VAR excp : Id.LocId;
  BEGIN
   (*
    *  This procedure has an exception handler. We must...
    *  (i)  define a local to hold the exception value
    *  (ii) parse the rescue statement sequence
    *)
    Expect(T.lparenSym);
    excp := identDef(pInhr, Id.varId)(Id.LocId);
    excp.SetKind(Id.conId);     (* mark immutable *)
    excp.type := G.ntvExc;
    excp.varOrd := pInhr.locals.tide;
    IF Sy.refused(excp, pInhr) THEN
      excp.IdError(4);
    ELSE
      Sy.AppendIdnt(pInhr.locals, excp);
      pInhr.except := excp;
    END;
    Expect(T.rparenSym);
    pInhr.rescue := statementSequence(NIL, pInhr); (* inhLp is NIL *)
  END ExceptBody;

(* ==================================================================== *)

  PROCEDURE ProcedureBody(pInhr : Id.Procs);(* Inherited descriptor *)
  BEGIN
    DeclarationSequence(pInhr);
    IF nextT.sym = T.BEGINSym THEN
      Get;
     (*
      *     "BEGIN" [ '[' "UNCHECKED_ARITHMETIC" ']' ] ...
      *)
      IF nextT.sym = T.lbrackSym THEN
        Get;
        Expect(T.identSym);
        IF NameHash.enterSubStr(token.pos, token.len) = Bi.noChkB THEN
          pInhr.ovfChk := FALSE;
        ELSE
          SemError(194);
        END;
        Expect(T.rbrackSym);
      END;
      pInhr.body := statementSequence(NIL, pInhr); (* inhLp is NIL *)
    END;
    IF nextT.sym = T.RESCUESym THEN
      Get;
      IF G.strict THEN SemError(221); END;
      ExceptBody(pInhr);
    END;
    Expect(T.ENDSym);
  END ProcedureBody;

(* ==================================================================== *)

  PROCEDURE procedureHeading(scope : Sy.Scope) : Id.Procs;
    VAR rcvD : Id.ParId;
        prcD : Id.Procs;
        mthD : Id.MthId;
        prcT : Ty.Procedure;
        name : LitValue.CharOpen;
  BEGIN
    IF nextT.sym # T.lparenSym THEN
      rcvD := NIL;
      prcD := identDef(scope, Id.conPrc)(Id.Procs);
      prcD.SetKind(Id.conPrc);
    ELSE
      rcvD := receiver(scope);
      mthD := identDef(scope, Id.conMth)(Id.MthId);
      mthD.SetKind(Id.conMth);
      IF (rcvD.type # NIL) &
         (rcvD.type.idnt # NIL) THEN
        IF rcvD.type.isInterfaceType() &
             (mthD.vMod = Sy.prvMode) THEN SemError(216);
        END;
      END;
      prcD := mthD;
      mthD.rcvFrm := rcvD;
      IF Sy.refused(rcvD, mthD) THEN     (* insert receiver in scope  *)
        rcvD.IdError(4);    (* refusal impossible maybe? *)
      ELSE
        rcvD.dfScp  := mthD;    (* Correct the defScp *)
        rcvD.varOrd := mthD.locals.tide;
        Sy.AppendIdnt(mthD.locals, rcvD);
      END;
    END;
    IF nextT.sym = T.lbrackSym THEN
      IF ~G.special THEN SemError(144) END;
      Get;
      Expect(T.stringSym);
      name := LitValue.subStrToCharOpen(token.pos+1, token.len-2);
      prcD.prcNm := name;
      Expect(T.rbrackSym);
      IF G.verbose THEN G.Message('external procName "' + name^ + '"') END;
    END;
    prcT := Ty.newPrcTp();
    prcT.idnt := prcD;
    prcD.type := prcT;
    IF prcD.vMod # Sy.prvMode THEN INCL(prcD.pAttr, Id.public) END;
    IF nextT.sym = T.lparenSym THEN
      FormalParameters(prcT, prcD, scope);
    END;
    IF nextT.sym = T.commaSym THEN
      Get;
      MethAttributes(prcD);  
    END;
    IF rcvD # NIL THEN prcT.receiver := rcvD.type END;
    prcD.EnterProc(rcvD, scope);
    RETURN prcD;
  END procedureHeading;

(* ==================================================================== *)

  PROCEDURE ProcDeclStuff(scope : Sy.Scope);
    VAR desc : Id.Procs;
        name : FileNames.NameString;
        pNam : FileNames.NameString;
        errN : INTEGER;
  BEGIN
    desc := procedureHeading(scope);
    WITH scope : Id.Procs DO               (* a nested proc *)
      Id.AppendProc(scope.nestPs, desc);
      desc.lxDepth := scope.lxDepth + 1;
    ELSE
      desc.lxDepth := 0;
    END;
    Id.AppendProc(G.thisMod.procs, desc);
    IF ~desc.isEmpty() & ~G.isForeign() THEN
      Expect(T.semicolonSym);
      ProcedureBody(desc);
      desc.endSpan := S.mkSpanTT(token, nextT);
      Expect(T.identSym);
     (* check closing name *)
      S.GetString(token.pos, token.len, name);
      Sy.getName.Of(desc, pNam);
      IF name # pNam THEN
        IF token.sym = T.identSym THEN errN := 1 ELSE errN := 0 END;
        SemErrorS1(errN, pNam$);
      END;
    END;
  END ProcDeclStuff;

(* ==================================================================== *)

  PROCEDURE ForwardStuff(scope : Sy.Scope);
    VAR desc : Id.Procs;
  BEGIN
    Get;        (* read past uparrowSym *)
    desc := procedureHeading(scope);
   (* Set lexical depth for forward procs as well. kjg Sep-2001 *)
    WITH scope : Id.Procs DO               (* a nested proc *)
      desc.lxDepth := scope.lxDepth + 1;
    ELSE
      desc.lxDepth := 0;
    END;
    IF desc.kind = Id.conMth THEN
      desc.setPrcKind(Id.fwdMth);
    ELSIF desc.kind = Id.conPrc THEN
      desc.setPrcKind(Id.fwdPrc);
    END;
    Id.AppendProc(G.thisMod.procs, desc);
  END ForwardStuff;

(* ==================================================================== *)

  PROCEDURE ProcedureStuff(scope : Sy.Scope);
  (* parse procedure and add to list in scope *)
  BEGIN
    Get;        (* read past PROCEDURESym *)
    IF nextT.sym = T.uparrowSym THEN
      ForwardStuff(scope);
    ELSIF (nextT.sym = T.identSym) OR
          (nextT.sym = T.lparenSym) THEN
      ProcDeclStuff(scope);
    ELSE Error(79);
    END;
  END ProcedureStuff;

(* ==================================================================== *)

  PROCEDURE guard(scope : Sy.Scope) : Sy.Expr;
    VAR expr : Xp.BinaryX;
        qual : Sy.Expr;
        dstX : Sy.Expr;       (* should be typeQualid *)
  BEGIN
    qual := Xp.mkIdLeaf(qualident(scope));
    Expect(T.colonSym);
    expr := Xp.newBinaryX(Xp.isOp, qual, NIL);
    dstX := Xp.mkIdLeaf(typeQualid(scope));
   (* Check #1 : that the expression has a type that is dynamic   *)
    IF ~qual.hasDynamicType() THEN qual.ExprError(17) END;
   (* Check #2 : that manifest type is a base of the asserted type  *)
    IF (qual.type # NIL) &
        ~qual.type.isBaseOf(dstX.type) &
        ~qual.type.isInterfaceType() &
        ~dstX.type.isInterfaceType() THEN SemError(15) END;
    expr.rKid := dstX;
    RETURN expr;
  END guard;

(* ==================================================================== *)

  PROCEDURE caseLabel(chTp  : BOOLEAN;
                      tide  : INTEGER;
                      scope : Sy.Scope) : StatDesc.Triple;
    VAR lExp, rExp : Sy.Expr;
        xpOk       : BOOLEAN;
        lo, hi     : INTEGER;
  BEGIN
    lo := 0; hi := 0;
    xpOk := TRUE;
    lExp := constExpression(scope);
    IF lExp # NIL THEN
      IF chTp THEN
        IF lExp.isCharLit() THEN
          lo := ORD(lExp(Xp.LeafX).charValue());
        ELSE
          lExp.ExprError(43); xpOk := FALSE;
        END;
      ELSE
        IF lExp.isNumLit() THEN
          lo := lExp(Xp.LeafX).value.int();
        ELSE
          lExp.ExprError(37); xpOk := FALSE;
        END;
      END;
    ELSE xpOk := FALSE;
    END;
    IF nextT.sym = T.pointpointSym THEN
      Get;
      rExp := constExpression(scope);
      IF rExp # NIL THEN
        IF chTp THEN
          IF rExp.isCharLit() THEN
            hi := ORD(rExp(Xp.LeafX).charValue());
          ELSE
            rExp.ExprError(43); xpOk := FALSE;
          END;
        ELSE
          IF rExp.isNumLit() THEN
            hi := rExp(Xp.LeafX).value.int();
          ELSE
            rExp.ExprError(37); xpOk := FALSE;
          END;
        END;
      ELSE xpOk := FALSE;
      END;
      IF xpOk & (lo > hi) THEN lExp.ExprError(30) END;
    ELSE
      hi := lo;
    END;
    RETURN StatDesc.newTriple(lo, hi, tide);
  END caseLabel;

(* ==================================================================== *)

  PROCEDURE CaseLabelList(VAR labels : StatDesc.TripleSeq;
                              isChar : BOOLEAN;
                              stTide : INTEGER;
                              scope  : Sy.Scope);
    VAR next : StatDesc.Triple;
  BEGIN
    next := caseLabel(isChar, stTide, scope);
    StatDesc.AppendTriple(labels, next);
    WHILE nextT.sym = T.commaSym DO
      Get;
      next := caseLabel(isChar, stTide, scope);
      StatDesc.AppendTriple(labels, next);
    END;
  END CaseLabelList;

(* ==================================================================== *)

  PROCEDURE Case(desc : StatDesc.CaseSt; inhLp : Sy.Stmt; scope : Sy.Scope);
  BEGIN
    IF in(symSet[3], nextT.sym) THEN
      CaseLabelList(desc.labels, desc.chrSel, desc.blocks.tide, scope);
      Expect(T.colonSym);
      Sy.AppendStmt(desc.blocks, statementSequence(inhLp, scope));
    END;
  END Case;

(* ==================================================================== *)

  PROCEDURE  ActualParameters(VAR rslt : Sy.ExprSeq; inhScp : Sy.Scope);
  BEGIN
    Expect(T.lparenSym);
    OptExprList(rslt, inhScp);
    Expect(T.rparenSym);
  END ActualParameters;

(* ==================================================================== *)

  PROCEDURE withStatement(inhLp : Sy.Stmt; scope : Sy.Scope) : Sy.Stmt;
    VAR synthS : StatDesc.Choice;
        predXp : Sy.Expr;
        block  : Sy.Stmt;
        savedI : Sy.Idnt;
        tmpId  : Id.LocId;
  BEGIN
    Get;
    synthS := StatDesc.newWithS();
    IF nextT.sym = T.ENDSym THEN
      Get;
      SemError(318);
      RETURN synthS;
    ELSIF nextT.sym = T.barSym THEN
      Get;
      IF G.strict THEN SemError(221); END;
    END;
    IF nextT.sym # T.ELSESym THEN
      predXp := guard(scope);
      Expect(T.DOSym);

      tmpId := Id.newLocId();
      tmpId.dfScp := scope;

      savedI := predXp.enterGuard(tmpId);
      block  := statementSequence(inhLp, scope);
      predXp.ExitGuard(savedI, tmpId);
      Sy.AppendIdnt(synthS.temps, tmpId);
      Sy.AppendExpr(synthS.preds, predXp);
      Sy.AppendStmt(synthS.blocks, block);
      WHILE nextT.sym = T.barSym DO
        Get;
        predXp := guard(scope);
        Expect(T.DOSym);

        tmpId := Id.newLocId();
        tmpId.dfScp := scope;

        savedI := predXp.enterGuard(tmpId);
        block  := statementSequence(inhLp, scope);
        predXp.ExitGuard(savedI, tmpId);
        Sy.AppendIdnt(synthS.temps, tmpId);
        Sy.AppendExpr(synthS.preds, predXp);
        Sy.AppendStmt(synthS.blocks, block);
      END;
    END;
    IF nextT.sym = T.ELSESym THEN
      Get;
      block  := statementSequence(inhLp, scope);
      Sy.AppendIdnt(synthS.temps, NIL);
      Sy.AppendExpr(synthS.preds, NIL);
      Sy.AppendStmt(synthS.blocks, block);
    END;
    Expect(T.ENDSym);
    RETURN synthS;
  END withStatement;

(* ==================================================================== *)

  PROCEDURE loopStatement(scope : Sy.Scope) : Sy.Stmt;
  (* This procedure ignores the inherited attribute which for   *
   * other cases designates the enclosing loop. This becomes    *
   * the source of the enclosing loop for all nested statements *)
    VAR newLoop : StatDesc.TestLoop;
  BEGIN
    Get;
    newLoop := StatDesc.newLoopS();
    newLoop.body := statementSequence(newLoop, scope);
    Expect(T.ENDSym);
    RETURN newLoop;
  END loopStatement;

(* ==================================================================== *)

  PROCEDURE forStatement(inhLp : Sy.Stmt; scope : Sy.Scope) : Sy.Stmt;
    VAR rslt : StatDesc.ForLoop;
        cIdn : Sy.Idnt;

   (* ------------------------- *)
    PROCEDURE Check(id : Sy.Idnt);
    BEGIN
      IF id = NIL THEN SemError(2);
      ELSIF ~(id IS Id.AbVar) THEN SemError(85);
      ELSIF ~id.mutable() THEN SemError(103);
      ELSIF (id.type # NIL) & ~id.type.isIntType() THEN SemError(84);
      END;
    END Check;
   (* ------------------------- *)

  BEGIN
    Get;
    rslt := StatDesc.newForStat();
    Expect(T.identSym);
    cIdn := bindToken(scope);
    Check(cIdn);
    Expect(T.colonequalSym);
    rslt.cVar := cIdn;
    rslt.loXp := expression(scope);
    Expect(T.TOSym);
    rslt.hiXp := expression(scope);
    IF (nextT.sym = T.BYSym) THEN
      Get;
      rslt.byXp := constExpression(scope);
      IF rslt.byXp # NIL THEN
        IF rslt.byXp.kind # Xp.numLt THEN
          rslt.byXp.ExprError(59);
        ELSIF rslt.byXp(Xp.LeafX).value.long() = 0 THEN
          rslt.byXp.ExprError(81);
        END;
      END;
    ELSE
      rslt.byXp := Xp.mkNumLt(1);
    END;
    Expect(T.DOSym);
    rslt.body := statementSequence(inhLp, scope);
    Expect(T.ENDSym);
    RETURN rslt;
  END forStatement;

(* ==================================================================== *)

  PROCEDURE repeatStatement(inhLp : Sy.Stmt; scope : Sy.Scope) : Sy.Stmt;
    VAR rslt : StatDesc.TestLoop;
  BEGIN
    Get;
    rslt := StatDesc.newRepeatS();
    rslt.body := statementSequence(inhLp, scope);
    Expect(T.UNTILSym);
    rslt.test := expression(scope);
    RETURN rslt;
  END repeatStatement;

(* ==================================================================== *)

  PROCEDURE whileStatement(inhLp : Sy.Stmt; scope : Sy.Scope) : Sy.Stmt;
    VAR rslt : StatDesc.TestLoop;
  BEGIN
    Get;
    rslt := StatDesc.newWhileS();
    rslt.test := expression(scope);
    Expect(T.DOSym);
    rslt.body := statementSequence(inhLp, scope);
    Expect(T.ENDSym);
    RETURN rslt;
  END whileStatement;

(* ==================================================================== *)

  PROCEDURE caseStatement(inhLp : Sy.Stmt; scope : Sy.Scope) : Sy.Stmt;
    VAR rslt : StatDesc.CaseSt;
        slct : Sy.Expr;
        isCh : BOOLEAN;
  BEGIN
    Get;
    rslt := StatDesc.newCaseS();
    slct := expression(scope);
    Expect(T.OFSym);
    IF slct # NIL THEN slct := slct.exprAttr() END;
    IF (slct # NIL) & (slct.type # NIL) THEN
      rslt.chrSel := FALSE;
      rslt.select := slct;
      IF slct.isCharExpr() THEN
        rslt.chrSel := TRUE;
      ELSIF slct.isIntExpr() THEN
        IF slct.type.isLongType() THEN slct.ExprError(141) END;
      ELSE
        slct.ExprError(88);
      END;
    END;
    IF nextT.sym = T.ENDSym THEN 
      SemError(317);
    ELSE
      Case(rslt, inhLp, scope);
      WHILE nextT.sym = T.barSym DO
        Get;
        Case(rslt, inhLp, scope);
      END;
      IF nextT.sym = T.ELSESym THEN
        Get;
        rslt.elsBlk := statementSequence(inhLp, scope);
      END;
    END;
    Expect(T.ENDSym);
    RETURN rslt;
  END caseStatement;

(* ==================================================================== *)

  PROCEDURE ifStatement(inhLp : Sy.Stmt; scope : Sy.Scope) : Sy.Stmt;
    VAR synthStat : StatDesc.Choice;
  BEGIN
    Get;
    synthStat := StatDesc.newIfStat();
    Sy.AppendExpr(synthStat.preds, expression(scope));
    Expect(T.THENSym);
    Sy.AppendStmt(synthStat.blocks, statementSequence(inhLp, scope));
    WHILE nextT.sym = T.ELSIFSym DO
      Get;
      Sy.AppendExpr(synthStat.preds, expression(scope));
      Expect(T.THENSym);
      Sy.AppendStmt(synthStat.blocks, statementSequence(inhLp, scope));
    END;
    IF (nextT.sym = T.ELSESym) THEN
      Get;
      Sy.AppendExpr(synthStat.preds, NIL);
      Sy.AppendStmt(synthStat.blocks, statementSequence(inhLp, scope));
    END;
    Expect(T.ENDSym);
    RETURN synthStat;
  END ifStatement;

(* ==================================================================== *)

  PROCEDURE^ ConvertOverloaded(VAR e : Sy.Expr);

  PROCEDURE^ makeCall(xCr : Sy.Expr; IN actuals : Sy.ExprSeq;
                     inhScp : Sy.Scope) : Sy.Expr;

(* ==================================================================== *)

  PROCEDURE identStatement(inhLp : Sy.Stmt; scope : Sy.Scope) : Sy.Stmt;
    VAR assign  : StatDesc.Assign;
        prCall  : StatDesc.ProcCall;
        argLst  : Sy.ExprSeq;
        desig   : Sy.Expr;
        value   : Sy.Expr;
        saveT   : S.Token;
  BEGIN
    saveT := nextT;
    desig := designator(scope);
    IF nextT.sym = T.colonequalSym THEN
      ConvertOverloaded(desig);
      IF desig # NIL THEN desig.tSpan := S.mkSpanTT(saveT, S.prevTok) END;
      Get;
      assign := StatDesc.newAssignS();
      value := expression(scope);
      assign.lhsX := desig;
      assign.rhsX := value;
      Xp.CheckIsVariable(desig);
      RETURN assign;
    ELSIF in(symSet[8], nextT.sym) THEN
      IF (desig # NIL) & ~(desig IS Xp.CallX) THEN
        desig := makeCall(desig,argLst,scope);
      END;
      prCall := StatDesc.newProcCall();
      prCall.expr := desig;
      IF desig # NIL THEN desig.tSpan := S.mkSpanTT(saveT, S.prevTok) END;
      IF (desig # NIL) & 
         (desig.type # NIL) & ~desig.type.isProperProcType() THEN
        desig.ExprError(182);
      END;
      RETURN prCall;
    ELSE Error(82); RETURN StatDesc.newEmptyS();
    END;
  END identStatement;

(* ==================================================================== *)

  PROCEDURE statement(inhLp : Sy.Stmt; inhSc : Sy.Scope) : Sy.Stmt;
    VAR synthStat : Sy.Stmt;
        synthExpr : Sy.Expr;
        keywordTk : S.Token;

   (* ------------------------- *)

    PROCEDURE newStatement(inhSc : Sy.Scope) : Sy.Stmt;
    (* This case is pulled out of line, so that the cost of *
     * initialisation of the sequence is only paid when needed  *)
      VAR argList : Sy.ExprSeq;
          callExp : Sy.Expr;
          qualId  : Sy.Expr;
          callNew : StatDesc.ProcCall;
    BEGIN
      Get;
      callNew := StatDesc.newProcCall();
      ActualParameters(argList, inhSc);
      qualId := Xp.mkIdLeaf(Bi.newPd);
      callExp := Xp.newCallX(Xp.prCall, argList, qualId);
      callExp.tSpan := S.mkSpanTT(callNew.token, S.prevTok); 
      callNew.expr := callExp;
      RETURN callNew;
    END newStatement;

   (* ------------------------- *)

  BEGIN
    keywordTk := nextT;
    IF in(symSet[9], nextT.sym) THEN
      CASE nextT.sym OF
      | T.identSym:
          synthStat := identStatement(inhLp, inhSc);
      | T.LOOPSym:
          synthStat := loopStatement(inhSc);
      | T.IFSym:
          synthStat := ifStatement(inhLp, inhSc);
      | T.CASESym:
          synthStat := caseStatement(inhLp, inhSc);
      | T.WHILESym:
          synthStat := whileStatement(inhLp, inhSc);
      | T.REPEATSym:
          synthStat := repeatStatement(inhLp, inhSc);
      | T.FORSym:
          synthStat := forStatement(inhLp, inhSc);
      | T.WITHSym:
          synthStat := withStatement(inhLp, inhSc);
      | T.NEWSym :
          synthStat := newStatement(inhSc);
      | T.EXITSym:
         (* Semantic action is inline *)
          Get;
          synthStat := StatDesc.newExitS(inhLp);
          IF inhLp = NIL THEN SemError(58) END;
      | T.RETURNSym :
         (* Semantic action is inline *)
          Get;
          IF in(symSet[3], nextT.sym) THEN
            synthExpr := expression(inhSc);
          ELSE
            synthExpr := NIL;
          END;
          synthStat := StatDesc.newReturnS(synthExpr);
          synthStat.token := keywordTk;
      ELSE synthStat := StatDesc.newEmptyS();

      END;
    ELSE
      synthStat := StatDesc.newEmptyS();
    END;
    RETURN synthStat;
  END statement;

(* ==================================================================== *)

  PROCEDURE statementSequence(inhLp : Sy.Stmt; inhSc : Sy.Scope) : Sy.Stmt;
    VAR block : StatDesc.Block;
        first : Sy.Stmt;
  BEGIN
    WHILE ~ (in(symSet[4], nextT.sym)) DO Error(80); Get END;
    first := statement(inhLp, inhSc);
    block := NIL;
    WHILE weakSeparator(22, 5, 6) DO
      WHILE ~ (in(symSet[4], nextT.sym)) DO Error(81); Get END;
      IF block = NIL THEN
        block := StatDesc.newBlockS(first.token);
        IF first.kind # StatDesc.emptyS THEN Sy.AppendStmt(block.sequ,first) END
      END;
      first := statement(inhLp, inhSc);
      IF first.kind # StatDesc.emptyS THEN Sy.AppendStmt(block.sequ,first) END;
    END;
    IF block = NIL THEN RETURN first ELSE RETURN block END;
  END statementSequence;

(* ==================================================================== *)

  PROCEDURE  element(defScp : Sy.Scope) : Sy.Expr;
    VAR rslt : Sy.Expr;
        xTop : Sy.Expr;
        dTok : S.Token;
  BEGIN
    rslt := expression(defScp);
    IF nextT.sym = T.pointpointSym THEN (* a range *)
      Get; dTok := token;
      xTop := expression(defScp);
      rslt := Xp.newBinaryT(Xp.range, rslt, xTop, dTok);;
    END;
    RETURN rslt;
  END element;

(* ==================================================================== *)

  PROCEDURE  set(defScp : Sy.Scope) : Sy.Expr;
    VAR rslt : Xp.SetExp;
  BEGIN
    Expect(T.lbraceSym);
    rslt := Xp.mkEmptySet();
    IF in(symSet[3], nextT.sym) THEN
      Sy.AppendExpr(rslt.varSeq, element(defScp));
      WHILE nextT.sym = T.commaSym DO
        Get;
        Sy.AppendExpr(rslt.varSeq, element(defScp));
      END;
    END;
    Expect(T.rbraceSym);
    RETURN rslt;
  END set;

(* ==================================================================== *)

  PROCEDURE  mulOperator() : INTEGER;
    VAR oSyn : INTEGER;
  BEGIN
    IF (nextT.sym = T.starSym) THEN
      Get;
      oSyn := Xp.mult;
    ELSIF (nextT.sym = T.slashSym) THEN
      Get;
      oSyn := Xp.slash;
    ELSIF (nextT.sym = T.DIVSym) THEN
      Get;
      oSyn := Xp.divOp;
    ELSIF (nextT.sym = T.MODSym) THEN
      Get;
      oSyn := Xp.modOp;
    ELSIF (nextT.sym = T.andSym) THEN
      Get;
      oSyn := Xp.blAnd;
    ELSIF (nextT.sym = T.DIV0Sym) THEN
      Get;
      oSyn := Xp.div0op;
    ELSIF (nextT.sym = T.REM0Sym) THEN
      Get;
      oSyn := Xp.rem0op;
    ELSE
      Error(83); oSyn := T.starSym;
    END;
    RETURN oSyn;
  END mulOperator;

(* ==================================================================== *)

  PROCEDURE  factor(scope : Sy.Scope) : Sy.Expr;
    VAR xSyn : Sy.Expr;
        junk : Sy.ExprSeq;
        long : LONGINT;
        save : S.Token;
  BEGIN
    CASE nextT.sym OF
      T.lbraceSym :
        xSyn := set(scope);
    | T.lparenSym :
        Get;
        xSyn := expression(scope);
        Expect(T.rparenSym);
    | T.integerSym :
        Get;
        xSyn := Xp.mkNumLt(S.tokToLong(token));
    | T.realSym :
        Get;
        xSyn := Xp.mkRealLt(S.tokToReal(token));
    | T.CharConstantSym :
        Get;
        xSyn := Xp.mkCharLt(S.tokToChar(token));
    | T.stringSym :
        Get;
        xSyn := Xp.tokToStrLt(token.pos, token.len);
    | T.bangStrSym :
        Get;
        xSyn := Xp.translateStrLt(token.pos, token.len);
    | T.NILSym :
        Get;
        xSyn := Xp.mkNilX();
    | T.identSym :
        xSyn := designator(scope);
        ConvertOverloaded(xSyn);
        IF (xSyn # NIL) & (xSyn.kind = Xp.prCall) THEN
          SemError(24);   (* use of proper proc as function *)
        END;
    | T.tildeSym :
        Get;
        xSyn := factor(scope);
        xSyn := Xp.newUnaryX(Xp.blNot, xSyn);
    ELSE
      Error(84); xSyn := NIL;
    END;
    RETURN xSyn;
  END factor;

(* ==================================================================== *)

  PROCEDURE  addOperator() : INTEGER;
    VAR oSyn : INTEGER;
  BEGIN
    IF (nextT.sym = T.plusSym) THEN
      Get;
      oSyn := Xp.plus;
    ELSIF (nextT.sym = T.minusSym) THEN
      Get;
      oSyn := Xp.minus;
    ELSIF (nextT.sym = T.ORSym) THEN
      Get;
      oSyn := Xp.blOr;
    ELSE
      Error(85); oSyn := T.plusSym;
    END;
    RETURN oSyn;
  END addOperator;

(* ==================================================================== *)

  PROCEDURE  term(scope : Sy.Scope) : Sy.Expr;
    VAR xSyn1 : Sy.Expr;
        xSyn2 : Sy.Expr;
        mulOp : INTEGER;
        saveT : S.Token;
  BEGIN
    xSyn1 := factor(scope);
    WHILE (nextT.sym = T.starSym) OR
          (nextT.sym = T.slashSym) OR
          (nextT.sym = T.DIVSym) OR
          (nextT.sym = T.MODSym) OR
          (nextT.sym = T.DIV0Sym) OR
          (nextT.sym = T.REM0Sym) OR
          (nextT.sym = T.andSym) DO
      mulOp := mulOperator(); saveT := token;
      xSyn2 := factor(scope);
      xSyn1 := Xp.newBinaryT(mulOp, xSyn1, xSyn2, saveT);
    END;
    RETURN xSyn1;
  END term;

(* ==================================================================== *)

  PROCEDURE  relation() : INTEGER;
    VAR oSyn : INTEGER;
  BEGIN
    CASE nextT.sym OF
    | T.equalSym :
        Get; oSyn := Xp.equal;
    | T.hashSym :
        Get; oSyn := Xp.notEq;
    | T.lessSym :
        Get; oSyn := Xp.lessT;
    | T.lessequalSym :
        Get; oSyn := Xp.lessEq;
    | T.greaterSym :
        Get; oSyn := Xp.greT;
    | T.greaterequalSym :
        Get; oSyn := Xp.greEq;
    | T.INSym :
        Get; oSyn := Xp.inOp;
    | T.ISSym :
        Get; oSyn := Xp.isOp;
    ELSE
      Error(86); oSyn := Xp.equal;
    END;
    RETURN oSyn;
  END relation;

(* ==================================================================== *)

  PROCEDURE  simpleExpression(scope : Sy.Scope) : Sy.Expr;
    VAR opNeg : BOOLEAN;
        addOp : INTEGER;
        term1 : Sy.Expr;
        term2 : Sy.Expr;
        saveT : S.Token;
  BEGIN
    opNeg := FALSE;
    IF nextT.sym = T.minusSym THEN
      Get; opNeg := TRUE;
    ELSIF nextT.sym = T.plusSym THEN
      Get;
    END;
    term1 := term(scope);
    IF opNeg THEN term1 := Xp.newUnaryX(Xp.neg, term1) END;
    WHILE (nextT.sym = T.minusSym) OR
          (nextT.sym = T.plusSym) OR
          (nextT.sym = T.ORSym) DO
      addOp := addOperator(); saveT := token;
      term2 := term(scope);
      term1 := Xp.newBinaryT(addOp, term1, term2, saveT);
    END;
    RETURN term1;
  END simpleExpression;

(* ==================================================================== *)

  PROCEDURE  OptExprList(VAR xList : Sy.ExprSeq; inhScp : Sy.Scope);
  BEGIN
    IF in(symSet[3], nextT.sym) THEN
      ExprList(xList, inhScp);
    ELSE      (* empty list *)
      xList.ResetTo(0);
    END;
  END OptExprList;

(* ==================================================================== *)

  PROCEDURE  ExprList(VAR xList : Sy.ExprSeq; inhScp : Sy.Scope);
  BEGIN
   (*
    *  To avoid aliassing, ALWAYS Discard old sequence.
    *)
    Sy.InitExprSeq(xList, 4);
    Sy.AppendExpr(xList, expression(inhScp));
    WHILE (nextT.sym = T.commaSym) DO
      Get;
      Sy.AppendExpr(xList, expression(inhScp));
    END;
  END ExprList;

(* ==================================================================== *)

  PROCEDURE findMatchingProcs(oId      : Id.OvlId; 
                              actuals  : Sy.ExprSeq;
                              VAR rslt : Id.PrcSeq);
  VAR
    index : INTEGER;
    visited : Id.PrcSeq;
    rec : Ty.Record;
    id : Sy.Idnt;
    prcTy : Ty.Procedure;
    finished : BOOLEAN;

    PROCEDURE seen(newP : Ty.Procedure; visited : Id.PrcSeq) : BOOLEAN;
    VAR
      index : INTEGER;
    BEGIN
      FOR index := 0 TO visited.tide-1 DO
        IF newP.sigsMatch(visited.a[index].type) THEN RETURN TRUE; END;
      END;
      RETURN FALSE;
    END seen;

  BEGIN
    Id.InitPrcSeq(rslt,1);
    Id.InitPrcSeq(visited,5);
    rec := oId.rec(Ty.Record);
    id := oId;
    finished := id = NIL;
    WHILE ~finished & (id # NIL) DO
      WITH id : Id.OvlId DO
        FOR index := 0 TO id.list.tide-1 DO
          prcTy := id.list.a[index].type(Ty.Procedure);
          IF Xp.MatchPars(prcTy.formals,actuals) & ~seen(prcTy,rslt) THEN
            Id.AppendProc(rslt,id.list.a[index]);
          END;
        END;
      | id : Id.Procs DO
          prcTy := id.type(Ty.Procedure);
          IF Xp.MatchPars(prcTy.formals,actuals) & ~seen(prcTy,rslt) THEN
            Id.AppendProc(rslt,id);
          END;
          finished := TRUE;
      ELSE
        finished := TRUE;
      END;
      IF (rec.baseTp = NIL) OR (rec.baseTp = Ty.anyRecTp) THEN
        finished := TRUE;
      ELSE
        rec := rec.baseTp.boundRecTp()(Ty.Record);
        id := rec.symTb.lookup(oId.hash);
      END;
    END;
  END findMatchingProcs;

  PROCEDURE FindBestMatch(IN actuals : Sy.ExprSeq; IN procs : Id.PrcSeq;
                           OUT match : BOOLEAN; OUT ix : INTEGER);
  VAR pIx : INTEGER;
      pTy : Ty.Procedure;

    PROCEDURE IsSameAs(lhs : Sy.Type; rhs : Sy.Type) : BOOLEAN;
    BEGIN
      IF lhs = rhs THEN RETURN TRUE;
      ELSE RETURN lhs.equalType(rhs);
      END;
    END IsSameAs;

    PROCEDURE IsSameWithNativeCoercions(lhs : Sy.Type; rhs : Sy.Type) : BOOLEAN;
    BEGIN
      IF lhs = rhs THEN RETURN TRUE;
      ELSIF lhs.isStringType() & rhs.isStringType() THEN RETURN TRUE;
      ELSIF lhs.isNativeObj() & rhs.isNativeObj() THEN RETURN TRUE;
      ELSE RETURN lhs.equalType(rhs);
      END;
    END IsSameWithNativeCoercions;



  BEGIN
    match := FALSE;
    ix := 0;  
    WHILE ~match & (ix < procs.tide) DO
      pIx := 0; match := TRUE;
      WHILE match & (pIx < actuals.tide) DO
        pTy := procs.a[ix].type(Ty.Procedure);
        match := IsSameAs(actuals.a[pIx].type, pTy.formals.a[pIx].type);
        INC(pIx);
      END;
      IF ~match THEN INC(ix) ELSE RETURN END;
    END;
    ix := 0;  
    WHILE ~match & (ix < procs.tide) DO
      pIx := 0; match := TRUE;
      WHILE match & (pIx < actuals.tide) DO
        pTy := procs.a[ix].type(Ty.Procedure);
        match := IsSameWithNativeCoercions(actuals.a[pIx].type, pTy.formals.a[pIx].type);
        INC(pIx);
      END;
      IF ~match THEN INC(ix) END;
    END;
    IF ~match THEN ix := 0 END;
  END FindBestMatch;
                              
(* ==================================================================== *)

  PROCEDURE makeCall(xCr        : Sy.Expr; 
                     IN actuals : Sy.ExprSeq;
                     inhScp     : Sy.Scope) : Sy.Expr;
  VAR
    procs : Id.PrcSeq;
    moreThanOne, found : BOOLEAN;
    oId : Id.OvlId;
    prcTy : Ty.Procedure;
    index, pIx, err : INTEGER;
    nam : LitValue.CharOpen;

   (* ------------------------- *)

    PROCEDURE RepMulErr(eNo : INTEGER; 
                        pNam : LitValue.CharOpen; 
                        frmSeq : Id.ParSeq);
    VAR
      ix : INTEGER;
      len : INTEGER;
      par : Id.ParId;
      cSeq : LitValue.CharOpenSeq;

    BEGIN
      LitValue.InitCharOpenSeq(cSeq,3);
      LitValue.AppendCharOpen(cSeq,pNam);
      LitValue.AppendCharOpen(cSeq,LitValue.strToCharOpen("("));
      len := frmSeq.tide - 1;
      FOR ix := 0 TO len DO
        par := frmSeq.a[ix];
        LitValue.AppendCharOpen(cSeq,par.type.name());
        IF ix < len THEN LitValue.AppendCharOpen(cSeq,comma) END;
      END;
      LitValue.AppendCharOpen(cSeq,LitValue.strToCharOpen(")"));
      S.SemError.RepSt1(eNo, LitValue.arrayCat(cSeq)^, token.lin, 0);
    END RepMulErr;

   (* ------------------------- *)

    PROCEDURE CheckSuper(xIn : Xp.IdentX);
      VAR fld : Sy.Idnt;      (* Selector identifier  *)
          sId : Sy.Idnt;      (* Super method ident *)
          mth : Id.MthId;     (* Method identifier  *)
          rcT : Ty.Record;    (* Method bound recType *)
    BEGIN
      fld := xIn.ident;
(*
 *    Console.WriteLn;
 *    fld.Diagnose(0);
 *    Console.WriteLn;
 *)
      IF (fld.kind # Id.conMth) & (fld.kind # Id.fwdMth) THEN
        SemError(119);      (* super call invalid   *)
      ELSE          (* OK, fld is a method  *)
       (* Find the receiver type, and check in scope of base type. *)
        mth := fld(Id.MthId);
        rcT := mth.bndType(Ty.Record);
        IF (rcT # NIL) &
           (rcT.baseTp # NIL) &
           (rcT.baseTp.kind = Ty.recTp) THEN
         (*
          *  Bind to the overridden method, not necessarily
          *  defined in the immediate supertype.
          *)
          sId := rcT.baseTp(Ty.Record).bindField(fld.hash);
         (*
          *  Inherited method could be overloaded
          *  Find single sId that matches mth
          *)
          IF (sId # NIL) & (sId IS Id.OvlId) THEN
            sId := sId(Id.OvlId).findProc(mth);
(*
 *          IF sId # NIL THEN
 *            Console.WriteLn;
 *            sId.Diagnose(0);
 *            Console.WriteLn;
 *          END;
 *)
          END;
         (*
          *  Now check various semantic constraints
          *)
          IF (sId # NIL) & (sId IS Id.MthId) THEN
            IF sId(Id.MthId).mthAtt * Id.mask # Id.extns THEN
              SemError(118);    (* call empty or abstract *)
            ELSE
              xIn.ident := sId;
              xIn.type  := sId.type;
            END;
          ELSE
            SemError(120);      (* unknown super method *)
          END;
        ELSE
          SemError(120);      (* unknown super method *)
        END;
      END;
    END CheckSuper;

   (* ------------------------- *)
  BEGIN
    moreThanOne := FALSE;
    IF (xCr = NIL) OR (xCr.type = NIL) OR (xCr IS Xp.CallX) THEN
      RETURN xCr;
    END;
    pIx := 0;
    IF xCr.type.kind = Ty.ovlTp THEN
      oId := xCr.type.idnt(Id.OvlId);
      nam := Sy.getName.ChPtr(oId);
      Xp.AttributePars(actuals);
      findMatchingProcs(oId,actuals,procs);
      IF procs.tide = 0 THEN
        SemError(218);
        RETURN NIL;
      ELSIF procs.tide > 1 THEN
        FindBestMatch(actuals,procs,found,pIx);
        IF ~found THEN err := 220 ELSE err := 312 END;
        FOR index := 0 TO procs.tide-1 DO
          IF ~(found & (index = pIx)) THEN (* just for info *)
            RepMulErr(err,nam,procs.a[index].type(Ty.Procedure).formals);
          END;
        END;
        IF found THEN
          RepMulErr(313,nam,procs.a[pIx].type(Ty.Procedure).formals);
          SemError(307);
        ELSE
          SemError(219);
          RETURN NIL;
        END;
      END;
      WITH xCr : Xp.IdLeaf DO
        xCr.ident := procs.a[pIx];
        xCr.type := xCr.ident.type;
      | xCr : Xp.IdentX DO
        xCr.ident := procs.a[pIx];
        xCr.type := xCr.ident.type;
      END;
    END;
   (*
    *   Overloading (if any) is by now resolved.
    *   Now check for super calls.  See if 
    *   we can find a match for xCr.ident
    *   in the supertype of 
    *   -  xCr.kid.ident(Id.MthId).bndType
    *)
    IF xCr.kind = Xp.sprMrk THEN
      CheckSuper(xCr(Xp.IdentX));
    END;
   (*
    *   Now create CallX node in tree.
    *)
    IF (xCr.type.kind # Ty.prcTp) &
       (xCr.type.kind # Ty.evtTp) THEN
      xCr.ExprError(224); RETURN NIL;
    ELSIF xCr.type.isProperProcType() THEN
      xCr := Xp.newCallX(Xp.prCall, actuals, xCr);
      xCr.NoteCall(inhScp);  (* note "inhScp" calls "eSyn.kid" *)
    ELSE
      xCr := Xp.newCallX(Xp.fnCall, actuals, xCr);
      xCr := Xp.checkCall(xCr(Xp.CallX));
      IF (xCr # NIL) THEN
        xCr.NoteCall(inhScp);  (* note "inhScp" calls "eSyn.kid" *)
        IF (xCr IS Xp.CallX) & (xCr(Xp.CallX).kid.kind = Xp.sprMrk) THEN
          Xp.CheckSuper(xCr(Xp.CallX), inhScp);
        END;
      END;
    END;
    RETURN xCr;
  END makeCall;

   (* ------------------------- *)

  PROCEDURE findFieldId(id : Id.OvlId) : Sy.Idnt;
  VAR
    fId : Sy.Idnt;
    rec : Ty.Record;
    ident : Sy.Idnt;
  BEGIN
    IF id = NIL THEN RETURN NIL END;
    fId := id.fld;
    rec := id.rec(Ty.Record);
    WHILE (fId = NIL) & (rec.baseTp # NIL) & (rec.baseTp IS Ty.Record) DO
      rec := rec.baseTp(Ty.Record);
      ident := rec.symTb.lookup(id.hash);
      IF ident IS Id.OvlId THEN
        fId := ident(Id.OvlId).fld;
      ELSIF (ident.kind = Id.fldId) OR (ident.kind = Id.varId) OR
            (ident.kind = Id.conId) THEN
        fId := ident;
      END;
    END;
    RETURN fId;
  END findFieldId;

   (* ------------------------- *)

  PROCEDURE FindOvlField(e : Sy.Expr);
  BEGIN
    ASSERT(e.type.kind = Ty.ovlTp);
    WITH e : Xp.IdentX DO
      e.ident := findFieldId(e.ident(Id.OvlId));
      IF e.ident = NIL THEN
        e.ExprError(9);
      ELSE
        e.type := e.ident.type;
      END;
    | e : Xp.IdLeaf DO
      e.ident := findFieldId(e.ident(Id.OvlId));
      IF e.ident = NIL THEN
        e.ExprError(9);
      ELSE
        e.type := e.ident.type;
      END;
    END;
  END FindOvlField;

   (* ------------------------- *)

  PROCEDURE ConvertOverloaded(VAR e : Sy.Expr);
  BEGIN
    IF (e # NIL) & (e.type IS Ty.Overloaded) THEN
(*
 *    WITH e : Xp.IdentX DO
 *      e.ident := e.ident(Id.OvlId).fld;
 *      IF (e.ident = NIL) THEN
 *        SemErrorT(9, e.token);
 *      ELSE
 *        e.type := e.ident.type;
 *      END;
 *    END;
 *)
      WITH e : Xp.IdentX DO
          e.ident := e.ident(Id.OvlId).fld;
          IF (e.ident = NIL) THEN
            SemErrorT(9, e.token);
          ELSE
            e.type := e.ident.type;
          END;
      | e : Xp.IdLeaf DO
          e.ident := e.ident(Id.OvlId).fld;
          IF (e.ident = NIL) THEN
            SemErrorT(9, e.token);
          ELSE
            e.type := e.ident.type;
          END;
      END;
    END;
  END ConvertOverloaded;

(* ==================================================================== *)

  PROCEDURE MethAttributes(pDesc : Id.Procs);
    VAR mAtt : SET;
        hash : INTEGER;
   (* ------------------------- *)
    PROCEDURE CheckBasecall(proc : Id.Procs);
      VAR idx : INTEGER;
          rec : Sy.Type;
          bRc : Ty.Record;
          sId : Sy.Idnt;
          bas : Id.BaseCall;
          sTp : Ty.Procedure;
          seq : Id.PrcSeq;
          mOk : BOOLEAN;
    BEGIN
      bRc := NIL;
      bas := proc.basCll;
      rec := proc.type.returnType();
      IF rec # NIL THEN rec := rec.boundRecTp(); bRc := rec(Ty.Record) END;
      IF rec # NIL THEN rec := rec(Ty.Record).baseTp END;
      IF rec # NIL THEN rec := rec.boundRecTp() END;
     (*
      *  Compute the apparent type of each actual.
      *)
      FOR idx := 0 TO bas.actuals.tide - 1 DO
        bas.actuals.a[idx] := bas.actuals.a[idx].exprAttr();
      END;
     (*
      *  Now try to find matching super-ctor.
      *  IF there are not actuals, then assume the existence of
      *  a noarg constructor.  TypeDesc.okToList will check this!
      *)
      IF bas.actuals.tide # 0 THEN 
        WITH rec : Ty.Record DO
          FOR idx := 0 TO rec.statics.tide - 1 DO
            sId := rec.statics.a[idx];
           (*
            *  If this is a .ctor, then try to match arguments ...
            *)
            IF sId.kind = Id.ctorP THEN
              sTp := sId.type(Ty.Procedure);
              IF  Xp.MatchPars(sTp.formals, bas.actuals) THEN
                Id.AppendProc(seq, sId(Id.Procs));
              END;
            END;
          END;
        END;
      ELSE
        Id.AppendProc(seq, NIL);
      END;
      IF seq.tide = 0 THEN SemError(202);
      ELSIF seq.tide = 1 THEN bas.sprCtor := seq.a[0];
      ELSE 
        FindBestMatch(bas.actuals, seq, mOk, idx);
        IF mOk THEN bas.sprCtor := seq.a[idx] ELSE SemError(147) END;
      END;
      IF bRc # NIL THEN 
        Sy.AppendIdnt(bRc.statics, proc);
       (*
        *  And, while we are at it, if this is a no-arg
        *  constructor, suppress emission of the default.
        *)
        IF proc.locals.tide = 1 THEN INCL(bRc.xAttr, Sy.xCtor) END;
      END;
    END CheckBasecall;
   (* ------------------------- *)
    PROCEDURE DummyParameters(VAR seq : Sy.ExprSeq; prT : Ty.Procedure);
      VAR idx : INTEGER;
          idl : Xp.IdLeaf;
    BEGIN
      FOR idx := 0 TO prT.formals.tide - 1 DO
        idl := Xp.mkIdLeaf(prT.formals.a[idx]);
        idl.type := idl.ident.type;
        Sy.AppendExpr(seq, idl);
      END;
    END DummyParameters;
   (* ------------------------- *)
    PROCEDURE InsertSelf(prc : Id.Procs);
      VAR par : Id.ParId;
          tmp : Sy.IdSeq;
          idx : INTEGER;
    BEGIN
      par := Id.newParId();
      par.hash := Bi.selfBk;
      par.dfScp := prc;
      par.parMod := Sy.in; (* so it is read only *)
      par.varOrd := 0;     (* both .NET and JVM  *)
      par.type := prc.type.returnType();
      ASSERT(prc.symTb.enter(par.hash, par));
     (*
      *  Now adjust the locals sequence.
      *)
      Sy.AppendIdnt(tmp, par);
      FOR idx := 0 TO prc.locals.tide-1 DO
        Sy.AppendIdnt(tmp, prc.locals.a[idx]);
        prc.locals.a[idx](Id.AbVar).varOrd := idx+1;
      END;
      prc.locals := tmp;
    END InsertSelf;
   (* ------------------------- *)
  BEGIN
    mAtt := {};
    IF nextT.sym = T.NEWSym THEN
      Get;
      mAtt := Id.isNew;
      IF nextT.sym = T.commaSym THEN Get; mAtt := otherAtts(mAtt) END;
    ELSIF nextT.sym = T.identSym THEN
      hash := NameHash.enterSubStr(nextT.pos, nextT.len);
      IF (hash = Bi.constB) OR (hash = Bi.basBkt) THEN 
        Get;
        IF G.strict THEN SemError(221); END;
        NEW(pDesc.basCll);
        IF hash = Bi.basBkt THEN 
          pDesc.basCll.empty := FALSE;
          ActualParameters(pDesc.basCll.actuals, pDesc);
         (*
          *  Insert the arg0 identifier "SELF"
          *)
          InsertSelf(pDesc);
        ELSE
          pDesc.basCll.empty := TRUE;
          DummyParameters(pDesc.basCll.actuals, pDesc.type(Ty.Procedure));
        END;
        CheckBasecall(pDesc);
        pDesc.SetKind(Id.ctorP);
      END;
    ELSIF (nextT.sym = T.ABSTRACTSym)   OR
          (nextT.sym = T.EXTENSIBLESym) OR
          (nextT.sym = T.EMPTYSym) THEN
      mAtt := otherAtts({});
    ELSE
      Error(78);
    END;
    IF pDesc IS Id.MthId THEN
      pDesc(Id.MthId).mthAtt := mAtt;
      IF pDesc.kind = Id.ctorP THEN SemError(146) END;
    ELSIF pDesc.kind # Id.ctorP THEN
      SemError(61);
    END;
  END MethAttributes;

(* ==================================================================== *)

  PROCEDURE getTypeAssertId(lst : Sy.ExprSeq) : Sy.Idnt;
  VAR
    lf : Xp.IdLeaf;
  BEGIN
    IF (lst.tide = 1) & (lst.a[0] IS Xp.IdLeaf) THEN
      lf := lst.a[0](Xp.IdLeaf);
      IF lf.ident IS Id.TypId THEN RETURN lf.ident; END;
    END;
    RETURN NIL;
  END getTypeAssertId;

(* ==================================================================== *)

  PROCEDURE  designator(inhScp : Sy.Scope) : Sy.Expr;
    VAR eSyn : Sy.Expr;     (* the synthesized expression attribute *)
        qual : Sy.Idnt;     (* the base qualident of the designator *)
        iLst : Sy.ExprSeq;  (* a list of array index expressions  *)
        exTp : Sy.Type;
        isTp : BOOLEAN;

   (* ------------------------- *)

    PROCEDURE implicitDerefOf(wrkX : Sy.Expr) : Sy.Expr;
    (* Make derefs explicit, returning NIL if invalid pointer type. *)
      VAR wrkT : Sy.Type;
          bndT : Sy.Type;
    save : S.Token;
    BEGIN
      IF (wrkX # NIL) &
         (wrkX.type # NIL) THEN
        wrkT := wrkX.type;
        WITH wrkT : Ty.Pointer DO
            bndT := wrkT.boundTp;
            IF bndT = NIL THEN RETURN NIL END;
            save := wrkX.token;
            wrkX := Xp.newUnaryX(Xp.deref, wrkX);
            wrkX.token := save;   (* point to the same token  *)
            wrkX.type  := bndT;   (* type is bound type of ptr. *)
        | wrkT : Ty.Base DO
            IF wrkT = Ty.anyPtrTp THEN
              save := wrkX.token;
              wrkX := Xp.newUnaryX(Xp.deref, wrkX);
              wrkX.token := save;
              wrkX.type  := Ty.anyRecTp;
            END;
        | wrkT : Ty.Event DO
            wrkX.type := wrkT.bndRec;
        ELSE (* skip *)
        END;
      END;
      RETURN wrkX;
    END implicitDerefOf;

   (* ------------------------- *)

    PROCEDURE checkRecord(xIn : Sy.Expr;        (* referencing expression *)
                          tok : S.Token;        (* field/procedure ident  *)
                          scp : Sy.Scope;       (* current scope of ref.  *)
                          tId : BOOLEAN) : Sy.Expr; (* left context is tp *)
      VAR fId : Sy.Idnt;    (* the field identifier desc. *)
          xNw : Sy.Expr;
     (* ------------------------- *)
      PROCEDURE Rep162(f : Sy.Idnt);
      BEGIN SemErrorS1(162, Sy.getName.ChPtr(f)) END Rep162;
     (* ------------------------- *)
    BEGIN       (* quit at first trouble sign *)
      ConvertOverloaded(xIn);
      xNw := implicitDerefOf(xIn);
      IF (xNw = NIL) OR (xNw.type = NIL) THEN RETURN NIL END;
      IF (xNw.type.kind # Ty.recTp) &
         (xNw.type.kind # Ty.enuTp) THEN SemError(8); RETURN NIL END;
      fId := bindFieldToken(xNw.type, tok);
      IF fId = NIL THEN
        SemErrorS1(9, xIn.type.name()); RETURN NIL;
      ELSE
        IF tId THEN (* fId must be a foreign, static feature! *)
          IF fId IS Id.FldId THEN SemError(196) END;
          IF fId IS Id.MthId THEN SemError(197) END;
          xNw := Xp.mkIdLeaf(fId);
        ELSE
          WITH fId : Id.VarId DO SemError(198);
             | fId : Id.PrcId DO SemError(199);
             | fId : Id.MthId DO
                 IF fId.callForbidden() THEN SemErrorT(127, tok) END;
(*
 *               IF (fId.vMod = Sy.rdoMode) &
 *                  xNw.type.isImportedType() THEN SemErrorT(127, tok) END;
 *)

          ELSE (* skip *)
          END;
          xNw := Xp.newIdentX(Xp.selct, fId, xNw);
        END;
        IF fId.vMod = Sy.protect THEN
         (*
          *  If fId is a protected feature (and hence
          *  foreign) then the context must be a method
          *  body.  Furthermore, the method scope must
          *  be derived from the field's defining scope.
          *)
          WITH scp : Id.MthId DO
            IF ~xIn.type.isBaseOf(scp.rcvFrm.type) THEN Rep162(fId) END;
          ELSE
            Rep162(fId);
          END;
        END;
        IF (fId.type # NIL) &
           (fId.type IS Ty.Opaque) THEN
           (* ------------------------------------------- *
            *  Permanently fix the field type attribute.
            * ------------------------------------------- *)
          fId.type := fId.type.elaboration();
        END;
        xNw.type := fId.type;
        RETURN xNw;
      END;
    END checkRecord;

   (* ------------------------- *)

    PROCEDURE checkArray(xCr : Sy.Expr; IN seq : Sy.ExprSeq) : Sy.Expr;
      VAR xTp : Sy.Type;    (* type of current expr xCr *)
          aTp : Ty.Array;   (* current array type of expr *)
          iCr : Sy.Expr;    (* the current index expression *)
          idx : INTEGER;    (* index into expr. sequence  *)
          tok : S.Token;
    BEGIN       (* quit at first trouble sign *)
      ConvertOverloaded(xCr);
      tok := xCr.token;
      FOR idx := 0 TO seq.tide-1 DO
        xCr := implicitDerefOf(xCr);
        IF xCr # NIL THEN xTp := xCr.type ELSE RETURN NIL END;
(* ----------- * 
 *      IF xTp.kind # Ty.arrTp THEN
 *        IF idx = 0 THEN xCr.ExprError(10) ELSE xCr.ExprError(11) END;
 *        RETURN NIL;
 *      ELSE
 *        aTp := xTp(Ty.Array);
 *      END;
 * ----------- *) 
        WITH xTp : Ty.Array DO
          aTp := xTp(Ty.Array);
        ELSE
          IF idx = 0 THEN xCr.ExprError(10) ELSE xCr.ExprError(11) END;
          RETURN NIL;
        END;
(* ----------- *) 
        xTp := aTp.elemTp;
        iCr := seq.a[idx];
        IF iCr # NIL THEN iCr := iCr.exprAttr() END;
        IF iCr # NIL THEN    (* check is integertype , literal in range *)
          IF ~iCr.isIntExpr() THEN iCr.ExprError(31) END;
          IF iCr.type = Bi.lIntTp THEN
            iCr := Xp.newIdentX(Xp.cvrtDn, Bi.intTp.idnt, iCr);
          END;
          IF iCr.isNumLit() & ~iCr.inRangeOf(aTp) THEN iCr.ExprError(32) END;
          tok := iCr.token;
        END;
        xCr := Xp.newBinaryT(Xp.index, xCr, iCr, tok);
        IF xTp # NIL THEN xCr.type := xTp ELSE RETURN NIL END;
      END;
      RETURN xCr;
    END checkArray;

   (* ------------------------- *)

    PROCEDURE checkTypeAssert(xpIn : Sy.Expr; tpId : Sy.Idnt) : Sy.Expr;
      VAR dstT : Sy.Type;
          recT : Ty.Record;
    BEGIN
      IF xpIn.type.kind = Ty.ovlTp THEN FindOvlField(xpIn); END;
      IF (xpIn = NIL) OR (tpId = NIL) OR (tpId.type = NIL) THEN RETURN NIL END;
      dstT := tpId.type;
      recT := dstT.boundRecTp()(Ty.Record);
     (* Check #1 : qualident must be a [possibly ptr to] record type  *)
      IF recT = NIL THEN SemError(18); RETURN NIL END;
     (* Check #2 : Check that the expression has some dynamic type  *)
      IF ~xpIn.hasDynamicType() THEN xpIn.ExprError(17); RETURN NIL END;
      IF dstT.kind = Ty.recTp THEN xpIn := implicitDerefOf(xpIn) END;
     (* Check #3 : Check that manifest type is a base of asserted type  *)
      IF G.extras THEN
        IF ~xpIn.type.isBaseOf(dstT) &
           ~xpIn.type.isInterfaceType() &
           ~dstT.isInterfaceType() &
           ~(dstT.isCompoundType() & recT.compoundCompat(xpIn.type) ) &
           ~dstT.isEventType() THEN SemError(15); RETURN NIL END;

      ELSE
        IF ~xpIn.type.isBaseOf(dstT) &
           ~xpIn.type.isInterfaceType() &
           ~dstT.isInterfaceType() &
           ~dstT.isEventType() &
           ~Ty.isBoxedStruct(xpIn.type, dstT) THEN SemError(15); RETURN NIL END;
      END; (* IF G.extras *)
     (* Geez, it seems to be ok! *)
      xpIn := Xp.newUnaryX(Xp.tCheck, xpIn);
      xpIn.type := dstT;
      RETURN xpIn;
    END checkTypeAssert;

   (* ------------------------- *)

    PROCEDURE mkSuperCall(xIn : Sy.Expr) : Sy.Expr;
      VAR new : Sy.Expr;
    BEGIN
      new := NIL;
      WITH xIn : Xp.IdentX DO
        new := Xp.newIdentX(Xp.sprMrk, xIn.ident, xIn.kid);
        new.type := xIn.ident.type;
      ELSE
        SemError(119);        (* super call invalid   *)
      END;
      RETURN new;
    END mkSuperCall;

   (* ------------------------- *)

    PROCEDURE stringifier(xIn : Sy.Expr) : Sy.Expr;
    BEGIN
      xIn := implicitDerefOf(xIn);
      IF xIn.isCharArray() THEN
        xIn := Xp.newUnaryX(Xp.mkStr, xIn);
        xIn.type := Bi.strTp;
      ELSE
        SemError(41); RETURN NIL;
      END;
      RETURN xIn;
    END stringifier;

   (* ------------------------- *)

    PROCEDURE explicitDerefOf(wrkX : Sy.Expr) : Sy.Expr;
    (* Make derefs explicit, returning NIL if invalid pointer type. *)
      VAR expT, bndT : Sy.Type;
    BEGIN
      expT := wrkX.type;
      WITH expT : Ty.Pointer DO
          bndT := expT.boundTp;
          IF bndT = NIL THEN RETURN NIL END;
          wrkX := Xp.newUnaryX(Xp.deref, wrkX);
          wrkX.type  := bndT;     (* type is bound type of ptr. *)
      | expT : Ty.Base DO
          IF expT = Ty.anyPtrTp THEN
            wrkX := Xp.newUnaryX(Xp.deref, wrkX);
            wrkX.type  := Ty.anyRecTp;  (* type is bound type of ptr. *)
          ELSE
            SemError(12); RETURN NIL; (* expr. not a pointer type *)
          END;
      | expT : Ty.Overloaded DO RETURN mkSuperCall(wrkX);
      | expT : Ty.Procedure  DO RETURN mkSuperCall(wrkX);
(*
 *    | expT : Ty.Procedure DO
 *        RETURN checkSuperCall(wrkX);
 *)
      ELSE
        SemError(12); RETURN NIL; (* expr. not a pointer type *)
      END;
      RETURN wrkX;
    END explicitDerefOf;

   (* ------------------------- *)

    PROCEDURE ReportIfOpaque(exp : Sy.Expr);
    BEGIN
      IF (exp # NIL) &
         (exp.type # NIL) &
         (exp.type.kind = Ty.namTp) &
         (exp.type.idnt # NIL) &
         (exp.type.idnt.dfScp # NIL) &
          exp.type.idnt.dfScp.isWeak() THEN
        SemErrorS1(176, Sy.getName.ChPtr(exp.type.idnt.dfScp));
      END;
    END ReportIfOpaque;

   (* ------------------------- *)

  BEGIN         (* body of designator *)
   (* --------------------------------------------------------- *
    *  First deal with the qualified identifier part.   *
    * --------------------------------------------------------- *)
    qual := qualident(inhScp);
    IF (qual # NIL) & (qual.type # NIL) THEN
      eSyn := Xp.mkIdLeaf(qual);
      eSyn.type := qual.type;
      isTp := qual IS Id.TypId;
    ELSE
      eSyn := NIL;
      isTp := FALSE;
    END;
   (* --------------------------------------------------------- *
    *  Now deal with each selector, in sequence, by a loop.     *
    *  It is an invariant of this loop, that if eSyn # NIL,     *
    *  the expression has a valid, non-NIL type value.          *
    * --------------------------------------------------------- *)
    WHILE (nextT.sym = T.pointSym)   OR
          (nextT.sym = T.lparenSym)  OR
          (nextT.sym = T.lbrackSym)  OR
          (nextT.sym = T.uparrowSym) OR
          (nextT.sym = T.dollarSym)  DO
     (* ------------------------------------------------------- *
      *  If this is an opaque, resolve it if possible
      * ------------------------------------------------------- *)
      IF (eSyn # NIL) & (eSyn.type IS Ty.Opaque) THEN
        eSyn.type := eSyn.type.elaboration();
        IF eSyn.type IS Ty.Opaque THEN ReportIfOpaque(eSyn) END;
      END;
     (* ------------------------------------------------------- *
      *  If expr is typeName, must be static feature selection
      * ------------------------------------------------------- *)
      IF isTp &
         (eSyn # NIL) &
         (eSyn IS Xp.IdLeaf) &
         (nextT.sym # T.pointSym) THEN eSyn.ExprError(85) END;

      IF nextT.sym = T.pointSym THEN
     (* ------------------------------------------------------- *
      *  This must be a field selection, or a method call
      * ------------------------------------------------------- *)
        Get;
        Expect(T.identSym);
       (* Check that this is a valid record type. *)
        IF eSyn # NIL THEN eSyn := checkRecord(eSyn, token, inhScp, isTp) END;
        isTp := FALSE;  (* clear the flag *)
      ELSIF (nextT.sym = T.lbrackSym) THEN
     (* ------------------------------------------------------- *
      *  This must be a indexed selection on an array type
      * ------------------------------------------------------- *)
        Get;
        ExprList(iLst, inhScp);
        Expect(T.rbrackSym);
       (* Check that this is a valid array type. *)
        IF eSyn # NIL THEN eSyn := checkArray(eSyn, iLst) END;
      ELSIF (nextT.sym = T.lparenSym) THEN
     (* -------------------------------------------------------------- *
      *  This could be a function/procedure call, or a type assertion  *
      * -------------------------------------------------------------- *)
        Get;
        OptExprList(iLst, inhScp);
        IF eSyn # NIL THEN
          qual := getTypeAssertId(iLst);
          IF (qual # NIL) & ~eSyn.isStdFunc() THEN
           (* 
            *  This must be a type test, so ...
            *
            *  This following test is inline in checkTypeAssert()
            *     IF eSyn.type.kind = Ty.ovlTp THEN FindOvlField(eSyn); END; 
            *)
            eSyn := checkTypeAssert(eSyn,qual);
          ELSIF (eSyn.type.kind = Ty.prcTp) OR 
                (eSyn.type.kind = Ty.ovlTp) OR
                (eSyn.type.kind = Ty.evtTp) THEN
            (* A (possibly overloaded) function/procedure call *)
            eSyn := makeCall(eSyn, iLst, inhScp);
          ELSE          (* A syntax error.  *)
            SemError(13);
            eSyn := NIL;
          END;
        END;
        Expect(T.rparenSym);
        IF (eSyn # NIL) & (eSyn.kind = Xp.prCall) THEN RETURN eSyn; END;
        (* Watch it! that was a semantically selected parser action.  *)
      ELSIF (nextT.sym = T.uparrowSym) THEN
     (* ------------------------------------------------------- *
      *  This can be an explicit dereference or a super call  *
      * ------------------------------------------------------- *)
        Get;
        IF eSyn # NIL THEN eSyn := explicitDerefOf(eSyn) END;
      ELSE
     (* ------------------------------------------------------- *
      *  This can only be an explicit make-string operator
      * ------------------------------------------------------- *)
        Get;
        IF eSyn # NIL THEN eSyn := stringifier(eSyn) END;
     (* ------------------------------------------------------- *)
      END;
    END;
   (* ------------------------------------------------------- *
    *  Some special case cleanup code for enums, opaques...
    * ------------------------------------------------------- *)
    IF eSyn # NIL THEN
      IF isTp THEN
        eSyn.type := Bi.metaTp;
      ELSIF eSyn.type # NIL THEN
        exTp := eSyn.type;
        WITH exTp : Ty.Enum DO
            eSyn.type := Bi.intTp;
        | exTp : Ty.Opaque DO
            eSyn.type := exTp.elaboration();
        ELSE (* skip *)
        END;
      END;
    END;
    RETURN eSyn;
  END designator;

(* ==================================================================== *)

  PROCEDURE FixAnon(defScp : Sy.Scope; tTyp : Sy.Type; mode : INTEGER);
    VAR iSyn : Sy.Idnt;
  BEGIN
    IF (tTyp # NIL) & (tTyp.idnt = NIL) THEN
      iSyn := Id.newAnonId(tTyp.serial);
      iSyn.SetMode(mode);
      tTyp.idnt := iSyn;
      iSyn.type := tTyp;
      ASSERT(CompState.thisMod.symTb.enter(iSyn.hash, iSyn));
    END;
  END FixAnon;

(* ==================================================================== *)

  PROCEDURE  VariableDeclaration(defScp : Sy.Scope);
    VAR vSeq : Sy.IdSeq;  (* idents of the shared type     *)
        tTyp : Sy.Type;   (* the shared variable type desc *)
        indx : INTEGER;
        neId : Sy.Idnt;   (* temp to hold Symbols.Idnet    *)
        vrId : Id.AbVar;  (* same temp, but cast to VarId  *)
        mOut : INTEGER;   (* maximum visibility of idlist  *)
  BEGIN
    IdentDefList(vSeq, defScp, Id.varId);
    CheckVisibility(vSeq, Sy.pubMode, mOut); (* no errors! *)
    Expect(T.colonSym);
    tTyp := type(defScp, mOut);
    IF mOut # Sy.prvMode THEN FixAnon(defScp, tTyp, mOut) END;
(*
 *  Expect(T.colonSym);
 *  tTyp := type(defScp, Sy.prvMode); (* not sure about this? *)
 *)
    FOR indx := 0 TO vSeq.tide-1 DO
      (* this works around a bug in the JVM boot compiler (kjg 7.jan.00) *)
      neId := vSeq.a[indx];
      vrId := neId(Id.AbVar);
      (* ------------------------- *)
      vrId.type := tTyp;
      vrId.varOrd := defScp.locals.tide;
      IF Sy.refused(vrId, defScp) THEN
        vrId.IdError(4);
      ELSE
        Sy.AppendIdnt(defScp.locals, vrId);
      END;
    END;
  END VariableDeclaration;

(* ==================================================================== *)

  PROCEDURE  FormalParameters(thsP : Ty.Procedure;
                              proc : Id.Procs;
                              scpe : Sy.Scope);
    VAR group : Id.ParSeq;
    (*  typId : Id.TypId; *)

   (* --------------------------- *)
    PROCEDURE EnterFPs(VAR grp, seq : Id.ParSeq; pSc, sSc : Sy.Scope);
      VAR index : INTEGER;
    param : Id.ParId;
    BEGIN
      FOR index := 0 TO grp.tide-1 DO
        param := grp.a[index];
        Id.AppendParam(seq, param);
        IF pSc # NIL THEN
          IF Sy.refused(param, pSc) THEN
            param.IdError(20);
          ELSE
            param.varOrd := pSc.locals.tide;
            param.dfScp  := pSc;
            Sy.AppendIdnt(pSc.locals, param);
          END;
        END;
      END;
    END EnterFPs;
   (* --------------------------- *)
    PROCEDURE isPrivate(t : Sy.Type) : BOOLEAN;
    BEGIN
      RETURN ~(t IS Ty.Base) & (t.idnt.vMod = Sy.prvMode);
    END isPrivate;
   (* --------------------------- *)
    PROCEDURE CheckRetType(tst : BOOLEAN; tok : S.Token; typ : Sy.Type);
      VAR bndT : Sy.Type;
    BEGIN
      IF typ = NIL THEN RETURN;
      ELSIF typ.kind = Ty.recTp THEN SemErrorT(78, tok);
      ELSIF typ.kind = Ty.arrTp THEN SemErrorT(79, tok);
      ELSIF typ.idnt # NIL THEN (* not anon *)
        IF tst & isPrivate(typ) THEN SemErrorT(151, tok) END;
      ELSIF typ.kind = Ty.ptrTp THEN 
        bndT := typ(Ty.Pointer).boundTp;
        IF tst & (bndT # NIL) & isPrivate(bndT) THEN SemErrorT(151, tok) END;
      END;
    END CheckRetType;
   (* --------------------------- *)
    PROCEDURE ReturnType(typ : Ty.Procedure; prc : Id.Procs; scp : Sy.Scope);
      VAR tpRt : Sy.Type;
          tokn : S.Token;
          test : BOOLEAN;
    BEGIN
      Get; (* read past colon symbol *)
      tokn := nextT;
      tpRt := type(scp, Sy.prvMode);
      typ.retType := tpRt;
      test := ~G.special & (prc # NIL) & (prc.vMod = Sy.pubMode);
      CheckRetType(test, tokn, tpRt);
    END ReturnType;
   (* --------------------------- *)
  BEGIN
    Get;        (* read past lparenSym  *)
    IF (nextT.sym = T.identSym) OR
       (nextT.sym = T.INSym) OR
       (nextT.sym = T.VARSym) OR
       (nextT.sym = T.OUTSym) THEN
      FPSection(group, proc, scpe);
      EnterFPs(group, thsP.formals, proc, scpe);

      WHILE weakSeparator(22, 10, 11) DO
        Id.ResetParSeq(group);
        FPSection(group, proc, scpe);
        EnterFPs(group, thsP.formals, proc, scpe);
      END;
    END;
    Expect(T.rparenSym);
    IF (nextT.sym = T.colonSym) THEN ReturnType(thsP, proc, scpe) END;
  END FormalParameters;

(* ==================================================================== *)

  PROCEDURE CheckVisibility(seq : Sy.IdSeq; in : INTEGER; OUT out : INTEGER);
    VAR ix : INTEGER;
        id : Sy.Idnt;
        md : INTEGER;
  BEGIN
    out := Sy.prvMode;
    FOR ix := 0 TO seq.tide-1 DO
      id := seq.a[ix];
      md := id.vMod;
      CASE in OF
      | Sy.prvMode : IF md # Sy.prvMode THEN id.IdError(183) END;
      | Sy.pubMode :
      | Sy.rdoMode : IF md = Sy.pubMode THEN id.IdError(184) END;
      END;
      out := Sy.maxMode(md, out);
    END;
  END CheckVisibility;

(* ==================================================================== *)

  PROCEDURE  IdentDefList(OUT iSeq : Sy.IdSeq; 
                              scp  : Sy.Scope; 
                              kind : INTEGER);
  BEGIN
    Sy.AppendIdnt(iSeq, identDef(scp, kind));
    WHILE (nextT.sym = T.commaSym) DO
      Get;
      Sy.AppendIdnt(iSeq, identDef(scp, kind));
    END;
  END IdentDefList;

(* ==================================================================== *)

  PROCEDURE  FieldList(recT   : Ty.Record;
                       defScp : Sy.Scope;
                       vMod   : INTEGER);
    VAR list : Sy.IdSeq;
        fTyp : Sy.Type;
        fDsc : Id.FldId;
        fIdx : INTEGER;
        vOut : INTEGER;
  BEGIN
    IF nextT.sym = T.identSym THEN
      IdentDefList(list, defScp, Id.fldId);
      CheckVisibility(list, vMod, vOut);
      Expect(T.colonSym);
      fTyp := type(defScp, vOut);
      IF vOut # Sy.prvMode THEN FixAnon(defScp, fTyp, vOut) END;

      FOR fIdx := 0 TO list.tide-1 DO
        fDsc := list.a[fIdx](Id.FldId);
        fDsc.type := fTyp;
        fDsc.recTyp := recT;
        Sy.AppendIdnt(recT.fields, fDsc);
      END;
    END;
  END FieldList;

(* ==================================================================== *)

  PROCEDURE FieldListSequence(recT   : Ty.Record;
                              defScp : Sy.Scope;
                              vMod   : INTEGER);
    VAR start : INTEGER;
        final : INTEGER;
        index : INTEGER;
        ident : Sy.Idnt;
  BEGIN
    start := recT.fields.tide;
    FieldList(recT, defScp, vMod);
    WHILE (nextT.sym = T.semicolonSym) DO
      Get;
      FieldList(recT, defScp, vMod);
    END;
    final := recT.fields.tide;
   (* now insert into the fieldname scope *)
    FOR index := start TO final-1 DO
      ident := recT.fields.a[index];
      IF ~recT.symTb.enter(ident.hash, ident) THEN ident.IdError(6) END;
    END;
  END FieldListSequence;

(* ==================================================================== *)

  PROCEDURE StaticStuff(recT   : Ty.Record;
                        defScp : Sy.Scope;
                        vMod   : INTEGER);  (* vMod ??? *)
   (* ----------------------------------------- *)
    PROCEDURE StaticProc(rec : Ty.Record; scp : Sy.Scope);
      VAR prcD : Id.Procs;
          prcT : Ty.Procedure;
          name : LitValue.CharOpen;
          oId  : Id.OvlId;
          ok   : BOOLEAN;
    BEGIN
      Get;    (* read past procedureSym *)
      prcD := identDef(scp, Id.conPrc)(Id.Procs);
      prcD.SetKind(Id.conPrc);
      prcD.bndType := rec;
      IF nextT.sym = T.lbrackSym THEN
        IF ~G.special THEN SemError(144) END;
        Get;
        Expect(T.stringSym);
        name := LitValue.subStrToCharOpen(token.pos+1, token.len-2);
        prcD.prcNm := name;
        Expect(T.rbrackSym);
        IF G.verbose THEN G.Message('external procName "' + name^ + '"') END;
      END;
      prcT := Ty.newPrcTp();
      prcT.idnt := prcD;
      IF prcD.vMod # Sy.prvMode THEN INCL(prcD.pAttr, Id.public) END;
      IF nextT.sym = T.lparenSym THEN
        FormalParameters(prcT, prcD, scp);
      END;
      prcD.type := prcT;
      Ty.InsertInRec(prcD,rec,FALSE,oId,ok);
      IF ok THEN
        Sy.AppendIdnt(rec.statics, prcD);
       (*
        *  Put this header on the procedure list,
        *  so that it gets various semantic checks.
        *)
        Id.AppendProc(G.thisMod.procs, prcD);
      ELSE
        prcD.IdError(6);
      END;
    END StaticProc;
   (* ----------------------------------------- *)
    PROCEDURE StaticConst(lst : Sy.IdSeq;
                          rec : Ty.Record;
                          scp : Sy.Scope);
      VAR vrId : Sy.Idnt;
          cnId : Id.ConId;
          cnEx : Sy.Expr;
          oId  : Id.OvlId;
          ok   : BOOLEAN;
    BEGIN
      Expect(T.equalSym);
     (*
      *  We have a list of VarId here. If the list
      *  has more than one element, then that is an
      *  error, otherwise copy info to a ConId ...
      *)
      IF lst.tide > 1 THEN lst.a[1].IdError(192); RETURN END;
      vrId := lst.a[0];
      cnId := Id.newConId();
      cnId.token := vrId.token;
      cnId.hash  := vrId.hash;
      cnId.dfScp := vrId.dfScp;
      cnId.SetMode(vrId.vMod);
      cnEx := constExpression(scp);
      cnId.conExp := cnEx;
      cnId.type   := cnEx.type;
      Ty.InsertInRec(cnId,rec,FALSE,oId,ok);
      IF ok THEN
        Sy.AppendIdnt(rec.statics, cnId);
      ELSE
        cnId.IdError(6);
      END;
    END StaticConst;
   (* ----------------------------------------- *)
    PROCEDURE StaticField(lst : Sy.IdSeq;
                          rec : Ty.Record;
                          scp : Sy.Scope);
      VAR flTp : Sy.Type;
          flId : Id.VarId;
          indx : INTEGER;
          oId  : Id.OvlId;
          ok   : BOOLEAN;
    BEGIN
      Get;    (* read past colon *)
      flTp := type(scp, Sy.pubMode);
      FOR indx := 0 TO lst.tide-1 DO
        flId := lst.a[indx](Id.VarId);
        flId.type := flTp;
        flId.recTyp := rec;
        Ty.InsertInRec(flId,rec,FALSE,oId,ok);
        IF ok THEN
          Sy.AppendIdnt(rec.statics, flId);
        ELSE
          flId.IdError(6);
        END;
      END;
    END StaticField;
   (* ----------------------------------------- *)
    PROCEDURE DoStatic(rec : Ty.Record;
                       scp : Sy.Scope);
     (*
      *  StatDef --> PROCEDURE ProcHeading
      *              |  IdentDef { ',' IdentDef } ":" Type
      *              |  IdentDef "=" Constant .
      *)
      VAR list : Sy.IdSeq;
    BEGIN
      IF nextT.sym = T.PROCEDURESym THEN
        StaticProc(rec, scp);
      ELSIF nextT.sym = T.identSym THEN
       (*
        *  There is a syntactic ambiguity here.
        *  after an abitrary lookahead we find
        *  the symbol which determines if this
        *  is a constant or a variable definition.
        *  We will "predict" a variable and then
        *  back-patch later, if necessary.
        *)
        IdentDefList(list, scp, Id.varId);
        IF nextT.sym = T.colonSym THEN
          StaticField(list, rec, scp);
        ELSIF nextT.sym = T.equalSym THEN
          StaticConst(list, rec, scp);
        ELSE
          SemError(192); Get;
        END;
      ELSE (* skip redundant semicolons *)
      END;
    END DoStatic;
   (* ----------------------------------------- *)
  BEGIN
    DoStatic(recT, defScp);
    WHILE (nextT.sym = T.semicolonSym) DO
      Get;
      DoStatic(recT, defScp);
    END;
  END StaticStuff;

(* ==================================================================== *)

  PROCEDURE EnumConst(enum   : Ty.Enum;
                      defScp : Sy.Scope;
                      vMod   : INTEGER);  (* vMod ??? *)
    VAR idnt : Sy.Idnt;
        cnId : Id.ConId;
        cnEx : Sy.Expr;
  BEGIN
    IF nextT.sym # T.identSym THEN RETURN END;    (* skip extra semis! *)
    idnt := identDef(defScp, Id.conId);
    cnId := idnt(Id.ConId);       (* don't insert yet! *)
    Expect(T.equalSym);
    cnEx := constExpression(defScp);
    cnId.conExp := cnEx;
    cnId.type   := cnEx.type;
    IF cnId.type # Bi.intTp THEN cnEx.ExprError(37) END;
    IF enum.symTb.enter(cnId.hash, cnId) THEN
      Sy.AppendIdnt(enum.statics, cnId);
    ELSE
      cnId.IdError(6);
    END;
  END EnumConst;

(* ==================================================================== *)

  PROCEDURE  ArrLength(defScp : Sy.Scope; OUT n : INTEGER; OUT p : BOOLEAN);
    VAR xSyn : Xp.LeafX;
  BEGIN
    n := 0;
    p := FALSE;
    xSyn := constExpression(defScp);
    IF xSyn # NIL THEN
      IF xSyn.kind = Xp.numLt THEN
        n := xSyn.value.int();
        IF n > 0 THEN p := TRUE ELSE SemError(68) END;
      ELSE
        SemError(31);
      END;
    END;
  END ArrLength;

(* ==================================================================== *)

  PROCEDURE  PointerType(pTyp : Ty.Pointer; defScp : Sy.Scope; vMod : INTEGER);
  BEGIN
    Expect(T.POINTERSym);
    Expect(T.TOSym);
    pTyp.boundTp := type(defScp, vMod);
  END PointerType;

(* ==================================================================== *)

  PROCEDURE  EventType(eTyp : Ty.Procedure; defScp : Sy.Scope; vMod : INTEGER);
  BEGIN
    Expect(T.EVENTSym);
    IF ~G.targetIsNET() THEN SemError(208);
    ELSIF G.strict THEN SemError(221); 
    END;
    IF ~(defScp IS Id.BlkId) THEN SemError(212) END;
    IF (nextT.sym = T.lparenSym) THEN
      FormalParameters(eTyp, NIL, defScp);
    ELSE SemError(209);
    END;
  END EventType;

(* ==================================================================== *)

  PROCEDURE  RecordType(rTyp : Ty.Record; defScp : Sy.Scope; vMod : INTEGER);
   (*
    *  Record --> RECORD ['(' tpQual { '+' tpQual } ')']
    *             FieldListSequence
    *             [ STATIC StatDef { ';' StatDef } ] END .
    *)
    VAR tpId : Id.TypId;
  BEGIN
    Expect(T.RECORDSym);
    IF Sy.frnMd IN G.thisMod.xAttr THEN
      INCL(rTyp.xAttr, Sy.isFn);         (* must be foreign *)
    END;
    IF (nextT.sym = T.lparenSym) THEN
      Get;
      IF nextT.sym # T.plusSym THEN
        tpId := typeQualid(defScp);
      ELSE
        tpId := Bi.anyTpId;
      END;
      IF tpId # NIL THEN rTyp.baseTp := tpId.type END;
      INCL(rTyp.xAttr, Sy.clsTp); (* must be a class *)
     (* interfaces ... *)
      WHILE (nextT.sym = T.plusSym) DO
        Get;
        IF G.strict & (nextT.sym = T.plusSym) THEN SemError(221); END;
        tpId := typeQualid(defScp);
        IF tpId # NIL THEN Sy.AppendType(rTyp.interfaces, tpId.type) END;
      END;
      Expect(T.rparenSym);
    END;
    FieldListSequence(rTyp, defScp, vMod);
    IF nextT.sym = T.STATICSym THEN
      Get;
      IF ~G.special THEN SemError(185) END;
      INCL(rTyp.xAttr, Sy.isFn);         (* must be foreign *)
      StaticStuff(rTyp, defScp, vMod);
    END;
    Expect(T.ENDSym);
  END RecordType;

(* ==================================================================== *)

  PROCEDURE  EnumType(enum : Ty.Enum; defScp : Sy.Scope; vMod : INTEGER);
   (*
    *  Enum --> ENUM RECORD StatDef { ';' StatDef } END .
    *)
  BEGIN
    IF ~G.special THEN SemError(185) END;
    Get;      (* read past ENUM *)
    (* Expect(T.RECORDSym); *)
    EnumConst(enum, defScp, vMod);
    WHILE (nextT.sym = T.semicolonSym) DO
      Get;
      EnumConst(enum, defScp, vMod);
    END;
    Expect(T.ENDSym);
  END EnumType;

(* ==================================================================== *)

  PROCEDURE  OptAttr (rTyp : Ty.Record);
  BEGIN
    INCL(rTyp.xAttr, Sy.clsTp);   (* must be a class *)
    IF nextT.sym = T.ABSTRACTSym THEN
      Get;
      rTyp.recAtt := Ty.isAbs;
    ELSIF nextT.sym = T.EXTENSIBLESym THEN
      Get;
      rTyp.recAtt := Ty.extns;
    ELSIF nextT.sym = T.LIMITEDSym THEN
      Get;
      rTyp.recAtt := Ty.limit;
    ELSIF nextT.sym = T.INTERFACESym THEN
      Get;
      IF G.strict THEN SemError(221); END;
      rTyp.recAtt := Ty.iFace;
    ELSE Error(87);
    END;
  END OptAttr;

(* ==================================================================== *)

  PROCEDURE  ArrayType (aTyp : Ty.Array; defScp : Sy.Scope; vMod : INTEGER);
    VAR length : INTEGER;
        ok     : BOOLEAN;
        elemT  : Ty.Array;
  BEGIN
    Expect(T.ARRAYSym);
    IF in(symSet[3], nextT.sym) THEN
      ArrLength(defScp, length, ok);
      IF ok THEN aTyp.length := length END;
      WHILE (nextT.sym = T.commaSym) DO
        Get;
        ArrLength(defScp, length, ok);
        elemT := Ty.newArrTp(); aTyp.elemTp := elemT; aTyp := elemT;
        IF ok THEN aTyp.length := length END;
      END;
    END;
    Expect(T.OFSym);
    aTyp.elemTp := type(defScp, vMod);

    IF vMod # Sy.prvMode THEN FixAnon(defScp, aTyp.elemTp, vMod) END;
  END ArrayType;

(* ==================================================================== *)

  PROCEDURE  VectorType (aTyp : Ty.Vector; defScp : Sy.Scope; vMod : INTEGER);
    VAR length : INTEGER;
        ok     : BOOLEAN;
        elemT  : Ty.Array;
  BEGIN
    Expect(T.VECTORSym);
    Expect(T.OFSym);
    aTyp.elemTp := type(defScp, vMod);
    IF vMod # Sy.prvMode THEN FixAnon(defScp, aTyp.elemTp, vMod) END;
    IF G.strict THEN SemError(221) END;
  END VectorType;

(* ==================================================================== *)

  PROCEDURE ProcedureType(pTyp : Ty.Procedure; defScp : Sy.Scope);
  BEGIN
    Expect(T.PROCEDURESym);
    IF (nextT.sym = T.lparenSym) THEN
      FormalParameters(pTyp, NIL, defScp);
    ELSIF (nextT.sym = T.rparenSym) OR
          (nextT.sym = T.ENDSym) OR
          (nextT.sym = T.semicolonSym) THEN
      (* skip *)
    ELSE Error(88);
    END;
  END ProcedureType;

(* ==================================================================== *)

  PROCEDURE CompoundType(defScp : Sy.Scope; firstType : Id.TypId) : Sy.Type;
  (* Parses a compound type from a series of comma separated qualidents.
   * One component of the compound type has already been parsed and is
   * passed as firstType. The next token is a comma. At most one of the
   * types can be a class, and all the others must be interfaces. The
   * type that is returned is a Pointer to a Record with the compound
   * type flag set. *)
  (* Things that could be checked here but aren't yet:
   *  - that any interfaces are not part of the base type
      - that any interfaces are not entered more than once
   *)
    VAR
      ptrT : Ty.Pointer;
      cmpT : Ty.Record;
      tpId : Id.TypId;

   (* Checks to make sure the type is suitable for use in a compound
    * type *)
    PROCEDURE checkType(type : Sy.Type) : BOOLEAN;
    BEGIN
      IF (type = NIL) OR
         ~(type.isRecordType() OR type.isDynamicType()) THEN
        Error(89);
        RETURN FALSE;
      ELSE
        RETURN TRUE;
      END;
    END checkType;

  BEGIN
    (* Check that we were passed an appropriate type and that
     * a comma is following *)
    IF ~checkType(firstType.type) THEN Error(89); RETURN NIL END;
    IF nextT.sym # T.commaSym THEN Error(12); RETURN NIL END;

    (* Create the compound type *)
    cmpT := Ty.newRecTp();
    cmpT.recAtt := Ty.cmpnd;

    IF firstType.type.isInterfaceType() THEN
      (* Add it to the list of interfaces *)
      Sy.AppendType(cmpT.interfaces, firstType.type);
    ELSE
      (* Make it our base type *)
      cmpT.baseTp := firstType.type;
    END;

    WHILE nextT.sym = T.commaSym DO
      Get;   (* Eat the comma *)
      IF nextT.sym # T.identSym THEN Error(T.identSym) END;
      tpId := typeQualid(defScp);
      IF ~checkType(tpId.type) THEN RETURN NIL END;
      IF tpId.type.isInterfaceType() THEN
        Sy.AppendType(cmpT.interfaces, tpId.type);
      ELSE
        IF cmpT.baseTp # NIL THEN Error(89); RETURN NIL END;
        cmpT.baseTp := tpId.type;
      END;
    END;
    INCL(cmpT.xAttr, Sy.clsTp);   (* must be a class *)
    ptrT := Ty.newPtrTp();
    ptrT.boundTp := cmpT;
    RETURN ptrT;
  END CompoundType;

(* ==================================================================== *)

  PROCEDURE type(defScp : Sy.Scope; vMod : INTEGER) : Sy.Type;
    VAR tpId : Id.TypId;
        prcT : Ty.Procedure;
        recT : Ty.Record;
        arrT : Ty.Array;
        vecT : Ty.Vector;
        ptrT : Ty.Pointer;
        enuT : Ty.Enum;
  BEGIN
    IF (nextT.sym = T.identSym) THEN
      tpId := typeQualid(defScp);
      IF tpId = NIL THEN RETURN NIL END;
      IF ~G.extras THEN RETURN tpId.type END;
      (* Compound type parsing... look for comma *)
      IF nextT.sym # T.commaSym THEN RETURN tpId.type
      ELSE RETURN CompoundType(defScp, tpId) END;
    ELSIF (nextT.sym = T.PROCEDURESym) THEN
      prcT := Ty.newPrcTp();
      ProcedureType(prcT, defScp); RETURN prcT;
    ELSIF (nextT.sym = T.ARRAYSym) THEN
      arrT := Ty.newArrTp();
      ArrayType(arrT, defScp, vMod); RETURN arrT;
    ELSIF (nextT.sym = T.VECTORSym) THEN
      vecT := Ty.newVecTp();
      VectorType(vecT, defScp, vMod); RETURN vecT;
    ELSIF (nextT.sym = T.ABSTRACTSym) OR
          (nextT.sym = T.EXTENSIBLESym) OR
          (nextT.sym = T.LIMITEDSym) OR
          (nextT.sym = T.INTERFACESym) OR
          (nextT.sym = T.RECORDSym) THEN
      recT := Ty.newRecTp();
      IF nextT.sym # T.RECORDSym THEN OptAttr(recT) END;
      RecordType(recT, defScp, vMod);  RETURN recT;
    ELSIF (nextT.sym = T.POINTERSym) THEN
      ptrT := Ty.newPtrTp();
      PointerType(ptrT, defScp, vMod); RETURN ptrT;
    ELSIF (nextT.sym = T.ENUMSym) THEN
      enuT := Ty.newEnuTp();
      EnumType(enuT, defScp, vMod);  RETURN enuT;
    ELSIF (nextT.sym = T.EVENTSym) THEN
      prcT := Ty.newEvtTp();
      EventType(prcT, defScp, vMod);  RETURN prcT;
    ELSE
      Error(89); RETURN NIL;
    END;
  END type;

(* ==================================================================== *)

  PROCEDURE  TypeDeclaration(defScp : Sy.Scope);
    VAR iTmp  : Sy.Idnt;
        stuck : BOOLEAN;
  BEGIN
    iTmp := identDef(defScp, Id.typId);
    IF iTmp.vMod = Sy.rdoMode THEN SemError(134) END;
    Expect(T.equalSym);
    iTmp.type := type(defScp, iTmp.vMod);
    IF (iTmp.type # NIL) & iTmp.type.isAnonType() THEN
      iTmp.type.idnt := iTmp;
    END;
    stuck := Sy.refused(iTmp, defScp);
    IF stuck THEN iTmp.IdError(4) END;
  END TypeDeclaration;

(* ==================================================================== *)

  PROCEDURE  expression(scope : Sy.Scope) : Sy.Expr;
    VAR relOp : INTEGER;
        expN1 : Sy.Expr;
        expN2 : Sy.Expr;
        saveT : S.Token;
        tokN1 : S.Token;
  (* ------------------------------------------ *)
    PROCEDURE MarkAssign(id : Sy.Idnt);
    BEGIN
      IF (id # NIL) & (id IS Id.Procs) THEN
        INCL(id(Id.Procs).pAttr, Id.assgnd);
      END;
    END MarkAssign;
  (* ------------------------------------------ *)
  BEGIN
    tokN1 := nextT;
    expN1 := simpleExpression(scope);
   (*
    *  Mark use of procedure-valued expressions.
    *)
    WITH expN1 : Xp.IdLeaf DO
        MarkAssign(expN1.ident);
    | expN1 : Xp.IdentX DO
        MarkAssign(expN1.ident);
    ELSE
    END;
   (*
    *  ... and parse the substructures!
    *)
    IF in(symSet[12], nextT.sym) THEN
      relOp := relation(); saveT := token;
      expN2 := simpleExpression(scope);
      expN1 := Xp.newBinaryT(relOp, expN1, expN2, saveT);
    END;
    IF expN1 # NIL THEN expN1.tSpan := S.mkSpanTT(tokN1, S.prevTok) END;
    RETURN expN1;
  END expression;

(* ==================================================================== *)

  PROCEDURE  constExpression(defScp : Sy.Scope) : Xp.LeafX;
    VAR expr : Sy.Expr;
        orig : S.Span;
  (* ------------------------------------------ *)
    PROCEDURE eval(exp : Sy.Expr) : Sy.Expr;
    BEGIN
      RETURN exp.exprAttr();
    RESCUE (junk)
      exp.ExprError(55);
      RETURN NIL;
    END eval;
  (* ------------------------------------------ *)
  BEGIN
    expr := expression(defScp);
    IF expr # NIL THEN
      orig := expr.tSpan;
      expr := eval(expr);
      IF expr = NIL THEN (* skip *)
      ELSIF (expr IS Xp.LeafX) &
            (expr.kind # Xp.setXp) THEN
        expr.tSpan := orig;
        RETURN expr(Xp.LeafX);
      ELSE
        expr.ExprError(25);   (* expr not constant *)
      END;
    END;
    RETURN NIL;
  END constExpression;

(* ==================================================================== *)

  PROCEDURE  ConstantDeclaration (defScp : Sy.Scope);
    VAR idnt : Sy.Idnt;
        cnId : Id.ConId;
        cnEx : Xp.LeafX;
  BEGIN
    idnt := identDef(defScp, Id.conId);
    cnId := idnt(Id.ConId);       (* don't insert yet! *)
    Expect(T.equalSym);
    cnEx := constExpression(defScp);
    IF Sy.refused(idnt, defScp) THEN idnt.IdError(4) END;
    IF (cnId # NIL) & (cnEx # NIL) THEN
      cnId.conExp := cnEx;
      cnId.type   := cnEx.type;
    END;
  END ConstantDeclaration;

(* ==================================================================== *)

  PROCEDURE  qualident(defScp : Sy.Scope) : Sy.Idnt;
  (*  Postcondition: returns a valid Id, or NIL.      *
   *  NIL ==> error already notified.           *)
    VAR idnt : Sy.Idnt;
        locl : Id.LocId;
        tpId : Id.TypId;
        tpTp : Sy.Type;
        modS : Sy.Scope;
        hash : INTEGER;
        eNum : INTEGER;
  BEGIN
    Expect(T.identSym);
    hash := NameHash.enterSubStr(token.pos, token.len);
    idnt := Sy.bind(hash, defScp);
    IF idnt = NIL THEN
      SemError(2); RETURN NIL;
    ELSIF (idnt.kind # Id.impId) & (idnt.kind # Id.alias) THEN
     (*
      *   This is a single token qualident.
      *   Now we check for uplevel addressing.
      *   Temporarily disallowed in boot version 0.n and 1.0
      *)
      IF (idnt.dfScp # NIL) &   (* There is a scope,  *)
         (idnt.dfScp # defScp) &  (* not current scope, *)
         (idnt IS Id.AbVar) &   (* is a variable, and *)
         (idnt.dfScp IS Id.Procs) THEN  (* scope is a PROC.   *)
        SemError(302);
        locl := idnt(Id.LocId);
        IF ~(Id.uplevA IN locl.locAtt) THEN
          eNum := 311;
          WITH locl : Id.ParId DO
            IF (locl.parMod # Sy.val) &
               (locl.type # NIL) & 
                ~G.targetIsJVM() &
                ~locl.type.isRefSurrogate() THEN
              eNum := 310;
              INCL(locl.locAtt, Id.cpVarP);
            END;
          ELSE (* skip *)
          END;
          locl.IdErrorStr(eNum, Sy.getName.ChPtr(idnt));
          INCL(idnt.dfScp(Id.Procs).pAttr, Id.hasXHR);
          INCL(locl.locAtt, Id.uplevA); (* uplevel Any *)
        END;
(*
 *     (*
 *      *  As of version 1.06 uplevel addressing is
 *      *  ok, except for reference params in .NET.
 *      *  This needs to be fixed by 
 *      *     (1) producing unverifiable code
 *      *     (2) adopting inexact semantics in this case.
 *      *  To be determined later ...
 *      *)
 *      WITH locl : Id.ParId DO
 *        IF (locl.parMod # Sy.val) &
 *           (locl.type # NIL) & 
 *            ~G.targetIsJVM() &
 *            ~locl.type.isRefSurrogate() THEN
 *           S.SemError.RepSt1(189, Sy.getName.ChPtr(idnt), token.lin, 0);
 *        END;
 *      ELSE (* skip *)
 *      END;
 *)
      END;
      RETURN idnt;
    ELSE
      modS := idnt(Sy.Scope);
    END;
    Expect(T.pointSym);
    Expect(T.identSym);
   (*
    *  At this point the only live control flow branch is
    *  the one predicated on the ident being a scope name.
    *)
    idnt := bindTokenLocal(modS);
    IF idnt = NIL THEN
      SemError(3);      (* name not known in qualified scope *)
    ELSIF modS.isWeak() THEN
      SemErrorS1(175, Sy.getName.ChPtr(modS));  (* mod not directly imported *)
    ELSE
      RETURN idnt;
    END;
    RETURN NIL;
  END qualident;

(* ==================================================================== *)

  PROCEDURE typeQualid(defScp : Sy.Scope) : Id.TypId;
  (** This procedure returns one of --
        a valid Id.TypId (possibly a forward type)
        NIL (with an error already notified)        *)
    VAR idnt : Sy.Idnt;
        tpId : Id.TypId;
        tpTp : Sy.Type;
        modS : Sy.Scope;
        hash : INTEGER;
  BEGIN
    Expect(T.identSym);
    hash := NameHash.enterSubStr(token.pos, token.len);
    idnt := Sy.bind(hash, defScp);
    modS := NIL;
    IF idnt = NIL THEN
     (*
      *  This _might_ just be a forward type.  It cannot be so
      *  if the next token is "." or if declarations in this
      *  scope are officially closed.
      *)
      IF (nextT.sym = T.pointSym) OR defScp.endDecl THEN
        SemError(2);
        IF nextT.sym # T.pointSym THEN RETURN NIL END;
      ELSE
        tpTp := Ty.newTmpTp();
        tpId := Id.newTypId(tpTp);
        tpId.dfScp := defScp;
        tpId.token := token;
        tpId.hash  := hash;
        tpTp.idnt  := tpId;
        RETURN tpId;
      END;
    ELSIF idnt.kind = Id.typId THEN
      RETURN idnt(Id.TypId);
    ELSIF (idnt.kind = Id.impId) OR (idnt.kind = Id.alias) THEN
      modS := idnt(Sy.Scope);
    ELSE
      SemError(5);
      IF nextT.sym # T.pointSym THEN RETURN NIL END;
    END;
    Expect(T.pointSym);
    Expect(T.identSym);
    IF modS = NIL THEN RETURN NIL END;
   (*
    *  At this point the only live control flow branch is
    *  the one predicated on the ident being a scope name.
    *)
    idnt := bindTokenLocal(modS);
    IF idnt = NIL THEN
      SemError(3);    (* name not known in qualified scope *)
    ELSIF modS.isWeak() THEN
      SemErrorS1(175, Sy.getName.ChPtr(modS));  (* mod not directly imported *)
    ELSIF idnt.kind # Id.typId THEN
      SemError(7);    (* name is not the name of a type    *)
    ELSE
      tpId := idnt(Id.TypId);
      RETURN tpId;
    END;
    RETURN NIL;
  END typeQualid;

(* ==================================================================== *)

  PROCEDURE  identDef(inhScp : Sy.Scope; tag : INTEGER) : Sy.Idnt;
  (** This non-terminal symbol creates an Id of prescribed kind for
      the ident. The Id has its parent scope assigned, but is not yet
      inserted into the prescribed scope.          *)
    VAR iSyn : Sy.Idnt;
  BEGIN
    CASE tag OF
    | Id.conId  : iSyn := Id.newConId();
    | Id.parId  : iSyn := Id.newParId();
    | Id.quaId  : iSyn := Id.newQuaId();
    | Id.modId  : iSyn := Id.newModId();
    | Id.impId  : iSyn := Id.newImpId();
    | Id.fldId  : iSyn := Id.newFldId();
    | Id.fwdMth : iSyn := Id.newMthId();
    | Id.conMth : iSyn := Id.newMthId();
    | Id.fwdPrc : iSyn := Id.newPrcId();
    | Id.conPrc : iSyn := Id.newPrcId();
    | Id.typId  : iSyn := Id.newTypId(NIL);
    | Id.fwdTyp : iSyn := Id.newTypId(NIL);
    | Id.varId  : IF inhScp IS Id.BlkId THEN
                    iSyn := Id.newVarId();
                  ELSE
                    iSyn := Id.newLocId();
                  END;
    END;
    IF iSyn IS Sy.Scope THEN iSyn(Sy.Scope).ovfChk := G.ovfCheck END;
    iSyn.token := nextT;
    iSyn.hash  := NameHash.enterSubStr(nextT.pos, nextT.len);
    IF G.verbose THEN iSyn.SetNameFromHash(iSyn.hash) END;
    iSyn.dfScp := inhScp;
    IF nextT.dlr & ~G.special THEN SemErrorT(186, nextT) END;
    Expect(T.identSym);
    IF (nextT.sym = T.starSym) OR
       (nextT.sym = T.bangSym) OR
       (nextT.sym = T.minusSym) THEN
      IF (nextT.sym = T.starSym) THEN
        Get;
        iSyn.SetMode(Sy.pubMode);
      ELSIF (nextT.sym = T.minusSym) THEN
        Get;
        iSyn.SetMode(Sy.rdoMode);
      ELSE
        Get;
        iSyn.SetMode(Sy.protect);
        IF ~G.special THEN SemError(161) END;
      END;
    END;
    IF  (iSyn.vMod # Sy.prvMode) & (inhScp # G.thisMod) THEN
      SemError(128);
    END;
    RETURN iSyn;
  END identDef;

(* ==================================================================== *)

  PROCEDURE Module;
    VAR err : INTEGER;
        nam : FileNames.NameString;
        hsh : INTEGER;
        tok : S.Token;
  BEGIN
    IF nextT.sym = T.identSym THEN
      hsh := NameHash.enterSubStr(nextT.pos, nextT.len);
      IF hsh = Bi.sysBkt THEN
        Get;
        INCL(G.thisMod.xAttr, Sy.rtsMd);
        IF G.verbose THEN G.Message("Compiling a SYSTEM Module") END;
        IF ~G.special THEN SemError(144) END;
      ELSIF hsh = Bi.frnBkt THEN
        Get;
        INCL(G.thisMod.xAttr, Sy.frnMd);
        IF G.verbose THEN G.Message("Compiling a FOREIGN Module") END;
        IF ~G.special THEN SemError(144) END;
      END;
      ForeignMod;
    ELSIF nextT.sym = T.MODULESym THEN
      (*  Except for empty bodies this next will be overwritten later *)
      G.thisMod.begTok := nextT;
      CPmodule;
    END;
    G.thisMod.endTok := nextT;
    Expect(T.ENDSym);
    Expect(T.identSym);
    S.GetString(token.pos, token.len, nam);
    IF nam # G.modNam THEN
      IF token.sym = T.identSym THEN err := 1 ELSE err := 0 END;
      SemErrorS1(err, G.modNam$);
    END;
    Expect(T.pointSym);
  END Module;

(* ==================================================================== *)

  PROCEDURE Parse*;
  BEGIN
    NEW(nextT); (* so that token is not even NIL initially *)
    S.Reset; Get;
    G.parseS := RTS.GetMillis();
    Module;
  END Parse;
  
(* ==================================================================== *)

  PROCEDURE parseTextAsStatement*(text : ARRAY OF LitValue.CharOpen; encScp : Sy.Scope) : Sy.Stmt;
    VAR result : Sy.Stmt;
  BEGIN
    G.SetQuiet;
    NEW(nextT);
    S.NewReadBuffer(text); Get;
    result := statementSequence(NIL, encScp);
    S.RestoreFileBuffer();
    G.RestoreQuiet;
    RETURN result;
  END parseTextAsStatement;

  PROCEDURE ParseDeclarationText*(text : ARRAY OF LitValue.CharOpen; encScp : Sy.Scope);
  BEGIN
    G.SetQuiet;
    NEW(nextT);
    S.NewReadBuffer(text); Get;
    DeclarationSequence(encScp);
    S.RestoreFileBuffer();
    G.RestoreQuiet;
  END ParseDeclarationText;

(* ==================================================================== *)

BEGIN
  comma := LitValue.strToCharOpen(",");
  errDist := minErrDist;
  (* ------------------------------------------------------------ *)

  symSet[ 0, 0] := {T.EOFSYM, T.identSym, T.ENDSym, T.semicolonSym};
  symSet[ 0, 1] := {T.EXITSym-32, T.RETURNSym-32, T.NEWSym-32, T.IFSym-32,
                    T.ELSIFSym-32, T.ELSESym-32, T.CASESym-32, T.barSym-32,
                    T.WHILESym-32, T.REPEATSym-32, T.UNTILSym-32, T.FORSym-32};
  symSet[ 0, 2] := {T.LOOPSym-64, T.WITHSym-64, T.CLOSESym-64};
  (* ------------------------------------------------------------ *)

  (* Follow comma in ident-list *)
  symSet[ 1, 0] := {T.identSym};
  symSet[ 1, 1] := {};
  symSet[ 1, 2] := {};
  (* ------------------------------------------------------------ *)

  (* Follow(ident-list) *)
  symSet[ 2, 0] := {T.colonSym};
  symSet[ 2, 1] := {};
  symSet[ 2, 2] := {};
  (* ------------------------------------------------------------ *)

  (* Start(expression) *)
  symSet[ 3, 0] := {T.identSym, T.integerSym, T.realSym, T.CharConstantSym,
                    T.stringSym, T.minusSym, T.lparenSym, T.plusSym};
  symSet[ 3, 1] := {T.NILSym-32, T.tildeSym-32, T.lbraceSym-32};
  symSet[ 3, 2] := {T.bangStrSym-64};
  (* ------------------------------------------------------------ *)

  (* lookahead of optional statement *)
  symSet[ 4, 0] := {T.EOFSYM, T.identSym, T.ENDSym, T.semicolonSym};
  symSet[ 4, 1] := {T.EXITSym-32, T.RETURNSym-32, T.NEWSym-32, T.IFSym-32,
                    T.ELSIFSym-32, T.ELSESym-32, T.CASESym-32, T.barSym-32,
                    T.WHILESym-32, T.REPEATSym-32, T.UNTILSym-32, T.FORSym-32};
  symSet[ 4, 2] := {T.LOOPSym-64, T.WITHSym-64, T.CLOSESym-64, T.RESCUESym-64};
  (* ------------------------------------------------------------ *)

  (* follow semicolon in statementSequence *)
  symSet[ 5, 0] := {T.identSym, T.ENDSym, T.semicolonSym};
  symSet[ 5, 1] := {T.EXITSym-32, T.RETURNSym-32, T.NEWSym-32, T.IFSym-32,
                    T.ELSIFSym-32, T.ELSESym-32, T.CASESym-32, T.barSym-32,
                    T.WHILESym-32, T.REPEATSym-32, T.UNTILSym-32, T.FORSym-32};
  symSet[ 5, 2] := {T.LOOPSym-64, T.WITHSym-64, T.CLOSESym-64, T.RESCUESym-64};
  (* ------------------------------------------------------------ *)

  (* Follow(statementSequence) *)
  symSet[ 6, 0] := {T.ENDSym};
  symSet[ 6, 1] := {T.ELSIFSym-32, T.ELSESym-32, T.barSym-32, T.UNTILSym-32};
  symSet[ 6, 2] := {T.CLOSESym-64, T.RESCUESym-64};
  (* ------------------------------------------------------------ *)

  (* Follow(barSym) *)
  symSet[ 7, 0] := {T.EOFSYM, T.identSym, T.integerSym, T.realSym,
                    T.CharConstantSym, T.stringSym, T.minusSym, T.lparenSym,
                    T.plusSym, T.ENDSym, T.semicolonSym};
  symSet[ 7, 1] := {T.NILSym-32, T.tildeSym-32, T.lbraceSym-32,
                    T.EXITSym-32, T.RETURNSym-32, T.NEWSym-32, T.IFSym-32,
                    T.ELSIFSym-32, T.ELSESym-32, T.CASESym-32, T.barSym-32,
                    T.WHILESym-32, T.REPEATSym-32, T.UNTILSym-32, T.FORSym-32};
  symSet[ 7, 2] := {T.LOOPSym-64, T.WITHSym-64, T.CLOSESym-64};
  (* ------------------------------------------------------------ *)

  (* lookahead to optional arglist *)
  symSet[ 8, 0] := {(*T.lparenSym,*) T.ENDSym, T.semicolonSym};
  symSet[ 8, 1] := {T.ELSIFSym-32, T.ELSESym-32, T.barSym-32, T.UNTILSym-32};
  symSet[ 8, 2] := {T.CLOSESym-64};
  (* ------------------------------------------------------------ *)

  (* Start(statement) *)
  symSet[ 9, 0] := {T.identSym};
  symSet[ 9, 1] := {T.EXITSym-32, T.RETURNSym-32, T.NEWSym-32, T.IFSym-32,
                    T.CASESym-32, T.WHILESym-32, T.REPEATSym-32, T.FORSym-32};
  symSet[ 9, 2] := {T.LOOPSym-64, T.WITHSym-64};
  (* ------------------------------------------------------------ *)

  (* follow semicolon in FormalParamLists *)
  symSet[10, 0] := {T.identSym};
  symSet[10, 1] := {T.INSym-32};
  symSet[10, 2] := {T.VARSym-64, T.OUTSym-64};
  (* ------------------------------------------------------------ *)

  (* Follow(FPsection-repetition) *)
  symSet[11, 0] := {T.rparenSym};
  symSet[11, 1] := {};
  symSet[11, 2] := {};
  (* ------------------------------------------------------------ *)

  (* Follow(simpleExpression) - Follow(expression) *)
  symSet[12, 0] := {T.equalSym, T.hashSym};
  symSet[12, 1] := {T.lessSym-32, T.lessequalSym-32, T.greaterSym-32,
                    T.greaterequalSym-32, T.INSym-32, T.ISSym-32};
  symSet[12, 2] := {};
  (* ------------------------------------------------------------ *)
END CPascalP.

