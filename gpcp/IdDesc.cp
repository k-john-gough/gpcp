(* ==================================================================== *)
(*                                                                      *)
(*  IdDesc Module for the Gardens Point Component Pascal Compiler.      *)
(*  Implements identifier descriptors that are extensions of            *)
(*  Symbols.Idnt                                                        *)
(*                                                                      *)
(*  Copyright (c) John Gough 1999, 2000.                                *)
(*                                                                      *)
(* ==================================================================== *)

MODULE IdDesc;

  IMPORT
        GPCPcopyright,
        GPText,
        Console,
        V := VarSets,
        S := CPascalS,
        D := Symbols,
        L := LitValue,
        H := DiagHelper,
        N := NameHash,
        FileNames;

(* ============================================================ *)

  CONST (* idnt-kinds *)
    errId*  = 0;  conId*  = 1;  varId*  = 2;  parId*  = 3;  quaId*  = 4;
    typId*  = 5;  modId*  = 6;  impId*  = 7;  alias*  = 8;  fldId*  = 9;
    fwdMth* = 10; conMth* = 11; fwdPrc* = 12; conPrc* = 13; fwdTyp* = 14;
    ctorP*  = 15;

  CONST (* method attributes *)
    newBit* = 0;
    final*  = {};  isNew* = {newBit}; isAbs* = {1};
    empty*  = {2}; extns* = {1,2};    mask*  = {1,2};
    covar*  = 3; (* ==> method has covariant type  *)
    boxRcv* = 4; (* ==> receiver is boxed in .NET  *)
    widen*  = 5; (* ==> visibility must be widened *)
                 (* in the runtime representation. *)
    noCall* = 6; (* ==> method is an override of   *)
                 (* an implement only method.      *)

  CONST (* procedure and method pAttr attributes *)
    hasXHR* = 0;        (* ==> has non-locally accessed data  *)
    assgnd* = 1;        (* ==> is assigned as a proc variable *)
    called* = 2;        (* ==> is directly called in this mod *)
    public* = 3;        (* ==> is exported from this module   *)
    useMsk* = {1,2,3};  (* pAttr*useMsk={} ==> a useless proc *)

