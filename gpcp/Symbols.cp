(* ==================================================================== *)
(*                                                                      *)
(*  Symbol Module for the Gardens Point Component Pascal Compiler.      *)
(*  Implements the abstract base classes for all descriptor types.      *)
(*  Copyright (c) John Gough 1999, 2000.                                *)
(*                                                                      *)
(* ==================================================================== *)

MODULE Symbols;

  IMPORT
        GPCPcopyright,
        GPText,
        Console,
        FileNames,
        NameHash,
        L := LitValue,
        V := VarSets,
        S := CPascalS,
        H := DiagHelper;

(* ============================================================ *)

  CONST (* mode-kinds *)
    prvMode* = 0; pubMode* = 1; rdoMode* = 2; protect* = 3;

  CONST (* param-modes *)
    val* = 0; in* = 1; out* = 2; var* = 3; notPar* = 4;

  CONST (* force-kinds *)
    noEmit* = 0; partEmit* = 1; forced* = 2;

  CONST
    standard* = 0;

  CONST
    tOffset*  = 16; (* backward compatibility with JavaVersion *)

(* ============================================================ *)
(*    Foreign attributes for modules, procedures and classes  *)
(* ============================================================ *)

  CONST (* module and type attributes for xAttr *)
    mMsk*  = { 0 ..  7};  main*   =  0; weak*  =  1; need*  =  2;
                          fixd*   =  3; rtsMd* =  4; anon*  =  5;
                          clsTp*  =  6; frnMd* =  7;
    rMsk*  = { 8 .. 15};  noNew*  =  8; asgnd* =  9; noCpy* = 10;
                          spshl*  = 11; xCtor* = 12;
    fMsk*  = {16 .. 23};  isFn*   = 16; extFn* = 17; fnInf* = 18;
    dMsk*  = {24 .. 31};  cMain*  = 24; wMain* = 25;

(* ============================================================ *)

  TYPE NameStr* = ARRAY 64 OF CHAR;

(* ============================================================ *)

  TYPE
    Idnt*   = POINTER TO ABSTRACT RECORD
                kind-  : INTEGER;   (* tag for unions *)
                token* : S.Token;   (* scanner token  *)
                type*  : Type;      (* typ-desc | NIL *)
                hash*  : INTEGER;   (* hash bucket no *)
                vMod-  : INTEGER;   (* visibility tag *)
                dfScp* : Scope;     (* defining scope *)
                tgXtn* : ANYPTR;    (* target stuff   *)
              END;   (* For fields: record-decl scope *)

    IdSeq*  = RECORD
                tide-, high : INTEGER;
                a- : POINTER TO ARRAY OF Idnt;
              END;

    Scope*  = POINTER TO ABSTRACT RECORD (Idnt)
                symTb*   : SymbolTable; (* symbol scope   *)
                endDecl* : BOOLEAN;
                ovfChk*  : BOOLEAN;
                locals*  : IdSeq;
                scopeNm* : L.CharOpen   (* external name  *)
              END;

    ScpSeq*  = RECORD
                tide-, high : INTEGER;
                a- : POINTER TO ARRAY OF Scope;
              END;

