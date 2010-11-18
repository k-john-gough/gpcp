(* ==================================================================== *)
(*                                                                      *)
(*  StatDesc Module for the Gardens Point Component Pascal Compiler.    *)
(*  Implements statement descriptors that are extensions of             *)
(*  Symbols.Stmt                                                        *)
(*                                                                      *)
(*  Copyright (c) John Gough 1999, 2000.                                *)
(*                                                                      *)
(* ==================================================================== *)
(* Empty Assign Return Block ProcCall ForLoop Choice ExitSt TestLoop CaseSt *)
(* ==================================================================== *)

MODULE StatDesc;

  IMPORT
        GPCPcopyright,
        GPText,
        Console,
        FileNames,
        LitValue,
        B := Builtin,
        V := VarSets,
        S := CPascalS,
        D := Symbols ,
        I := IdDesc  ,
        T := TypeDesc,
        E := ExprDesc,
        G := CompState,
        H := DiagHelper;

(* ============================================================ *)

  CONST (* stmt-kinds *)
    emptyS* = 0;  assignS* = 1; procCall* = 2;  ifStat*  = 3;
    caseS*  = 4;  whileS*  = 5; repeatS*  = 6;  forStat* = 7;
    loopS*  = 8;  withS*   = 9; exitS*    = 10; returnS* = 11;
    blockS* = 12;

(* ============================================================ *)

  CONST (* case statement density *)
    DENSITY = 0.7;


(* ============================================================ *)

  TYPE
    Empty*  = POINTER TO RECORD (D.Stmt)
             (* ----------------------------------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* stmt first tok *)
              * ----------------------------------------- *)
              END;

(* ============================================================ *)

  TYPE
    Return* = POINTER TO RECORD (D.Stmt)
             (* ----------------------------------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* stmt first tok *)
              * ----------------------------------------- *)
                retX*  : D.Expr;  (* NIL ==> void   *)
                prId*  : D.Scope; (* Parent Ident   *)
              END;

(* ============================================================ *)

  TYPE
    Block* = POINTER TO RECORD (D.Stmt)
             (* ----------------------------------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* stmt first tok *)
              * ----------------------------------------- *)
                sequ*  : D.StmtSeq;
              END;

(* ============================================================ *)

  TYPE
    Assign* = POINTER TO RECORD (D.Stmt)
             (* ----------------------------------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* stmt first tok *)
              * ----------------------------------------- *)
                lhsX*  : D.Expr;
                rhsX*  : D.Expr;
              END;

(* ============================================================ *)

  TYPE
    ProcCall* = POINTER TO RECORD (D.Stmt)
               (* ----------------------------------------- *
                * kind-  : INTEGER; (* tag for unions *)
                * token* : S.Token; (* stmt first tok *)
                * ----------------------------------------- *)
                  expr*  : D.Expr;
                END;

(* ============================================================ *)

  TYPE
    ForLoop* = POINTER TO RECORD (D.Stmt)
              (* ----------------------------------------- *
               * kind-  : INTEGER; (* tag for unions *)
               * token* : S.Token; (* stmt first tok *)
               * ----------------------------------------- *)
                 cVar*  : D.Idnt;  (* c'trl variable *)
                 loXp*  : D.Expr;  (* low limit expr *)
                 hiXp*  : D.Expr;  (* high limit exp *)
                 byXp*  : D.Expr;  (* must be numLt  *)
                 body*  : D.Stmt;  (* possibly block *)
               END;

(* ============================================================ *)

  TYPE
    Choice* = POINTER TO RECORD (D.Stmt)
             (* ----------------------------------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* stmt first tok *)
              * ----------------------------------------- *
              *  This descriptor is used for IF and WITH. *
              *  In the case of IF each predicate in the  *
              *  sequence has a boolean type, and the     *
              *  "predicate" corresponding to else is NIL *
              *  For the WITH statement, each predicate   *
              *  syntactically denoted as <id> ":" <tp>   *
              *  is represented by an IS binary nodetype. *
              * ----------------------------------------- *)
                preds*  : D.ExprSeq;  (* else test NIL  *)
                blocks* : D.StmtSeq;  (* stmt choices   *)
                temps*  : D.IdSeq;    (* with tempvars  *)
              END;

(* ============================================================ *)

  TYPE
    ExitSt* = POINTER TO RECORD (D.Stmt)
             (* ----------------------------------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* stmt first tok *)
              * ----------------------------------------- *)
                loop*  : TestLoop;  (* enclosing loop *)
              END;

(* ============================================================ *)

  TYPE
    TestLoop* = POINTER TO RECORD (D.Stmt)
               (* ----------------------------------------- *
                * kind-  : INTEGER; (* tag for unions *)
                * token* : S.Token; (* stmt first tok *)
                * ----------------------------------------- *
                *  This descriptor is used for WHILE and    *
                *  REPEAT loops.  These are distinguished   *
                *  by the different tag values in v.kind    *
                *  LOOPs use the structure with a NIL test  *
                * ----------------------------------------- *)
                  test*  : D.Expr;  (* the loop test  *)
                  body*  : D.Stmt;  (* possibly block *)
                  label* : INTEGER; (* readonly field *)
                  tgLbl* : ANYPTR;
                  merge  : V.VarSet;
                END;