(* ============================================================ *)

  TYPE
    TypId*  = POINTER TO RECORD (D.Idnt)
             (* ---- ... inherited from Idnt ...  ------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : Scanner.Token; (* scanner token  *)
              * type*  : D.Type;  (* typ-desc | NIL *)
              * hash*  : INTEGER; (* hash bucket no *)
              * vMod-  : INTEGER; (* visibility tag *)
              * dfScp* : Scope;   (* defining scope *)
              * tgXtn* : ANYPTR;
              * ----------------------------------------- *)
              END;  (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    ConId*  = POINTER TO RECORD (D.Idnt)
             (* ---- ... inherited from Idnt ...  ------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : Scanner.Token; (* scanner token  *)
              * type*  : D.Type;  (* typ-desc | NIL *)
              * hash*  : INTEGER; (* hash bucket no *)
              * vMod-  : INTEGER; (* visibility tag *)
              * dfScp* : Scope;   (* defining scope *)
              * tgXtn* : ANYPTR;
              * ----------------------------------------- *)
                recTyp* : D.Type;
                conExp* : D.Expr;
                isStd-  : BOOLEAN;  (* false if ~std  *)
              END;  (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    AbVar*  = POINTER TO ABSTRACT RECORD (D.Idnt)
              (* Abstract Variables ... *)
                varOrd* : INTEGER;  (* local var ord. *)
              END;

(* ============================================================ *)

  TYPE
    VarId*  = POINTER TO RECORD (AbVar)
             (* ---- ... inherited from Idnt ...  ------- *
              * kind-  : INTEGER;       (* tag for unions *)
              * token* : Scanner.Token; (* scanner token  *)
              * type*  : D.Type;        (* typ-desc | NIL *)
              * hash*  : INTEGER;       (* hash bucket no *)
              * vMod-  : INTEGER;       (* visibility tag *)
              * dfScp* : Scope;         (* defining scope *)
              * tgXtn* : ANYPTR;
              * ---- ... inherited from AbVar ... ------- *
              * varOrd* : INTEGER;      (* local var ord. *)
              * ----------------------------------------- *)
                recTyp* : D.Type;
                clsNm*  : L.CharOpen;   (* external name  *)
                varNm*  : L.CharOpen;   (* external name  *)
              END;      (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    FldId*  = POINTER TO RECORD (AbVar)
             (* ---- ... inherited from Idnt ...  ------- *
              * kind-  : INTEGER;       (* tag for unions *)
              * token* : Scanner.Token; (* scanner token  *)
              * type*  : D.Type;        (* typ-desc | NIL *)
              * hash*  : INTEGER;       (* hash bucket no *)
              * vMod-  : INTEGER;       (* visibility tag *)
              * dfScp* : Scope;         (* defining scope *)
              * tgXtn* : ANYPTR;
              * ---- ... inherited from AbVar ... ------- *
              * varOrd* : INTEGER;      (* local var ord. *)
              * ----------------------------------------- *)
                recTyp* : D.Type;
                fldNm*  : L.CharOpen;   (* external name  *)
              END;      (* ------------------------------ *)

(* ============================================================ *)

  CONST (* local variable and arg access attribs *)
    addrsd* = 0;  (* This bit is set if object has adrs taken   *)
    uplevR* = 1;  (* This bit is set if local is uplevel read   *)
    uplevW* = 2;  (* This bit set if local is uplevel written   *)
    uplevA* = 3;  (* This bit is set if Any uplevel access      *)
    cpVarP* = 4;  (* This bit denotes uplevel access to var-par *)
    xMark* = -1;  (* varOrd is set to xMark is local is uplevel *)
                  (* BUT ... not until after flow attribution!  *)

  TYPE
    LocId*  = POINTER TO EXTENSIBLE RECORD (AbVar)
             (* NB: LocId sometimes have kind = conId!    *
              * ---- ... inherited from Idnt ...  ------- *
              * kind-  : INTEGER;       (* tag for unions *)
              * token* : D.Token;       (* scanner token  *)
              * type*  : D.Type;        (* typ-desc | NIL *)
              * hash*  : INTEGER;       (* hash bucket no *)
              * vMod-  : INTEGER;       (* visibility tag *)
              * dfScp* : Scope;         (* defining scope *)
              * tgXtn* : ANYPTR;
              * ---- ... inherited from AbVar ... ------- *
              * varOrd* : INTEGER;      (* local var ord. *)
              * ----------------------------------------- *)
                locAtt* : SET;
                boxOrd* : INTEGER;      (* if boxd in RTS *)
              END;      (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    ParId*  = POINTER TO RECORD (LocId)
             (* ---- ... inherited from Idnt ...  ------- *
              * kind-  : INTEGER;       (* tag for unions *)
              * token* : Scanner.Token; (* scanner token  *)
              * type*  : D.Type;        (* typ-desc | NIL *)
              * hash*  : INTEGER;       (* hash bucket no *)
              * vMod-  : INTEGER;       (* visibility tag *)
              * dfScp* : Scope;         (* defining scope *)
              * tgXtn* : ANYPTR;
              * ---- ... inherited from AbVar ... ------- *
              * varOrd* : INTEGER;      (* local var ord. *)
              * ---- ... inherited from LocId ... ------- *
              * locAtt* : SET;   
              * boxOrd* : INTEGER;      (* if boxd in RTS *)
              * ----------------------------------------- *)
                parMod* : INTEGER;      (* parameter mode *)
                isRcv*  : BOOLEAN;      (* this is "this" *)
                rtsTmp* : INTEGER;      (* caller box ref *)
                rtsSrc* : VarId;        (* used for quasi *)
              END;      (* ------------------------------ *)

    ParSeq* = RECORD
                tide-, high : INTEGER;
                a- : POINTER TO ARRAY OF ParId;
              END;

(* ============================================================ *)

  TYPE
    BaseCall* = POINTER TO RECORD
                  actuals* : D.ExprSeq;
                  sprCtor* : Procs;
                  empty*   : BOOLEAN;
                END;

(* ============================================================ *)

  TYPE
    Procs*  = POINTER TO ABSTRACT RECORD (D.Scope)
             (* ---- ... inherited from Idnt ...  ------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : Scanner.Token; (* scanner token  *)
              * type*  : D.Type;  (* typ-desc | NIL *)
              * hash*  : INTEGER; (* hash bucket no *)
              * vMod-  : INTEGER; (* visibility tag *)
              * dfScp* : Scope;   (* defining scope *)
              * tgXtn* : ANYPTR;
              * ---- ... inherited from Scope ...  ------ *
              * symTb*   : SymbolTable; (* symbol scope   *)
              * endDecl* : BOOLEAN; (* can't add more *)
              * ovfChk*  : BOOLEAN; (* check overflow *)
              * locals*  : IdSeq; (* varId sequence *)
              * scopeNm* : L.CharOpen;  (* external name  *)
              * ----------------------------------------- *)
                prcNm*   : L.CharOpen;  (* external name  *)
                body*    : D.Stmt;      (* procedure-code *)
                except*  : LocId;       (* except-object  *)
                rescue*  : D.Stmt;      (* except-handler *)
                resolve* : Procs;       (* fwd resolution *)
                rtsFram* : INTEGER;     (* RTS local size *)
                nestPs*  : PrcSeq;      (* local proclist *)
                pAttr*   : SET;         (* procAttributes *)
                lxDepth* : INTEGER;     (* lexical depth  *)
                bndType* : D.Type;      (* bound RecTp    *)
                xhrType* : D.Type;      (* XHR rec. type  *)
                basCll*  : BaseCall;    (* for ctors only *)
                endSpan* : S.Span;    (* END ident span *)
              END;      (* ------------------------------ *)

    PrcSeq*  = RECORD
                tide-, high : INTEGER;
                a- : POINTER TO ARRAY OF Procs;
              END;

    PrcId*  = POINTER TO EXTENSIBLE RECORD (Procs)
                clsNm*  : L.CharOpen;   (* external name  *)
                stdOrd* : INTEGER;
              END;      (* ------------------------------ *)

    MthId*  = POINTER TO RECORD (Procs)
                mthAtt*  : SET;         (* mth attributes *)
                rcvFrm*  : ParId;       (* receiver frmal *)
              END;      (* ------------------------------ *)

(* ============================================================ *)

   (* ------------------------------------------------------- *
    *  OvlIds do not occur in pure Component Pascal.  They    *
    *  appear transiently as descriptors of identifiers that  *
    *  are bound to overloaded names from foreign libraries.  *
    * ------------------------------------------------------- *)
    OvlId*  = POINTER TO RECORD (D.Idnt)
                list* : PrcSeq;
                rec*  : D.Type;
                fld*  : D.Idnt;
              END;

(* ============================================================ *)

  TYPE
    BlkId*  = POINTER TO RECORD (D.Scope)
             (* ---- ... inherited from Idnt ...  ------- *
              * kind-  : INTEGER;       (* tag for unions *)
              * token* : Scanner.Token; (* scanner token  *)
              * type*  : D.Type;        (* typ-desc | NIL *)
              * hash*  : INTEGER;       (* hash bucket no *)
              * vMod-  : INTEGER;       (* visibility tag *)
              * dfScp* : D.Scope;       (* defining scope *)
              * tgXtn* : ANYPTR;
              * ---- ... inherited from Scope ...  ------ *
              * symTb*   : SymbolTable; (* symbol scope   *)
              * endDecl* : BOOLEAN;     (* can't add more *)
              * ovfChk*  : BOOLEAN;     (* check overflow *)
              * locals*  : IdSeq;       (* varId sequence *)
              * scopeNm* : L.CharOpen   (* external name  *)
              * ----------------------------------------- *)
                modBody*  : D.Stmt;     (* mod init-stmts *)
                modClose* : D.Stmt;     (* mod finaliz'n  *)
                impOrd*   : INTEGER;    (* implement ord. *)
                modKey*   : INTEGER;    (* module magicNm *)
                main*     : BOOLEAN;    (* module is main *)
                procs*    : PrcSeq;     (* local proclist *)
                expRecs*  : D.TypeSeq;  (* exported recs. *)
                xAttr*    : SET;        (* external types *)
                xName*    : L.CharOpen; (* ext module nam *)
                pkgNm*    : L.CharOpen; (* package name   *)
                clsNm*    : L.CharOpen; (* dummy class nm *)
                verNm*    : POINTER TO ARRAY 6 OF INTEGER;
                begTok*   : S.Token; 
                endTok*   : S.Token;
              END;      (* ------------------------------ *)

(* ============================================================ *)
(*               Append for the PrcSeq, ParSeq types.   *)
(* ============================================================ *)

  PROCEDURE InitPrcSeq*(VAR seq : PrcSeq; capacity : INTEGER);
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitPrcSeq;

  PROCEDURE ResetPrcSeq*(VAR seq : PrcSeq);
  BEGIN
    seq.tide := 0;
    IF seq.a = NIL THEN InitPrcSeq(seq, 2) END;
    seq.a[0] := NIL;
  END ResetPrcSeq;

  PROCEDURE AppendProc*(VAR seq : PrcSeq; elem : Procs);
    VAR temp : POINTER TO ARRAY OF Procs;
        i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN
      InitPrcSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendProc;

  PROCEDURE RemoveProc*(VAR seq : PrcSeq; elemPos : INTEGER);
  VAR
    ix : INTEGER;
  BEGIN
    FOR ix := elemPos TO seq.tide-2 DO
      seq.a[ix] := seq.a[ix+1];
    END;
    DEC(seq.tide);
  END RemoveProc;

(* -------------------------------------------- *)

  PROCEDURE InitParSeq*(VAR seq : ParSeq; capacity : INTEGER);
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitParSeq;

  PROCEDURE ResetParSeq*(VAR seq : ParSeq);
  BEGIN
    seq.tide := 0;
    IF seq.a = NIL THEN InitParSeq(seq, 2) END;
    seq.a[0] := NIL;
  END ResetParSeq;

  PROCEDURE AppendParam*(VAR seq : ParSeq; elem : ParId);
    VAR temp : POINTER TO ARRAY OF ParId;
        i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN
      InitParSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendParam;

(* ============================================================ *)
(*        Predicate implementations     *)
(* ============================================================ *)

  PROCEDURE (s : AbVar)mutable*() : BOOLEAN,EXTENSIBLE;
  (** Determine if this variable is mutable in this scope.  *
   *  Overrides mutable() for Symbols.Idnt      *)
  BEGIN
    IF s.kind = conId THEN RETURN FALSE;
    ELSE RETURN (s.vMod = D.pubMode)  (* public vars are RW *)
             OR (s.vMod = D.protect)  (* bad access caught elsewhere *)
             OR ((s.dfScp # NIL)      (* or scope not import  *)
                & (s.dfScp.kind # impId)
                & (s.dfScp.kind # alias));
    END;
  END mutable;

(* -------------------------------------------- *)

  PROCEDURE (s : AbVar)CheckMutable*(x : D.Expr),EXTENSIBLE;
  (** Determine if this variable is mutable in this scope.  *
   *  Overrides CheckMutable() for Symbols.Idnt     *)
  BEGIN
    IF s.kind = conId THEN x.ExprError(180) END;
    IF ~((s.vMod = D.pubMode)   (* public vars are RW *)
        OR ((s.dfScp # NIL)     (* or scope not import  *)
              & (s.dfScp.kind # impId)
              & (s.dfScp.kind # alias))) THEN x.ExprError(180);
    END;
  END CheckMutable;

(* -------------------------------------------- *)

  PROCEDURE (s : ParId)mutable*() : BOOLEAN;
  (** Determine if this variable is mutable in this scope.  *
   *  Overrides mutable() for IdDesc.AbVar      *)
  BEGIN
    RETURN (s.parMod # D.in)    (* ok if param not IN *)
  END mutable;

(* -------------------------------------------- *)

  PROCEDURE (s : ParId)CheckMutable*(x : D.Expr);
  (** Determine if this variable is mutable in this scope.  *
   *  Overrides CheckMutable() for IdDesc.AbVar     *)
  BEGIN
    IF s.parMod = D.in THEN x.ExprError(179) END;
  END CheckMutable;

(* -------------------------------------------- *)

  PROCEDURE (s : BlkId)isImport*() : BOOLEAN;
  (** Determine if this block is an module-import descriptor. *
   *  Overrides isImport() for Symbols.Scope.     *)
  BEGIN RETURN s.kind # modId END isImport;

(* -------------------------------------------- *)

  PROCEDURE (s : BlkId)isWeak*() : BOOLEAN;
  (** Determine if this block is an indirect module-import. *
   *  Overrides isWeak() for Symbols.Scope.     *)
  BEGIN RETURN D.weak IN s.xAttr END isWeak;

(* -------------------------------------------- *)

  PROCEDURE (s : AbVar)isStatic*() : BOOLEAN;
  (** Determine if this variable is a static variable.    *
   *  Overrides isStatic() for Symbols.Idnt.      *)
  BEGIN
    RETURN (s.dfScp # NIL)    (* Var is static iff: *)
         & (s.dfScp IS BlkId);    (* parent is a BlkId.   *)
  END isStatic;

(* -------------------------------------------- *)

  PROCEDURE (s : Procs)isStatic*() : BOOLEAN;
  (** Determine if this procedure is a static procedure.  *
   *  Overrides isStatic() for Symbols.Idnt.      *)
  BEGIN
    RETURN (s.kind = conPrc)    (* Proc is static iff:  *)
        OR (s.kind = fwdPrc);   (* it is not a method.  *)
  END isStatic;

(* -------------------------------------------- *)

  PROCEDURE (s : LocId)isLocalVar*() : BOOLEAN;
  (** Determine if this variable is a local var or parameter.   *
   *  Overrides isLocalVar() for Symbols.Idnt.                  *
   *
   *  This predicate is called by JavaMaker.  It should return  *
   *  FALSE if the variable is in an XHR (non-locally accessed)   *)
  BEGIN
    RETURN ~(uplevA IN s.locAtt);
  (*
    RETURN TRUE;
   *)
  END isLocalVar;

(* -------------------------------------------- *)

  PROCEDURE (s : AbVar)isDynamic*() : BOOLEAN,EXTENSIBLE;
  (** Determine if this variable is of dynamic type.    *
   *  A variable is dynamic if it is a pointer to a record. *
   *  Overrides isDynamic() for Symbols.Idnt.     *)
  BEGIN
    RETURN (s.type # NIL) & s.type.isDynamicType();
  END isDynamic;

(* -------------------------------------------- *)

  PROCEDURE (s : ParId)isDynamic*() : BOOLEAN;
  (** Determine if this parameter is of dynamic type.   *
   *  A parameter is dynamic if it is a pointer to a record,  *
   *  OR if it is a VAR or IN parameter of record type.   *
   *  Overrides isDynamic() for IdDesc.AbVar.     *)
    VAR sTp : D.Type;
  BEGIN
    sTp := s.type;
    IF sTp # NIL THEN
      RETURN sTp.isDynamicType()
          OR sTp.isRecordType() & ((s.parMod = D.var) OR (s.parMod = D.in));
    END;
    RETURN FALSE;
  END isDynamic;

(* -------------------------------------------- *)

  PROCEDURE (s : MthId)isAbstract*() : BOOLEAN;
  (** Determine if this method is an abstract method.   *
   *  Overrides isAbstract() for Symbols.IdDesc.    *)
  BEGIN
    RETURN s.mthAtt * mask = isAbs;
  END isAbstract;

(* -------------------------------------------- *)

  PROCEDURE (s : MthId)isImported*() : BOOLEAN;
   (*  Overrides isImported() for Symbols.IdDesc.   *)
  BEGIN
    RETURN (s.bndType # NIL) & s.bndType.isImportedType();
  END isImported;

(* -------------------------------------------- *)

  PROCEDURE (s : MthId)callForbidden*() : BOOLEAN,NEW;
   (*
    *  A call is forbidden if
    *  (1) this is an override of an implement-only method
    *  (2) this is an imported, implement-only method
    *)
  BEGIN
    RETURN (noCall IN s.mthAtt) OR
           (s.vMod = D.rdoMode) & s.bndType.isImportedType();
  END callForbidden;

(* -------------------------------------------- *)

  PROCEDURE (s : MthId)isEmpty*() : BOOLEAN;
  (** Determine if this method is an abstract method.   *
   *  Overrides isEmpty() for Symbols.IdDesc.     *)
    VAR set : SET;
  BEGIN
    set := s.mthAtt * mask;
    RETURN (set = empty) OR (set = isAbs);
  END isEmpty;

(* -------------------------------------------- *)

  PROCEDURE (s : PrcId)isEmpty*() : BOOLEAN,EXTENSIBLE;
  (** Determine if this procedure is a .ctor method.    *
   *  Overrides isEmpty() for Symbols.IdDesc.     *)
  BEGIN
    RETURN (s.kind = ctorP) & 
           ((s.basCll = NIL) OR s.basCll.empty);
  END isEmpty;

(* -------------------------------------------- *)

  PROCEDURE (s : ParId)parMode*() : INTEGER;
  (** Return the parameter mode.    *
   *  Overrides pMode() for Symbols.IdDesc. *)
  BEGIN
    RETURN s.parMod;
  END parMode;

(* -------------------------------------------- *)

  PROCEDURE (s : LocId)isIn*(set : V.VarSet) : BOOLEAN;
  (** Determine if this variable is in this live set.   *
   *  Overrides isIn() for Symbols.IdDesc.              *)
  BEGIN 
    RETURN set.includes(s.varOrd);
  END isIn;

(* -------------------------------------------- *)

  PROCEDURE (id : OvlId)findProc*(p : Procs) : Procs, NEW;
  VAR
    index : INTEGER;
  BEGIN
    ASSERT(id.hash = p.hash);
    FOR index := 0 TO id.list.tide-1 DO
      IF p.type.sigsMatch(id.list.a[index].type) THEN
        RETURN id.list.a[index];
      END;
    END;
    RETURN NIL;
  END findProc;

(* ============================================================ *)
(*     Constructor procedures for Subtypes    *)
(* ============================================================ *)

  PROCEDURE newConId*() : ConId;
    VAR rslt : ConId;
  BEGIN
    NEW(rslt);
    rslt.isStd := FALSE;
    rslt.SetKind(conId);
    RETURN rslt;
  END newConId;

(* -------------------------------------------- *)

  PROCEDURE newTypId*(type : D.Type) : TypId;
    VAR rslt : TypId;
  BEGIN
    NEW(rslt);
    rslt.type  := type;
    rslt.SetKind(typId);
    RETURN rslt;
  END newTypId;

(* -------------------------------------------- *)

  PROCEDURE newDerefId*(ptrId : D.Idnt) : TypId;
    VAR rslt : TypId;
  BEGIN
    rslt := newTypId(NIL);
(*
 *  rslt.hash  := N.enterStr(N.charOpenOfHash(ptrId.hash)^ + '^');
 *)
    rslt.hash  := ptrId.hash;
    rslt.dfScp := ptrId.dfScp;
    RETURN rslt;
  END newDerefId;


(* -------------------------------------------- *)

  PROCEDURE newAnonId*(ord : INTEGER) : TypId;
    VAR rslt : TypId;
        iStr : ARRAY 16 OF CHAR;
  BEGIN
    rslt := newTypId(NIL);
    GPText.IntToStr(ord, iStr);
    rslt.hash  := N.enterStr(D.anonMrk + iStr);
    RETURN rslt;
  END newAnonId;

(* -------------------------------------------- *)

  PROCEDURE newSfAnonId*(ord : INTEGER) : TypId;
    VAR rslt : TypId;
        iStr : ARRAY 16 OF CHAR;
  BEGIN
    rslt := newTypId(NIL);
    GPText.IntToStr(ord, iStr);
    rslt.hash  := N.enterStr("__t" + iStr);
    RETURN rslt;
  END newSfAnonId;

(* -------------------------------------------- *)

  PROCEDURE newVarId*() : VarId;
    VAR rslt : VarId;
  BEGIN
    NEW(rslt); rslt.SetKind(varId); RETURN rslt;
  END newVarId;

(* -------------------------------------------- *)

  PROCEDURE newLocId*() : LocId;
    VAR rslt : LocId;
  BEGIN
    NEW(rslt); rslt.SetKind(varId); RETURN rslt;
  END newLocId;

(* -------------------------------------------- *)

  PROCEDURE newFldId*() : FldId;
    VAR rslt : FldId;
  BEGIN
    NEW(rslt); rslt.SetKind(fldId); RETURN rslt;
  END newFldId;

(* -------------------------------------------- *)

  PROCEDURE newParId*() : ParId;
    VAR rslt : ParId;
  BEGIN
    NEW(rslt); rslt.SetKind(parId); RETURN rslt;
  END newParId;

(* -------------------------------------------- *)

  PROCEDURE newQuaId*() : ParId;
    VAR rslt : ParId;
  BEGIN
    NEW(rslt); rslt.SetKind(quaId); RETURN rslt;
  END newQuaId;

(* -------------------------------------------- *)

  PROCEDURE newOvlId*() : OvlId;
    VAR rslt : OvlId;
  BEGIN
    NEW(rslt);
    rslt.SetKind(errId);
    InitPrcSeq(rslt.list, 2);
    RETURN rslt;
  END newOvlId;

(* -------------------------------------------- *)

  PROCEDURE newPrcId*() : PrcId;
    VAR rslt : PrcId;
  BEGIN
    NEW(rslt);
    rslt.SetKind(errId);
    rslt.stdOrd := 0;
    RETURN rslt;
  END newPrcId;

(* -------------------------------------------- *)

  PROCEDURE newMthId*() : MthId;
    VAR rslt : MthId;
  BEGIN
    NEW(rslt);
    rslt.SetKind(errId);
    rslt.mthAtt := {};
    RETURN rslt;
  END newMthId;

(* -------------------------------------------- *)

  PROCEDURE newImpId*() : BlkId;
    VAR rslt : BlkId;
  BEGIN
    NEW(rslt);
    INCL(rslt.xAttr, D.weak);
    rslt.SetKind(impId);
    RETURN rslt;
  END newImpId;

(* -------------------------------------------- *)

  PROCEDURE newAlias*() : BlkId;
    VAR rslt : BlkId;
  BEGIN
    NEW(rslt); rslt.SetKind(alias); RETURN rslt;
  END newAlias;

(* -------------------------------------------- *)

  PROCEDURE newModId*() : BlkId;
    VAR rslt : BlkId;
  BEGIN
    NEW(rslt); rslt.SetKind(modId); RETURN rslt;
  END newModId;

(* ============================================================ *)
(*    Set procedures for ReadOnly fields    *)
(* ============================================================ *)

  PROCEDURE (c : ConId)SetStd*(),NEW;
  BEGIN
    c.isStd := TRUE;
  END SetStd;

(* -------------------------------------------- *)

  PROCEDURE (c : PrcId)SetOrd*(n : INTEGER),NEW;
  BEGIN
    c.stdOrd := n;
  END SetOrd;

(* -------------------------------------------- *)

  PROCEDURE (p : Procs)setPrcKind*(kind : INTEGER),NEW;
  BEGIN
    ASSERT((kind = conMth) OR (kind = conPrc) OR
           (kind = fwdMth) OR (kind = fwdPrc) OR
           (kind = ctorP));
    p.SetKind(kind);
  END setPrcKind;

(* ============================================================ *)
(*   Methods on PrcId type, for procedure/method entry. *)
(* ============================================================ *)

  PROCEDURE (desc : Procs)CheckElab*(fwd : D.Idnt),NEW,EMPTY;

(* -------------------------------------------- *)

  PROCEDURE (desc : PrcId)CheckElab*(fwd : D.Idnt);
    VAR fwdD : PrcId;
  BEGIN
    fwdD := fwd(PrcId);
    IF (fwdD.type # NIL) & (desc.type # NIL) THEN
      IF ~desc.type.procMatch(fwdD.type) THEN
        desc.IdError(65);
      ELSIF ~desc.type.namesMatch(fwdD.type) THEN
        desc.IdError(70);
      ELSIF fwdD.pAttr * useMsk # {} THEN
        desc.pAttr := desc.pAttr + fwdD.pAttr;
      END;
      IF desc.vMod = D.prvMode THEN desc.SetMode(fwd.vMod) END; (* copy *)
      fwdD.resolve := desc;
      (* ### *)
      fwdD.type := desc.type;
    END;
  END CheckElab;

(* -------------------------------------------- *)

  PROCEDURE (desc : MthId)CheckElab*(fwd : D.Idnt);
    VAR fwdD : MthId;
  BEGIN
    fwdD := fwd(MthId);
    IF desc.mthAtt # fwdD.mthAtt THEN desc.IdError(66) END;
    IF (desc.rcvFrm # NIL) & (fwdD.rcvFrm # NIL) THEN
      IF desc.rcvFrm.parMod # fwdD.rcvFrm.parMod THEN desc.IdError(64) END;
      IF desc.rcvFrm.hash   # fwdD.rcvFrm.hash   THEN desc.IdError(65) END;
      IF desc.rcvFrm.type   # fwdD.rcvFrm.type   THEN desc.IdError(70) END;
    END;
    IF (fwdD.type # NIL) & (desc.type # NIL) THEN
      IF ~desc.type.procMatch(fwdD.type) THEN
        desc.IdError(65);
      ELSIF ~desc.type.namesMatch(fwdD.type) THEN
        desc.IdError(70);
      ELSIF fwdD.pAttr * useMsk # {} THEN
        desc.pAttr := desc.pAttr + fwdD.pAttr;
      END;
      IF desc.vMod = D.prvMode THEN desc.SetMode(fwd.vMod) END; (* copy *)
      fwdD.resolve := desc;
      (* ### *)
      fwdD.type := desc.type;
    END;
  END CheckElab;

(* -------------------------------------------- *)

  PROCEDURE (desc : Procs)EnterProc*(rcv : ParId; scp : D.Scope),NEW,EMPTY;

(* -------------------------------------------- *)

  PROCEDURE (desc : PrcId)EnterProc*(rcv : ParId; scp : D.Scope);
    VAR fwd : D.Idnt;
  BEGIN
    ASSERT(rcv = NIL);
    IF D.refused(desc, scp) THEN
      fwd := scp.symTb.lookup(desc.hash);
      IF fwd.kind = fwdPrc THEN (* check the elaboration *)
        desc.CheckElab(fwd);
        scp.symTb.Overwrite(desc.hash, desc);
      ELSIF fwd.kind = fwdMth THEN
        fwd.IdError(62);
      ELSE
        desc.IdError(4);
      END;
    ELSE
    END;
  END EnterProc;

(* -------------------------------------------- *)

  PROCEDURE (desc : MthId)EnterProc*(rcv : ParId; scp : D.Scope);
    VAR fwd : D.Idnt;
        rTp : D.Type;
  BEGIN
    rTp := NIL;
    ASSERT(rcv # NIL);
    IF desc.dfScp.kind # modId THEN
       desc.IdError(122); RETURN;     (* PREMATURE RETURN *)
    END;
    IF rcv.isDynamic() THEN
      rTp := rcv.type.boundRecTp();
      IF (rcv.parMod # D.val) & rcv.type.isPointerType() THEN
        rcv.IdError(206); RETURN;     (* PREMATURE RETURN *)
      ELSIF rTp.isImportedType() THEN
        rcv.IdErrorStr(205, rTp.name()); RETURN;  (* PREMATURE RETURN *)
      END;
    ELSIF (rcv.type # NIL) & rcv.type.isRecordType() THEN
      desc.IdError(107); RETURN;      (* PREMATURE RETURN *)
    ELSE
      desc.IdError(104); RETURN;      (* PREMATURE RETURN *)
    END;
    IF rTp # NIL THEN       (* insert in rec. scope *)
      rTp.InsertMethod(desc);
      desc.bndType := rTp;
    END;
  END EnterProc;

(* -------------------------------------------- *)

  PROCEDURE (desc : Procs)MethodAttr(),NEW,EMPTY;

(* -------------------------------------------- *)

  PROCEDURE (mDesc : MthId)MethodAttr();
    VAR rcvTp : D.Type;
        bndTp : D.Type;
        inhId : D.Idnt;
        prevM : MthId;
        mMask, pMask : SET;
  BEGIN
    bndTp := mDesc.bndType;
    rcvTp := mDesc.rcvFrm.type;
    mMask := mDesc.mthAtt * mask;
    IF (mMask # isAbs) & bndTp.isInterfaceType() THEN
      mDesc.IdError(188); RETURN;
    END;
   (*
    *  Check #1: is there an equally named method inherited?
    *)
    inhId := bndTp.inheritedFeature(mDesc);
   (*
    *  Check #2: are the method attributes consistent
    *)
    IF inhId = NIL THEN
     (*
      *  2.0 If not an override, then must be NEW
      *)
      IF ~(newBit IN mDesc.mthAtt) THEN mDesc.IdError(105);
      ELSIF (rcvTp.idnt.vMod = D.prvMode) &
            (mDesc.vMod = D.pubMode) THEN mDesc.IdError(195);
      END;
    ELSIF inhId.kind = conMth THEN
      prevM := inhId(MthId);
      pMask := prevM.mthAtt * mask;
     (*
      *  2.1 Formals must match, with retType covariant maybe
      *)
      prevM.type.CheckCovariance(mDesc);
     (*
      *  2.2 If an override, then must not be NEW
      *)
      IF newBit IN mDesc.mthAtt THEN mDesc.IdError(106) END;
     (*
      *  2.3 Super method must be extensible
      *)
      IF pMask = final THEN mDesc.IdError(108) END;
     (*
      *  2.4 If this is abstract, so must be the super method
      *)
      IF (mMask = isAbs) & (pMask # isAbs) THEN mDesc.IdError(109) END;
     (*
      *  2.5 If empty, the super method must be abstract or empty
      *)
      IF (mMask = empty) &
         (pMask # isAbs) & (pMask # empty) THEN mDesc.IdError(112) END;
     (*
      *  2.6 If inherited method is exported, then so must this method
      *)

     (*
      *  Not clear about the semantics here.  The ComPlus2 VOS
      *  (and the JVM) rejects redefined methods that try to 
      *  limit access, even if the receiver is not public.
      *
      *  It would be possible to only reject cases where the 
      *  receiver is exported, and then secretly mark the method
      *  definition in the IL as public after all ...
      *                                      (kjg 17-Dec-2001)
      *  ... and this is the implemented semantics from gpcp 1.1.5
      *                                      (kjg 10-Jan-2002)
      *)
      IF (prevM.vMod = D.pubMode) &
         (mDesc.vMod # D.pubMode) THEN 
        IF rcvTp.idnt.vMod = D.pubMode THEN
          mDesc.IdError(113);
        ELSE
          INCL(mDesc.mthAtt, widen);
        END; 
      ELSIF (prevM.vMod = D.rdoMode) &
            (mDesc.vMod # D.rdoMode) THEN 
        IF rcvTp.idnt.vMod = D.pubMode THEN
          mDesc.IdError(223);
        ELSIF rcvTp.idnt.vMod = D.prvMode THEN
          INCL(mDesc.mthAtt, widen);
        END; 
      END;
     (*
      *  If inherited method is overloaded, then so must this be.
      *)
      IF prevM.prcNm # NIL THEN mDesc.prcNm := prevM.prcNm END;
    ELSE
      mDesc.IdError(4);
    END;
    IF (mMask = isAbs) & ~bndTp.isAbsRecType() THEN
     (*
      *  Check #3: if method is abstract bndTp must be abstract
      *)
      rcvTp.TypeError(110);
    ELSIF mMask = empty THEN
     (*
      *  Check #4: if method is empty then no-ret and no OUTpars
      *)
      mDesc.type.CheckEmptyOK();
      IF (newBit IN mDesc.mthAtt) & ~bndTp.isExtnRecType() THEN
       (*
        *  Check #5: if mth is empty and new, rcv must be extensible
        *)
        rcvTp.TypeError(111);
      END;
    ELSIF (mMask = extns) & ~bndTp.isExtnRecType() THEN
     (*
      *  Check #6: if mth is ext. rcv must be abs. or extensible
      *)
      S.SemError.RepSt1(117,
                        D.getName.ChPtr(rcvTp.idnt),
                        mDesc.token.lin, mDesc.token.col);
    END;
  END MethodAttr;

(* -------------------------------------------- *)

  PROCEDURE (desc : Procs)retTypBound*() : D.Type,NEW,EXTENSIBLE;
  BEGIN RETURN NIL END retTypBound;

(* -------------------------------------------- *)

  PROCEDURE (mDesc : MthId)retTypBound*() : D.Type;
    VAR bndTp : D.Type;
        prevM : MthId;
  BEGIN
    bndTp := mDesc.bndType;
    prevM := bndTp.inheritedFeature(mDesc)(MthId);
    IF covar IN prevM.mthAtt THEN
      RETURN prevM.retTypBound();
    ELSE
      RETURN prevM.type.returnType();
    END;
  END retTypBound;

(* -------------------------------------------- *)

  PROCEDURE (prc : Procs)RetCheck(fin : V.VarSet; eNm : INTEGER),NEW;
  BEGIN
    IF ~prc.type.isProperProcType() & (* ==> function procedure *)
       ~prc.isAbstract() &    (* ==> concrete procedure *)
       ~fin.isUniv() THEN   (* ==> flow missed RETURN *)
      prc.IdError(136);
      prc.IdError(eNm);
    END;
  END RetCheck;

(* -------------------------------------------- *)

  PROCEDURE (var : AbVar)VarInit(ini : V.VarSet),NEW;
  BEGIN
    WITH var : ParId DO
        IF (var.parMod # D.out) OR
           ~var.type.isScalarType() THEN ini.Incl(var.varOrd) END;
    | var : LocId DO
        IF ~var.type.isScalarType() THEN ini.Incl(var.varOrd) END;
    | var : VarId DO
        IF ~var.type.isScalarType() THEN ini.Incl(var.varOrd) END;
    ELSE
    END;
  END VarInit;

(* -------------------------------------------- *)

  PROCEDURE (mod : BlkId)LiveInitialize*(ini : V.VarSet);
    VAR var : D.Idnt;
        ix  : INTEGER;
  BEGIN
    (* initialize the local vars *)
    FOR ix := 0 TO mod.locals.tide-1 DO
      var := mod.locals.a[ix];
      var(AbVar).VarInit(ini);
    END;
  END LiveInitialize;

(* -------------------------------------------- *)

  PROCEDURE (prc : Procs)LiveInitialize*(ini : V.VarSet);
    VAR var : D.Idnt;
        ix  : INTEGER;
  BEGIN
    (* [initialize the receiver] *)
    (* initialize the parameters *)
    (* initialize the quasi-pars *)
    (* initialize the local vars *)
    FOR ix := 0 TO prc.locals.tide-1 DO
      var := prc.locals.a[ix];
      var(AbVar).VarInit(ini);
    END;
  END LiveInitialize;

(* -------------------------------------------- *)

  PROCEDURE (prc : Procs)UplevelInitialize*(ini : V.VarSet);
    VAR var : LocId;
        ix  : INTEGER;
  BEGIN
    FOR ix := 0 TO prc.locals.tide-1 DO
     (*  
      *  If we were setting uplevR and uplevW separately, we
      *  could be less conservative and test uplevW only.
      *)
      var := prc.locals.a[ix](LocId);
      IF uplevA IN var.locAtt THEN ini.Incl(var.varOrd) END;
    END;
  END UplevelInitialize;

(* ============================================================ *)
(*    Methods on BlkId type, for mainline computation *)
(* ============================================================ *)

  PROCEDURE (b : BlkId)EmitCode*(),NEW;
  BEGIN
  END EmitCode;

(* -------------------------------------------- *)

  PROCEDURE (b : BlkId)TypeErasure*(sfa : D.SymForAll), NEW;
    VAR prcIx : INTEGER;
        iDesc : D.Idnt;
        pDesc : Procs;
  BEGIN
    FOR prcIx := 0 TO b.procs.tide - 1 DO
      iDesc := b.procs.a[prcIx];
      pDesc := iDesc(Procs);
      IF (pDesc.kind # fwdPrc) &
         (pDesc.kind # fwdMth) &
         (pDesc.body # NIL)    THEN
        IF pDesc.body # NIL THEN pDesc.body.TypeErase(pDesc) END;
        IF pDesc.rescue # NIL THEN pDesc.rescue.TypeErase(pDesc) END;
      END;
    END;
    IF b.modBody  # NIL THEN b.modBody.TypeErase(b) END;
    IF b.modClose # NIL THEN b.modClose.TypeErase(b) END;
    (* Erase types in the symbol table *)
    b.symTb.Apply(sfa);
  END TypeErasure;

(* -------------------------------------------- *)

  PROCEDURE (b : BlkId)StatementAttribution*(sfa : D.SymForAll),NEW;
    VAR prcIx : INTEGER;
        iDesc : D.Idnt;
        pDesc : Procs;
        bType : D.Type;
        dName : L.CharOpen;
   (* ---------------------------------------- *)
    PROCEDURE parentIsCalled(mthd : MthId) : BOOLEAN;
      VAR prId : D.Idnt;
    BEGIN
     (*
      *  Invariant : ~(called IN mthd.pAttr)
      *)
      LOOP
        IF newBit IN mthd.mthAtt THEN RETURN FALSE;
        ELSE
          prId := mthd.bndType.inheritedFeature(mthd);
         (* This next can never be true for correct programs *)
          IF prId = NIL THEN RETURN FALSE END;
          mthd := prId(MthId);
          IF prId.isImported() OR
             (mthd.pAttr * useMsk # {}) THEN RETURN TRUE END;
        END;
      END;
    END parentIsCalled;
   (* ---------------------------------------- *)
  BEGIN
    FOR prcIx := 0 TO b.procs.tide - 1 DO
      iDesc := b.procs.a[prcIx];
      pDesc := iDesc(Procs);
      IF (pDesc.kind = fwdPrc) OR (pDesc.kind = fwdMth) THEN
        IF pDesc.resolve = NIL THEN pDesc.IdError(72) END;
      ELSIF pDesc.kind = ctorP THEN
        bType := pDesc.type.returnType();
        IF bType # NIL THEN bType := bType.boundRecTp() END;
        IF bType = NIL THEN
          pDesc.IdError(201);
        ELSIF bType.isImportedType() THEN
          pDesc.IdError(200);
        ELSE (* remainder of semantic checks in AppendCtor *)
          bType.AppendCtor(pDesc);
        END;
      ELSE
        IF pDesc.kind = conMth THEN pDesc.MethodAttr() END;
        IF pDesc.body # NIL THEN pDesc.body.StmtAttr(pDesc) END;;
        IF pDesc.rescue # NIL THEN pDesc.rescue.StmtAttr(pDesc) END;;
       (*
        *  Now we generate warnings for useless procedures.
        *)
        IF pDesc.pAttr * useMsk = {} THEN
          WITH pDesc : MthId DO
           (*
            *  The test here is tricky: if an overridden
            *  method is called, then this method might
            *  be dynamically dispatched.  We check this.
            *)
            IF ~parentIsCalled(pDesc) THEN pDesc.IdError(304) END;
          ELSE
           (*
            *  On the other hand, if it is static, not exported
            *  and is not called then it definitely is useless.
            *)
            pDesc.IdError(304);
          END;
        END;
      END;
    END;
    b.symTb.Apply(sfa);
   (*
    *  Now we must check if the synthetic static class
    *  in the .NET version will have a name clash with
    *  any other symbol in the assembly.
    *  If so, we must mangle the explicit name.
    *)  
    IF D.trgtNET & 
       ~(D.rtsMd IN b.xAttr) &
       (b.symTb.lookup(b.hash) # NIL) THEN
      dName := D.getName.ChPtr(b);
      b.scopeNm := LitValue.strToCharOpen("[" + dName^ + "]" + dName^);
      b.hash := N.enterStr("__" + dName^);
      S.SemError.RepSt1(308, D.getName.ChPtr(b), b.token.lin, b.token.col);
    END;
    IF b.modBody  # NIL THEN b.modBody.StmtAttr(b) END;
    IF b.modClose # NIL THEN b.modClose.StmtAttr(b) END;
  END StatementAttribution;

(* -------------------------------------------- *)

  PROCEDURE (b : BlkId)DataflowAttribution*(),NEW;
    VAR prcIx : INTEGER;
        iDesc : D.Idnt;
        pDesc : Procs;
        initL : V.VarSet;
  BEGIN
   (*
    *  Fix up the modes of quasi parameters here ...
    *)

   (*
    *  Now do dataflow analysis on each procedure ...
    *)
    FOR prcIx := 0 TO b.procs.tide - 1 DO
      iDesc := b.procs.a[prcIx];
      pDesc := iDesc(Procs);
      IF (pDesc.kind # fwdPrc) &
         (pDesc.kind # fwdMth) &
         (pDesc.body # NIL)    THEN
       (*
        *  We do flow analysis even if there are no local
        *  variables, in order to diagnose paths that miss
        *  RETURN in function procedures.
        *
        *  Note that we throw an extra, dummy variable into
        *  the set so that the RetCheck will always have a
        *  missing local if there has been no return stmt.
        *)
        initL := V.newSet(pDesc.locals.tide+1);
        pDesc.LiveInitialize(initL);
        initL := pDesc.body.flowAttr(pDesc, initL);
        pDesc.RetCheck(initL, 136);
        pDesc.type.OutCheck(initL);
        IF (pDesc.rescue # NIL) THEN
          initL := V.newSet(pDesc.locals.tide+1);
          pDesc.LiveInitialize(initL);
          initL.Incl(pDesc.except.varOrd);
          initL := pDesc.rescue.flowAttr(pDesc, initL);
          pDesc.RetCheck(initL, 138);
          pDesc.type.OutCheck(initL);
        END;
      END;
    END;
    initL := V.newSet(b.locals.tide);
    b.LiveInitialize(initL);
    IF b.modBody  # NIL THEN initL := b.modBody.flowAttr(b, initL) END;
    IF b.modClose # NIL THEN initL := b.modClose.flowAttr(b, initL) END;
  END DataflowAttribution;

(* ============================================================ *)
(*      Diagnostic methods      *)
(* ============================================================ *)

  PROCEDURE PType(t : D.Type);
  BEGIN
    IF t # NIL THEN Console.WriteString(t.name()) END;
  END PType;

  (* ------------------------------- *)

  PROCEDURE KType*(i : INTEGER);
  BEGIN
    CASE i OF
    | errId   : Console.WriteString("errId  ");
    | conId   : Console.WriteString("conId  ");
    | varId   : Console.WriteString("varId  ");
    | parId   : Console.WriteString("parId  ");
    | quaId   : Console.WriteString("quaId  ");
    | typId   : Console.WriteString("typId  ");
    | modId   : Console.WriteString("modId  ");
    | impId   : Console.WriteString("impId  ");
    | alias   : Console.WriteString("alias  ");
    | fldId   : Console.WriteString("fldId  ");
    | fwdMth  : Console.WriteString("fwdMth ");
    | conMth  : Console.WriteString("conMth ");
    | fwdPrc  : Console.WriteString("fwdPrc ");
    | conPrc  : Console.WriteString("conPrc ");
    | fwdTyp  : Console.WriteString("fwdTyp ");
    | ctorP   : Console.WriteString("ctorP  ");
    ELSE Console.WriteString("ERROR ");
    END;
  END KType;

  (* ------------------------------- *)

  PROCEDURE (s : ConId)Diagnose*(i : INTEGER);
  BEGIN
    s.SuperDiag(i);
    H.Indent(i+2); KType(s.kind); Console.WriteLn;
    IF s.conExp # NIL THEN s.conExp.Diagnose(i+4) END;
  END Diagnose;

  PROCEDURE (s : FldId)Diagnose*(i : INTEGER);
  BEGIN
    s.SuperDiag(i);
    H.Indent(i+2); KType(s.kind);
    IF s.type # NIL THEN PType(s.type) END;
    Console.WriteLn;
  END Diagnose;

  PROCEDURE (s : TypId)Diagnose*(i : INTEGER);
  BEGIN
    s.SuperDiag(i);
    H.Indent(i+2); KType(s.kind);
    IF s.type # NIL THEN
      PType(s.type); Console.WriteLn;
      s.type.SuperDiag(i+2);
    END;
    Console.WriteLn;
  END Diagnose;

  PROCEDURE (s : AbVar)Diagnose*(i : INTEGER),EXTENSIBLE;
  BEGIN
    s.SuperDiag(i);
    H.Indent(i+2); KType(s.kind);
    IF s.type # NIL THEN PType(s.type) END;
    Console.WriteLn;
  END Diagnose;

  PROCEDURE (s : ParId)Diagnose*(i : INTEGER);
  BEGIN
    s.SuperDiag(i);
    H.Indent(i+2); KType(s.kind);
    IF s.type # NIL THEN PType(s.type) END;
    Console.WriteLn;
  END Diagnose;

  PROCEDURE (s : ParId)DiagPar*(),NEW;
    VAR str : L.CharOpen;
  BEGIN
    Console.WriteString(D.modStr[s.parMod]);
    str := D.getName.ChPtr(s);
    IF str # NIL THEN
      Console.WriteString(str);
    ELSE
      Console.WriteString("(p#");
      Console.WriteInt(s.varOrd,1);
      Console.Write(")");
    END;
    Console.WriteString(" : ");
    Console.WriteString(s.type.name());
  END DiagPar;

  PROCEDURE (s : LocId)DiagVar*(),NEW;
  BEGIN
    Console.WriteString(D.getName.ChPtr(s));
    Console.WriteString(" (#");
    Console.WriteInt(s.varOrd,1);
    Console.Write(")");
    Console.WriteString(" : ");
    Console.WriteString(s.type.name());
    Console.Write(";");
  END DiagVar;

  PROCEDURE (s : Procs)DiagVars(i : INTEGER),NEW;
    VAR var : D.Idnt;
        ix  : INTEGER;
  BEGIN
    H.Indent(i); Console.Write("{");
    IF s.locals.tide = 0 THEN
      Console.Write("}");
    ELSE
      Console.WriteLn;
      FOR ix := 0 TO s.locals.tide-1 DO
        H.Indent(i+4);
        var := s.locals.a[ix];
        var(LocId).DiagVar();
        Console.WriteLn;
      END;
      H.Indent(i); Console.Write("}");
    END;
    Console.WriteLn;
  END DiagVars;

  PROCEDURE (s : PrcId)Diagnose*(i : INTEGER);
  BEGIN
    H.Indent(i); Console.WriteString("PROCEDURE");
    IF s.kind = fwdPrc THEN Console.Write("^") END;
    Console.Write(" ");
    Console.WriteString(D.getName.ChPtr(s));
    s.type.DiagFormalType(i+4);
    IF s.kind = ctorP THEN Console.WriteString(",CONSTRUCTOR") END;
    Console.WriteLn;
    s.DiagVars(i);
    D.DoXName(i, s.prcNm);
    D.DoXName(i, s.clsNm);
    D.DoXName(i, s.scopeNm);
  END Diagnose;

  PROCEDURE (s : MthId)Diagnose*(i : INTEGER);
  BEGIN
    H.Indent(i); Console.WriteString("PROCEDURE");
    IF s.kind = fwdMth THEN Console.Write("^") END;
    Console.Write(" ");
    Console.Write("(");
    s.rcvFrm.DiagPar();
    Console.Write(")");
    Console.WriteString(D.getName.ChPtr(s));
    s.type.DiagFormalType(i+4);
    Console.WriteLn;
    s.DiagVars(i);
    D.DoXName(i, s.prcNm);
  END Diagnose;

  PROCEDURE (s : OvlId)Diagnose*(i : INTEGER);
  VAR
    index : INTEGER;
  BEGIN
    H.Indent(i); Console.WriteString("OVERLOADED PROCS with name <");
    Console.WriteString(D.getName.ChPtr(s));
    Console.WriteString(">");
    Console.WriteLn;
    FOR index := 0 TO s.list.tide-1 DO
      s.list.a[index].Diagnose(i+2);
    END;
    H.Indent(i); Console.WriteString("END OVERLOADED PROCS with name ");
    Console.WriteString(D.getName.ChPtr(s));
    Console.WriteString(">");
    Console.WriteLn;
  END Diagnose;

  PROCEDURE (s : BlkId)Diagnose*(i : INTEGER);
  BEGIN
    s.SuperDiag(i);
    H.Indent(i+2); KType(s.kind);
    IF D.weak IN s.xAttr THEN Console.WriteString(" (weak)") END;
    Console.WriteLn;
    s.symTb.Dump(i+4);
    D.DoXName(i, s.scopeNm);
    D.DoXName(i, s.xName);
  END Diagnose;

(* ============================================================ *)
BEGIN (* ====================================================== *)
END IdDesc.   (* ============================================== *)
(* ============================================================ *)