(* ============================================================ *)

  TYPE
    Type*   = POINTER TO ABSTRACT RECORD
                idnt*   : Idnt;         (* Id of typename *)
                kind-   : INTEGER;      (* tag for unions *)
                serial- : INTEGER;      (* type serial-nm *)
                force*  : INTEGER;      (* force sym-emit *)
                xName*  : L.CharOpen;   (* full ext name  *)
                dump*,depth* : INTEGER; (* scratch loc'ns *)
                tgXtn*  : ANYPTR;       (* target stuff   *)
              END;

    TypeSeq*  = RECORD
                  tide-, high : INTEGER;
                  a- : POINTER TO ARRAY OF Type;
                END;

(* ============================================================ *)

  TYPE
    Stmt*   = POINTER TO ABSTRACT RECORD
                kind-  : INTEGER; (* tag for unions *)
                token* : S.Token; (* stmt first tok *)
              END;

    StmtSeq*  = RECORD
                  tide-, high : INTEGER;
                  a- : POINTER TO ARRAY OF Stmt;
                END;

(* ============================================================ *)

  TYPE
    Expr*   = POINTER TO ABSTRACT RECORD
                kind-  : INTEGER; (* tag for unions *)
                token* : S.Token; (* exp marker tok *)
                tSpan* : S.Span;  (* start expr tok *)
                type*  : Type;
              END;

    ExprSeq*  = RECORD
                  tide-, high : INTEGER;
                  a- : POINTER TO ARRAY OF Expr;
                END;

(* ============================================================ *)

  TYPE  (* Symbol tables are implemented by a binary tree *)
    SymInfo = POINTER TO RECORD         (* private stuff  *)
                key : INTEGER;          (* hash key value *)
                val : Idnt;             (* id-desc. value *)
                lOp : SymInfo;          (* left child     *)
                rOp : SymInfo;          (* right child    *)
              END;

    SymbolTable* = RECORD
                     root : SymInfo;
                   END;

(* ============================================================ *)
(*  SymForAll is the base type of a visitor type.               *)
(*  Instances of extensions of SymForAll type are passed to     *)
(*  SymbolTables using                                          *)
(*    symTab.Apply(sfa : SymForAll);                            *)
(*  This recurses over the table, applying sfa.Op(id) to each   *)
(*  Idnt descriptor in the scope.                               *)
(* ============================================================ *)

  TYPE
    SymForAll*  = POINTER TO ABSTRACT RECORD END;

    SymTabDump* = POINTER TO RECORD (SymForAll)
                    indent : INTEGER;
                  END;

    NameDump*   = POINTER TO RECORD (SymForAll)
                    tide, high : INTEGER;
                    a : L.CharOpen;
                  END;

(* ============================================================ *)

  TYPE
    SccTable*  = POINTER TO RECORD
                   symTab*  : SymbolTable;
                   target*  : Type;
                   reached* : BOOLEAN;
                 END;

(* ============================================================ *)

  TYPE
    NameFetch* = POINTER TO RECORD END;
    (** This type exports two methods only:       *)
    (*  (g : NameFetch)Of*(i : Idnt; OUT s : ARRAY OF CHAR);    *)
    (*  (g : NameFetch)ChPtr*(id : Idnt) : L.CharOpen;   *)

(* ============================================================ *)

  VAR modStr-  : ARRAY 4 OF ARRAY 5 OF CHAR;
      modMrk-  : ARRAY 5 OF CHAR;
      anonMrk- : ARRAY 3 OF CHAR;
      trgtNET- : BOOLEAN;
      getName* : NameFetch;
      next     : INTEGER; (* private: next serial number. *)

(* ============================================================ *)

  PROCEDURE SetTargetIsNET*(p : BOOLEAN);
  BEGIN
    trgtNET := p;
    IF p THEN anonMrk := "@T" ELSE anonMrk := "$T" END;
  END SetTargetIsNET;

(* ============================================================ *)
(*             Abstract attribution methods                     *)
(* ============================================================ *)

  PROCEDURE (i : Expr)exprAttr*() : Expr,NEW,ABSTRACT;
  PROCEDURE (s : Stmt)StmtAttr*(t : Scope),NEW,ABSTRACT;
  PROCEDURE (s : Stmt)flowAttr*(t : Scope; i : V.VarSet):V.VarSet,NEW,ABSTRACT;

(* ============================================================ *)
(*             Abstract type erase methods                      *)
(* ============================================================ *)

  PROCEDURE (s : Stmt)TypeErase*(t : Scope), NEW, ABSTRACT;
  PROCEDURE (s : Expr)TypeErase*() : Expr, NEW, ABSTRACT;
  PROCEDURE (i : Type)TypeErase*() : Type, NEW, ABSTRACT;

(* ============================================================ *)
(*             Abstract diagnostic methods                      *)
(* ============================================================ *)

  PROCEDURE (t : Idnt)Diagnose*(i : INTEGER),NEW,ABSTRACT;
  PROCEDURE (t : Type)Diagnose*(i : INTEGER),NEW,ABSTRACT;
  PROCEDURE (t : Expr)Diagnose*(i : INTEGER),NEW,ABSTRACT;
  PROCEDURE (t : Stmt)Diagnose*(i : INTEGER),NEW,ABSTRACT;
  PROCEDURE (t : Type)name*() : L.CharOpen,NEW,ABSTRACT;
  
(* ============================================================ *)
(*             Base Class text-span method                      *)
(* ============================================================ *)

  PROCEDURE (s : Stmt)Span*() : S.Span,NEW,EXTENSIBLE;
  BEGIN
    RETURN S.mkSpanT(s.token);
  END Span;

(* ============================================================ *)
(*    Base predicates on Idnt extensions                        *)
(* If the predicate needs a different implementation for each   *)
(* of the direct subclasses, then it is ABSTRACT, otherwise it  *)
(* should be implemented here with a default return value.      *)
(* ============================================================ *)

  PROCEDURE (s : Idnt)isImport*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isImport;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)isImported*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN
    RETURN (s.dfScp # NIL) & s.dfScp.isImport();
  END isImported;

(* -------------------------------------------- *)

  PROCEDURE (s : Type)isImportedType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN
    RETURN (s.idnt # NIL) &
     (s.idnt.dfScp # NIL) &
      s.idnt.dfScp.isImport();
  END isImportedType;

(* -------------------------------------------- *)
  PROCEDURE^ (xp : Expr)ExprError*(n : INTEGER),NEW;

  PROCEDURE (s : Idnt)mutable*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END mutable;

  PROCEDURE (s : Idnt)CheckMutable*(x : Expr),NEW,EXTENSIBLE;
  BEGIN x.ExprError(181) END CheckMutable;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)isStatic*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isStatic;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)isLocalVar*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isLocalVar;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)isWeak*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isWeak;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)isDynamic*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isDynamic;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)isAbstract*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isAbstract;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)isEmpty*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isEmpty;

(* -------------------------------------------- *)

  PROCEDURE (i : Idnt)parMode*() : INTEGER,NEW,EXTENSIBLE;
  BEGIN RETURN notPar END parMode;

(* -------------------------------------------- *)
(* ????
  PROCEDURE (s : Idnt)isRcv*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isRcv;
 *)
(* -------------------------------------------- *)
(* ????
  PROCEDURE (s : Idnt)isAssignProc*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isAssignProc;
 *)