(* ============================================================ *)

  TYPE
    Triple* = POINTER TO RECORD
                loC- : INTEGER;   (* low of range   *)
                hiC- : INTEGER;   (* high of range  *)
                ord- : INTEGER;   (* case block ord *)
              END;

  (* ---------------------------------- *)

  TYPE
    TripleSeq* = RECORD
                   tide- : INTEGER;
                   high : INTEGER;
                   a- : POINTER TO ARRAY OF Triple;
                 END;

  (* ---------------------------------- *)

  TYPE
    CaseSt* = POINTER TO RECORD (D.Stmt)
             (* ----------------------------------------- *
              * kind-  : INTEGER; (* tag for unions *)
              * token* : S.Token; (* stmt first tok *)
              * ----------------------------------------- *)
                select* : D.Expr; (* case selector  *)
                chrSel* : BOOLEAN;  (* ==> use chars  *)
                blocks* : D.StmtSeq;  (* case bodies    *)
                elsBlk* : D.Stmt; (* elseCase | NIL *)
                labels* : TripleSeq;  (* label seqence  *)
                groups- : TripleSeq;  (* dense groups   *)
              END;
  (* ---------------------------------------------------------- *
   *  Notes on the semantics of this structure. "blocks" holds  *
   *  an ordered list of case statement code blocks. "labels" *
   *  is a list of ranges, intially in textual order, with flds *
   *  loC, hiC and ord corresponding to the range min, max and  *
   *  the selected block ordinal number.  This list is later  *
   *  sorted on the loC value, and adjacent values merged if  *
   *  they select the same block.  The "groups" list of triples *
   *  groups ranges into dense subranges in the selector space. *
   *  The fields loC, hiC, and ord to hold the lower and upper  *
   *  indices into the labels list, and the number of non-  *
   *  default values in the group. Groups are guaranteed to *
   *  have density (nonDefN / (max-min+1)) > DENSITY    *
   * ---------------------------------------------------------- *)

(* ============================================================ *)

  PROCEDURE newTriple*(lo,hi,ord : INTEGER) : Triple;
    VAR new : Triple;
  BEGIN
    NEW(new); new.loC := lo; new.hiC := hi; new.ord := ord; RETURN new;
  END newTriple;

  (* ---------------------------------- *)

  PROCEDURE InitTripleSeq*(VAR seq : TripleSeq; capacity : INTEGER);
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitTripleSeq;

  (* ---------------------------------- *)

  PROCEDURE (VAR seq : TripleSeq)ResetTo(newTide : INTEGER),NEW;
  BEGIN
    ASSERT(newTide <= seq.tide);
    seq.tide := newTide;
  END ResetTo;

  (* ---------------------------------- *)

  PROCEDURE AppendTriple*(VAR seq : TripleSeq; elem : Triple);
    VAR temp : POINTER TO ARRAY OF Triple;
        i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN
      InitTripleSeq(seq, 8);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, seq.high+1);
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendTriple;

  (* ---------------------------------- *)

(*
 *PROCEDURE (VAR seq : TripleSeq)Diagnose(IN str : ARRAY OF CHAR),NEW;
 *  VAR index : INTEGER;
 *BEGIN
 *  Console.WriteString("Diagnose TripleSeq " + str); Console.WriteLn;
 *  FOR index := 0 TO seq.tide-1 DO
 *    Console.WriteInt(index, 3);
 *    Console.WriteInt(seq.a[index].loC, 8);
 *    Console.WriteInt(seq.a[index].hiC, 8);
 *    Console.WriteInt(seq.a[index].ord, 8);
 *    Console.WriteLn;
 *  END;
 *END Diagnose;
 *)

(* ============================================================ *)
(*            Various Statement Text-Span Constructors          *)
(* ============================================================ *)

  PROCEDURE (s : Empty)Span*() : S.Span;
  BEGIN
    RETURN NIL;
  END Span;
  
  PROCEDURE (s : Return)Span*() : S.Span;
    VAR rslt : S.Span;
  BEGIN
    rslt := S.mkSpanT(s.token);
    IF s.retX # NIL THEN rslt := S.Merge(rslt, s.retX.tSpan) END;
    RETURN rslt; 
  END Span;
  
  PROCEDURE (s : Block)Span*() : S.Span;
  BEGIN
    RETURN NIL;
  END Span;
  
  PROCEDURE (s : Assign)Span*() : S.Span;
  BEGIN
    RETURN S.Merge(s.lhsX.tSpan, s.rhsX.tSpan);
  END Span;
  
  PROCEDURE (s : ProcCall)Span*() : S.Span;
  BEGIN
    RETURN s.expr.tSpan;
  END Span;
  
  (*PROCEDURE (s : ProcCall)Span*() : S.Span;
  BEGIN
    RETURN s.expr.tSpan;
  END Span;*)
  
  
