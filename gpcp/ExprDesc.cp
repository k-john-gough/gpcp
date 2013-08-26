(* ==================================================================== *)
(*                                                                      *)
(*  ExprDesc Module for the Gardens Point Component Pascal Compiler.    *)
(*  Implements Expr. Descriptors that are extensions of Symbols.Expr    *)
(*  Copyright (c) John Gough 1999, 2000.                                *)
(*                                                                      *)
(* ==================================================================== *)

MODULE ExprDesc;

  IMPORT
        GPCPcopyright,
        RTS,
        Console,
        Builtin,
        G := CompState,
        S := CPascalS,
        L := LitValue,
        D := Symbols,
        I := IdDesc,
        T := TypeDesc,
        H := DiagHelper,
        V := VarSets,
        FileNames;

(* ============================================================ *)

  CONST (* expr-kinds *)
  (* leaves *)
    qualId* = 0;  numLt*  = 1;  realLt* = 2;  charLt* = 3;  strLt* = 4;
    nilLt*  = 5;  tBool*  = 6;  fBool*  = 7;  setLt*  = 8;  setXp* = 9;

  (* unaries *)
    deref*  = 10; selct*  = 11; tCheck* = 12; mkStr*  = 13; fnCall* = 14;
    prCall* = 15; mkBox*  = 16; compl*  = 17; sprMrk* = 18; neg*    = 19;
    absVl*  = 20; entVl*  = 21; capCh*  = 22; strLen* = 23; strChk* = 24;
    cvrtUp* = 25; cvrtDn* = 26; oddTst* = 27; mkNStr* = 28; getTp*  = 29;

  (* leaves *)
    typOf*  = 30; infLt*  = 31; nInfLt* = 32;

  (* binaries *)                index*  = 32; range*  = 33; lenOf*  = 34;
    maxOf*  = 35; minOf*  = 36; bitAnd* = 37; bitOr*  = 38; bitXor* = 39;
    plus*   = 40; minus*  = 41; greT*   = 42; greEq*  = 43; notEq*  = 44;
    lessEq* = 45; lessT*  = 46; equal*  = 47; isOp*   = 48; inOp*   = 49;
    mult*   = 50; slash*  = 51; modOp*  = 52; divOp*  = 53; blNot*  = 54;
    blOr*   = 55; blAnd*  = 56; strCat* = 57; ashInt* = 58; rem0op* = 59;
    div0op* = 60; lshInt* = 61; rotInt* = 62;

  (* more unaries *)
    adrOf*  = 70; 


(* ============================================================ *)

  TYPE
    LeafX*  = POINTER TO EXTENSIBLE RECORD (D.Expr)
             (* ... inherited from Expr ... ------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* exp mark token *)
              * type*  : Type;
              * ----------------------------------------- *)
                value* : L.Value;
              END;  (* ------------------------------ *)

    IdLeaf* = POINTER TO RECORD (LeafX)
                ident*   : D.Idnt;  (* qualified-idnt *)
              END;

    SetExp* = POINTER TO RECORD (LeafX)
                varSeq*  : D.ExprSeq;
              END;


(* ============================================================ *)

  TYPE
    UnaryX* = POINTER TO EXTENSIBLE RECORD (D.Expr)
             (* ... inherited from Expr ... ------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* exp mark token *)
              * type*  : Type;
              * ----------------------------------------- *)
                kid*   : D.Expr;
              END;  (* ------------------------------ *)

    IdentX* = POINTER TO RECORD (UnaryX)
                ident*   : D.Idnt;  (* field selction *)
              END;

    CallX*  = POINTER TO RECORD (UnaryX)
                actuals* : D.ExprSeq;
              END;

(* ============================================================ *)

  TYPE
    BinaryX* = POINTER TO RECORD (D.Expr)
             (* ... inherited from Expr ... ------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* exp mark token *)
              * type*  : Type;
              * ----------------------------------------- *)
                lKid*  : D.Expr;
                rKid*  : D.Expr;
              END;  (* ------------------------------ *)

(* ============================================================ *)

  PROCEDURE isPowerOf2(val : LONGINT) : BOOLEAN;
    VAR lo, hi : INTEGER;
  BEGIN
    IF val < 0 THEN
      RETURN FALSE;
    ELSE
      lo := RTS.loInt(val);
      hi := RTS.hiInt(val);
      IF hi = 0 THEN
        RETURN BITS(lo) * BITS(-lo) = BITS(lo);
      ELSIF lo = 0 THEN
        RETURN BITS(hi) * BITS(-hi) = BITS(hi);
      ELSE
        RETURN FALSE;
      END;
    END;
  END isPowerOf2;

(* -------------------------------------------- *)

  PROCEDURE coverType(a,b : D.Type) : D.Type;
  BEGIN
    IF    a.includes(b) THEN RETURN a;
    ELSIF b.includes(a) THEN RETURN b;
    ELSIF a = Builtin.uBytTp THEN RETURN coverType(Builtin.sIntTp, b);
    ELSIF b = Builtin.uBytTp THEN RETURN coverType(a, Builtin.sIntTp);
    ELSE RETURN NIL;
    END;
  END coverType;

(* -------------------------------------------- *)

  PROCEDURE log2(val : LONGINT) : INTEGER;
    VAR lo, hi, nm : INTEGER;
  BEGIN
    lo := RTS.loInt(val);
    hi := RTS.hiInt(val);
    IF hi = 0 THEN
      FOR nm := 0 TO 31 DO
        IF ODD(lo) THEN RETURN nm ELSE lo := lo DIV 2 END;
      END;
    ELSE
      FOR nm := 32 TO 63 DO
        IF ODD(hi) THEN RETURN nm ELSE hi := hi DIV 2 END;
      END;
    END;
    THROW("Bad log2 argument");
    RETURN 0;
  END log2;

(* ============================================================ *)
  PROCEDURE^ (x : LeafX)charValue*() : CHAR,NEW;
  PROCEDURE^ convert(expr : D.Expr; dstT : D.Type) : D.Expr;
  PROCEDURE^ FormalsVsActuals*(prcX : D.Expr; actSeq : D.ExprSeq);
(* ============================================================ *)
(*          LeafX Constructor methods     *)
(* ============================================================ *)

  PROCEDURE mkLeafVal*(k : INTEGER; v : L.Value) : LeafX;
    VAR n : LeafX;
  BEGIN
    NEW(n); n.token := S.prevTok;
    n.SetKind(k); n.value := v; RETURN n;
  END mkLeafVal;

(* -------------------------------------------- *)

  PROCEDURE mkNilX*() : LeafX;
    VAR n : LeafX;
  BEGIN
    NEW(n);
    n.type := Builtin.anyPtr;
    n.token := S.prevTok;
    n.SetKind(nilLt); RETURN n;
  END mkNilX;

(* -------------------------------------------- *)

  PROCEDURE mkInfX*() : LeafX;
    VAR n : LeafX;
  BEGIN
    NEW(n);
   (* 
    *  Here is a dirty trick!
    *   We assign this the type SHORTREAL, and trap
    *   the attempt to coerce the value to REAL.
    *   If the value is coerced we assign it the type 
    *   Bi.realTp  so that the correct constant is emitted.
    *)
    n.type := Builtin.sReaTp;      
    n.token := S.prevTok;
    n.SetKind(infLt); RETURN n;
  END mkInfX;

(* -------------------------------------------- *)

  PROCEDURE mkNegInfX*() : LeafX;
    VAR n : LeafX;
  BEGIN
    NEW(n);
    n.type := Builtin.sReaTp;
    n.token := S.prevTok;
    n.SetKind(nInfLt); RETURN n;
  END mkNegInfX;

(* -------------------------------------------- *)

  PROCEDURE mkTrueX*() : LeafX;
    VAR n : LeafX;
  BEGIN
    NEW(n);
    n.type := Builtin.boolTp;
    n.token := S.prevTok;
    n.SetKind(tBool); RETURN n;
  END mkTrueX;

(* -------------------------------------------- *)

  PROCEDURE mkFalseX*() : LeafX;
    VAR n : LeafX;
  BEGIN
    NEW(n);
    n.type := Builtin.boolTp;
    n.token := S.prevTok;
    n.SetKind(fBool); RETURN n;
  END mkFalseX;

(* -------------------------------------------- *)

  PROCEDURE mkIdLeaf*(id : D.Idnt) : IdLeaf;
    VAR l : IdLeaf;
  BEGIN
    NEW(l);
    (* l.type := NIL; *)
    l.token := S.prevTok;
    l.SetKind(qualId); l.ident := id; RETURN l;
  END mkIdLeaf;

(* -------------------------------------------- *)

  PROCEDURE mkEmptySet*() : SetExp;
    VAR l : SetExp;
  BEGIN
    NEW(l);
    l.type := Builtin.setTp;
    l.token := S.prevTok;
    l.SetKind(setXp); RETURN l;
  END mkEmptySet;

(* -------------------------------------------- *)

  PROCEDURE mkSetLt*(s : SET) : SetExp;
    VAR l : SetExp;
  BEGIN
    NEW(l);
    l.token := S.prevTok;
    l.SetKind(setLt);
    l.type := Builtin.setTp;
    l.value := L.newSetVal(s); RETURN l;
  END mkSetLt;

(* -------------------------------------------- *)

  PROCEDURE mkCharLt*(ch : CHAR) : LeafX;
    VAR l : LeafX;
  BEGIN
    NEW(l);
    l.token := S.prevTok;
    l.type := Builtin.charTp;
    l.SetKind(charLt);
    l.value := L.newChrVal(ch); RETURN l;
  END mkCharLt;

(* -------------------------------------------- *)

  PROCEDURE mkNumLt*(nm : LONGINT) : LeafX;
    VAR l : LeafX;
  BEGIN
    NEW(l);
    l.token := S.prevTok;
    l.SetKind(numLt);
    IF (nm <= MAX(INTEGER)) & (nm >= MIN(INTEGER)) THEN
      l.type := Builtin.intTp;
    ELSE
      l.type := Builtin.lIntTp;
    END;
    l.value := L.newIntVal(nm); RETURN l;
  END mkNumLt;

(* -------------------------------------------- *)

  PROCEDURE mkRealLt*(rv : REAL) : LeafX;
    VAR l : LeafX;
  BEGIN
    NEW(l);
    l.token := S.prevTok;
    l.type := Builtin.realTp;
    l.SetKind(realLt);
    l.value := L.newFltVal(rv); RETURN l;
  END mkRealLt;

(* -------------------------------------------- *)

  PROCEDURE mkStrLt*(IN sv : ARRAY OF CHAR) : LeafX;
    VAR l : LeafX;
  BEGIN
    NEW(l);
    l.token := S.prevTok;
    l.SetKind(strLt);
    l.type := Builtin.strTp;
    l.value := L.newStrVal(sv); RETURN l;
  END mkStrLt;

(* -------------------------------------------- *)

  PROCEDURE mkStrLenLt*(str : L.CharOpen; len : INTEGER) : LeafX;
    VAR l : LeafX;
  BEGIN
    NEW(l);
    l.token := S.prevTok;
    l.SetKind(strLt);
    l.type := Builtin.strTp;
    l.value := L.newStrLenVal(str, len); RETURN l;
  END mkStrLenLt;

(* -------------------------------------------- *)

  PROCEDURE tokToStrLt*(pos,len : INTEGER) : LeafX;
  (** Generate a LeafX for this string, stripping off the quote *
    * characters which surround it in the scanner buffer. *)
    VAR l : LeafX;
  BEGIN
    NEW(l);
    l.token := S.prevTok;
    l.SetKind(strLt);
    l.type := Builtin.strTp;
    l.value := L.newBufVal(pos+1,len-2); RETURN l;
  END tokToStrLt;

(* -------------------------------------------- *)

  PROCEDURE translateStrLt*(pos,len : INTEGER) : LeafX;
  (** Generate a LeafX for this string, stripping off the quote *
    * characters which surround it in the scanner buffer. *)
    VAR l : LeafX;
  BEGIN
    NEW(l);
    l.token := S.prevTok;
    l.SetKind(strLt);
    l.type := Builtin.strTp;
    l.value := L.escapedString(pos+2,len-3); RETURN l;
  END translateStrLt;

(* ============================================================ *)
(*         UnaryX Constructor methods     *)
(* ============================================================ *)

  PROCEDURE newUnaryX*(tag : INTEGER; kid : D.Expr) : UnaryX;
    VAR u : UnaryX;
  BEGIN
    NEW(u); u.token := S.prevTok;
    u.SetKind(tag); u.kid := kid; RETURN u;
  END newUnaryX;

(* -------------------------------------------- *)

  PROCEDURE mkDeref*(kid : D.Expr) : D.Expr;
    VAR new : UnaryX;
  BEGIN
    new := newUnaryX(deref, kid);
    new.token := kid.token;
    new.type  := kid.type(T.Pointer).boundTp;
    RETURN new;
  END mkDeref;

(* ---------------------------- *)

  PROCEDURE newIdentX*(tag : INTEGER; id : D.Idnt; kid : D.Expr) : IdentX;
    VAR u : IdentX;
  BEGIN
    NEW(u); u.token := S.prevTok;
    u.SetKind(tag); u.ident := id; u.kid := kid; RETURN u;
  END newIdentX;

(* -------------------------------------------- *)

  PROCEDURE newCallX*(tag : INTEGER; prm : D.ExprSeq; kid : D.Expr) : CallX;
    VAR u : CallX;
  BEGIN
(*
 *
 *  NEW(u); u.token := S.prevTok;
 *
 *  EXPERIMENTAL
 *)
    NEW(u); u.token := kid.token;
    u.SetKind(tag); u.actuals := prm; u.kid := kid; RETURN u;
  END newCallX;

(* -------------------------------------------- *)

  PROCEDURE newCallT*(tag : INTEGER; prm : D.ExprSeq;
                      kid : D.Expr;  tok : S.Token) : CallX;
    VAR u : CallX;
  BEGIN
    NEW(u); u.token := tok;
    u.SetKind(tag); u.actuals := prm; u.kid := kid; RETURN u;
  END newCallT;

(* ============================================================ *)
(*        BinaryX Constructor methods     *)
(* ============================================================ *)

  PROCEDURE newBinaryX*(tag : INTEGER; lSub,rSub : D.Expr) : BinaryX;
    VAR b : BinaryX;
  BEGIN
    NEW(b); b.token := S.prevTok;
    b.SetKind(tag); b.lKid := lSub; b.rKid := rSub; RETURN b;
  END newBinaryX;