(* ============================================================ *)
(*    Base predicates on Type extensions                        *)
(* ============================================================ *)

  PROCEDURE (l : Type)equalOpenOrVector*(r : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END equalOpenOrVector;

(* -------------------------------------------- *)

  PROCEDURE (l : Type)procMatch*(r : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END procMatch;

(* -------------------------------------------- *)

  PROCEDURE (l : Type)namesMatch*(r : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END namesMatch;

(* -------------------------------------------- *)

  PROCEDURE (l : Type)sigsMatch*(r : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END sigsMatch;

(* -------------------------------------------- *)

  PROCEDURE (l : Type)equalPointers*(r : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END equalPointers;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isAnonType*() : BOOLEAN,NEW;
  BEGIN RETURN (i.idnt = NIL) OR (i.idnt.dfScp = NIL) END isAnonType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isBaseType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isBaseType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isIntType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isIntType;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)isIn*(set : V.VarSet) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN TRUE END isIn;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isNumType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isNumType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isScalarType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN TRUE END isScalarType; (* all except arrays, records *)

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isSetType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isSetType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isRealType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isRealType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isCharType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isCharType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isBooleanType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isBooleanType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isStringType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isStringType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)nativeCompat*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END nativeCompat;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isCharArrayType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isCharArrayType;

(* -------------------------------------------- *)

  PROCEDURE (s : Type)isRefSurrogate*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isRefSurrogate;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isPointerType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isPointerType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isRecordType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isRecordType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isProcType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isProcType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isProperProcType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isProperProcType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isDynamicType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isDynamicType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isAbsRecType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isAbsRecType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isLimRecType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isLimRecType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isExtnRecType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isExtnRecType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isOpenArrType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isOpenArrType;

  PROCEDURE (i : Type)isVectorType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isVectorType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)needsInit*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN TRUE END needsInit;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isForeign*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isForeign;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)valCopyOK*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN TRUE END valCopyOK;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isInterfaceType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isInterfaceType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isEventType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isEventType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isCompoundType*() : BOOLEAN,NEW,EXTENSIBLE;
  (* Returns TRUE if the type is a compound type *)
  BEGIN RETURN FALSE END isCompoundType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)ImplementationType*() : Type,NEW,EXTENSIBLE;
  (* Returns the type that this type will be implemented
   * as. Usually this is just an identity function, but
   * for types that can be erased, it may be a different
   * type. *)
  BEGIN RETURN i END ImplementationType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)implements*(x : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END implements;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)implementsAll*(x : Type) : BOOLEAN,NEW,EXTENSIBLE;
  (* Returns true iff i is a type that implements all of the
   * interfaces of x. x and i must be types that are capable of
   * implementing interfaces (a record or pointer) *)
  BEGIN RETURN FALSE END implementsAll;

(* -------------------------------------------- *)

  PROCEDURE (b : Type)isBaseOf*(x : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isBaseOf;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isLongType*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isLongType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isNativeObj*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isNativeObj;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isNativeStr*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isNativeStr;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)isNativeExc*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isNativeExc;

(* -------------------------------------------- *)

  PROCEDURE (b : Type)includes*(x : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END includes;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)boundRecTp*() : Type,NEW,EXTENSIBLE;
  BEGIN RETURN NIL END boundRecTp;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)superType*() : Type,NEW,EXTENSIBLE;
  BEGIN RETURN NIL END superType;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)elaboration*() : Type,NEW,EXTENSIBLE;
  BEGIN RETURN i END elaboration;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)inheritedFeature*(m : Idnt) : Idnt,NEW,EXTENSIBLE;
  BEGIN
    RETURN NIL;
  END inheritedFeature;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)returnType*() : Type,NEW,EXTENSIBLE;
  BEGIN RETURN NIL END returnType;

(* -------------------------------------------- *)

  PROCEDURE (recT : Type)AppendCtor*(prcI : Idnt),NEW,EMPTY;
  PROCEDURE (oldT : Type)CheckCovariance*(newI : Idnt),NEW,EMPTY;
  PROCEDURE (mthT : Type)CheckEmptyOK*(),NEW,EMPTY;
  PROCEDURE (theT : Type)ConditionalMark*(),NEW,ABSTRACT;
  PROCEDURE (theT : Type)UnconditionalMark*(),NEW,ABSTRACT;
  PROCEDURE (prcT : Type)OutCheck*(s : V.VarSet),NEW,EMPTY;
  PROCEDURE (s : Scope)LiveInitialize*(i : V.VarSet),NEW,EMPTY;
  PROCEDURE (s : Scope)UplevelInitialize*(i : V.VarSet),NEW,EMPTY;
  PROCEDURE (o : Idnt)OverloadFix*(),NEW,EMPTY;

(* -------------------------------------------- *)

  PROCEDURE (i : Type)resolve*(d : INTEGER) : Type,NEW,ABSTRACT;
  PROCEDURE (i : Type)TypeFix*(IN a : TypeSeq),NEW,ABSTRACT;
  PROCEDURE (i : Type)InsertMethod*(m : Idnt),NEW,EMPTY;
  PROCEDURE (i : Type)SccTab*(t : SccTable),NEW,ABSTRACT;

(* ============================================================ *)
(*    Base predicates on Expr extensions                        *)
(* ============================================================ *)

  PROCEDURE (i : Expr)isNil*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isNil;