(* ============================================================ *)
(*      Various Statement Descriptor Constructors   *)
(* ============================================================ *)

  PROCEDURE newEmptyS*() : Empty;
    VAR new : Empty;
  BEGIN
    NEW(new); new.SetKind(emptyS);
    new.token := S.prevTok; RETURN new;
  END newEmptyS;

  (* ---------------------------------- *)

  PROCEDURE newBlockS*(t : S.Token) : Block;
    VAR new : Block;
  BEGIN
    NEW(new); new.SetKind(blockS);
    new.token := t; RETURN new;
  END newBlockS;

  (* ---------------------------------- *)

  PROCEDURE newReturnS*(retX : D.Expr) : Return;
    VAR new : Return;
  BEGIN
    NEW(new); new.token := S.prevTok;
    new.retX := retX; new.SetKind(returnS); RETURN new;
  END newReturnS;

  (* ---------------------------------- *)

  PROCEDURE newAssignS*() : Assign;
    VAR new : Assign;
  BEGIN
    NEW(new); new.SetKind(assignS);
    new.token := S.prevTok; RETURN new;
  END newAssignS;

  (* ---------------------------------- *)

  PROCEDURE newWhileS*() : TestLoop;
    VAR new : TestLoop;
  BEGIN
    NEW(new); new.SetKind(whileS);
    new.token := S.prevTok; RETURN new;
  END newWhileS;

  (* ---------------------------------- *)

  PROCEDURE newRepeatS*() : TestLoop;
    VAR new : TestLoop;
  BEGIN
    NEW(new); new.SetKind(repeatS);
    new.token := S.prevTok; RETURN new;
  END newRepeatS;

  (* ---------------------------------- *)

  PROCEDURE newIfStat*() : Choice;
    VAR new : Choice;
  BEGIN
    NEW(new); new.SetKind(ifStat);
    new.token := S.prevTok; RETURN new;
  END newIfStat;

  (* ---------------------------------- *)

  PROCEDURE newWithS*() : Choice;
    VAR new : Choice;
  BEGIN
    NEW(new); new.SetKind(withS);
    new.token := S.prevTok; RETURN new;
  END newWithS;

  (* ---------------------------------- *)

  PROCEDURE newForStat*() : ForLoop;
    VAR new : ForLoop;
  BEGIN
    NEW(new); new.SetKind(forStat);
    new.token := S.prevTok; RETURN new;
  END newForStat;

  (* ---------------------------------- *)

  PROCEDURE newProcCall*() : ProcCall;
    VAR new : ProcCall;
  BEGIN
    NEW(new); new.token := S.prevTok;
    new.SetKind(procCall); RETURN new;
  END newProcCall;

  (* ---------------------------------- *)

  PROCEDURE newExitS*(loop : D.Stmt) : ExitSt;
    VAR new : ExitSt;
  BEGIN
    NEW(new); new.token := S.prevTok;
    new.loop  := loop(TestLoop); new.SetKind(exitS); RETURN new;
  END newExitS;

  (* ---------------------------------- *)

  PROCEDURE newLoopS*() : TestLoop;
    VAR new : TestLoop;
  BEGIN
    NEW(new); new.SetKind(loopS);
    new.token := S.prevTok; RETURN new;
  END newLoopS;

  (* ---------------------------------- *)

  PROCEDURE newCaseS*() : CaseSt;
    VAR new : CaseSt;
  BEGIN
    NEW(new); new.SetKind(caseS);
    new.token := S.prevTok; RETURN new;
  END newCaseS;

(* ============================================================ *)

  PROCEDURE (for : ForLoop)isSimple*() : BOOLEAN,NEW;
  (* A for loop is simple if it always executes at least once. *)
    VAR loVal : LONGINT;
        hiVal : LONGINT;
        byVal : LONGINT;
  BEGIN
    IF  (for.loXp.kind = E.numLt) &
        (for.hiXp.kind = E.numLt) THEN
      loVal := for.loXp(E.LeafX).value.long();
      hiVal := for.hiXp(E.LeafX).value.long();
      byVal := for.byXp(E.LeafX).value.long();
      IF byVal > 0 THEN
        RETURN hiVal >= loVal;
      ELSE
        RETURN hiVal <= loVal;
      END;
    ELSE
      RETURN FALSE;
    END;
  END isSimple;

(* ============================================================ *)
(*          Type Erasure                                        *)
(* ============================================================ *)
  PROCEDURE (s : Empty)TypeErase*(t : D.Scope); BEGIN END TypeErase;

  PROCEDURE (s : Block)TypeErase*(t : D.Scope);
      VAR index : INTEGER;
    BEGIN
      FOR index := 0 TO s.sequ.tide - 1 DO
        s.sequ.a[index].TypeErase(t);
      END;
  END TypeErase;

  PROCEDURE (s : Assign)TypeErase*(t : D.Scope);
  BEGIN
    s.rhsX := s.rhsX.TypeErase();
  END TypeErase;

  PROCEDURE (s : Return)TypeErase*(t : D.Scope); BEGIN END TypeErase;
  PROCEDURE (s : ProcCall)TypeErase*(t : D.Scope); BEGIN END TypeErase;
  PROCEDURE (s : ForLoop)TypeErase*(t : D.Scope); BEGIN END TypeErase;
  PROCEDURE (s : Choice)TypeErase*(t : D.Scope); BEGIN END TypeErase;
  PROCEDURE (s : ExitSt)TypeErase*(t : D.Scope); BEGIN END TypeErase;
  PROCEDURE (s : TestLoop)TypeErase*(t : D.Scope); BEGIN END TypeErase;
  PROCEDURE (s : CaseSt)TypeErase*(t : D.Scope); BEGIN END TypeErase;