(* -------------------------------------------- *)

  PROCEDURE newBinaryT*(k : INTEGER; l,r : D.Expr; t : S.Token) : BinaryX;
    VAR b : BinaryX;
  BEGIN
    NEW(b); b.token := t;
    b.SetKind(k); b.lKid := l; b.rKid := r; RETURN b;
  END newBinaryT;

(* -------------------------------------------- *)

  PROCEDURE maxOfType*(t : T.Base) : LeafX;
  BEGIN
    CASE t.tpOrd OF
    | T.byteN : RETURN mkNumLt(MAX(BYTE));
    | T.uBytN : RETURN mkNumLt(255);
    | T.sIntN : RETURN mkNumLt(MAX(SHORTINT));
    | T.intN  : RETURN mkNumLt(MAX(INTEGER));
    | T.lIntN : RETURN mkNumLt(MAX(LONGINT));
    | T.sReaN : RETURN mkRealLt(MAX(SHORTREAL));
    | T.realN : RETURN mkRealLt(MAX(REAL));
    | T.sChrN : RETURN mkCharLt(MAX(SHORTCHAR));
    | T.charN : RETURN mkCharLt(MAX(CHAR));
    | T.setN  : RETURN mkNumLt(31);
    ELSE
      RETURN NIL;
    END;
  END maxOfType;

(* -------------------------------------------- *)

  PROCEDURE minOfType*(t : T.Base) : LeafX;
  BEGIN
    CASE t.tpOrd OF
    | T.byteN : RETURN mkNumLt(MIN(BYTE));
    | T.uBytN : RETURN mkNumLt(0);
    | T.sIntN : RETURN mkNumLt(MIN(SHORTINT));
    | T.intN  : RETURN mkNumLt(MIN(INTEGER));
    | T.lIntN : RETURN mkNumLt(MIN(LONGINT));
    | T.sReaN : RETURN mkRealLt(-MAX(SHORTREAL));   (* for bootstrap *)
    | T.realN : RETURN mkRealLt(-MAX(REAL));        (* for bootstrap *)
(*
 *  | T.sReaN : RETURN mkRealLt(MIN(SHORTREAL));    (* production version *)
 *  | T.realN : RETURN mkRealLt(MIN(REAL));         (* production version *)
 *)
    | T.sChrN,
      T.charN : RETURN mkCharLt(0X);
    | T.setN  : RETURN mkNumLt(0);
    ELSE
      RETURN NIL;
    END;
  END minOfType;

(* ============================================================ *)

  PROCEDURE coerceUp*(x : D.Expr; t : D.Type) : D.Expr;
  (* 
   *   Fix to string arrays coerced to native strings: kjg April 2006
   *)
  BEGIN
    IF x.kind = realLt THEN RETURN x;
    ELSIF (t = G.ntvStr) OR (t = G.ntvObj) & x.isString() THEN
      RETURN newUnaryX(mkNStr, x);
    ELSIF x.kind = numLt THEN
      IF ~t.isRealType() THEN 
        x.type := t; RETURN x;
      ELSE
        RETURN mkRealLt(x(LeafX).value.long());
      END;
    ELSIF x.isInf() THEN
      x.type := t; RETURN x;
    ELSE
      RETURN convert(x, t);
    END;
  END coerceUp;

(* ============================================================ *)
(*         Various attribution methods    *)
(* ============================================================ *)

  PROCEDURE (i : LeafX)TypeErase*() : D.Expr, EXTENSIBLE;
  (* If the type of the leaf is a compound, it must be erased *)
  BEGIN
    IF i.type.isCompoundType() THEN
      Console.WriteString("FOUND A COMPOUND LEAFX!");Console.WriteLn;
    END;
    RETURN i;
  END TypeErase;

  PROCEDURE (i : IdLeaf)TypeErase*() : D.Expr;
  BEGIN
    RETURN i;
  END TypeErase;

  PROCEDURE (i : SetExp)TypeErase*() : D.Expr;
  VAR
    exprN : D.Expr;
    index : INTEGER;
  BEGIN
      FOR index := 0 TO i.varSeq.tide - 1 DO
        exprN := i.varSeq.a[index];
        IF exprN # NIL THEN
          exprN := exprN.TypeErase();
        END;
      END;
      RETURN i;
  END TypeErase;

  PROCEDURE (i : UnaryX)TypeErase*() : D.Expr,EXTENSIBLE;
  BEGIN
      IF i.kid = NIL THEN RETURN NIL END;
      i.kid := i.kid.TypeErase();
      IF i.kid = NIL THEN RETURN NIL END;
      RETURN i;
  END TypeErase;

  PROCEDURE (i : IdentX)TypeErase*() : D.Expr;
  BEGIN
    (* If the IdentX is a type assertion node, and
     * the assertion is to a compound type, replace
     * the IdentX with a sequance of assertions *)
     IF i.kind = tCheck THEN
      (* IF i.ident.... *)
     END;
  RETURN i; END TypeErase;

  PROCEDURE (i : CallX)TypeErase*() : D.Expr;
  VAR
    exprN : D.Expr;
    index : INTEGER;
  BEGIN
      FOR index := 0 TO i.actuals.tide - 1 DO
        exprN := i.actuals.a[index];
        IF exprN # NIL THEN
          exprN := exprN.TypeErase();
        END;
      END;
      RETURN i;
  END TypeErase;

  PROCEDURE (i : BinaryX)TypeErase*() : D.Expr;
  VAR rslt : D.Expr;
  BEGIN
    IF (i.lKid = NIL) OR (i.rKid = NIL) THEN RETURN NIL END;
    i.lKid := i.lKid.TypeErase();    (* process subtree  *)
    i.rKid := i.rKid.TypeErase();    (* process subtree  *)
    IF (i.lKid = NIL) OR (i.rKid = NIL) THEN RETURN NIL END;
    RETURN i;
  END TypeErase;

(* -------------------------------------------- *)

  PROCEDURE isRelop(op : INTEGER) : BOOLEAN;
  BEGIN
    RETURN (op = equal) OR (op = notEq) OR (op = lessEq) OR
     (op = lessT) OR (op = greEq) OR (op = greT) OR
     (op = inOp)  OR (op = isOp);
  END isRelop;

(* -------------------------------------------- *)

  PROCEDURE getQualType*(exp : D.Expr) : D.Type;
  (* Return the qualified type with TypId descriptor in the   *
   * IdLeaf exp, otherwise return the NIL pointer.    *)
    VAR leaf : IdLeaf;
        tpId : D.Idnt;
  BEGIN
    IF ~(exp IS IdLeaf) THEN RETURN NIL END;
    leaf := exp(IdLeaf);
    IF ~(leaf.ident IS I.TypId) THEN RETURN NIL END;
    tpId := leaf.ident;
    RETURN tpId.type;
  END getQualType;