(* -------------------------------------------- *)
  PROCEDURE (i : Expr)isInf*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isInf;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isWriteable*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isWriteable;

  PROCEDURE (x : Expr)CheckWriteable*(),NEW,EXTENSIBLE;
  BEGIN x.ExprError(103) END CheckWriteable;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isVarDesig*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isVarDesig;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isProcVar*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isProcVar;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isJavaInit*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isJavaInit;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isSetExpr*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & (x.type.isSetType()) END isSetExpr;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isBooleanExpr*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & (x.type.isBooleanType()) END isBooleanExpr;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isCharArray*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & (x.type.isCharArrayType()) END isCharArray;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isCharLit*() : BOOLEAN,NEW,EXTENSIBLE;
  (** A literal character, or a literal string of length = 1. *)
  BEGIN RETURN FALSE END isCharLit;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isCharExpr*() : BOOLEAN,NEW;
  BEGIN
    RETURN x.isCharLit() OR
    (x.type # NIL) & (x.type.isCharType());
  END isCharExpr;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isString*() : BOOLEAN,NEW;
  (** A literal string or the result of a string concatenation. *)
  BEGIN RETURN (x.type # NIL) & (x.type.isStringType()) END isString;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isNumLit*() : BOOLEAN,NEW,EXTENSIBLE;
  (** Any literal integer. *)
  BEGIN RETURN FALSE END isNumLit;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isStrLit*() : BOOLEAN,NEW,EXTENSIBLE;
  (** Any literal string. *)
  BEGIN RETURN FALSE END isStrLit;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isProcLit*() : BOOLEAN,NEW,EXTENSIBLE;
  (** Any literal procedure. *)
  BEGIN RETURN FALSE END isProcLit;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isPointerExpr*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & x.type.isPointerType() END isPointerExpr;

  PROCEDURE (x : Expr)isVectorExpr*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & x.type.isVectorType() END isVectorExpr;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isProcExpr*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & x.type.isProcType() END isProcExpr;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isIntExpr*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & x.type.isIntType() END isIntExpr;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isRealExpr*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & x.type.isRealType() END isRealExpr;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isNumericExpr*() : BOOLEAN,NEW;
  BEGIN RETURN (x.type # NIL) & x.type.isNumType() END isNumericExpr;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isStdFunc*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isStdFunc;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)hasDynamicType*() : BOOLEAN,NEW,EXTENSIBLE;
  (* overridden for IdLeaf extension of LeafX expression type *)
  BEGIN
    RETURN (x.type # NIL) & x.type.isDynamicType();
  END hasDynamicType;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)isStdProc*() : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN RETURN FALSE END isStdProc;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)inRangeOf*(t : Type) : BOOLEAN,NEW,EXTENSIBLE;
  (* If t is an ordinal type, return x in range, or for array *
   * type  t return x is within the index range.    *)
  BEGIN RETURN FALSE END inRangeOf;

(* ============================================================ *)

  PROCEDURE RepTypesError*(n : INTEGER; lT,rT : Type; ln,cl : INTEGER);
  BEGIN
    S.SemError.RepSt2(n, lT.name(), rT.name(), ln, cl);
  END RepTypesError;

  PROCEDURE RepTypesErrTok*(n : INTEGER; lT,rT : Type; tk : S.Token);
  BEGIN
    S.SemError.RepSt2(n, lT.name(), rT.name(), tk.lin, tk.col);
  END RepTypesErrTok;

(* ============================================================ *)
(*    Various Type Compatability tests.                         *)
(* ============================================================ *)

  PROCEDURE (lhT : Type)equalType*(rhT : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN
    RETURN (lhT = rhT)
        OR lhT.equalPointers(rhT)
        OR lhT.equalOpenOrVector(rhT)
        OR lhT.procMatch(rhT);
  END equalType;

(* -------------------------------------------- *)

  PROCEDURE (lhT : Type)assignCompat*(x : Expr) : BOOLEAN,NEW;
    VAR rhT : Type;
  BEGIN
    IF (x = NIL) OR (x.type = NIL) THEN RETURN TRUE; END;
    rhT := x.type;

    (* Compound type compatibility. *)
    IF lhT.isCompoundType() THEN
      IF ~lhT.isBaseOf(rhT) THEN RETURN FALSE END;
      IF (rhT.isExtnRecType()) THEN RETURN TRUE END;
      (* rhT is not extensible. It must support all of lhT's interfaces
       * statically *)
      RETURN rhT.implementsAll(lhT);
    END;

    IF lhT.equalType(rhT) & ~lhT.isExtnRecType() & ~lhT.isOpenArrType() THEN
        RETURN TRUE END;
    IF lhT.includes(rhT) THEN
        RETURN TRUE END;
    IF lhT.isPointerType() & lhT.isBaseOf(rhT) THEN
        RETURN TRUE END;
    IF x.isNil() THEN
        RETURN lhT.isPointerType() OR lhT.isProcType() END;
    IF x.isNumLit()  & lhT.isIntType() OR
       x.isCharLit() & lhT.isCharType() OR
       x.isStrLit()  & lhT.isCharArrayType() THEN
        RETURN x.inRangeOf(lhT) END;
    IF x.isString() THEN
        RETURN  lhT.nativeCompat() OR lhT.isCharArrayType() END;
    IF lhT.isInterfaceType() THEN
        RETURN  rhT.implements(lhT) END;
    RETURN FALSE;
  END assignCompat;

(* -------------------------------------------- *)

  PROCEDURE (formal : Idnt)paramCompat*(actual : Expr) : BOOLEAN,NEW;
    VAR acType : Type;
        fmType : Type;
  BEGIN
    IF (actual = NIL) OR (actual.type = NIL) OR (formal.type = NIL) THEN
      RETURN TRUE;
    ELSE
      acType := actual.type;
      fmType := formal.type;
    END;

    IF fmType.equalType(acType) THEN RETURN TRUE;
    ELSE
      CASE formal.parMode() OF
      | val : RETURN fmType.assignCompat(actual);
      | out : RETURN fmType.isPointerType() & acType.isBaseOf(fmType);
      | var : RETURN fmType.isExtnRecType() & fmType.isBaseOf(acType);
      | in  : RETURN fmType.isExtnRecType() & fmType.isBaseOf(acType) OR
                     fmType.isPointerType() & fmType.assignCompat(actual);
      (* Special case:  CP-strings ok with IN-mode NativeString/Object *)
      ELSE RETURN FALSE;
      END;
    END;
  END paramCompat;

(* -------------------------------------------- *)

  PROCEDURE (lhT : Type)arrayCompat*(rhT : Type) : BOOLEAN,NEW,EXTENSIBLE;
  BEGIN
    RETURN lhT.equalType(rhT);  (* unless it is an array *)
  END arrayCompat;

(* ============================================================ *)
(*        Various Appends, for the abstract types.              *)
(* ============================================================ *)

  PROCEDURE InitIdSeq*(VAR seq : IdSeq; capacity : INTEGER);
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitIdSeq;

  (* ---------------------------------- *)

  PROCEDURE ResetIdSeq*(VAR seq : IdSeq);
  BEGIN
    seq.tide := 0;
    IF seq.a = NIL THEN InitIdSeq(seq, 2) END;
  END ResetIdSeq;

  (* ---------------------------------- *)

  PROCEDURE (VAR seq : IdSeq)ResetTo*(newTide : INTEGER),NEW;
  BEGIN
    ASSERT(newTide <= seq.tide);
    seq.tide := newTide;
  END ResetTo;

  (* ---------------------------------- *)

  PROCEDURE AppendIdnt*(VAR seq : IdSeq; elem : Idnt);
    VAR temp : POINTER TO ARRAY OF Idnt;
        i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN
      InitIdSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, seq.high+1);
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendIdnt;

(* -------------------------------------------- *)

  PROCEDURE InitTypeSeq*(VAR seq : TypeSeq; capacity : INTEGER);
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitTypeSeq;

  PROCEDURE ResetTypeSeq*(VAR seq : TypeSeq);
  BEGIN
    seq.tide := 0;
    IF seq.a = NIL THEN InitTypeSeq(seq, 2) END;
  END ResetTypeSeq;

  PROCEDURE AppendType*(VAR seq : TypeSeq; elem : Type);
    VAR temp : POINTER TO ARRAY OF Type;
        i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN
      InitTypeSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendType;

(* -------------------------------------------- *)

  PROCEDURE InitScpSeq*(VAR seq : ScpSeq; capacity : INTEGER);
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitScpSeq;

  PROCEDURE ResetScpSeq*(VAR seq : ScpSeq);
  BEGIN
    seq.tide := 0;
    IF seq.a = NIL THEN InitScpSeq(seq, 2) END;
  END ResetScpSeq;

  PROCEDURE AppendScope*(VAR seq : ScpSeq; elem : Scope);
    VAR temp : POINTER TO ARRAY OF Scope;
        i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN
      InitScpSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendScope;

(* ============================================================ *)

  PROCEDURE InitExprSeq*(VAR seq : ExprSeq; capacity : INTEGER);
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitExprSeq;

  (* ---------------------------------- *)

  PROCEDURE ResetExprSeq*(VAR seq : ExprSeq);
  BEGIN
    seq.tide := 0;
    IF seq.a = NIL THEN InitExprSeq(seq, 2) END;
  END ResetExprSeq;

  (* ---------------------------------- *)

  PROCEDURE (VAR seq : ExprSeq)ResetTo*(newTide : INTEGER),NEW;
  BEGIN
    ASSERT(newTide <= seq.tide);
    seq.tide := newTide;
  END ResetTo;

  (* ---------------------------------- *)

  PROCEDURE AppendExpr*(VAR seq : ExprSeq; elem : Expr);
    VAR temp : POINTER TO ARRAY OF Expr;
        i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN
      InitExprSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendExpr;

(* -------------------------------------------- *)

  PROCEDURE InitStmtSeq*(VAR seq : StmtSeq; capacity : INTEGER);
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitStmtSeq;

  PROCEDURE AppendStmt*(VAR seq : StmtSeq; elem : Stmt);
    VAR temp : POINTER TO ARRAY OF Stmt;
        i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN
      InitStmtSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendStmt;

(* ============================================================ *)

  PROCEDURE (p : Expr)NoteCall*(s : Scope),NEW,EMPTY;

(* ============================================================ *)

  PROCEDURE (p : Expr)enterGuard*(tmp : Idnt) : Idnt,NEW,EXTENSIBLE;
  BEGIN RETURN NIL END enterGuard;

(* -------------------------------------------- *)

  PROCEDURE (p : Expr)ExitGuard*(sav : Idnt; tmp : Idnt),NEW,EXTENSIBLE;
  BEGIN END ExitGuard;

(* -------------------------------------------- *)

  PROCEDURE (p : Expr)checkLive*(s : Scope;
                                 l : V.VarSet) : V.VarSet,NEW,EXTENSIBLE;
  BEGIN RETURN l END checkLive;

(* -------------------------------------------- *)

  PROCEDURE (p : Expr)assignLive*(s : Scope;
                                  l : V.VarSet) : V.VarSet,NEW,EXTENSIBLE;
  BEGIN RETURN p.checkLive(s,l) END assignLive;

(* -------------------------------------------- *)

  PROCEDURE (p : Expr)BoolLive*(scpe : Scope;
                                lvIn : V.VarSet;
                            OUT tSet : V.VarSet;
                            OUT fSet : V.VarSet),NEW,EXTENSIBLE;
  BEGIN
    tSet := p.checkLive(scpe, lvIn);
    fSet := tSet;
  END BoolLive;

(* ============================================================ *)
(*    Set methods for the read-only fields                      *)
(* ============================================================ *)

  PROCEDURE (s : Idnt)SetMode*(m : INTEGER),NEW;
  BEGIN s.vMod := m END SetMode;

(* -------------------------------------------- *)

  PROCEDURE (s : Idnt)SetKind*(m : INTEGER),NEW;
  BEGIN s.kind := m END SetKind;

(* -------------------------------------------- *)

  PROCEDURE (s : Type)SetKind*(m : INTEGER),NEW;
  (** set the "kind" field AND allocate a serial#. *)
  BEGIN
    s.kind := m;
    IF m # standard THEN s.serial := next; INC(next) END;
  END SetKind;

(* -------------------------------------------- *)

  PROCEDURE (s : Expr)SetKind*(m : INTEGER),NEW;
  BEGIN s.kind := m END SetKind;

(* -------------------------------------------- *)

  PROCEDURE (s : Stmt)SetKind*(m : INTEGER),NEW;
  BEGIN s.kind := m END SetKind;

(* ============================================================ *)
(*  Abstract method of the SymForAll visitor base type          *)
(* ============================================================ *)

  PROCEDURE (s : SymForAll)Op*(id : Idnt),NEW,ABSTRACT;

(* ============================================================ *)
(*  Name-fetch methods for type-name diagnostic strings         *)
(* ============================================================ *)

  PROCEDURE (g : NameFetch)Of*(id : Idnt; OUT s : ARRAY OF CHAR),NEW;
    VAR chO : L.CharOpen;
  BEGIN
    chO := NameHash.charOpenOfHash(id.hash);
    IF chO = NIL THEN s := "<NIL>" ELSE GPText.Assign(chO^,s) END;
  END Of;

(* -------------------------------------------- *)

  PROCEDURE (g : NameFetch)ChPtr*(id : Idnt) : L.CharOpen,NEW;
  BEGIN
    RETURN NameHash.charOpenOfHash(id.hash);
  END ChPtr;

(* ============================================================ *)
(*  Private methods of the symbol-table info-blocks             *)
(* ============================================================ *)

  PROCEDURE mkSymInfo(h : INTEGER; d : Idnt) : SymInfo;
    VAR rtrn : SymInfo;
  BEGIN
    NEW(rtrn); rtrn.key := h; rtrn.val := d; RETURN rtrn;
  END mkSymInfo;

(* -------------------------------------------- *)

  PROCEDURE (i : SymInfo)enter(h : INTEGER; d : Idnt) : BOOLEAN,NEW;
  BEGIN
    IF h < i.key THEN
      IF i.lOp = NIL THEN i.lOp := mkSymInfo(h,d); RETURN TRUE;
      ELSE RETURN i.lOp.enter(h,d);
      END;
    ELSIF h > i.key THEN
      IF i.rOp = NIL THEN i.rOp := mkSymInfo(h,d); RETURN TRUE;
      ELSE RETURN i.rOp.enter(h,d);
      END;
    ELSE (* h must equal i.key *) RETURN FALSE;
    END;
  END enter;

(* -------------------------------------------- *)

  PROCEDURE (i : SymInfo)rmLeaf(h : INTEGER) : SymInfo,NEW;
  BEGIN
    IF h < i.key THEN    i.lOp := i.lOp.rmLeaf(h);
    ELSIF h > i.key THEN i.rOp := i.rOp.rmLeaf(h);
    ELSE (* h must equal i.key *) RETURN NIL;
    END;
    RETURN i;
  END rmLeaf;

(* -------------------------------------------- *)

  PROCEDURE (i : SymInfo)write(h : INTEGER; d : Idnt) : SymInfo,NEW;
    VAR rtrn : SymInfo;
  BEGIN
    rtrn := i;      (* default: return self *)
    IF    h < i.key THEN i.lOp := i.lOp.write(h,d);
    ELSIF h > i.key THEN i.rOp := i.rOp.write(h,d);
    ELSE  rtrn.val := d;
    END;
    RETURN rtrn;
  END write;

(* -------------------------------------------- *)

  PROCEDURE (i : SymInfo)lookup(h : INTEGER) : Idnt,NEW;
  BEGIN
    IF h < i.key THEN
      IF i.lOp = NIL THEN RETURN NIL ELSE RETURN i.lOp.lookup(h) END;
    ELSIF h > i.key THEN
      IF i.rOp = NIL THEN RETURN NIL ELSE RETURN i.rOp.lookup(h) END;
    ELSE (* h must equal i.key *)
      RETURN i.val;
    END;
  END lookup;

(* -------------------------------------------- *)

  PROCEDURE (i : SymInfo)Apply(s : SymForAll),NEW;
  BEGIN
    s.Op(i.val);        (* Apply Op() to this node  *)
    IF i.lOp # NIL THEN i.lOp.Apply(s) END; (* Recurse to left subtree  *)
    IF i.rOp # NIL THEN i.rOp.Apply(s) END; (* Recurse to right subtree *)
  END Apply;

(* ============================================================ *)
(*      Public methods of the symbol-table type                 *)
(* ============================================================ *)

  PROCEDURE (IN s : SymbolTable)isEmpty*() : BOOLEAN,NEW;
  BEGIN RETURN s.root = NIL END isEmpty;

(* -------------------------------------------- *)

  PROCEDURE (VAR s : SymbolTable)enter*(hsh : INTEGER; id : Idnt) : BOOLEAN,NEW;
  (* Enter value in SymbolTable; Return value signals successful insertion. *)
  BEGIN
    IF s.root = NIL THEN
      s.root := mkSymInfo(hsh,id); RETURN TRUE;
    ELSE
      RETURN s.root.enter(hsh,id);
    END;
  END enter;

(* -------------------------------------------- *)

  PROCEDURE (VAR s : SymbolTable)Overwrite*(hsh : INTEGER; id : Idnt),NEW;
  (* Overwrite value in SymbolTable; value must be present. *)
  BEGIN
    s.root := s.root.write(hsh,id);
  END Overwrite;

(* -------------------------------------------- *)

  PROCEDURE (VAR s : SymbolTable)RemoveLeaf*(hsh : INTEGER),NEW;
  (* Remove value in SymbolTable; value must be a leaf. *)
  BEGIN
    s.root := s.root.rmLeaf(hsh);
  END RemoveLeaf;

(* -------------------------------------------- *)

  PROCEDURE (IN s : SymbolTable)lookup*(h : INTEGER) : Idnt,NEW;
  (* Find value in symbol table, else return NIL. *)
  BEGIN
    IF s.root = NIL THEN RETURN NIL ELSE RETURN s.root.lookup(h) END;
  END lookup;

(* -------------------------------------------- *)

  PROCEDURE (IN tab : SymbolTable)Apply*(sfa : SymForAll),NEW;
  (* Apply sfa.Op() to each entry in the symbol table. *)
  BEGIN
    IF tab.root # NIL THEN tab.root.Apply(sfa) END;
  END Apply;

(* ============================================================ *)
(*      Public static methods on symbol-tables                  *)
(* ============================================================ *)

  PROCEDURE refused*(id : Idnt; scp : Scope) : BOOLEAN;
    VAR fail  : BOOLEAN;
        clash : Idnt;
  BEGIN
    fail := ~scp.symTb.enter(id.hash, id);
    IF fail THEN
      clash := scp.symTb.lookup(id.hash);
      IF clash.isImport() & clash.isWeak() THEN
        scp.symTb.Overwrite(id.hash, id); fail := FALSE;
      END;
    END;
    RETURN fail;
  END refused;

(* -------------------------------------------- *)

  PROCEDURE bindLocal*(hash : INTEGER; scp : Scope) : Idnt;
  BEGIN
    RETURN scp.symTb.lookup(hash);
  END bindLocal;

(* -------------------------------------------- *)

  PROCEDURE bind*(hash : INTEGER; scp : Scope) : Idnt;
    VAR resId : Idnt;
  BEGIN
    resId := scp.symTb.lookup(hash);
    IF resId = NIL THEN
      scp := scp.dfScp;
      WHILE (resId = NIL) & (scp # NIL) DO
        resId := scp.symTb.lookup(hash);
        scp := scp.dfScp;
      END;
    END;
    RETURN resId;
  END bind;

(* -------------------------------------------- *)

  PROCEDURE maxMode*(i,j : INTEGER) : INTEGER;
  BEGIN
    IF    (i = pubMode) OR (j = pubMode) THEN RETURN pubMode;
    ELSIF (i = rdoMode) OR (j = rdoMode) THEN RETURN rdoMode;
    ELSE RETURN prvMode;
    END;
  END maxMode;

(* ============================================================ *)
(*        Various diagnostic methods                            *)
(* ============================================================ *)

  PROCEDURE (IN tab : SymbolTable)Dump*(i : INTEGER),NEW;
    VAR sfa : SymTabDump;
  BEGIN
    H.Indent(i);
    Console.WriteString("+-------- Symtab dump ---------"); Console.WriteLn;
    NEW(sfa);
    sfa.indent := i;
    tab.Apply(sfa);
    H.Indent(i);
    Console.WriteString("+-------- dump ended ----------"); Console.WriteLn;
  END Dump;

(* -------------------------------------------- *)

  PROCEDURE (id : Idnt)IdError*(n : INTEGER),NEW;
    VAR l,c : INTEGER;
  BEGIN
    IF id.token # NIL THEN l := id.token.lin; c := id.token.col;
    ELSE l := S.line; c := S.col;
    END;
    S.SemError.Report(n, l, c);
  END IdError;

(* -------------------------------------------- *)

  PROCEDURE (id : Idnt)IdErrorStr*(n : INTEGER;
                                IN s : ARRAY OF CHAR),NEW;
    VAR l,c : INTEGER;
  BEGIN
    IF id.token # NIL THEN l := id.token.lin; c := id.token.col;
    ELSE l := S.line; c := S.col;
    END;
    S.SemError.RepSt1(n,s,l,c);
  END IdErrorStr;

(* -------------------------------------------- *)

  PROCEDURE (ty : Type)TypeError*(n : INTEGER),NEW,EXTENSIBLE;
    VAR l,c : INTEGER;
  BEGIN
    IF (ty.idnt # NIL) & (ty.idnt.token # NIL) THEN
      l := ty.idnt.token.lin; c := ty.idnt.token.col;
    ELSE l := S.line; c := S.col;
    END;
    S.SemError.Report(n,l,c);
  END TypeError;

(* -------------------------------------------- *)

  PROCEDURE (ty : Type)TypeErrStr*(n : INTEGER;
                                IN s : ARRAY OF CHAR),NEW,EXTENSIBLE;
    VAR l,c : INTEGER;
  BEGIN
    IF (ty.idnt # NIL) & (ty.idnt.token # NIL) THEN 
      l := ty.idnt.token.lin; c := ty.idnt.token.col;
    ELSE l := S.line; c := S.col;
    END;
    S.SemError.RepSt1(n,s,l,c);
  END TypeErrStr;

(* -------------------------------------------- *)

  PROCEDURE (xp : Expr)ExprError*(n : INTEGER),NEW;
    VAR l,c : INTEGER;
  BEGIN
    IF xp.token # NIL THEN l := xp.token.lin; c := xp.token.col;
    ELSE l := S.line; c := S.col;
    END;
    S.SemError.Report(n,l,c);
  END ExprError;

(* -------------------------------------------- *)

  PROCEDURE (st : Stmt)StmtError*(n : INTEGER),NEW;
    VAR l,c : INTEGER;
  BEGIN
    IF st.token # NIL THEN l := st.token.lin; c := st.token.col;
    ELSE l := S.line; c := S.col;
    END;
    S.SemError.Report(n,l,c);
  END StmtError;

(* -------------------------------------------- *)

  PROCEDURE (id : Idnt)name*() : L.CharOpen, NEW;
  BEGIN
    RETURN NameHash.charOpenOfHash(id.hash);
  END name;

  PROCEDURE (t : Idnt)WriteName*(),NEW;
    VAR name : FileNames.NameString;
  BEGIN
    getName.Of(t, name);
    Console.WriteString(name$);
  END WriteName;

(* -------------------------------------------- *)

  PROCEDURE DoXName*(i : INTEGER; s : L.CharOpen);
  BEGIN
    H.Indent(i);
    Console.WriteString("name = ");
    IF s # NIL THEN Console.WriteString(s) ELSE
                                        Console.WriteString("<nil>") END;
    Console.WriteLn;
  END DoXName;

(* -------------------------------------------- *)

  PROCEDURE (t : Idnt)SuperDiag*(i : INTEGER),NEW;
    VAR dump : INTEGER;
  BEGIN
    dump := 0;
    (* H.Class("Idnt",t,i); *)
    H.Indent(i); Console.WriteString("Idnt: name = ");
    Console.WriteString(getName.ChPtr(t));
    Console.Write(modMrk[t.vMod]);
    Console.WriteString(" (");
    IF t.type = NIL THEN
      Console.WriteString("no type");
    ELSE
      dump := t.type.dump;
      Console.WriteString(t.type.name());
    END;
    IF dump # 0 THEN
      Console.WriteString(") t$");
      Console.WriteInt(dump, 1);
    ELSE
      Console.Write(")");
    END;
    Console.Write("#"); Console.WriteInt(t.hash,1);
    Console.WriteLn;
  END SuperDiag;

(* -------------------------------------------- *)

  PROCEDURE (t : Type)SuperDiag*(i : INTEGER),NEW;
  BEGIN
    (* H.Class("Type",t,i); *)
    H.Indent(i); Console.WriteString("Type: ");
    Console.WriteString(t.name());
    IF t.dump # 0 THEN
      Console.WriteString(" t$");
      Console.WriteInt(t.dump, 1);
      Console.Write(",");
    END;
    Console.WriteString(" s#");
    Console.WriteInt(t.serial, 1);
    Console.WriteLn;
  END SuperDiag;

(* -------------------------------------------- *)

  PROCEDURE (t : Expr)SuperDiag*(i : INTEGER),NEW;
  BEGIN
    H.Class("Expr",t,i);
  END SuperDiag;

(* -------------------------------------------- *)

  PROCEDURE (t : Stmt)SuperDiag*(i : INTEGER),NEW;
  BEGIN
    H.Class("Stmt",t,i);
    IF t.token # NIL THEN
      H.Indent(i);
      Console.WriteString("(lin:col ");
      Console.WriteInt(t.token.lin, 1); Console.Write(":");
      Console.WriteInt(t.token.col, 1); Console.Write(")");
      Console.WriteLn;
    END;
  END SuperDiag;

(* -------------------------------------------- *)

  PROCEDURE (s : SymTabDump)Op*(id : Idnt);
  BEGIN
    id.Diagnose(s.indent);
  END Op;

(* -------------------------------------------- *)

  PROCEDURE (s : Type)DiagFormalType*(i : INTEGER),NEW,EMPTY;

(* -------------------------------------------- *)

  PROCEDURE (x : Expr)DiagSrcLoc*(),NEW;
  BEGIN
    IF x.token # NIL THEN
      Console.WriteString("Expr at ");
      Console.WriteInt(x.token.lin,1);
      Console.Write(":");
      Console.WriteInt(x.token.col,1);
    ELSE
      Console.WriteString("no src token");
    END;
    Console.WriteLn;
  END DiagSrcLoc;

(* -------------------------------------------- *)

  PROCEDURE newNameDump() : NameDump;
    VAR dump : NameDump;
  BEGIN
    NEW(dump);
    NEW(dump.a, 32);
    dump.high := 31;
    dump.tide := 0;
    RETURN dump;
  END newNameDump;

 (* --------------------------- *)

  PROCEDURE (sfa : NameDump)Op*(id : Idnt);
    VAR name : L.CharOpen;
        temp : L.CharOpen;
        indx : INTEGER;
        newH : INTEGER;
        char : CHAR;
  BEGIN
    name := NameHash.charOpenOfHash(id.hash);
(*
 *  IF sfa.tide + LEN(name) >= sfa.tide THEN         OOPS!
 *)
    IF sfa.tide + LEN(name) >= sfa.high THEN
      temp := sfa.a;
      newH := sfa.high + 3 * LEN(name);
      NEW(sfa.a, newH+1);
      FOR indx := 0 TO sfa.tide - 1 DO
        sfa.a[indx] := temp[indx];
      END;
      sfa.high := newH;
    END;
    IF sfa.tide > 0 THEN
      sfa.a[sfa.tide-1] := ",";
      sfa.a[sfa.tide  ] := " ";
      INC(sfa.tide);
    END;
    indx := 0;
    REPEAT
      char := name[indx];
      sfa.a[sfa.tide] := char;
      INC(sfa.tide);
      INC(indx);
    UNTIL char = 0X;
  END Op;

 (* --------------------------- *)

  PROCEDURE dumpList*(s : SymbolTable) : L.CharOpen;
    VAR sfa : NameDump;
  BEGIN
    sfa := newNameDump();
    s.Apply(sfa);
    RETURN sfa.a;
  END dumpList;

(* ============================================================ *)
BEGIN (* ====================================================== *)
  NEW(getName);
  modMrk      := " *-!";
  modStr[val] := "";
  modStr[in ] := "IN ";
  modStr[out] := "OUT ";
  modStr[var] := "VAR ";
END Symbols.  (* ============================================== *)
(* ============================================================ *)