(* ============================================================ *)
(*          Statement Attribution     *)
(* ============================================================ *)

  PROCEDURE (s : Empty)StmtAttr*(scope : D.Scope);
  BEGIN END StmtAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : Block)StmtAttr*(scope : D.Scope);
    VAR index : INTEGER;
  BEGIN
    FOR index := 0 TO s.sequ.tide - 1 DO
      s.sequ.a[index].StmtAttr(scope);
    END;
  END StmtAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : Assign)StmtAttr*(scope : D.Scope);
    VAR lTp, rTp : D.Type;
        eNm : INTEGER;
  BEGIN
   (*
    * Assert: lhsX is a designator, it has been
    * attributed during parsing, and has a non-null type.
    *
    * First: attribute the right-hand-side expression.
    *)
    s.rhsX := s.rhsX.exprAttr();
   (*
    * First check: is the designator writeable.
    *)
    s.lhsX.CheckWriteable();

    IF (s.rhsX # NIL) & (s.rhsX.type # NIL) THEN
      lTp := s.lhsX.type;
      rTp := s.rhsX.type;
     (*
      * Second check: does the expression need dereferencing.
      *)
      IF (lTp.kind = T.recTp) & (rTp.kind = T.ptrTp) THEN
        s.rhsX := E.mkDeref(s.rhsX);
        rTp := s.rhsX.type;
      END;
      IF lTp.assignCompat(s.rhsX) THEN
       (*
        * Third check: does the expression need type coercion.
        *)
        IF (rTp # lTp) & (rTp IS T.Base) THEN
          s.rhsX := E.coerceUp(s.rhsX, lTp);
          rTp := lTp;
        END;
       (*
        * Fourth check: are value copies allowed here.
        *)
        IF ~rTp.valCopyOK() THEN s.rhsX.ExprError(152) END;
        IF rTp IS T.Procedure THEN
          s.StmtError(301);
          IF G.targetIsJVM() THEN s.StmtError(213);
          ELSIF (rTp # lTp) & ~s.rhsX.isProcLit() THEN s.StmtError(191);
          END;
        END;
      ELSE (* sort out which error to report *)
        IF    rTp.isOpenArrType() THEN eNm := 142;
        ELSIF rTp.isExtnRecType() THEN eNm := 143;
        ELSIF (rTp.kind = T.prcTp) &
              (s.rhsX.kind = E.qualId) &
              ~s.rhsX.isProcVar() THEN eNm := 165;
        ELSIF lTp.isCharArrayType() &
              rTp.isStringType() THEN  eNm :=  27;
        ELSE             eNm :=  83;
        END;
        IF eNm # 83 THEN s.rhsX.ExprError(eNm);
        ELSE D.RepTypesErrTok(83, lTp, rTp, s.token);
        END;
      END;
    END;
  END StmtAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : Return)StmtAttr*(scope : D.Scope);
    VAR prId : IdDesc.Procs;
        rTyp : D.Type;
        xTyp : D.Type;
        rExp : D.Expr;
  BEGIN
    IF scope.kind = IdDesc.modId THEN
      s.StmtError(73);
    ELSE
      prId := scope(IdDesc.Procs);
      s.prId := prId;
      rTyp := prId.type(TypeDesc.Procedure).retType;
      IF rTyp = NIL THEN
        IF s.retX # NIL THEN s.retX.ExprError(74) END;
      ELSE
        IF s.retX = NIL THEN
          s.StmtError(75);
        ELSE
          rExp := s.retX.exprAttr();
          s.retX := rExp;
          xTyp := rExp.type;
          IF rExp # NIL THEN (* fixed 28 July 2001 *)
            IF ~rTyp.assignCompat(rExp) THEN
              D.RepTypesErrTok(76, rTyp, xTyp, s.token);
            ELSIF rTyp # xTyp THEN
              IF xTyp IS T.Base THEN
                rExp := E.coerceUp(rExp, rTyp);
                s.retX := rExp;
              ELSIF rTyp IS T.Procedure THEN
                rExp.type := rTyp;
              END;
            END;
            IF scope.kind = IdDesc.ctorP THEN
              WITH rExp : E.IdLeaf DO
                IF rExp.ident.hash # B.selfBk THEN rExp.ExprError(225) END;
              ELSE rExp.ExprError(225);
              END;
            END;
          END;
        END;
      END;
    END;
  END StmtAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : ProcCall)StmtAttr*(scope : D.Scope);
    VAR callX : E.CallX;
        tempX : D.Expr;
        idntX : E.IdentX;
  BEGIN
    callX  := s.expr(E.CallX);
    s.expr := E.checkCall(callX);
    IF (s.expr # NIL) &
       (callX.kid.kind = E.sprMrk) THEN E.CheckSuper(callX, scope) END;
  END StmtAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : ForLoop)StmtAttr*(scope : D.Scope);
  BEGIN
    s.loXp := s.loXp.exprAttr();
    s.hiXp := s.hiXp.exprAttr();
    IF (s.loXp # NIL) & ~s.loXp.isIntExpr() THEN s.loXp.ExprError(37) END;
    IF (s.hiXp # NIL) & ~s.hiXp.isIntExpr() THEN s.hiXp.ExprError(37) END;
    s.body.StmtAttr(scope);
  END StmtAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : Choice)StmtAttr*(scope : D.Scope);
    VAR index : INTEGER;
        predN : D.Expr;
        nextN : D.Expr;
        blokN : D.Stmt;
  BEGIN
    FOR index := 0 TO s.preds.tide - 1 DO
      predN := s.preds.a[index];
      blokN := s.blocks.a[index];
      IF predN # NIL THEN
        nextN := predN.exprAttr();
        IF nextN # NIL THEN
          IF nextN # predN THEN s.preds.a[index] := nextN END;
          IF ~nextN.isBooleanExpr() THEN predN.ExprError(36) END;
        END;
      END;
      IF blokN # NIL THEN blokN.StmtAttr(scope) END;
    END;
  END StmtAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : ExitSt)StmtAttr*(scope : D.Scope);
  BEGIN END StmtAttr; (* nothing to do *)

  (* ---------------------------------- *)

  PROCEDURE (s : TestLoop)StmtAttr*(scope : D.Scope);
  BEGIN
    IF s.test # NIL THEN s.test := s.test.exprAttr() END;
    IF (s.test # NIL) & ~s.test.isBooleanExpr() THEN s.test.ExprError(36) END;
    s.body.StmtAttr(scope);
  END StmtAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : CaseSt)StmtAttr*(scope : D.Scope);
  (* At this point the select expression has already been attributed  *)
  (* during parsing, and the raw case ordinals have been checked. *)
    VAR index : INTEGER;

  (* ------------------------- *)

    PROCEDURE QuickSort(VAR array : TripleSeq; min, max : INTEGER);
      VAR i,j : INTEGER;
          key : INTEGER;
          tmp : Triple;
    BEGIN
      i := min; j := max;
      key := array.a[(min+max) DIV 2].loC;
      REPEAT
        WHILE array.a[i].loC < key DO INC(i) END;
        WHILE array.a[j].loC > key DO DEC(j) END;
        IF i <= j THEN
          tmp := array.a[i]; array.a[i] := array.a[j]; array.a[j] := tmp;
          INC(i); DEC(j);
        END;
      UNTIL i > j;
      IF min < j THEN QuickSort(array, min,j) END;
      IF i < max THEN QuickSort(array, i,max) END;
    END QuickSort;

  (* ------------------------- *)

    PROCEDURE DoErr89(cs : CaseSt; ix,mx : INTEGER);
      VAR n1, n2 : ARRAY 32 OF CHAR;
          lo, hi : INTEGER;
          o1, o2 : INTEGER;
          tr     : Triple;
          s1,s2  : D.Stmt;
    BEGIN
      tr := cs.labels.a[ix];
      lo := tr.loC; hi := tr.hiC; o1 := tr.ord;
      (* overlap is from "lo" to MIN(mx, hi) ... *)
      hi := MIN(hi, mx);
      GPText.IntToStr(lo, n1);
      IF lo # hi THEN (* range overlap *)
        GPText.IntToStr(hi, n2);
        n1 := n1 + " .. " + n2;
      END;
      o2 := cs.labels.a[ix-1].ord;
     (*
      *  We want to place a full diagnostic on the earlier
      *  of the two cases, if there are two. Place a simple
      *  diagnostic on the second of the two cases.
      *)
      s1 := cs.blocks.a[o1];
      s2 := cs.blocks.a[o2];
      IF o1 < o2 THEN
        S.SemError.RepSt1(89, n1, s1.token.lin, s1.token.col);
        S.SemError.Report(89, s2.token.lin, s2.token.col);
      ELSIF o1 > o2 THEN
        S.SemError.RepSt1(89, n1, s2.token.lin, s2.token.col);
        S.SemError.Report(89, s1.token.lin, s1.token.col);
      ELSE  (* list once only *)
        S.SemError.RepSt1(89, n1, s1.token.lin, s1.token.col);
      END;
    END DoErr89;

  (* ------------------------- *)

    PROCEDURE Compact(cs : CaseSt);
      VAR index : INTEGER;    (* read index on sequence *)
          write : INTEGER;    (* write index on new seq *)
          nextI : INTEGER;    (* adjacent selector val  *)
          crOrd : INTEGER;    (* current case ordinal   *)
          thisT : Triple;
    BEGIN
      write := -1;
      nextI := MIN(INTEGER);
      crOrd := MIN(INTEGER);
      FOR index := 0 TO cs.labels.tide - 1 DO
        thisT := cs.labels.a[index];
        (* test for overlaps ! *)
        IF thisT.loC < nextI THEN DoErr89(cs, index, nextI-1) END;
        IF (thisT.loC = nextI) & (thisT.ord = crOrd) THEN (* merge *)
          cs.labels.a[write].hiC := thisT.hiC;
        ELSE
          INC(write);
          crOrd := thisT.ord;
          cs.labels.a[write].loC := thisT.loC;
          cs.labels.a[write].hiC := thisT.hiC;
          cs.labels.a[write].ord := thisT.ord;
        END;
        nextI := thisT.hiC + 1;
      END;
      cs.labels.ResetTo(write+1);
    END Compact;

  (* ------------------------- *)

    PROCEDURE FindGroups(cs : CaseSt);
      VAR index : INTEGER;    (* read index on sequence *)
          sm,sM : INTEGER;    (* group min/Max selector *)
          nextN : INTEGER;    (* updated group ordNum.  *)
          dense : BOOLEAN;
          crGrp : Triple;   (* current group triple   *)
          crRng : Triple;   (* triple to cond. add on *)
          p1Grp : TripleSeq;    (* temporary sequence.    *)
    BEGIN
   (* IF G.verbose THEN cs.labels.Diagnose("selector labels") END; *)
     (*
      *  Perform the backward pass, merging dense groups.
      *  Indices are between cs.labels.tide-1 and 0.
      *)
      index := cs.labels.tide-1; dense := FALSE; crGrp := NIL;
      WHILE (index >= 0) & ~dense  DO
       (* Invariant: all ranges with index > "index" have been  *
        * grouped and appended to the first pass list p1Grp.  *)
        dense := TRUE;
        crRng := cs.labels.a[index];
        sM := crRng.hiC;
        crGrp := newTriple(index, index, sM - crRng.loC + 1);
        WHILE (index > 0) & dense DO
         (* Invariant: crGrp groups info on all ranges with   *
          * index >= "index" not already appended to tempGP *)
          DEC(index);
          crRng := cs.labels.a[index];
          nextN := crGrp.ord + crRng.hiC -crRng.loC + 1;
          IF nextN / (sM - crRng.loC + 1) > DENSITY THEN
            crGrp.loC := index; crGrp.ord := nextN; (* add to crGrp *)
          ELSE
            AppendTriple(p1Grp, crGrp); dense := FALSE; (* append; exit *)
          END;
        END;
      END;
      IF dense THEN AppendTriple(p1Grp, crGrp) END;
   (* IF G.verbose THEN p1Grp.Diagnose("first pass groups") END; *)
     (*
      *  Perform the forward pass, merging dense groups.
      *  Indices are between 0 and p1Grp.tide-1.
      *  Note the implicit list reversal here.
      *)
      index := p1Grp.tide-1; dense := FALSE;
      WHILE (index >= 0) & ~dense DO
       (* Invariant: all groups with index > "index" have been  *
        * grouped and appended to the final list cs.groups. *)
        dense := TRUE;
        crGrp := p1Grp.a[index];
        sm := cs.labels.a[crGrp.loC].loC;
        WHILE (index > 0) & dense DO
         (* Invariant: crGrp contains info on all groups with   *
          * index >= "index" not already appended to tempGP *)
          DEC(index);
          crRng := p1Grp.a[index];
          sM := cs.labels.a[crRng.hiC].hiC;
          nextN := crGrp.ord + crRng.ord;
          IF nextN / (sM - sm + 1) > DENSITY THEN
            crGrp.hiC := crRng.hiC; crGrp.ord := nextN; (* add to crGrp *)
          ELSE
            AppendTriple(cs.groups, crGrp);     (* append; exit *)
            dense := FALSE;
          END;
        END;
      END;
      IF dense THEN AppendTriple(cs.groups, crGrp) END;
   (* IF G.verbose THEN cs.groups.Diagnose("final groups") END; *)
    END FindGroups;

  (* ------------------------- *)

  BEGIN
    IF s.blocks.tide = 0 THEN RETURN END; (* Empty case statement *)
   (*
    *  First: do all controlled statement attribution.
    *)
    FOR index := 0 TO s.blocks.tide - 1 DO
      s.blocks.a[index].StmtAttr(scope);
    END;
    IF s.elsBlk # NIL THEN s.elsBlk.StmtAttr(scope) END;
   (*
    *  Next: sort all triples on the loC value.
    *)
 (* IF G.verbose THEN s.labels.Diagnose("unsorted labels") END; *)
    QuickSort(s.labels, 0, s.labels.tide - 1);
 (* IF G.verbose THEN s.labels.Diagnose("sorted labels") END; *)
   (*
    *  Next: compact adjacent cases with same block-ord.
    *)
    Compact(s);
   (*
    *  Next: create lists of dense subranges.
    *)
    FindGroups(s);
  END StmtAttr;

(* ============================================================ *)
(*  Flow attribute evaluation for all statement types *)
(* ============================================================ *)

  PROCEDURE (s : Block)flowAttr*(t : D.Scope; i : V.VarSet) : V.VarSet;
    VAR ix : INTEGER;
  BEGIN
    FOR ix := 0 TO s.sequ.tide-1 DO
      i := s.sequ.a[ix].flowAttr(t, i);
    END;
    RETURN i;
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : Assign)flowAttr*(t : D.Scope; lvIn : V.VarSet) : V.VarSet;
   (* Invariant: param lvIn is unchanged by this procedure *)
    VAR lhLv, rhLv : V.VarSet;
  BEGIN
    rhLv := s.rhsX.checkLive(t, lvIn);
    lhLv := s.lhsX.assignLive(t, lvIn); (* specialized for Assign | others *)
    RETURN lhLv.cup(rhLv);
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : Empty)flowAttr*(t : D.Scope; i : V.VarSet) : V.VarSet;
  BEGIN
    RETURN i;
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : Return)flowAttr*(t : D.Scope; live : V.VarSet) : V.VarSet;
  BEGIN
    IF s.retX # NIL THEN live := s.retX.checkLive(t, live) END;
    t.type.OutCheck(live);
    RETURN V.newUniv(live.cardinality());
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : ProcCall)flowAttr*(t : D.Scope; i : V.VarSet) : V.VarSet;
  BEGIN
    RETURN s.expr.checkLive(t, i);
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : ForLoop)flowAttr*(t : D.Scope; live : V.VarSet) : V.VarSet;
    VAR junk : V.VarSet;
  BEGIN
   (*
    *   The limits are evaluated in a prescribed order,
    *   chaining the live set. The body may or may not
    *   be evaluated.  [We might later test this for static
    *   evaluation, but might need to emit different code
    *   for the two cases, to keep the verifier happy.]
    *   [This is now done, 30-Mar-2000, (kjg)]
    *)
    live := s.loXp.checkLive(t, live);
    live := s.hiXp.checkLive(t, live);
    live := live.newCopy();
    live.Incl(s.cVar(I.AbVar).varOrd);
    junk := s.body.flowAttr(t,live);
    IF s.isSimple() THEN
     (*
      *   If this for loop is simple, it will be executed
      *   at least once.  Thus the flow-attribution consequences
      *   of execution will be included in live-out var-set.
      *)
      live := live.cup(junk);
    END;
    RETURN live;
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : Choice)flowAttr*(t : D.Scope; live : V.VarSet) : V.VarSet;
    VAR idx : INTEGER;
        out : V.VarSet;
        tru : V.VarSet;
        fal : V.VarSet;
        pred : D.Expr;
        else : BOOLEAN;
  BEGIN
    out := V.newUniv(live.cardinality());
    tru := live;
    fal := live;
    IF s.kind = ifStat THEN
     (*
      *  In the case of IF statements there is always the possiblity
      *  that a predicate evaluation will have a side-effect. Thus ...
      *)
      else := FALSE;
      FOR idx := 0 TO s.preds.tide-1 DO
        pred := s.preds.a[idx];
        IF pred # NIL THEN
          pred.BoolLive(t, fal, tru, fal);
          out  := out.cap(s.blocks.a[idx].flowAttr(t, tru));
        ELSE (* must be elsepart *)
          else := TRUE;
          out  := out.cap(s.blocks.a[idx].flowAttr(t, fal));
        END;
      END;
     (*
      *   If we did not find an elsepart, then we must
      *   merge the result of executing the implicit "skip".
      *)
      IF ~else THEN out  := out.cap(fal) END;
    ELSE
     (*
      *  In the case of WITH statements there is no evaluation
      *  involved in the predicate test, and hence no side-effect.
      *)
      FOR idx := 0 TO s.preds.tide-1 DO
        pred := s.preds.a[idx];
        IF pred # NIL THEN
          tru := pred(E.BinaryX).lKid.checkLive(t, live);
        END;
        out := out.cap(s.blocks.a[idx].flowAttr(t, tru));
      END;
    END;
    RETURN out;
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : ExitSt)flowAttr*(t : D.Scope; live : V.VarSet) : V.VarSet;
  (* Merge all exit sets into the "merge" set of the enclosing  *)
  (* LOOP.  Return the input live set, unchanged.   *)
  BEGIN
    s.loop.merge := live.cap(s.loop.merge);
    RETURN V.newUniv(live.cardinality());
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : TestLoop)flowAttr*(t : D.Scope; live : V.VarSet) : V.VarSet;
    VAR tSet, fSet, junk : V.VarSet;
  BEGIN
    IF s.kind = whileS THEN
     (*
      *  For a WHILE statement, the expression is evaluated first.
      *)
      s.test.BoolLive(t, live, tSet, fSet);
      junk := s.body.flowAttr(t, tSet);
      RETURN fSet;
    ELSIF s.kind = repeatS THEN
     (*
      *  For a REPEAT statement, the expression is evaluated last.
      *)
      junk := s.body.flowAttr(t, live);
      s.test.BoolLive(t, junk, tSet, fSet);
      RETURN fSet;
    ELSE (* must be loopS *)
      s.merge := V.newUniv(live.cardinality());
      junk := s.body.flowAttr(t, live);
      RETURN s.merge;
    END;
    RETURN live;
  END flowAttr;

  (* ---------------------------------- *)

  PROCEDURE (s : CaseSt)flowAttr*(t : D.Scope; live : V.VarSet) : V.VarSet;
    VAR lvOu : V.VarSet;
        indx : INTEGER;
        tmp : V.VarSet;
  BEGIN
    lvOu := V.newUniv(live.cardinality());
    live := s.select.checkLive(t, live);
   (*
    *  The live-out set of this statement is the intersection
    *  of the live-out of all of the components of the CASE.
    *  All cases receive the same input set: the result of the
    *  evaluation of the select expression.
    *)
    FOR indx := 0 TO s.blocks.tide-1 DO
      lvOu := lvOu.cap(s.blocks.a[indx].flowAttr(t, live));
    END;
   (*
    *  In the event that there is no ELSE case, and unlike the
    *  case of the IF statement, the program aborts and does
    *  not effect the accumulated live-out set, lvOu.
    *)
    IF s.elsBlk # NIL THEN
      lvOu := lvOu.cap(s.elsBlk.flowAttr(t, live));
    END;
    RETURN lvOu;
  END flowAttr;