(* -------------------------------------------- *)

  PROCEDURE CheckIsVariable*(e : D.Expr);
  VAR
    isVar : BOOLEAN;
  BEGIN
    IF (e = NIL) THEN RETURN; END;
    WITH e : IdentX DO
      IF e.ident IS I.OvlId THEN e.ident := e.ident(I.OvlId).fld; END;
      isVar := (e.ident # NIL) & (e.ident IS I.FldId);
    | e : IdLeaf DO
      IF e.ident IS I.OvlId THEN e.ident := e.ident(I.OvlId).fld; END;
      isVar := (e.ident # NIL) & ((e.ident IS I.VarId) OR (e.ident IS I.LocId));
    | e : BinaryX DO
      isVar := e.kind = index;
    | e : UnaryX DO
      IF e.kind = tCheck THEN
        isVar := TRUE;
        e.ExprError(222); 
      ELSE 
        isVar := e.kind = deref;
      END;
    ELSE
      isVar := FALSE;
    END;
    IF (~isVar) THEN e.ExprError(85); END;
  END CheckIsVariable;

(* -------------------------------------------- *)

  PROCEDURE (i : LeafX)exprAttr*() : D.Expr,EXTENSIBLE;
  BEGIN (* most of these are done already ... *)
    IF (i.kind = numLt) & i.inRangeOf(Builtin.intTp) THEN
      i.type := Builtin.intTp;
    END;
    RETURN i;
  END exprAttr;

(* -------------------------------------------- *)

  PROCEDURE (i : IdLeaf)exprAttr*() : D.Expr;
  (* If this references a constant, then return literal *)
   (* ----------------------------------------- *)
    PROCEDURE constClone(i : IdLeaf) : D.Expr;
      VAR conXp : D.Expr;
    clone : LeafX;
   (* ----------------------------------------- *
    *  We must clone the literal rather than  *
    *  just take a reference copy, as it may  *
    *  appear in a later error message. If it   *
    *  does, it needs to have correct line:col. *
    * ----------------------------------------- *)
    BEGIN
      conXp := i.ident(I.ConId).conExp;
      WITH conXp : SetExp DO
        clone := mkSetLt({});
        clone.value := conXp.value;
      |    conXp : LeafX DO
        clone := mkLeafVal(conXp.kind, conXp.value);
        clone.type  := conXp.type;
      END;
      clone.token := i.token;
      RETURN clone;
    END constClone;
   (* --------------------------------- *)
  BEGIN
    IF (i.ident # NIL) & (i.ident IS I.ConId) THEN
      IF i.ident(I.ConId).isStd THEN
        IF    i.ident = Builtin.trueC THEN RETURN mkTrueX();
        ELSIF i.ident = Builtin.falsC THEN RETURN mkFalseX();
        ELSIF i.ident = Builtin.nilC  THEN RETURN mkNilX();
        ELSIF i.ident = Builtin.infC  THEN RETURN mkInfX();
        ELSIF i.ident = Builtin.nInfC THEN RETURN mkNegInfX();
        ELSE  i.ExprError(19); RETURN NIL;
        END;
      ELSE
        RETURN constClone(i);
      END;
    ELSE
      RETURN i;
    END;
  END exprAttr;

(* -------------------------------------------- *)

  PROCEDURE (i : SetExp)exprAttr*() : D.Expr;
    VAR exprN : D.Expr;     (* the n-th expression  *)
        index : INTEGER;    (* reading index  *)
        write : INTEGER;    (* writing index  *)
        cPart : SET;        (* constant accumulator *)
        rngXp : BinaryX;
        num   : INTEGER;

    (* ----------------------------------- *)

    PROCEDURE isLitRange(exp : BinaryX) : BOOLEAN;
    BEGIN
      RETURN (exp.lKid # NIL) &
             (exp.rKid # NIL) &
             (exp.lKid.kind = numLt) &
             (exp.rKid.kind = numLt);
    END isLitRange;

    (* ----------------------------------- *)

    PROCEDURE mkSetFromRange(exp : BinaryX) : SET;
      VAR ln,rn : INTEGER;
    BEGIN
      ln := exp.lKid(LeafX).value.int();
      rn := exp.rKid(LeafX).value.int();
      IF (ln > 31) OR (ln < 0) THEN exp.lKid.ExprError(28); RETURN {} END;
      IF (rn > 31) OR (rn < 0) THEN exp.rKid.ExprError(29); RETURN {} END;
      IF rn < ln THEN exp.ExprError(30); RETURN {} END;
      RETURN {ln .. rn}
    END mkSetFromRange;

    (* ----------------------------------- *)

  BEGIN (* body of (i : SetExp)exprAttr *)
    write := 0;
    cPart := {};
    FOR index := 0 TO i.varSeq.tide - 1 DO
      exprN := i.varSeq.a[index];
      IF exprN # NIL THEN
        exprN := exprN.exprAttr();
        IF exprN.kind = numLt THEN  (* singleton element  *)
          num := exprN(LeafX).value.int();
          IF (num < 32) & (num >= 0) THEN
            INCL(cPart, num);
          ELSE
            exprN.ExprError(303);
          END;
        ELSIF exprN.kind = range THEN
          rngXp := exprN(BinaryX);
          IF isLitRange(rngXp) THEN (* const elem range *)
            cPart := cPart + mkSetFromRange(rngXp);
          ELSE
            IF ~rngXp.lKid.isIntExpr() THEN rngXp.lKid.ExprError(37) END;
            IF ~rngXp.rKid.isIntExpr() THEN rngXp.rKid.ExprError(37) END;
            i.varSeq.a[write] := exprN; INC(write);
          END;
        ELSE        (* variable element(s)  *)
          IF ~exprN.isIntExpr() THEN exprN.ExprError(37) END;
          i.varSeq.a[write] := exprN; INC(write);
        END;
      END;
    END;
    IF write # i.varSeq.tide THEN   (* expression changed *)
      i.value := L.newSetVal(cPart);
      IF write = 0 THEN     (* set is all constant  *)
        i.SetKind(setLt);
      END;
      i.varSeq.ResetTo(write);    (* truncate elem list *)
    ELSIF write = 0 THEN    (* this is empty set  *)
      i.SetKind(setLt);
    END;
    i.type := Builtin.setTp;
    RETURN i;
  END exprAttr;

(* -------------------------------------------- *)

  PROCEDURE (i : UnaryX)exprAttr*() : D.Expr,EXTENSIBLE;
    VAR leaf : LeafX;
        rslt : D.Expr;
  BEGIN
    IF i.kid = NIL THEN RETURN NIL END;
    i.kid := i.kid.exprAttr();
    IF i.kid = NIL THEN RETURN NIL END;
    rslt := i;
    CASE i.kind OF
    | neg :      (* Fold constants and mark sets *)
        IF i.kid.kind = setXp THEN
          i.SetKind(compl);
          i.type := Builtin.setTp;
        ELSIF i.kid.kind = setLt THEN
          leaf := i.kid(LeafX);
          leaf.value := L.newSetVal(-leaf.value.set());
          rslt := leaf;
        ELSIF i.kid.kind = numLt THEN
          leaf := i.kid(LeafX);
          leaf.value := L.newIntVal(-leaf.value.long());
          rslt := leaf;
        ELSIF i.kid.kind = realLt THEN
          leaf := i.kid(LeafX);
          leaf.value := L.newFltVal(-leaf.value.real());
          rslt := leaf;
        ELSE
          i.type := i.kid.type;
        END;
    | blNot : (* Type check subtree, and fold consts *)
        IF i.kid.type # Builtin.boolTp THEN i.ExprError(36) END;
        IF i.kid.kind = blNot THEN (* fold double negation *)
          rslt := i.kid(UnaryX).kid;
        ELSIF i.kid.kind = tBool THEN
          rslt := mkFalseX();
        ELSIF i.kid.kind = fBool THEN
          rslt := mkTrueX();
        ELSE
          i.type := Builtin.boolTp;
        END;
    ELSE (* Nothing to do. Parser did type check already *)
         (* mkStr, absVl, convert, capCh, entVl, strLen, lenOf, oddTst *)
         (* tCheck *)
    END;
    RETURN rslt;
  END exprAttr;

(* -------------------------------------------- *)

  PROCEDURE (i : IdentX)exprAttr*() : D.Expr;
  BEGIN
    IF (i.kind = selct) & (i.ident # NIL) & (i.ident IS I.ConId) THEN
      RETURN i.ident(I.ConId).conExp.exprAttr();
    ELSE
      ASSERT((i.kind = selct)  OR
       (i.kind = cvrtUp) OR (i.kind = cvrtDn));
      RETURN i;
    END;
  END exprAttr;

(* -------------------------------------------- *)

  PROCEDURE (i : CallX)exprAttr*() : D.Expr;
 (* fnCall nodes are attributed during parsing of the designator  *
  * so there is nothing left to do here.  Do not recurse further down.  *)
  BEGIN RETURN i END exprAttr;

(* -------------------------------------------- *)

  PROCEDURE checkCall*(i : CallX) : D.Expr;
    VAR prTp : T.Procedure;
        prXp : D.Expr;

    (* --------------------------- *)

    PROCEDURE length(arg0 : D.Expr; arg1 : LeafX) : D.Expr;
      VAR dimN : INTEGER;
          dIdx : INTEGER;
          cTyp : D.Type;
          cLen : INTEGER;
    BEGIN
      dimN := arg1.value.int();
      IF dimN < 0 THEN arg1.ExprError(46); RETURN NIL END;

     (*
      *   Take care of LEN(typename) case ... kjg December 2004
      *)
      WITH arg0 : IdLeaf DO
        IF arg0.ident IS I.TypId THEN arg0.type := arg0.ident.type END;
      ELSE
      END;

      IF arg0.type.kind = T.ptrTp THEN arg0 := mkDeref(arg0) END;
      cLen := 0;
      cTyp := arg0.type;
      IF cTyp.kind = T.vecTp THEN
        IF dimN # 0 THEN arg1.ExprError(231) END;
      ELSE
        FOR dIdx := 0 TO dimN DO
          IF cTyp.kind = T.arrTp THEN
            cLen := cTyp(T.Array).length;
            cTyp := cTyp(T.Array).elemTp;
          ELSE
            arg1.ExprError(40); RETURN NIL;
          END;
        END;
      END;
      IF cLen = 0 THEN (* must compute at runtime *)
        RETURN newBinaryX(lenOf, arg0, arg1);
      ELSE
        RETURN mkNumLt(cLen);
      END;
    END length;

    (* --------------------------- *)

    PROCEDURE stdFunction(i : CallX; act : D.ExprSeq) : D.Expr;
    (* Assert: prc holds a procedure ident descriptor of a standard Fn. *)
      VAR prc  : IdLeaf;
          funI : I.PrcId;
          rslt : D.Expr;
          leaf : LeafX;
          arg0 : D.Expr;
          arg1 : D.Expr;
          typ0 : D.Type;
          dstT : D.Type;
          funN : INTEGER;
          lVal : LONGINT;
          rVal : REAL;
          ptrT : T.Pointer;
    BEGIN
      prc  := i.kid(IdLeaf);
      rslt := NIL;
      arg0 := NIL;
      arg1 := NIL;
      funI := prc.ident(I.PrcId);
      funN := funI.stdOrd;
      IF act.tide >= 1 THEN
        arg0 := act.a[0];
        IF arg0 # NIL THEN arg0 := arg0.exprAttr() END;
        IF act.tide >= 2 THEN
          arg1 := act.a[1];
          IF arg1 # NIL THEN arg1 := arg1.exprAttr() END;
          IF arg1 = NIL THEN RETURN NIL END;
        END;
        IF arg0 = NIL THEN RETURN NIL END;
      END;
     (*
      *  Now we check the per-case semantics.
      *)
      CASE funN OF
      (* ---------------------------- *)
      | Builtin.absP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            IF arg0.isNumericExpr() THEN
              IF arg0.kind = numLt THEN
                leaf := arg0(LeafX); leaf.value := L.absV(leaf.value);
          IF leaf.value = NIL THEN arg0.ExprError(39)END;
          rslt := leaf;
              ELSIF arg0.kind = realLt THEN
          rslt := mkRealLt(ABS(arg0(LeafX).value.real()));
              ELSE
          rslt := newUnaryX(absVl, arg0);
              END;
              rslt.type := arg0.type;
            ELSE
              arg0.ExprError(38);
            END;
          END;
      (* ---------------------------- *)
      (* Extended to LONGINT (1:01:2013) *)
      (* ---------------------------- *)
      | Builtin.ashP :
          IF    act.tide < 2 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF ~arg0.isIntExpr() THEN arg0.ExprError(37) END;
            IF ~arg1.isIntExpr() THEN arg1.ExprError(37) END;
	   (* NO FOLDING IN THIS VERSION
            IF (arg0.kind = numLt) & (arg1.kind = numLt) THEN
              rslt := mkNumLt(ASH(arg0(LeafX).value.int(),
                arg1(LeafX).value.int()));
            ELSE
	   *)
              IF arg0.type = Builtin.lIntTp THEN
                dstT := Builtin.lIntTp;
			  ELSE
                IF arg0.type # Builtin.intTp THEN
                  arg0 := convert(arg0, Builtin.intTp);
				END;
                dstT := Builtin.intTp;
              END;
              IF arg1.type # Builtin.intTp THEN
                arg1 := convert(arg1, Builtin.intTp);
              END;
              rslt := newBinaryX(ashInt, arg0, arg1);
	   (*
            END;
            *)
            rslt.type := dstT;
          END;
      (* ---------------------------- *)
      | Builtin.lshP :
          IF    act.tide < 2 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF ~arg0.isIntExpr() THEN arg0.ExprError(37) END;
            IF ~arg1.isIntExpr() THEN arg1.ExprError(37) END;
			(* FIXME, no folding yet ... *)
			IF arg0.type = Builtin.lIntTp THEN
			  dstT := Builtin.lIntTp;
			ELSE
              IF arg0.type # Builtin.intTp THEN
                arg0 := convert(arg0, Builtin.intTp);
			  END;
			  dstT := Builtin.intTp;
            END;
            IF arg1.type # Builtin.intTp THEN
              arg1 := convert(arg1, Builtin.intTp);
            END;
            rslt := newBinaryX(lshInt, arg0, arg1);
            rslt.type := dstT;
          END;
      (* ---------------------------- *)
      | Builtin.rotP :
          IF    act.tide < 2 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF ~arg0.isIntExpr() THEN arg0.ExprError(37) END;
            IF ~arg1.isIntExpr() THEN arg1.ExprError(37) END;
			(* Do not convert arg0 to intTp *)
            IF arg1.type # Builtin.intTp THEN
              arg1 := convert(arg1, Builtin.intTp);
            END;
            rslt := newBinaryX(rotInt, arg0, arg1);
            rslt.type := arg0.type;
          END;
      (* ---------------------------- *)
      | Builtin.bitsP :
          IF    act.tide < 1 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            rslt := arg0;
            IF rslt.isIntExpr() THEN
              (*
               *  BITS accepts an integer expression
               *  which may be either short or long.
               *  In the case of short values these
               *  are sign-extended to 32 bits.
               *  In the case of long values gpcp
               *  performs an unsigned conversion to
               *  uint32, capturing the 32 least
               *  significant bits of the long value.
               *)
              IF rslt.kind = numLt THEN
                (* Pull out ALL of the bits of the numLt.   *)
                (* At compile-time gpcp will convert from   *)
                (* int64 to uint32 using the elsepart below *)
                rslt := mkSetLt(BITS(arg0(LeafX).value.long()));
                rslt.type := Builtin.setTp;
              ELSE
                (* Graft an unchecked conversion onto the   *)
                (* root of the argument expression tree.    *)
                rslt := convert(rslt, Builtin.setTp); 
              END;
            ELSE
              arg0.ExprError(56);
            END;
          END;
      (* ---------------------------- *)
      | Builtin.capP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            IF arg0.isCharExpr() THEN
              IF arg0.isCharLit() THEN
                rslt := mkCharLt(CAP(arg0(LeafX).charValue()));
              ELSE
                rslt := newUnaryX(capCh, arg0);
              END;
              rslt.type := Builtin.charTp;
            ELSE
              arg0.ExprError(43);
            END;
          END;
      (* ---------------------------- *)
      | Builtin.chrP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            IF arg0.isIntExpr() THEN
              IF arg0.kind = numLt THEN
                lVal := arg0(LeafX).value.long();
                IF (lVal >= 0) & (lVal <= LONG(ORD(MAX(CHAR)))) THEN
                  rslt := mkCharLt(CHR(lVal));
                  rslt.type := Builtin.charTp;
                ELSE
                  arg0.ExprError(44);
                END;
              ELSE
                rslt := convert(arg0, Builtin.charTp);
              END;
            ELSE
              arg0.ExprError(37);
            END;
          END;
      (* ---------------------------- *)
      | Builtin.entP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            dstT := Builtin.lIntTp;
            IF arg0.isRealExpr() THEN
              IF arg0.kind = realLt THEN
                leaf := mkLeafVal(numLt, L.entV(arg0(LeafX).value));
                IF leaf.value = NIL THEN
                   arg0.ExprError(55);
                ELSIF i.inRangeOf(Builtin.intTp) THEN
                  dstT := Builtin.intTp;
                END;
                rslt := leaf;
              ELSE
                rslt := newUnaryX(entVl, arg0);
              END;
              rslt.type := dstT;
            ELSE
              arg0.ExprError(45);
            END;
          END;
      (* ---------------------------- *)
      | Builtin.lenP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSIF act.tide = 1 THEN
            IF arg0.kind = strLt THEN
              rslt := mkNumLt(arg0(LeafX).value.len());
            ELSIF arg0.kind = mkStr THEN
              rslt := newUnaryX(strLen, arg0);
            ELSE        (* add default dimension *)
              D.AppendExpr(act, mkNumLt(0));
            END;
          END;
          IF act.tide = 2 THEN
            arg1 := act.a[1];
            IF arg1.kind = numLt THEN
              rslt := length(arg0, arg1(LeafX));
            ELSE
              arg1.ExprError(46);
            END;
          END;
          IF rslt # NIL THEN rslt.type := Builtin.intTp END;
      (* ---------------------------- *)
      | Builtin.tpOfP :
          IF G.strict THEN prc.ExprError(221); END;
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSIF arg0.type = Builtin.metaTp THEN
            ASSERT(arg0 IS IdLeaf);
            rslt := arg0;
            rslt.SetKind(typOf);
          ELSIF arg0.isVarDesig() THEN
            IF arg0.type.isDynamicType() THEN
              rslt := newUnaryX(getTp, arg0);
            ELSE
              dstT := arg0.type;    
              IF dstT.idnt = NIL THEN          (* Anonymous type *)
                dstT.idnt := I.newAnonId(dstT.serial);
                dstT.idnt.type := dstT;
              END; 
              rslt := mkIdLeaf(dstT.idnt);
              rslt.SetKind(typOf);
            END;
          ELSE arg0.ExprError(85);
          END;
          IF rslt # NIL THEN rslt.type := G.ntvTyp END;
      (* ---------------------------- *)
      | Builtin.adrP :
          IF G.strict THEN prc.ExprError(221); END;
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSIF arg0.isVarDesig() THEN
            rslt := newUnaryX(adrOf, arg0);
          ELSE arg0.ExprError(85);
          END;
          IF rslt # NIL THEN rslt.type := Builtin.intTp END;
      (* ---------------------------- *)
      | Builtin.maxP,
        Builtin.minP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSIF act.tide = 1 THEN (* should be the MAX(TypeName) case *)
            dstT := getQualType(arg0);
            IF dstT.kind # T.basTp THEN prc.ExprError(48) END;
            IF funN = Builtin.maxP THEN
              rslt := maxOfType(dstT(T.Base));
            ELSE
              rslt := minOfType(dstT(T.Base));
            END;
            IF rslt # NIL THEN rslt.type := dstT END;
          ELSE (*  must be the MAX(exp1, exp2) case *)
           (*
            *  Note that for literals, coverType is always >= int.
            *)
            dstT := coverType(arg0.type, arg1.type);
            IF dstT = NIL THEN arg0.ExprError(38);
            ELSIF (arg0.kind = numLt) & (arg1.kind = numLt) THEN
              IF funN = Builtin.maxP THEN
                lVal := MAX(arg0(LeafX).value.long(),arg1(LeafX).value.long());
              ELSE
                lVal := MIN(arg0(LeafX).value.long(),arg1(LeafX).value.long());
              END;
              rslt := mkNumLt(lVal);
            ELSIF (arg0.kind = realLt) & (arg1.kind = realLt) THEN
              IF funN = Builtin.maxP THEN
                rVal := MAX(arg0(LeafX).value.real(),arg1(LeafX).value.real());
              ELSE
                rVal := MIN(arg0(LeafX).value.real(),arg1(LeafX).value.real());
              END;
              rslt := mkRealLt(rVal);
            ELSE
              IF arg0.type # dstT THEN arg0 := convert(arg0, dstT) END;
              IF arg1.type # dstT THEN arg1 := convert(arg1, dstT) END;
              IF funN = Builtin.maxP THEN
                rslt := newBinaryX(maxOf, arg0, arg1)
              ELSE
                rslt := newBinaryX(minOf, arg0, arg1)
              END;
            END;
            IF rslt # NIL THEN rslt.type := dstT END;
          END;
      (* ---------------------------- *)
      | Builtin.oddP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            rslt := arg0;
            IF ~rslt.isIntExpr() THEN rslt.ExprError(37);
            ELSIF rslt.kind = numLt THEN  (* calculate right now  *)
              IF ODD(rslt(LeafX).value.int()) THEN
                rslt := mkTrueX();
              ELSE
                rslt := mkFalseX();
              END;
            ELSE        (* else leave to runtime*)
              rslt := newUnaryX(oddTst, rslt);
            END;
            rslt.type := Builtin.boolTp;
          END;
      (* ---------------------------- *)
      | Builtin.ordP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            rslt := arg0;
            IF rslt.isCharLit() THEN
              rslt := mkNumLt(ORD(rslt(LeafX).charValue()));
            ELSIF rslt.kind = setLt THEN
              rslt := mkNumLt(rslt(LeafX).value.int());
            ELSIF rslt.isCharExpr() OR rslt.isSetExpr() THEN
              rslt := convert(rslt, Builtin.intTp);
            ELSE
              prc.ExprError(50);
            END;
            rslt.type := Builtin.intTp;
          END;
      (* ---------------------------- *)
      | Builtin.uBytP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            rslt := arg0;
            dstT := Builtin.uBytTp;
            IF rslt.kind = numLt THEN
              IF ~rslt.inRangeOf(dstT) THEN rslt.ExprError(26) END;
            ELSIF arg0.isNumericExpr() THEN
              rslt := convert(rslt, dstT);
            ELSE
              rslt.ExprError(226);
            END;
            rslt.type := dstT;
          END;
      (* ---------------------------- *)
      | Builtin.mStrP :
          IF G.strict THEN prc.ExprError(221); END;
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSIF ~arg0.isString() & ~arg0.isCharArray() THEN
            arg0.ExprError(41);
          END;
          rslt := newUnaryX(mkNStr, arg0);
          rslt.type := G.ntvStr;
      (* ---------------------------- *)
      | Builtin.boxP :
          IF G.strict THEN prc.ExprError(221); END;
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            typ0 := arg0.type;
            rslt := newUnaryX(mkBox, arg0);
            WITH typ0 : T.Record DO 
                 IF D.isFn IN typ0.xAttr THEN
                   ptrT := G.ntvObj(T.Pointer);
                 ELSE
                   ptrT := T.newPtrTp();
                   ptrT.boundTp := typ0;
                 END;
            | typ0 : T.Array  DO 
                 ptrT := T.newPtrTp();
                 IF typ0.length = 0 THEN  (* typ0 already an open array *)
                   ptrT.boundTp := typ0;
                 ELSE                     (* corresponding open array   *)
                   ptrT.boundTp := T.mkArrayOf(typ0.elemTp);
                 END;
            ELSE 
              ptrT := T.newPtrTp();
              IF typ0.isStringType() THEN 
                ptrT.boundTp := Builtin.chrArr;
              ELSE 
                arg0.ExprError(140);
              END;
            END;
            rslt.type := ptrT;
          END;
      (* ---------------------------- *)
      | Builtin.shrtP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            rslt := arg0;
            IF rslt.kind = charLt THEN      (* do right away *)
              IF ~rslt.inRangeOf(Builtin.sChrTp) THEN rslt.ExprError(168) END;
              rslt.type := Builtin.sChrTp;
            ELSIF rslt.kind = strLt THEN    (* do right away *)
              IF ~L.isShortStr(rslt(LeafX).value) THEN
                rslt.ExprError(168) END;
              rslt.type := Builtin.sStrTp;
            ELSIF rslt.type = Builtin.strTp  THEN (* do at runtime *)
              rslt := newUnaryX(strChk, rslt);
              rslt.type := Builtin.sStrTp;
            ELSE
              IF    rslt.type = Builtin.lIntTp THEN dstT := Builtin.intTp;
              ELSIF rslt.type = Builtin.intTp  THEN dstT := Builtin.sIntTp;
              ELSIF rslt.type = Builtin.sIntTp THEN dstT := Builtin.byteTp;
              ELSIF rslt.type = Builtin.realTp THEN dstT := Builtin.sReaTp;
              ELSIF rslt.type = Builtin.charTp THEN dstT := Builtin.sChrTp;
              ELSE  rslt.ExprError(51);             dstT := Builtin.intTp;
              END;
              IF rslt.kind = numLt THEN
                IF ~rslt.inRangeOf(dstT) THEN rslt.ExprError(26) END;
              ELSE
                rslt := convert(rslt, dstT);
              END;
            END;
          END;
      (* ---------------------------- *)
      | Builtin.longP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSE
            rslt := arg0;
            IF    rslt.type = Builtin.intTp  THEN dstT := Builtin.lIntTp;
            ELSIF rslt.type = Builtin.sIntTp THEN dstT := Builtin.intTp;
            ELSIF rslt.type = Builtin.byteTp THEN dstT := Builtin.sIntTp;
            ELSIF rslt.type = Builtin.sReaTp THEN dstT := Builtin.realTp;
            ELSIF rslt.type = Builtin.sChrTp THEN dstT := Builtin.charTp;
            ELSE  rslt.ExprError(47);     dstT := Builtin.lIntTp;
            END;
            rslt := convert(rslt, dstT);
          END;
      (* ---------------------------- *)
      | Builtin.sizeP :
          prc.ExprError(167);
      (* ---------------------------- *)
      ELSE
        prc.ExprError(42);
      END;
      RETURN rslt;
    END stdFunction;

    (* --------------------------- *)

    PROCEDURE StdProcedure(i : CallX; act : D.ExprSeq);
    (* Assert: prc holds a procedure ident descriptor of a standard Pr. *)
      VAR prc  : IdLeaf;
          funI : I.PrcId;
          funN : INTEGER;
          argN : INTEGER;
          errN : INTEGER;
          arg0 : D.Expr;
          arg1 : D.Expr;
          argT : D.Type;
          bndT : D.Type;
          ptrT : T.Pointer;
    (* --------------------------- *)
      PROCEDURE CheckNonZero(arg : D.Expr);
      BEGIN
        IF arg(LeafX).value.int() <= 0 THEN arg.ExprError(68) END;
      END CheckNonZero;
    (* --------------------------- *)
    BEGIN
      prc  := i.kid(IdLeaf);
      arg0 := NIL;
      arg1 := NIL;
      funI := prc.ident(I.PrcId);
      funN := funI.stdOrd;
      IF act.tide >= 1 THEN
        arg0 := act.a[0].exprAttr();
        act.a[0] := arg0;
        IF act.tide >= 2 THEN
          arg1 := act.a[1].exprAttr();
          IF arg1 = NIL THEN RETURN END;
          act.a[1] := arg1;
        END;
        IF arg0 = NIL THEN RETURN END;
      END;
     (*
      *  Now we check the per-case semantics.
      *)
      CASE funN OF
      (* ---------------------------- *)
      | Builtin.asrtP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF arg0.type # Builtin.boolTp THEN
              arg0.ExprError(36);
            END;
            IF (arg1 # NIL) & (arg1.kind # numLt) THEN
              arg1.ExprError(91);
            END;
          END;
      (* ---------------------------- *)
      | Builtin.incP,
        Builtin.decP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF arg0.isVarDesig() THEN
              arg0.CheckWriteable();
              IF ~arg0.isIntExpr()   THEN arg0.ExprError(37) END;
            ELSE
              arg0.ExprError(85);
            END;
            IF arg1 = NIL THEN
              D.AppendExpr(act, mkNumLt(1));
            ELSIF ~arg1.isIntExpr() THEN
              arg1.ExprError(37);
            END;
          END;
      (* ---------------------------- *)
      | Builtin.inclP,
        Builtin.exclP :
          IF    act.tide < 2 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF arg0.isVarDesig() THEN
              arg0.CheckWriteable();
              IF ~arg0.isSetExpr()   THEN arg0.ExprError(35) END;
              IF ~arg1.isIntExpr()   THEN arg1.ExprError(37) END;
            ELSE
              arg0.ExprError(85);
            END;
            IF arg1.isIntExpr() THEN
              IF (arg1.kind = numLt) &    (* Should be warning only? *)
                 ~arg1.inRangeOf(Builtin.setTp) THEN arg1.ExprError(303) END;
            ELSE
              arg1.ExprError(37);
            END;
          END;
      (* ---------------------------- *)
      | Builtin.getP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF arg0.type # Builtin.intTp THEN arg0.ExprError(37) END;
            IF ~arg1.isVarDesig() THEN arg1.ExprError(85) END; 
            IF arg1.type.kind # T.basTp THEN arg1.ExprError(48) END;
          END;
      (* ---------------------------- *)
      | Builtin.putP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF arg0.type # Builtin.intTp THEN arg0.ExprError(37) END;
            IF arg1.type.kind # T.basTp THEN arg1.ExprError(48) END;
          END;
      (* ---------------------------- *)
      | Builtin.cutP  :
          IF    act.tide < 2 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSIF arg0.isVarDesig() THEN
            arg0.CheckWriteable();
            IF ~arg0.isVectorExpr() THEN arg0.ExprError(229) END;
          ELSE
            arg0.ExprError(85);
          END;
          IF ~arg1.isIntExpr() THEN arg1.ExprError(37) END;
      (* ---------------------------- *)
      | Builtin.apndP :
          IF    act.tide < 2 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSIF arg0.isVarDesig() THEN
            argT := arg0.type;
            arg0.CheckWriteable();
            WITH argT : T.Vector DO
              IF ~argT.elemTp.assignCompat(arg1) THEN
                IF    arg1.type.isOpenArrType() THEN errN := 142;
                ELSIF arg1.type.isExtnRecType() THEN errN := 143;
                ELSIF (arg1.type.kind = T.prcTp) &
                      (arg1.kind = qualId) &
                      ~arg1.isProcVar() THEN errN := 165;
                ELSIF argT.elemTp.isCharArrayType() &
                      arg1.type.isStringType() THEN  errN :=  27;
                ELSE             errN :=  83;
                END;
                IF errN # 83 THEN arg1.ExprError(errN);
                ELSE D.RepTypesErrTok(83, argT.elemTp, arg1.type, arg1.token);
                END;
              END;
            ELSE 
              arg0.ExprError(229);
            END;
          ELSE
            arg0.ExprError(85);
          END;
      (* ---------------------------- *)
      | Builtin.subsP,
        Builtin.unsbP :
          IF G.strict THEN prc.ExprError(221); END;
          IF    act.tide < 2 THEN prc.ExprError(22);
          ELSIF act.tide > 2 THEN prc.ExprError(23);
          ELSE
            IF arg0.isVarDesig() THEN
              arg0.CheckWriteable();
              IF ~arg0.type.isEventType() THEN arg0.ExprError(210) END;
              IF ~arg1.isProcLit()        THEN arg1.ExprError(211) END;
              IF ~arg0.type.assignCompat(arg1) THEN arg1.ExprError(83) END;
            ELSE
              arg0.ExprError(85);
            END;
          END;
      (* ---------------------------- *)
      | Builtin.haltP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSIF arg0.kind # numLt THEN arg0.ExprError(93);
          END;
      (* ---------------------------- *)
      | Builtin.throwP :
          IF G.strict THEN prc.ExprError(221); END;
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF act.tide > 1 THEN prc.ExprError(23);
          ELSIF G.ntvExc.assignCompat(arg0) OR
                G.ntvStr.assignCompat(arg0) THEN (* skip *)
          ELSE  arg0.ExprError(193);
          END;
      (* ---------------------------- *)
      | Builtin.newP :
          IF    act.tide = 0 THEN prc.ExprError(22);
          ELSIF arg0.type # NIL THEN
            argT := arg0.type;
            IF ~arg0.isVarDesig() THEN
              arg0.ExprError(85);
            ELSE
              arg0.CheckWriteable();
              WITH argT : T.Base DO
                  arg0.ExprError(90);
              | argT : T.Vector DO
                  IF act.tide = 1 THEN prc.ExprError(95);
                  ELSIF act.tide > 2 THEN prc.ExprError(97);
                  ELSIF ~arg1.isIntExpr() THEN
                    arg1.ExprError(98);
                  ELSIF arg1.kind = numLt THEN 
                    CheckNonZero(arg1);
                  END;
              | argT : T.Pointer DO
                  bndT := argT.boundTp;
                  IF act.tide = 1 THEN
                   (*
                    *  Bound-type must be a record or a fixed
                    *  length, one-dimensional array type.
                    *)
                    IF bndT.kind = T.recTp THEN
                      bndT(T.Record).InstantiateCheck(arg0.token);
                    ELSIF bndT.kind = T.arrTp THEN
                      IF bndT.isOpenArrType() THEN arg0.ExprError(95) END;
                    ELSE
                      arg0.ExprError(96);
                    END;
                  ELSE
                   (*
                    *  This must be a possibly multi-dimensional array type.
                    *)
                    IF ~bndT.isOpenArrType() THEN
                      arg0.ExprError(99);
                    ELSIF ~arg1.isIntExpr() THEN
                      arg1.ExprError(98);
                    ELSE
                      IF arg1.kind = numLt THEN CheckNonZero(arg1) END;
                      bndT := bndT(T.Array).elemTp;
                      FOR argN := 2 TO act.tide-1 DO
                        arg1 := act.a[argN].exprAttr();
                        IF arg1.kind = numLt THEN CheckNonZero(arg1) END;
                        IF ~bndT.isOpenArrType() THEN
                          arg0.ExprError(97);
                        ELSIF ~arg1.isIntExpr() THEN
                          arg1.ExprError(98);
                        ELSE
                          bndT := bndT(T.Array).elemTp;
                        END;
                        act.a[argN] := arg1;  (* update expression *)
                      END;
                      (* check if we need more length params *)
                      IF bndT.isOpenArrType() THEN arg1.ExprError(100) END;
                    END;
                  END;
              ELSE
                arg0.ExprError(94);
              END; (* with argT *)
            END; (* if isVarDesig() *)
          END; (* if *)
      (* ---------------------------- *)
      ELSE
        prc.ExprError(92);
      END;
    END StdProcedure;

    (* --------------------------- *)

  BEGIN (* body of checkCall *)
    prXp := i.kid;
    prTp := prXp.type(T.Procedure);
    IF i.kind = prCall THEN
      IF prXp.isStdProc() THEN
        StdProcedure(i, i.actuals);
      ELSIF prXp.kind = fnCall THEN
        prXp.ExprError(80);
      ELSE
        FormalsVsActuals(prXp, i.actuals);
        IF prTp.retType # NIL THEN i.ExprError(74) END;
        i.type := NIL;
      END;
    ELSIF i.kind = fnCall THEN
      IF prXp.isStdFunc() THEN
        RETURN stdFunction(i, i.actuals);
      ELSE
        FormalsVsActuals(prXp, i.actuals);
        IF prTp.retType = NIL THEN
          i.ExprError(24);
        ELSIF prTp.retType IS T.Opaque THEN
          prTp.retType := prTp.retType.elaboration();
        END;
        i.type := prTp.retType;
      END;
    ELSE
      Console.WriteString("unexpected callx"); Console.WriteLn; i.Diagnose(0);
    END;
    RETURN i;
  END checkCall;

(* -------------------------------------------- *)

  PROCEDURE CheckSuper*(c : CallX; s : D.Scope);
    VAR kid1, kid2 : D.Expr;
  BEGIN
   (* ------------------------------------------------- *
    * Precondition: c.kid.kind = sprMrk.
    * The only correct expression cases are
    *
    *   CallX
    *       IdentX --- (kind = sprMrk)
    *           IdLeaf --- (ident = s(MthId).rcvFrm)
    *
    *   CallX
    *       IdentX --- (kind = sprMrk)
    *           UnaryX --- (kind = deref)
    *               IdLeaf --- (ident = s(MthId).rcvFrm)
    *
    * ------------------------------------------------- *)
    kid1 := c.kid;
    kid1.ExprError(300);                (* A warning only ...   *)
    WITH kid1 : IdentX DO
      kid2 := kid1.kid;
      IF kid2.kind = deref THEN kid2 := kid2(UnaryX).kid END;
      WITH kid2 : IdLeaf DO
        WITH s : I.MthId DO
          IF kid2.ident # s.rcvFrm THEN c.ExprError(166) END;
        ELSE
          c.ExprError(166);
        END;
      ELSE
        c.ExprError(166);
      END;
    ELSE
      c.ExprError(166);
    END;
  END CheckSuper;

(* -------------------------------------------- *)

  PROCEDURE (i : BinaryX)exprAttr*() : D.Expr;
    VAR rslt : D.Expr;
        kind : INTEGER;

    (* --------------------------- *)

    PROCEDURE chrOp(i : BinaryX) : D.Expr;
      VAR ch1,ch2 : CHAR;
          dRes : BOOLEAN;
          rslt : D.Expr;
    BEGIN
      rslt := i;
      IF i.lKid.isCharLit() & i.rKid.isCharLit() THEN
        ch1 := i.lKid(LeafX).charValue();
        ch2 := i.rKid(LeafX).charValue();
        CASE i.kind OF
        | greT   : dRes := ch1 > ch2;
        | greEq  : dRes := ch1 >= ch2;
        | notEq  : dRes := ch1 # ch2;
        | lessEq : dRes := ch1 <= ch2;
        | lessT  : dRes := ch1 < ch2;
        | equal  : dRes := ch1 = ch2;
        ELSE i.ExprError(171); RETURN NIL;
        END;
        IF dRes THEN
          rslt := mkTrueX();
        ELSE
          rslt := mkFalseX();
        END;
      ELSIF ~isRelop(i.kind) THEN
        i.ExprError(171);
      ELSE
        i.lKid.type := Builtin.charTp;
        i.rKid.type := Builtin.charTp;
      END;
      rslt.type := Builtin.boolTp; RETURN rslt;
    END chrOp;

    (* --------------------------- *)

    PROCEDURE strOp(i : BinaryX) : D.Expr;
      VAR fold : BOOLEAN;
          sRes : INTEGER;
          bRes : BOOLEAN;
          rslt : D.Expr;
    BEGIN (* Pre: lKid,rKid are a string-valued expressions *)
      IF i.kind = strCat THEN RETURN i END;                  (* ALREADY DONE *)
      fold := i.lKid.isStrLit() & i.rKid.isStrLit();
      rslt := i;
      IF i.kind = plus THEN
        IF fold THEN
          rslt :=  mkLeafVal(strLt, L.concat(i.lKid(LeafX).value,
                             i.rKid(LeafX).value));
        ELSE
          i.SetKind(strCat); (* can't assign via rslt, it is readonly! *)
        END;
        rslt.type := Builtin.strTp;
      ELSIF isRelop(i.kind) THEN
        IF fold THEN
          sRes := L.strCmp(i.lKid(LeafX).value, i.rKid(LeafX).value);
          CASE i.kind OF
          | greT   : bRes := sRes > 1;
          | greEq  : bRes := sRes >= 0;
          | notEq  : bRes := sRes # 0;
          | lessEq : bRes := sRes <= 0;
          | lessT  : bRes := sRes < 0;
          | equal  : bRes := sRes = 0;
          END;
          IF bRes THEN
            rslt := mkTrueX();
          ELSE
            rslt := mkFalseX();
          END;
        (* ELSE nothing to do *)
        END;
        rslt.type := Builtin.boolTp;
      ELSE
        i.ExprError(171); RETURN NIL;
      END;
      RETURN rslt;
    END strOp;

    (* --------------------------- *)

    PROCEDURE setOp(i : BinaryX) : D.Expr;
      VAR newX : D.Expr;
          rsTp : D.Type;
          dRes : BOOLEAN;
          lSet,rSet,dSet : SET;
    BEGIN (* Pre: lKid is a set-valued expression *)
      rsTp := Builtin.setTp;
      dRes := FALSE; dSet := {};
      IF ~i.rKid.isSetExpr() THEN i.rKid.ExprError(35); RETURN NIL END;
      IF (i.lKid.kind = setLt) & (i.rKid.kind = setLt) THEN
        lSet := i.lKid(LeafX).value.set();
        rSet := i.rKid(LeafX).value.set();
        CASE i.kind OF
        | plus, bitOr:  dSet := lSet + rSet;
        | minus :   dSet := lSet - rSet;
        | mult, bitAnd: dSet := lSet * rSet;
        | slash,bitXor: dSet := lSet / rSet;
        | greT   :  dRes := lSet > rSet;  rsTp := Builtin.boolTp;
        | greEq  :  dRes := lSet >= rSet; rsTp := Builtin.boolTp;
        | notEq  :  dRes := lSet # rSet;  rsTp := Builtin.boolTp;
        | lessEq :  dRes := lSet <= rSet; rsTp := Builtin.boolTp;
        | lessT  :  dRes := lSet < rSet;  rsTp := Builtin.boolTp;
        | equal  :  dRes := lSet = rSet;  rsTp := Builtin.boolTp;
        ELSE i.ExprError(171);
        END;
        IF rsTp # Builtin.boolTp THEN
          newX := mkSetLt(dSet);
        ELSIF dRes THEN
          newX := mkTrueX();
        ELSE
          newX := mkFalseX();
        END;
      ELSE
        CASE i.kind OF
        | plus   : i.SetKind(bitOr);
        | mult   : i.SetKind(bitAnd);
        | slash  : i.SetKind(bitXor);
        | minus  : i.SetKind(bitAnd);
             i.rKid := newUnaryX(compl, i.rKid);
             i.rKid.type := rsTp;
        | greT, greEq, notEq, lessEq, lessT, equal : rsTp := Builtin.boolTp;
        ELSE i.ExprError(171);
        END;
        newX := i;
      END;
      newX.type := rsTp; RETURN newX;
    END setOp;

    (* --------------------------- *)

    PROCEDURE numOp(i : BinaryX) : D.Expr;
      VAR newX : D.Expr;
          rsTp : D.Type;
          dRes : BOOLEAN;
          rLit : LONGINT;
          lVal, rVal, dVal : L.Value;
          lFlt, rFlt, dFlt : REAL;
    BEGIN (* Pre: rKid is a numeric expression *)
      dRes := FALSE; dFlt := 0.0; dVal := NIL;
      IF ~i.lKid.isNumericExpr() THEN i.lKid.ExprError(38); RETURN NIL END;
      IF i.kind = slash THEN
        rsTp := Builtin.realTp;
      ELSE
        rsTp := coverType(i.lKid.type, i.rKid.type);
        IF rsTp = NIL THEN i.ExprError(38); RETURN NIL END;
      END;
      (* First we coerce to a common type, if that is necessary *)
      IF rsTp # i.lKid.type THEN i.lKid := coerceUp(i.lKid, rsTp) END;
      IF rsTp # i.rKid.type THEN i.rKid := coerceUp(i.rKid, rsTp) END;

      IF (i.lKid.kind = numLt) & (i.rKid.kind = numLt) THEN
        lVal := i.lKid(LeafX).value;
        rVal := i.rKid(LeafX).value;
        CASE i.kind OF
        | plus   : dVal := L.addV(lVal, rVal);
        | minus  : dVal := L.subV(lVal, rVal);
        | mult   : dVal := L.mulV(lVal, rVal);
        | modOp  : dVal := L.modV(lVal, rVal);
        | divOp  : dVal := L.divV(lVal, rVal);
      
        | rem0op : dVal := L.rem0V(lVal, rVal);
        | div0op : dVal := L.div0V(lVal, rVal);
      
        | slash  : dVal := L.slashV(lVal, rVal);       rsTp := Builtin.realTp;
        | greT   : dRes := lVal.long() >  rVal.long(); rsTp := Builtin.boolTp;
        | greEq  : dRes := lVal.long() >= rVal.long(); rsTp := Builtin.boolTp;
        | notEq  : dRes := lVal.long() #  rVal.long(); rsTp := Builtin.boolTp;
        | lessEq : dRes := lVal.long() <= rVal.long(); rsTp := Builtin.boolTp;
        | lessT  : dRes := lVal.long() <  rVal.long(); rsTp := Builtin.boolTp;
        | equal  : dRes := lVal.long() =  rVal.long(); rsTp := Builtin.boolTp;
        ELSE i.ExprError(171);
        END;
        IF rsTp = Builtin.realTp THEN
          newX := mkRealLt(dFlt);
        ELSIF rsTp # Builtin.boolTp THEN (* ==> some int type *)
          newX := mkLeafVal(numLt, dVal);
        ELSIF dRes THEN
          newX := mkTrueX();
        ELSE
          newX := mkFalseX();
        END;
      ELSIF (i.lKid.kind = realLt) & (i.rKid.kind = realLt) THEN
        lFlt := i.lKid(LeafX).value.real(); rFlt := i.rKid(LeafX).value.real();
        CASE i.kind OF
        | plus   : dFlt := lFlt + rFlt;
        | minus  : dFlt := lFlt - rFlt;
        | mult   : dFlt := lFlt * rFlt;
        | slash  : dFlt := lFlt / rFlt;
        | greT   : dRes := lFlt > rFlt;   rsTp := Builtin.boolTp;
        | greEq  : dRes := lFlt >= rFlt;  rsTp := Builtin.boolTp;
        | notEq  : dRes := lFlt # rFlt;   rsTp := Builtin.boolTp;
        | lessEq : dRes := lFlt <= rFlt;  rsTp := Builtin.boolTp;
        | lessT  : dRes := lFlt < rFlt;   rsTp := Builtin.boolTp;
        | equal  : dRes := lFlt = rFlt;   rsTp := Builtin.boolTp;
        ELSE i.ExprError(171);
        END;
        IF rsTp # Builtin.boolTp THEN
           newX := mkRealLt(dFlt);
        ELSIF dRes THEN
          newX := mkTrueX();
        ELSE
          newX := mkFalseX();
        END;
(* 
 *    SHOULD FOLD IEEE INFINITIES HERE! 
 *)
      ELSE
        CASE i.kind OF
        | plus, minus, mult, slash :
            (* skip *)
        | rem0op, div0op :
            IF rsTp.isRealType() THEN i.ExprError(45) END;
        | modOp, divOp :
            IF rsTp.isRealType() THEN
              i.ExprError(45);
            ELSIF  (i.rKid.kind = numLt) THEN
              rLit := i.rKid(LeafX).value.long();
              IF isPowerOf2(rLit) THEN
                IF i.kind = modOp THEN
                  i.SetKind(bitAnd);
                  i.rKid := mkNumLt(rLit - 1);
                ELSE
                  i.SetKind(ashInt);
                  i.rKid := mkNumLt(-log2(rLit)); (* neg ==> right shift *)
                END;
              END;
            END;
        | greT,  greEq, notEq, lessEq, lessT, equal :
            rsTp := Builtin.boolTp;
        ELSE i.ExprError(171);
        END;
        newX := i;
      END;
      newX.type := rsTp; RETURN newX;
    END numOp;

    (* --------------------------- *)

    PROCEDURE isTest(b : BinaryX) : D.Expr;
      VAR dstT : D.Type;
    BEGIN
      IF b.lKid.type = NIL THEN RETURN NIL END;
      dstT := getQualType(b.rKid);
      IF dstT = NIL THEN b.rKid.ExprError(5); RETURN NIL END;
      IF ~b.lKid.hasDynamicType() THEN b.lKid.ExprError(17); RETURN NIL END;
      IF ~b.lKid.type.isBaseOf(dstT) THEN b.ExprError(34); RETURN NIL END;
      b.type := Builtin.boolTp; RETURN b;
    END isTest;

    (* --------------------------- *)

    PROCEDURE inTest(b : BinaryX) : D.Expr;
      VAR sVal : SET;
          iVal : INTEGER;
          rslt : D.Expr;
    BEGIN
      IF ~b.lKid.isIntExpr() THEN b.lKid.ExprError(37); RETURN NIL END;
      IF ~b.rKid.isSetExpr() THEN b.rKid.ExprError(35); RETURN NIL END;
      rslt := b;
      IF (b.lKid.kind = strLt) & (b.rKid.kind = setLt) THEN
        iVal := b.lKid(LeafX).value.int();
        sVal := b.rKid(LeafX).value.set();
        IF iVal IN sVal THEN
          rslt := mkTrueX();
        ELSE
          rslt := mkFalseX();
        END;
      END;
      rslt.type := Builtin.boolTp; RETURN rslt;
    END inTest;

    (* --------------------------- *)

    PROCEDURE EqualOkCheck(node : BinaryX);
      VAR lTp,rTp : D.Type;
    BEGIN
      lTp := node.lKid.type;
      rTp := node.rKid.type;
      IF (lTp = NIL) OR (rTp = NIL) THEN RETURN END;
     (*
      *  The permitted cases here are:
      *   comparisons of Booleans
      *   comparisons of pointers (maybe sanity checked?)
      *   comparisons of procedures (maybe sanity checked?)
      *)
      IF (node.lKid.isBooleanExpr() & node.rKid.isBooleanExpr()) OR
         (node.lKid.isPointerExpr() & node.rKid.isPointerExpr()) OR
         (node.lKid.isProcExpr()    & node.rKid.isProcExpr()) THEN
        node.type := Builtin.boolTp;
      ELSE
        D.RepTypesErrTok(57, node.lKid.type, node.rKid.type, node.token);
      END;
    END EqualOkCheck;

    (* --------------------------- *)

    PROCEDURE boolBinOp(i : BinaryX) : D.Expr;
      VAR rslt : D.Expr;
    BEGIN
      IF i.lKid.type # Builtin.boolTp THEN i.lKid.ExprError(36) END;
      IF i.rKid.type # Builtin.boolTp THEN i.rKid.ExprError(36) END;
      IF i.lKid.kind = tBool THEN
        IF i.kind = blOr THEN
          rslt := i.lKid;       (* return the TRUE  *)
        ELSE
          rslt := i.rKid;       (* return the rhs-expr  *)
        END;
      ELSIF i.lKid.kind = fBool THEN
        IF i.kind = blOr THEN
          rslt := i.rKid;       (* return the rhs-expr  *)
        ELSE
          rslt := i.lKid;       (* return the FALSE *)
        END;
      ELSE
        rslt := i;
        rslt.type := Builtin.boolTp;
      END;
      RETURN rslt;
    END boolBinOp;

    (* --------------------------- *)

  BEGIN  (* BinaryX exprAttr body *)
    rslt := NIL;
    kind := i.kind;
   (*
    *  The following cases are fully attributed already
    *  perhaps as a result of a call of checkCall()
    *)
    IF (kind = index) OR (kind = ashInt) OR
       (kind = lshInt) OR (kind = rotInt) OR 
       (kind = lenOf) OR (kind = minOf) OR (kind = maxOf) THEN RETURN i END;
   (*
    *  First, attribute the subtrees.
    *)
    IF (i.lKid = NIL) OR (i.rKid = NIL) THEN RETURN NIL END;
    i.lKid := i.lKid.exprAttr();    (* process subtree  *)
    i.rKid := i.rKid.exprAttr();    (* process subtree  *)
    IF (i.lKid = NIL) OR (i.rKid = NIL) THEN RETURN NIL END;
   (*
    *  Deal with unique cases first... IN and IS, then OR and &
    *)
    IF kind = range THEN
      rslt := i;
    ELSIF kind = inOp THEN
      rslt := inTest(i);
    ELSIF kind = isOp THEN
      rslt := isTest(i);
    ELSIF (kind = blOr) OR
    (kind = blAnd) THEN
      rslt := boolBinOp(i);
   (*
    *  Deal with set-valued expressions, including constant folding.
    *)
    ELSIF i.lKid.isSetExpr() THEN
      rslt := setOp(i);
   (*
    *  Deal with numerical expressions, including constant folding.
    *  Note that we test the right subtree, to avoid (num IN set) case.
    *)
    ELSIF i.rKid.isNumericExpr() THEN
      rslt := numOp(i);
   (*
    *  Deal with string expressions, including constant folding.
    *  Note that this must be done before dealing characters so
    *  as to correctly deal with literal strings of length one.
    *)
    ELSIF (i.lKid.isString() OR i.lKid.isCharArray()) &
    (i.rKid.isString() OR i.rKid.isCharArray()) THEN
      rslt := strOp(i);
   (*
    *  Deal with character expressions, including constant folding.
    *)
    ELSIF i.lKid.isCharExpr() & i.rKid.isCharExpr() THEN
      rslt := chrOp(i);
   (*
    *  Now all the irregular cases.
    *)
    ELSIF (kind = equal) OR (kind = notEq) THEN
      EqualOkCheck(i);
      i.type := Builtin.boolTp;
      rslt := i;
    ELSE
      i.ExprError(171);
    END;
    RETURN rslt;
  END exprAttr;

(* ============================================================ *)
(*         Flow attribution for actual parameter lists    *)
(* ============================================================ *)

  PROCEDURE (cXp : CallX)liveActuals(scp : D.Scope;
                                     set : V.VarSet) : V.VarSet,NEW;
    VAR idx : INTEGER;
        act : D.Expr;
        xKd : D.Expr;
        frm : I.ParId;
        pTp : T.Procedure;
        new : V.VarSet;
  BEGIN
    new := set.newCopy();
    xKd := cXp.kid;
    pTp := xKd.type(T.Procedure);
    FOR idx := 0 TO cXp.actuals.tide-1 DO
      act := cXp.actuals.a[idx];
      frm := pTp.formals.a[idx];
      IF frm.parMod # D.out THEN
       (*
        *  We accumulate the effect of each evaluation, using
        *  "set" as input in each case.  This is conservative,
        *  assuming parallel (but strict) evaluation.
        *)
        new := act.checkLive(scp, set).cup(new);
      ELSE
        new := act.assignLive(scp, new);
      END;
    END;
   (* 
    *   If locals are uplevel addressed we presume that they
    *   might be initialized by any call of a nested procedure.
    *)
    IF scp IS I.Procs THEN
      WITH xKd : IdentX DO
          IF xKd.ident.dfScp = scp THEN scp.UplevelInitialize(new) END;
      | xKd : IdLeaf DO
          IF xKd.ident.dfScp = scp THEN scp.UplevelInitialize(new) END;
      | xKd : UnaryX DO
          ASSERT(xKd.kind = tCheck);
      END (* skip *)
    END;
    (* #### kjg, Sep-2001 *)
    RETURN new;
  END liveActuals;

(* -------------------------------------------- *)

  PROCEDURE (x : CallX)liveStdProc(scp : D.Scope;
                                   set : V.VarSet) : V.VarSet,NEW;
  (** Compute the live-out set as a result of the call of this  *)
  (*  standard procedure.  Standard functions are all inline.   *)
    VAR funI : I.PrcId;
        funN : INTEGER;
        arg0 : D.Expr;
        tmpS : V.VarSet;
        indx : INTEGER;
  BEGIN
    funI := x.kid(IdLeaf).ident(I.PrcId);
    funN := funI.stdOrd;
    arg0 := x.actuals.a[0];
   (*
    *  Now we check the per-case semantics.
    *)
    IF funN = Builtin.newP THEN
     (*
      *  It is tempting, but incorrect to omit the newCopy()
      *  and chain the values from arg to arg.  However we do
      *  not guarantee the order of evaluation (for native code).
      *  Likewise, it is not quite correct to skip the "cup" with
      *      tmpS := arg0.assignLive(scp, tmpS);
      *  since one of the LEN evals might have a side-effect on
      *  the base qualId of the first parameter.
      *)
      IF x.actuals.tide > 1 THEN
        tmpS := set.newCopy();
        FOR indx := 1 TO x.actuals.tide-1 DO
          tmpS := tmpS.cup(x.actuals.a[indx].checkLive(scp, set));
        END;
        tmpS := tmpS.cup(arg0.assignLive(scp, set));
      ELSE
        tmpS := arg0.assignLive(scp, set);
      END;
    ELSIF funN = Builtin.asrtP THEN
      tmpS := arg0.checkLive(scp, set); (* arg1 is a literal! *)
    ELSIF (funN = Builtin.haltP) OR (funN = Builtin.throwP) THEN
      tmpS := arg0.checkLive(scp, set); (* and discard *)
      tmpS := V.newUniv(set.cardinality()); 
    ELSIF funN = Builtin.getP THEN
      tmpS := arg0.checkLive(scp, set);
      tmpS := tmpS.cup(x.actuals.a[1].assignLive(scp, set));
    ELSIF funN = Builtin.putP THEN
      tmpS := arg0.checkLive(scp, set);
      tmpS := tmpS.cup(x.actuals.a[1].checkLive(scp, set));
    ELSE (* Builtin.incP, decP, inclP, exclP, cutP, apndP *)
      tmpS := arg0.assignLive(scp, set);
      IF x.actuals.tide = 2 THEN
        tmpS := tmpS.cup(x.actuals.a[1].checkLive(scp, set));
      END;
    END;
    RETURN tmpS;
  END liveStdProc;

(* ============================================================ *)
(*    Flow attribution for leaves: nothing to do for LeafX  *)
(* ============================================================ *)

  PROCEDURE (x : IdLeaf)checkLive*(scp : D.Scope;
                                   lIn : V.VarSet) : V.VarSet;
  (* If the variable is local, check that is is live *)
  (* Assert: expression has been fully attributed.   *)
  BEGIN
    IF  (x.ident.kind # I.conId) &
        (x.ident.dfScp = scp) &
        ~x.ident.isIn(lIn) THEN 
      IF x.isPointerExpr() THEN
        x.ExprError(316);
      ELSE
        x.ExprError(135);
      END;
    END;
    RETURN lIn;
  END checkLive;

(* -------------------------------------------- *)

  PROCEDURE (x : SetExp)checkLive*(scp : D.Scope;
                                   lIn : V.VarSet) : V.VarSet;
  (* Assert: expression has been fully attributed.   *)
  BEGIN
      (* Really: recurse over set elements *)
    RETURN lIn;
  END checkLive;

(* -------------------------------------------- *)

  PROCEDURE (x : LeafX)BoolLive*(scp : D.Scope;
                                 set : V.VarSet;
                             OUT tru,fal : V.VarSet);
  BEGIN
    IF    x.kind = tBool THEN
      tru := set;
      fal := V.newUniv(set.cardinality());
    ELSIF x.kind = fBool THEN
      tru := V.newUniv(set.cardinality());
      fal := set;
    ELSE
      tru := x.checkLive(scp, set);
      fal := tru;
    END;
  END BoolLive;

(* ============================================================ *)
(*    Flow attribution for unaries: nothing to do for IdentX  *)
(* ============================================================ *)

  PROCEDURE (x : UnaryX)BoolLive*(scp : D.Scope;
                                  set : V.VarSet;
                              OUT tru,fal : V.VarSet);
  BEGIN
    IF x.kind = blNot THEN
      x.kid.BoolLive(scp, set, fal, tru);
    ELSE
      tru := x.checkLive(scp, set);
      fal := tru;
    END;
  END BoolLive;

(* -------------------------------------------- *)

  PROCEDURE (x : UnaryX)checkLive*(scp : D.Scope;
                                   lIn : V.VarSet) : V.VarSet,EXTENSIBLE;
  (* Assert: expression has been fully attributed.   *)
  BEGIN
    RETURN x.kid.checkLive(scp, lIn);
  END checkLive;

(* -------------------------------------------- *)

  PROCEDURE (x : CallX)checkLive*(scp : D.Scope;
                                  lIn : V.VarSet) : V.VarSet;
  (* Assert: expression has been fully attributed.   *)
    VAR tmpS : V.VarSet;
  BEGIN
    tmpS :=  x.kid.checkLive(scp, lIn);
    IF (x.kind = prCall) & x.kid.isStdProc() THEN
      RETURN x.liveStdProc(scp, tmpS);
    ELSE
      RETURN x.liveActuals(scp, tmpS);
    END;
  END checkLive;

(* ============================================================ *)
(*      Flow attribution for binary expressions     *)
(* ============================================================ *)

  PROCEDURE (x : BinaryX)BoolLive*(scp : D.Scope;
                                   set : V.VarSet;
                               OUT tru,fal : V.VarSet);
  (** If this is a short-circuit operator evaluate the two      *)
  (*  subtrees and combine. Otherwise return unconditional set. *)
    VAR lhT, lhF, rhT, rhF : V.VarSet;
  BEGIN
    IF x.kind = blOr THEN
      x.lKid.BoolLive(scp, set, lhT, lhF);
      x.rKid.BoolLive(scp, lhF, rhT, fal);
      tru := lhT.cap(rhT);
    ELSIF x.kind = blAnd THEN
      x.lKid.BoolLive(scp, set, lhT, lhF);
      x.rKid.BoolLive(scp, lhT, tru, rhF);
      fal := lhF.cap(rhF);
    ELSE
      tru := x.checkLive(scp, set);
      fal := tru;
    END;
  END BoolLive;

(* -------------------------------------------- *)

  PROCEDURE (x : BinaryX)checkLive*(scp : D.Scope;
                                    lIn : V.VarSet) : V.VarSet;
  (* Assert: expression has been fully attributed.   *)
  (** Compute the live-out set resulting from the evaluation of *)
  (*  this expression, and check that any used occurrences of   *)
  (*  local variables are in the live set. Beware of the case   *)
  (*  where this is a Boolean expression with side effects!     *)
    VAR fSet, tSet : V.VarSet;
  BEGIN
    IF (x.kind = blOr) OR (x.kind = blAnd) THEN
      x.lKid.BoolLive(scp, lIn, tSet, fSet);
      IF x.kind = blOr THEN
       (*
        *  If this evaluation short circuits, then the result
        *  is tSet.  If the second factor is evaluated, the result
        *  is obtained by passing fSet as input to the second
        *  term evaluation. Thus the guaranteed output is the
        *  intersection of tSet and x.rKid.checkLive(fSet).
        *)
        RETURN tSet.cap(x.rKid.checkLive(scp, fSet));
      ELSE (* x.kind = blAnd *)
       (*
        *  If this evaluation short circuits, then the result
        *  is fSet.  If the second factor is evaluated, the result
        *  is obtained by passing tSet as input to the second
        *  factor evaluation. Thus the guaranteed output is the
        *  intersection of fSet and x.rKid.checkLive(tSet).
        *)
        RETURN fSet.cap(x.rKid.checkLive(scp, tSet));
      END;
    ELSE
      (* TO DO : check that this is OK for all the inlined standard functions *)
      RETURN x.lKid.checkLive(scp, lIn).cup(x.rKid.checkLive(scp, lIn));
    END;
  END checkLive;

(* ============================================================ *)
(*     Assign flow attribution for qualified id expressions   *)
(* ============================================================ *)

  PROCEDURE (p : IdLeaf)assignLive*(scpe : D.Scope;
                                    lvIn : V.VarSet) : V.VarSet;
    VAR tmpS : V.VarSet;
  BEGIN
    (* Invariant: input set lvIn is unchanged *)
    IF p.ident.dfScp = scpe THEN
      tmpS := lvIn.newCopy();
      tmpS.Incl(p.ident(I.AbVar).varOrd);
      RETURN tmpS;
    ELSE
      RETURN lvIn;
    END;
  END assignLive;

(* ============================================================ *)
(*         Predicates on Expr extensions              *)
(* ============================================================ *)

  PROCEDURE (x : IdLeaf)hasDynamicType*() : BOOLEAN;
  BEGIN
    RETURN (x.ident # NIL) & x.ident.isDynamic();
  END hasDynamicType;

(* -------------------------------------------- *)
(* -------------------------------------------- *)

  PROCEDURE (x : IdLeaf)isWriteable*() : BOOLEAN;
  (* A qualident is writeable if the IdLeaf is writeable  *)
  BEGIN
    RETURN x.ident.mutable();
  END isWriteable;

  PROCEDURE (x : IdLeaf)CheckWriteable*();
  (* A qualident is writeable if the IdLeaf is writeable  *)
  BEGIN
    x.ident.CheckMutable(x);
  END CheckWriteable;

(* -------------------------------------------- *)

  PROCEDURE (x : UnaryX)isWriteable*() : BOOLEAN,EXTENSIBLE;
  (* A referenced object is always writeable. *)
  (* tCheck nodes are always NOT writeable.   *)
  BEGIN RETURN x.kind = deref END isWriteable;

  PROCEDURE (x : UnaryX)CheckWriteable*(),EXTENSIBLE;
  (* A referenced object is always writeable. *)
  (* tCheck nodes are always NOT writeable.   *)
  BEGIN
    IF x.kind # deref THEN x.ExprError(103) END;
  END CheckWriteable;

(* -------------------------------------------- *)

  PROCEDURE (x : IdentX)isWriteable*() : BOOLEAN;
  (*  This case depends on the mutability of the record field,  *
   *  other cases of IdentX are not writeable at all.   *)
  BEGIN
    RETURN (x.kind = selct) & x.ident.mutable() & x.kid.isWriteable();
  END isWriteable;

  PROCEDURE (x : IdentX)CheckWriteable*();
  (*  This case depends on the mutability of the record field,  *
   *  other cases of IdentX are not writeable at all.   *)
  BEGIN
    IF x.kind = selct THEN
      x.ident.CheckMutable(x);
      x.kid.CheckWriteable();
    ELSE
      x.ExprError(103);
    END;
  END CheckWriteable;

(* -------------------------------------------- *)

  PROCEDURE (x : BinaryX)isWriteable*() : BOOLEAN;
  (*  The only possibly writeable case here is for array  *
   *  elements.  These are writeable if the underlying array is *)
  BEGIN
    RETURN (x.kind = index) & x.lKid.isWriteable();
  END isWriteable;

  PROCEDURE (x : BinaryX)CheckWriteable*();
  (*  The only possibly writeable case here is for array  *
   *  elements.  These are writeable if the underlying array is *)
  BEGIN
    IF x.kind # index THEN
      x.ExprError(103);
    ELSE
      x.lKid.CheckWriteable();
    END;
  END CheckWriteable;

(* -------------------------------------------- *)
(* -------------------------------------------- *)

  PROCEDURE (x : IdLeaf)isVarDesig*() : BOOLEAN;
  BEGIN
    RETURN x.ident IS I.AbVar; (* varId or parId *)
  END isVarDesig;

(* -------------------------------------------- *)

  PROCEDURE (x : UnaryX)isVarDesig*() : BOOLEAN,EXTENSIBLE;
  BEGIN RETURN x.kind = deref END isVarDesig;

(* -------------------------------------------- *)

  PROCEDURE (x : IdentX)isVarDesig*() : BOOLEAN;
  BEGIN
    RETURN x.kind = selct;
  END isVarDesig;

(* -------------------------------------------- *)

  PROCEDURE (x : BinaryX)isVarDesig*() : BOOLEAN;
  BEGIN
    RETURN x.kind = index;
  END isVarDesig;

(* -------------------------------------------- *)
(* -------------------------------------------- *)

  PROCEDURE (x : IdLeaf)isProcLit*() : BOOLEAN;
  BEGIN
   (*
    *  True if this is a concrete procedure
    *)
    RETURN (x.ident.kind = I.conPrc) OR
     (x.ident.kind = I.fwdPrc);
  END isProcLit;

(* -------------------------------------------- *)

  PROCEDURE (x : IdentX)isProcLit*() : BOOLEAN;
  BEGIN
   (*
    *  True if this is a concrete procedure
    *)
    RETURN (x.ident.kind = I.conMth) OR
     (x.ident.kind = I.fwdMth);
  END isProcLit;

(* -------------------------------------------- *)
(* -------------------------------------------- *)

  PROCEDURE (x : IdLeaf)isProcVar*() : BOOLEAN;
  BEGIN
   (*
    *  True if this has procedure type, but is not a concrete procedure
    *)
    RETURN x.type.isProcType() &
     (x.ident.kind # I.conPrc) &
     (x.ident.kind # I.fwdPrc) &
     (x.ident.kind # I.ctorP);
  END isProcVar;

(* -------------------------------------------- *)

  PROCEDURE (x : IdentX)isProcVar*() : BOOLEAN;
  BEGIN
   (*
    *  True if this is a selct, and field has procedure type
    *)
    RETURN (x.kind = selct) &
     (x.ident IS I.FldId) &
      x.type.isProcType();
  END isProcVar;

(* -------------------------------------------- *)

  PROCEDURE (x : UnaryX)isProcVar*() : BOOLEAN,EXTENSIBLE;
  BEGIN
   (*
    *  This depends on the fact that x.kid will be
    *  of System.Delegate type, and is being cast
    *  to some subtype of Procedure or Event type.
    *)
    RETURN (x.kind = tCheck) & x.type.isProcType();
  END isProcVar;

(* -------------------------------------------- *)

  PROCEDURE (x : BinaryX)isProcVar*() : BOOLEAN;
  BEGIN
   (*
    *  True if this is an index, and element has procedure type
    *)
    RETURN (x.kind = index) & x.type.isProcType();
  END isProcVar;

(* -------------------------------------------- *)

  PROCEDURE (x : LeafX)isNil*() : BOOLEAN;
  BEGIN RETURN x.kind = nilLt END isNil;

(* -------------------------------------------- *)

  PROCEDURE (x : LeafX)isInf*() : BOOLEAN;
  BEGIN RETURN (x.kind = infLt) OR (x.kind = nInfLt) END isInf;

(* -------------------------------------------- *)

  PROCEDURE (x : LeafX)isNumLit*() : BOOLEAN;
  BEGIN RETURN x.kind = numLt END isNumLit;

(* -------------------------------------------- *)

  PROCEDURE (x : LeafX)isCharLit*() : BOOLEAN;
  (** A literal character, or a literal string of length = 1.   *)
  BEGIN
    RETURN (x.kind = charLt)
        OR ((x.kind = strLt) & (x.value.len() = 1));
  END isCharLit;

(* -------------------------------------------- *)

  PROCEDURE (x : LeafX)isStrLit*() : BOOLEAN;
  (* If this is a LeafX of string type, it must b a lit-string. *)
  BEGIN RETURN x.kind = strLt END isStrLit;

(* ==================================================================== *)
(*      Possible structures of procedure call expressions are:          *)
(* ==================================================================== *)
(*                  o                               o                   *)
(*                 /                               /                    *)
(*              [CallX]                         [CallX]                 *)
(*               / +--- actuals --> ...          / +--- actuals --> ... *)
(*              /                               /                       *)
(*          [IdentX]                        [IdLeaf]                    *)
(*            /  +--- ident ---> [PrcId]      +--- ident ---> [PrcId]   *)
(*           /                                                          *)
(*       kid expr                                                       *)
(*                                                                      *)
(* ==================================================================== *)
(*  only the right hand side case can be a standard proc or function    *)
(* ==================================================================== *)

  PROCEDURE (x : IdLeaf)isStdFunc*() : BOOLEAN;
  BEGIN
    RETURN (x.ident # NIL)
         & (x.ident.kind = I.conPrc)
         & (x.ident(I.PrcId).stdOrd # 0);
  END isStdFunc;

(* -------------------------------------------- *)

  PROCEDURE (x : IdLeaf)isStdProc*() : BOOLEAN;
  BEGIN
    RETURN (x.ident # NIL)
         & (x.ident.kind = I.conPrc)
         & (x.ident(I.PrcId).stdOrd # 0);
  END isStdProc;

(* -------------------------------------------- *)

  PROCEDURE (p : CallX)NoteCall*(s : D.Scope);
  BEGIN
    p.kid.NoteCall(s);
  END NoteCall;

(* -------------------------------------------- *)

  PROCEDURE (p : IdLeaf)NoteCall*(s : D.Scope);
    VAR proc : I.PrcId;
  BEGIN
    IF (p.ident # NIL) &
       ((p.ident.kind = I.fwdPrc) OR
        (p.ident.kind = I.conPrc)) THEN
      proc := p.ident(I.PrcId);
      IF proc.stdOrd = 0 THEN INCL(proc.pAttr, I.called) END;
    END;
  END NoteCall;

(* -------------------------------------------- *)

  PROCEDURE (p : IdentX)NoteCall*(s : D.Scope);
    VAR proc : I.MthId;
  BEGIN
    IF (p.ident # NIL) &
       ((p.ident.kind = I.fwdMth) OR
        (p.ident.kind = I.conMth)) THEN
      proc := p.ident(I.MthId);
      INCL(proc.pAttr, I.called);
    END;
  END NoteCall;

(* -------------------------------------------- *)

  PROCEDURE (x : LeafX)inRangeOf*(dst : D.Type) : BOOLEAN;
    VAR lVal : LONGINT;
        cVal : CHAR;
        sLen : INTEGER;
        aLen : INTEGER;
  BEGIN
    IF x.kind = numLt THEN
      lVal := x.value.long();
      IF dst.kind = T.vecTp THEN RETURN TRUE;
      ELSIF dst.kind = T.arrTp THEN
        sLen := dst(T.Array).length;
        RETURN (lVal >= 0) &      (* check open array later *)
               ((sLen = 0) OR (lVal < sLen))  (* otherwise check now    *)
      ELSIF dst = Builtin.setTp THEN
        RETURN (lVal >= 0) & (lVal <= 31);
      ELSIF ~dst.isNumType() THEN
        RETURN FALSE;
      ELSE
        CASE dst(T.Base).tpOrd OF
        | T.uBytN : RETURN (lVal >= ORD(MIN(SHORTCHAR))) &
                           (lVal <= ORD(MAX(SHORTCHAR)));
        | T.byteN : RETURN (lVal >= MIN(BYTE))   & (lVal <= MAX(BYTE));
        | T.sIntN : RETURN (lVal >= MIN(SHORTINT)) & (lVal <= MAX(SHORTINT));
        | T.intN  : RETURN (lVal >= MIN(INTEGER))  & (lVal <= MAX(INTEGER));
        | T.lIntN : RETURN TRUE;
        ELSE RETURN FALSE;
        END
      END;
(*
 *  Changed for 1.2.3.4 to allow S1 to be compat with ARRAY OF CHAR (kjg)
 *
 *  ELSIF x.isCharLit() THEN
 *    IF ~dst.isCharType() THEN
 *)
    ELSIF dst.isCharType() THEN
      IF ~x.isCharLit() THEN
        RETURN FALSE;
      ELSE
        cVal := x.charValue();
        IF dst(T.Base).tpOrd = T.sChrN THEN
          RETURN (cVal >= MIN(SHORTCHAR)) & (cVal <= MAX(SHORTCHAR));
        ELSE
          RETURN TRUE;
        END;
      END;
(*
 *  ELSIF x.kind = strLt THEN
 *    IF ~dst.isCharArrayType() THEN
 *)
    ELSIF dst.isCharArrayType() THEN
      IF x.kind # strLt THEN
        RETURN FALSE;
      ELSE
        aLen := dst(T.Array).length;
        sLen := x.value.len();
        RETURN  (aLen = 0) OR   (* lhs is open array, runtime test *)
                (aLen > sLen);    (* string fits in fixed array OK   *)
      END;
    ELSE
      RETURN FALSE;
    END;
  END inRangeOf;

(* ============================================================ *)

  PROCEDURE (x : LeafX)charValue*() : CHAR,NEW;
  (** A literal character, or a literal string of length = 1.   *)
    VAR chr : CHAR;
  BEGIN
    IF x.kind = charLt THEN
      chr := x.value.char();
    ELSE (* x.kind = strLt *)
      chr := x.value.chr0();
    END;
    RETURN chr;
  END charValue;

(* -------------------------------------------- *)

  PROCEDURE convert(expr : D.Expr; dstT : D.Type) : D.Expr;
  (* Make permitted base-type coercions explicit in the AST *)
    VAR rslt : D.Expr;
        expT : D.Type;
		valu : INTEGER;
  BEGIN
    expT := expr.type;
    IF  (expT = dstT) OR
        (dstT.kind # T.basTp) OR
        (dstT = Builtin.anyPtr) THEN
      RETURN expr;
    ELSIF (dstT = Builtin.charTp) & (expT = Builtin.strTp) THEN	  
      expr.type := dstT;
	  RETURN expr;
    ELSIF (dstT = Builtin.sChrTp) & (expT = Builtin.strTp) THEN
      valu := ORD(expr(LeafX).value.chr0());
	  IF (valu < 255) THEN  
        expr.type := dstT;
		RETURN expr;
      ELSE
        expr.type := Builtin.charTp;
      END;
    END;
    IF dstT.includes(expr.type) THEN
      rslt := newIdentX(cvrtUp, dstT.idnt, expr);
    ELSE
      rslt := newIdentX(cvrtDn, dstT.idnt, expr);
    END;
    rslt.type := dstT;
    RETURN rslt;
  END convert;

(* ============================================================ *)

  PROCEDURE FormalsVsActuals*(prcX : D.Expr; actSeq : D.ExprSeq);
    VAR prcT   : T.Procedure;
        index  : INTEGER;
        bound  : INTEGER;
        frmMod : INTEGER;
        actual : D.Expr;
        formal : I.ParId;
        frmTyp : D.Type;
        actTyp : D.Type;
        frmSeq : I.ParSeq;
        fIsPtr : BOOLEAN;

(* ---------------------------- *)

    PROCEDURE CheckCompatible(frm : D.Idnt; act : D.Expr);
    BEGIN
      IF frm.paramCompat(act) OR
         frm.type.arrayCompat(act.type) THEN (* is OK, skip *)
      ELSE
        D.RepTypesErrTok(21, act.type, frm.type, act.token);
        IF (act.type IS T.Opaque) &
           (act.type.idnt # NIL) &
           (act.type.idnt.dfScp # NIL) THEN
          S.SemError.RepSt1(175,
                            D.getName.ChPtr(act.type.idnt.dfScp),
                            act.token.lin, act.token.col);
        END;
      END;
    END CheckCompatible;

(* ---------------------------- *)

    PROCEDURE CheckVarModes(mod : INTEGER; exp : D.Expr);

     (* ---------------------------- *)

      PROCEDURE hasReferenceType(t : D.Type) : BOOLEAN;
      BEGIN
        RETURN  (t.kind = T.ptrTp) OR
                (t.kind = T.recTp) OR
                (t.kind = T.arrTp) OR
                (t.kind = T.namTp) OR
                (t = Builtin.strTp) OR
                (t = Builtin.anyPtr);
      END hasReferenceType;

     (* ---------------------------- *)

      PROCEDURE MarkAddrsd(id : D.Idnt);
      BEGIN
        WITH id : I.LocId DO INCL(id.locAtt, I.addrsd); ELSE END;
      END MarkAddrsd;

     (* ---------------------------- *)

    BEGIN (* Assert: mod is IN, OUT, or VAR *)
      IF mod = D.in THEN      (* IN mode only   *)
       (*
        *  Not strictly correct according to the report, but an *
        *  innocuous extension -- allow literal strings here. *
        *
        * IF (exp.type # Builtin.strTp) & ~exp.isVarDesig() THEN
        *)
        IF ~exp.isVarDesig() &
           (exp.type # NIL) & ~hasReferenceType(exp.type) THEN
          exp.ExprError(174);
        END;
      ELSE
        exp.CheckWriteable();     (* OUT and VAR modes  *)
        WITH exp : IdLeaf DO MarkAddrsd(exp.ident) ELSE END;
      END;
    END CheckVarModes;

(* ---------------------------- *)

  BEGIN
    prcT := prcX.type(T.Procedure);
    frmSeq := prcT.formals;
    bound  := MIN(actSeq.tide, frmSeq.tide) - 1;
    FOR index := 0 TO bound DO
      formal := frmSeq.a[index];
      actual := actSeq.a[index];

     (* compute attributes for the actual param expression *)
      IF actual # NIL THEN actual := actual.exprAttr() END;
     (* Now check the semantic rules for conformance *)
      IF (actual # NIL) &
         (formal # NIL) &
         (actual.type # NIL) &
         (formal.type # NIL) THEN
        frmTyp := formal.type;
        actTyp := actual.type;

        IF frmTyp IS T.Procedure THEN
          formal.IdError(301);
          IF G.targetIsJVM() THEN formal.IdError(320);
          ELSIF (frmTyp # actTyp) &
             ~actual.isProcLit() THEN formal.IdError(191) END;
        END;
        IF frmTyp IS T.Opaque THEN
          formal.type := frmTyp.resolve(1);
          frmTyp := formal.type;
        END;
        frmMod := formal.parMode();
        fIsPtr := frmTyp.isPointerType();
        IF (actTyp.kind = T.ptrTp) &
           ~fIsPtr THEN actual := mkDeref(actual) END;
        CheckCompatible(formal, actual);
        IF frmMod # D.val THEN    (* IN, OUT or VAR modes *)
          CheckVarModes(frmMod, actual);
          IF (frmMod = D.out) & (actTyp # frmTyp) & actTyp.isDynamicType() THEN
            D.RepTypesErrTok(306, actTyp, frmTyp, actual.token);
          END;
        ELSIF actTyp # frmTyp THEN
          actual := convert(actual, frmTyp);
          IF ~frmTyp.valCopyOK() THEN formal.IdError(153) END;
        END;
        actSeq.a[index] := actual;
      END;
    END;
    IF frmSeq.tide > actSeq.tide THEN
      IF actSeq.tide = 0 THEN
        prcX.ExprError(149);
      ELSE
        actSeq.a[actSeq.tide-1].ExprError(22);
      END;
    ELSIF actSeq.tide > frmSeq.tide THEN
      actual := actSeq.a[frmSeq.tide];
      IF actual # NIL THEN 
        actSeq.a[frmSeq.tide].ExprError(23);
      ELSE
        prcX.ExprError(23);
      END;
    END;
  END FormalsVsActuals;

(* ============================================================ *)

  PROCEDURE AttributePars*(actSeq : D.ExprSeq);
  VAR actual : D.Expr;
      index : INTEGER;
  BEGIN
    FOR index := 0 TO actSeq.tide-1 DO
      actual := actSeq.a[index];
      IF actual # NIL THEN actSeq.a[index] := actual.exprAttr(); END;
    END;
  END AttributePars;

(* ============================================================ *)

  PROCEDURE MatchPars*(frmSeq : I.ParSeq; actSeq : D.ExprSeq) : BOOLEAN;
    VAR
        index  : INTEGER;
        actual : D.Expr;
        formal : I.ParId;
        frmTyp : D.Type;
        actTyp : D.Type;
        fIsPtr : BOOLEAN;

  BEGIN
    IF (frmSeq.tide # actSeq.tide) THEN RETURN FALSE; END;
    FOR index := 0 TO frmSeq.tide-1 DO
      formal := frmSeq.a[index];
      actual := actSeq.a[index];
     (* Now check the semantic rules for conformance *)
      IF (actual # NIL) &
         (formal # NIL) &
         (actual.type # NIL) &
         (formal.type # NIL) THEN
        IF ~(formal.paramCompat(actual) OR
             formal.type.arrayCompat(actual.type)) THEN
          RETURN FALSE;
        END;
      ELSE
        RETURN FALSE;
      END;
    END;
    RETURN TRUE;
  END MatchPars;

(* ============================================================ *)

  PROCEDURE (p : BinaryX)enterGuard*(tmp : D.Idnt) : D.Idnt;
    VAR oldI  : D.Idnt;
        junk  : BOOLEAN;
        lHash : INTEGER;
        lQual : IdLeaf;
        rQual : IdLeaf;
  BEGIN
    IF  (p.lKid = NIL) OR
        ~(p.lKid IS IdLeaf) OR
        (p.rKid = NIL) OR
        ~(p.rKid IS IdLeaf) THEN RETURN NIL END;

    lQual := p.lKid(IdLeaf);
    rQual := p.rKid(IdLeaf);
    IF (lQual.ident = NIL) OR (rQual.ident = NIL) THEN RETURN NIL END;
   (*
    *  We first determine if this is a local variable.
    *  If it is, we must overwrite this in the local scope
    *  with the temporary of the guard type.
    *  If any case, we return the previous local.
    *)
    lHash := lQual.ident.hash;
    tmp.hash := lHash;
    tmp.type := rQual.ident.type;
   (* 
    *  It is an essential requirement of the host execution systems
	*  that the runtime type of the guarded variable may not be changed
	*  within the guarded region.  In the case of pointer to record
	*  types this is guaranteed by making the "tmp" copy immutable.
	*  Note that making the pointer variable read-only does not prevent
	*  the guarded region from mutating fields of the record.
	*
	*  In case the the guarded variable is an extensible record type.
	*  no action is required.  Any attempt to perform an entire
	*  assignment to the guarded variable will be a type-error.
	*  Every assignment to the entire variable will be either - 
	*  Error 83 (Expression not assignment compatible with destination), OR
	*  Error 143 (Cannot assign entire extensible or abstract record). 
    *)
	IF ~tmp.type.isRecordType() THEN tmp.SetKind(I.conId) END; (* mark immutable *)
    oldI := tmp.dfScp.symTb.lookup(lHash);
    IF oldI = NIL THEN (* not local *)
      junk := tmp.dfScp.symTb.enter(lHash, tmp);
      ASSERT(junk);
    ELSE
      tmp.dfScp.symTb.Overwrite(lHash, tmp);
    END;
    RETURN oldI;
  END enterGuard;

  PROCEDURE (p : BinaryX)ExitGuard*(sav : D.Idnt; tmp : D.Idnt);
  BEGIN
    IF tmp.type = NIL THEN RETURN END;
    IF sav = NIL THEN
      (* remove tmp from tmp.dfScp.symTb *)
      tmp.dfScp.symTb.RemoveLeaf(tmp.hash);
    ELSE
      (* overwrite with previous value   *)
      tmp.dfScp.symTb.Overwrite(tmp.hash, sav);
    END;
  END ExitGuard;

(* ============================================================ *)
(*      Diagnostic methods      *)
(* ============================================================ *)

  PROCEDURE Diag(i : INTEGER; e : D.Expr);
  BEGIN
    IF e = NIL THEN
      H.Indent(i); Console.WriteString("<nil>"); Console.WriteLn;
    ELSE
      e.Diagnose(i);
    END;
  END Diag;

  (* ------------------------------- *)

  PROCEDURE PType(t : D.Type);
  BEGIN
    IF t # NIL THEN
      Console.WriteString(t.name());
    ELSE
      Console.WriteString("<nil>");
    END;
  END PType;

(* -------------------------------------------- *)

  PROCEDURE (s : LeafX)Diagnose*(i : INTEGER),EXTENSIBLE;
    VAR name : FileNames.NameString;
  BEGIN
    H.Indent(i);
    CASE s.kind OF
    | realLt : Console.WriteString("realLt  ");
         RTS.RealToStr(s.value.real(), name);
         Console.WriteString(name$);
    | numLt  : Console.WriteString("numLt   ");
         Console.WriteInt(s.value.int(), 0);
    | charLt : Console.WriteString("charLt  '");
         Console.Write(s.value.char());
         Console.Write("'");
    | strLt  : Console.WriteString("strLt   ");
         s.value.GetStr(name);
         Console.Write('"');
         Console.WriteString(name$);
         Console.WriteString('" LEN=');
         Console.WriteInt(s.value.len(),1);
    | infLt  : Console.WriteString("INF     "); PType(s.type);
    | nInfLt : Console.WriteString("NEG-INF "); PType(s.type);
    | nilLt  : Console.WriteString("NIL     "); PType(s.type);
    | tBool  : Console.WriteString("TRUE    BOOLEAN");
    | fBool  : Console.WriteString("FALSE   BOOLEAN");
    ELSE       Console.WriteString("?leaf?  ");
    END;
    Console.WriteLn;
  END Diagnose;

  (* ------------------------------- *)

  PROCEDURE (s : IdLeaf)Diagnose*(i : INTEGER);
    VAR name : FileNames.NameString;
  BEGIN
    H.Indent(i);
    D.getName.Of(s.ident, name);
    Console.WriteString(name);
    Console.Write(':');
    Console.Write(' ');
    PType(s.type);
    Console.WriteLn;
  END Diagnose;

  (* ------------------------------- *)

  PROCEDURE (s : SetExp)Diagnose*(i : INTEGER);
    VAR j  : INTEGER;
        v  : SET;
        ch : CHAR;
  BEGIN
    ch := 0X;
    H.Indent(i);
    Console.WriteString("setLt  {");
    IF s.value # NIL THEN
      v := s.value.set();
      FOR j := 0 TO 31 DO
        IF j IN v THEN ch := '1' ELSE ch := '.' END;
        Console.Write(ch);
      END;
    END;
    Console.Write("}");
    IF s.kind = setLt THEN
      Console.WriteLn;
    ELSE
      Console.WriteString(" + "); Console.WriteLn;
      FOR j := 0 TO s.varSeq.tide - 1 DO
        Diag(i+4, s.varSeq.a[j]);
      END;
    END;
  END Diagnose;

  (* ------------------------------- *)

  PROCEDURE (s : UnaryX)Diagnose*(i : INTEGER),EXTENSIBLE;
  BEGIN
    H.Indent(i);
    CASE s.kind OF
    | deref  : Console.WriteString("'^'    ");
    | compl  : Console.WriteString("compl  ");
    | sprMrk : Console.WriteString("super  ");
    | neg    : Console.WriteString("neg    ");
    | absVl  : Console.WriteString("ABS    ");
    | entVl  : Console.WriteString("ENTIER ");
    | capCh  : Console.WriteString("CAP    ");
    | strLen : Console.WriteString("strLen ");
    | strChk : Console.WriteString("strChk ");
    | mkStr  : Console.WriteString("$      ");
    | tCheck : Console.WriteString("tCheck ");
               IF s.type # NIL THEN Console.WriteString(s.type.name()) END;
    END;
    PType(s.type);
    Console.WriteLn;
    Diag(i+4, s.kid);
  END Diagnose;

  (* ------------------------------- *)

  PROCEDURE (s : IdentX)Diagnose*(i : INTEGER);
    VAR name : FileNames.NameString;
  BEGIN
    H.Indent(i);
    D.getName.Of(s.ident, name);
    IF    s.kind = sprMrk THEN Console.WriteString("sprMrk " + name);
    ELSIF s.kind = cvrtUp THEN Console.WriteString("cvrtUp: " + name);
    ELSIF s.kind = cvrtDn THEN Console.WriteString("cvrtDn: " + name);
    ELSE Console.WriteString("selct: " + name);
    END;
    Console.Write(' ');
    PType(s.type);
    Console.WriteLn;
    Diag(i+4, s.kid);
  END Diagnose;

  (* ------------------------------- *)

  PROCEDURE (s : CallX)Diagnose*(i : INTEGER);
  BEGIN
    H.Indent(i);
    IF s.kind = fnCall THEN
      Console.WriteString("CallX(fn) "); PType(s.type);
    ELSE
      Console.WriteString("CallX(pr)");
    END;
    Console.WriteLn;
    Diag(i+4, s.kid);
  END Diagnose;

  (* ------------------------------- *)

  PROCEDURE (s : BinaryX)Diagnose*(i : INTEGER);
  BEGIN
    H.Indent(i);
    CASE s.kind OF
    | index  : Console.WriteString("index  ");
    | range  : Console.WriteString("range  ");
    | lenOf  : Console.WriteString("lenOf  ");
    | maxOf  : Console.WriteString("maxOf  ");
    | minOf  : Console.WriteString("minOf  ");
    | bitAnd : Console.WriteString("bitAND ");
    | bitOr  : Console.WriteString("bitOR  ");
    | bitXor : Console.WriteString("bitXOR ");
    | plus   : Console.WriteString("'+'    ");
    | minus  : Console.WriteString("'-'    ");
    | greT   : Console.WriteString("'>'    ");
    | greEq  : Console.WriteString("'>='   ");
    | notEq  : Console.WriteString("'#'    ");
    | lessEq : Console.WriteString("'<='   ");
    | lessT  : Console.WriteString("'<'    ");
    | equal  : Console.WriteString("'='    ");
    | isOp   : Console.WriteString("IS     ");
    | inOp   : Console.WriteString("IN     ");
    | mult   : Console.WriteString("'*'    ");
    | slash  : Console.WriteString("'/'    ");
    | modOp  : Console.WriteString("MOD    ");
    | divOp  : Console.WriteString("DIV    ");
    | rem0op : Console.WriteString("REM0   ");
    | div0op : Console.WriteString("DIV0   ");
    | blNot  : Console.WriteString("'~'    ");
    | blOr   : Console.WriteString("OR     ");
    | blAnd  : Console.WriteString("'&'    ");
    | strCat : Console.WriteString("strCat ");
    | ashInt : Console.WriteString("ASH    ");
    | lshInt : Console.WriteString("LSH    ");
    | rotInt : Console.WriteString("ROT    ");
    END;
    PType(s.type);
    Console.WriteLn;
    Diag(i+4, s.lKid);
    Diag(i+4, s.rKid);
  END Diagnose;

(* ============================================================ *)
BEGIN (* ====================================================== *)
END ExprDesc.  (* ============================================== *)
(* ============================================================ *)