(* ============================================================ *)
(*          Diagnostic Procedures     *)
(* ============================================================ *)

  PROCEDURE WriteTag(t : D.Stmt; ind : INTEGER);
  BEGIN
    H.Indent(ind);
    CASE t.kind OF
    | emptyS   : Console.WriteString("emptyS   ");
    | assignS  : Console.WriteString("assignS  ");
    | procCall : Console.WriteString("procCall ");
    | ifStat   : Console.WriteString("ifStat   ");
    | caseS    : Console.WriteString("caseS    ");
    | whileS   : Console.WriteString("whileS   ");
    | repeatS  : Console.WriteString("repeatS  ");
    | forStat  : Console.WriteString("forStat  ");
    | loopS    : Console.WriteString("loopS    ");
    | withS    : Console.WriteString("withS    ");
    | exitS    : Console.WriteString("exitS    ");
    | returnS  : Console.WriteString("returnS  ");
    | blockS   : Console.WriteString("blockS   ");
    ELSE
      Console.WriteString("unknown stmt, tag="); Console.WriteInt(t.kind,1);
    END;
    IF t.token # NIL THEN
      Console.WriteString("(lin:col ");
      Console.WriteInt(t.token.lin, 1); Console.Write(":");
      Console.WriteInt(t.token.col, 1); Console.Write(")");
    END;
  END WriteTag;

  (* ---------------------------------- *)

  PROCEDURE (t : Empty)Diagnose*(i : INTEGER);
  BEGIN
    WriteTag(t, i); Console.WriteLn;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : Return)Diagnose*(i : INTEGER);
  BEGIN
    WriteTag(t, i); Console.WriteLn;
    IF t.retX # NIL THEN t.retX.Diagnose(i+4) END;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : Block)Diagnose*(i : INTEGER);
    VAR index : INTEGER;
  BEGIN
    WriteTag(t, i);
    Console.WriteString(" {"); Console.WriteLn;
    FOR index := 0 TO t.sequ.tide - 1 DO
      t.sequ.a[index].Diagnose(i+4);
    END;
    H.Indent(i); Console.Write("}"); Console.WriteLn;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : Assign)Diagnose*(i : INTEGER);
  BEGIN
    WriteTag(t, i); Console.WriteLn;
    IF t.lhsX # NIL THEN t.lhsX.Diagnose(i+4) END;
    IF t.rhsX # NIL THEN t.rhsX.Diagnose(i+4) END;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : ProcCall)Diagnose*(i : INTEGER);
  BEGIN
    WriteTag(t, i); Console.WriteLn;
    IF t.expr # NIL THEN t.expr.Diagnose(i+4) END;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : ForLoop)Diagnose*(i : INTEGER);
  BEGIN
    WriteTag(t, i);
    IF t.cVar # NIL THEN t.cVar.WriteName END;
    Console.WriteLn;
    IF t.loXp # NIL THEN t.loXp.Diagnose(i+2) END;
    IF t.hiXp # NIL THEN t.hiXp.Diagnose(i+2) END;
    H.Indent(i); Console.Write("{"); Console.WriteLn;
    t.body.Diagnose(i+4);
    H.Indent(i); Console.Write("}"); Console.WriteLn;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : Choice)Diagnose*(i : INTEGER);
    CONST nil = "<nil>";
    VAR index : INTEGER;
        stmt  : D.Stmt;
        expr  : D.Expr;
  BEGIN
    WriteTag(t, i); Console.Write("{"); Console.WriteLn;
    FOR index := 0 TO t.preds.tide - 1 DO
      expr := t.preds.a[index];
      stmt := t.blocks.a[index];
      IF expr = NIL THEN
        H.Indent(i); Console.WriteString(nil); Console.WriteLn;
      ELSE
        expr.Diagnose(i);
      END;
      IF stmt = NIL THEN
        H.Indent(i+4); Console.WriteString(nil); Console.WriteLn;
      ELSE
        stmt.Diagnose(i+4);
      END;
    END;
    H.Indent(i); Console.Write("}"); Console.WriteLn;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : ExitSt)Diagnose*(i : INTEGER);
  BEGIN
    WriteTag(t, i); Console.WriteLn;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : TestLoop)Diagnose*(i : INTEGER);
  BEGIN
    WriteTag(t, i); Console.WriteLn;
    IF t.test # NIL THEN t.test.Diagnose(i) END;
    H.Indent(i); Console.Write("{"); Console.WriteLn;
    t.body.Diagnose(i+4);
    H.Indent(i); Console.Write("}"); Console.WriteLn;
  END Diagnose;

  (* ---------------------------------- *)

  PROCEDURE (t : CaseSt)Diagnose*(i : INTEGER);
    VAR index : INTEGER;
        trio  : Triple;
        next  : Triple;
        stIx  : INTEGER;

  (* ------------------------- *)
    PROCEDURE WriteTrio(p : Triple);
    BEGIN
      Console.WriteInt(p.loC, 0);
      IF p.loC # p.hiC THEN
        Console.WriteString(" ..");
        Console.WriteInt(p.hiC, 0);
      END;
    END WriteTrio;
  (* ------------------------- *)

  BEGIN
    WriteTag(t, i); Console.WriteLn;
    IF t.select # NIL THEN t.select.Diagnose(i) END;
    H.Indent(i); Console.Write("{"); Console.WriteLn;
    index := 0;
    IF t.labels.tide > 0 THEN
      H.Indent(i); Console.Write("|");
      trio := t.labels.a[index]; stIx := trio.ord; INC(index);
      WHILE index < t.labels.tide DO
        next := t.labels.a[index]; INC(index);
        IF next.ord = stIx THEN (* write out previous label *)
          WriteTrio(trio);
          trio := next;
          Console.WriteString(", ");
        ELSE (* next label belongs to the next case *)
          WriteTrio(trio);
          Console.WriteString(" : #");
          Console.WriteInt(trio.ord, 1); Console.WriteLn;
          H.Indent(i); Console.Write("|");
          trio := next; stIx := trio.ord;
        END;
      END;
      (* write out last label and case *)
      WriteTrio(trio);
      Console.WriteString(" : #");
      Console.WriteInt(trio.ord, 1); Console.WriteLn;
      FOR index := 0 TO t.blocks.tide - 1 DO
        H.Indent(i); Console.Write("#"); Console.WriteInt(index, 1);
        Console.WriteString(" -->"); Console.WriteLn;
        t.blocks.a[index].Diagnose(i+4);
      END;
    END;
    H.Indent(i); Console.WriteString("else");
    IF t.elsBlk # NIL THEN
      Console.WriteLn;
      t.elsBlk.Diagnose(i+4);
    ELSE
      Console.WriteString(" trap here");
      Console.WriteLn;
    END;
    H.Indent(i); Console.Write("}"); Console.WriteLn;
  END Diagnose;

(* ============================================================ *)
BEGIN (* ====================================================== *)
END StatDesc. (* ============================================== *)
(* ============================================================ *)

