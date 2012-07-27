(* ==================================================================== *)
(*                                                                      *)
(*  TypeDesc Module for the Gardens Point Component Pascal Compiler.    *)
(*  Implements type descriptors that are extensions of Symbols.Type     *)
(*                                                                      *)
(*  Copyright (c) John Gough 1999, 2000.                                *)
(*           version 1.1.4 2002:Jan:14                                  *)
(*                                                                      *)
(* ==================================================================== *)

MODULE TypeDesc;

  IMPORT
        GPCPcopyright,
        Con := Console,
        GPText,
        VarSets,
        NameHash,
        FileNames,
        CSt := CompState,
        Id  := IdDesc,
        Sy  := Symbols,
        Lv  := LitValue,
        S   := CPascalS,
        H   := DiagHelper,
		RTS;

(* ============================================================ *)

  CONST (* type-kinds *)
    basTp* = Symbols.standard;
    tmpTp* = 1;  namTp* = 2; arrTp* = 3;
    recTp* = 4;  ptrTp* = 5; prcTp* = 6;
    enuTp* = 7;  evtTp* = 8; ovlTp* = 9;
    vecTp* = 10;

  CONST (* base-ordinals *)
  (* WARNING:  these are locked in.  If they are changed, there *)
  (* is a consequential change in CPJrts for the JVM version.   *)
    notBs   =  0;
    boolN*  =  1;
    sChrN*  =  2; charN*  = 3;
    byteN*  =  4; sIntN*  = 5;  intN*  =  6; lIntN* =  7;
    sReaN*  =  8; realN*  = 9;
    setN*   = 10;
    anyRec* = 11; anyPtr* = 12;
    strN*   = 13; sStrN*  = 14; uBytN* = 15;
    metaN*  = 16;

  CONST (* record attributes *)
    noAtt* = 0; isAbs* = 1; limit* = 2;
    extns* = 3; iFace* = 4;
    cmpnd* = 5;   (* Marker for Compound Types                  *)
    noNew* = 8;   (* These two attributes are really for xAttr, *)
    valRc* = 16;  (* but piggy-back on recAtt in the symbolfile *)
    clsRc* = 32;  (* but piggy-back on recAtt in the symbolfile *)

(* ============================================================ *)

  CONST (* traversal depth markers *)
    initialMark = 0;
    finishMark  = -1;
    errorMark   = 0FFFFFH;

(* ============================================================ *)

   (* ------------------------------------------------------- *
    *  Overloadeds do not occur in pure Component Pascal.     *
    *  They appear transiently as descriptors of types of     *
    *  idents bound to overloaded members from foriegn libs.  *
    * ------------------------------------------------------- *)
  TYPE
    Overloaded* = POINTER TO EXTENSIBLE RECORD (Sy.Type)
                 (* ... inherited from Type ... ------------- *
                  * idnt*   : Idnt;         (* Id of typename *)
                  * kind-   : INTEGER;      (* tag for unions *)
                  * serial- : INTEGER;      (* type serial-nm *)
                  * force*  : INTEGER;      (* force sym-emit *)
                  * xName*  : Lv.CharOpen;  (* proc signature *)
                  * dump*,depth* : INTEGER; (* scratch loc'ns *)
                  * tgXtn*  : ANYPTR;       (* target stuff   *)
                  * ----------------------------------------- *)
                  END;

(* ============================================================ *)

  TYPE
    Base*   = POINTER TO RECORD (Sy.Type)
             (* ... inherited from Type ... ------------- *
              * idnt*   : Idnt;         (* Id of typename *)
              * kind-   : INTEGER;      (* tag for unions *)
              * serial- : INTEGER;      (* type serial-nm *)
              * force*  : INTEGER;      (* force sym-emit *)
              * xName*  : Lv.CharOpen;  (* full ext name  *)
              * dump*,depth* : INTEGER; (* scratch loc'ns *)
              * tgXtn*  : ANYPTR;       (* target stuff   *)
              * ----------------------------------------- *)
                tpOrd* : INTEGER;
              END;      (* ------------------------------ *)

(* ============================================================ *)

  VAR anyRecTp- : Base; (* Descriptor for the base type ANYREC. *)
      anyPtrTp- : Base; (* Descriptor for the base type ANYPTR. *)
      integerT* : Sy.Type;
      nilStr    : Lv.CharOpen;

(* ============================================================ *)

  TYPE
    Opaque* = POINTER TO RECORD (Sy.Type)
             (* ... inherited from Type ... ------------- *
              * idnt*   : Idnt;         (* Id of typename *)
              * kind-   : INTEGER;      (* tag for unions *)
              * serial- : INTEGER;      (* type serial-nm *)
              * force*  : INTEGER;      (* force sym-emit *)
              * xName*  : Lv.CharOpen;  (* full ext name  *)
              * dump*,depth* : INTEGER; (* scratch loc'ns *)
              * tgXtn*  : ANYPTR;       (* target stuff   *)
              * ----------------------------------------- *)
                resolved* : Sy.Type;    (* ptr to real-Tp *)
                scopeNm*  : Lv.CharOpen;
              END;      (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    Array*  = POINTER TO EXTENSIBLE RECORD (Sy.Type)
             (* ... inherited from Type ... ------------- *
              * idnt*   : Idnt;         (* Id of typename *)
              * kind-   : INTEGER;      (* tag for unions *)
              * serial- : INTEGER;      (* type serial-nm *)
              * force*  : INTEGER;      (* force sym-emit *)
              * xName*  : Lv.CharOpen;  (* full ext name  *)
              * dump*,depth* : INTEGER; (* scratch loc'ns *)
              * tgXtn*  : ANYPTR;       (* target stuff   *)
              * ----------------------------------------- *)
                elemTp* : Sy.Type;      (* element tpDesc *)
                length* : INTEGER;      (* 0 for open-arr *)
              END;      (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    Vector* = POINTER TO RECORD (Array)
             (* ... inherited from Type ... ------------- *
              * idnt*   : Idnt;         (* Id of typename *)
              * kind-   : INTEGER;      (* tag for unions *)
              * serial- : INTEGER;      (* type serial-nm *)
              * force*  : INTEGER;      (* force sym-emit *)
              * xName*  : Lv.CharOpen;  (* full ext name  *)
              * dump*,depth* : INTEGER; (* scratch loc'ns *)
              * tgXtn*  : ANYPTR;       (* target stuff   *)
              * ... inherited from Array ... ------------ *
              * elemTp* : Sy.Type;      (* element tpDesc *)
              * length* : INTEGER;      (* unused for Vec *)
              * ----------------------------------------- *)
              END;      (* ------------------------------ *)

(* ============================================================ *)

  TYPE
   (* ====================================================== *
    * When should a record type be implemented as a          *
    * reference class?   When any of the following is true:- *
    *  >  It is extensible or abstract                       *
    *  >  It extends a type other than System.ValueType      *
    *  >  It has an embedded non-value structure             *
    *  >  It is only declared as a pointer target            *
    *  >  If the target does not support value records       *
    * ====================================================== *)
    Record*    = POINTER TO RECORD (Sy.Type)
                (* ... inherited from Type ... ------------- *
                 * idnt*   : Idnt;         (* Id of typename *)
                 * kind-   : INTEGER;      (* tag for unions *)
                 * serial- : INTEGER;      (* type serial-nm *)
                 * force*  : INTEGER;      (* force sym-emit *)
                 * xName*  : Lv.CharOpen;  (* full ext name  *)
                 * dump*,depth* : INTEGER; (* scratch loc'ns *)
                 * tgXtn*  : ANYPTR;       (* target stuff   *)
                 * ----------------------------------------- *)
                   baseTp*     : Sy.Type;  (* immediate base *)
                   bindTp*     : Sy.Type;  (* ptrTo if anon. *)
                   encCls*     : Sy.Type;  (* if nested cls. *)
                   recAtt*     : INTEGER;
                   symTb*      : Sy.SymbolTable;
                   extrnNm*    : Lv.CharOpen;
                   scopeNm*    : Lv.CharOpen;
                   fields*     : Sy.IdSeq; (* list of fields *)
                   methods*    : Sy.IdSeq; (* list of meth's *)
                   statics*    : Sy.IdSeq; (* list of stat's *)
                   interfaces* : Sy.TypeSeq;(* impl-sequence *)
                   events*     : Sy.IdSeq; (* event-sequence *)
                   xAttr*      : SET;      (* external attrs *)
                 END;      (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    Enum*      = POINTER TO RECORD (Sy.Type)
                (* ... inherited from Type ... ------------- *
                 * idnt*   : Idnt;         (* Id of typename *)
                 * kind-   : INTEGER;      (* tag for unions *)
                 * serial- : INTEGER;      (* type serial-nm *)
                 * force*  : INTEGER;      (* force sym-emit *)
                 * xName*  : Lv.CharOpen;  (* full ext name  *)
                 * dump*,depth* : INTEGER; (* scratch loc'ns *)
                 * tgXtn*  : ANYPTR;       (* target stuff   *)
                 * ----------------------------------------- *)
                   symTb*      : Sy.SymbolTable;
                   statics*    : Sy.IdSeq; (* list of stat's *)
                 END;      (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    Pointer*   = POINTER TO EXTENSIBLE RECORD (Sy.Type)
                (* ... inherited from Type ... ------------- *
                 * idnt*   : Idnt;         (* Id of typename *)
                 * kind-   : INTEGER;      (* tag for unions *)
                 * serial- : INTEGER;      (* type serial-nm *)
                 * force*  : INTEGER;      (* force sym-emit *)
                 * xName*  : Lv.CharOpen;  (* full ext name  *)
                 * dump*,depth* : INTEGER; (* scratch loc'ns *)
                 * tgXtn*  : ANYPTR;       (* target stuff   *)
                 * ----------------------------------------- *)
                   boundTp* : Sy.Type;     (* ptr bound type *)
                 END;      (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    Procedure* = POINTER TO EXTENSIBLE RECORD (Sy.Type)
                (* ... inherited from Type ... ------------- *
                 * idnt*   : Idnt;         (* Id of typename *)
                 * kind-   : INTEGER;      (* tag for unions *)
                 * serial- : INTEGER;      (* type serial-nm *)
                 * force*  : INTEGER;      (* force sym-emit *)
                 * xName*  : Lv.CharOpen;  (* proc signature *)
                 * dump*,depth* : INTEGER; (* scratch loc'ns *)
                 * tgXtn*  : ANYPTR;       (* target stuff   *)
                 * ----------------------------------------- *)
                   tName*    : Lv.CharOpen;(* proc-type name *)
                   retType*  : Sy.Type;    (* ret-type | NIL *)
                   receiver* : Sy.Type;    (* element tpDesc *)
                   formals*  : Id.ParSeq;  (* formal params  *)
                   retN*,argN* : INTEGER;
                END;       (* ------------------------------ *)

(* ============================================================ *)

  TYPE
    Event*     = POINTER TO RECORD (Procedure)
                (* ... inherited from Type ... ------------- *
                 * xName*  : Lv.CharOpen;  (* proc signature *)
                 * tName*  : Lv.CharOpen;  (* proc-type name *)
                 * tgXtn*  : ANYPTR;       (* target stuff   *)
                 * ----------------------------------------- *
                 * ... inherited from Procedure ... -------- *
                 * tName*    : Lv.CharOpen;(* proc-type name *)
                 * retType*  : Sy.Type;    (* ret-type | NIL *)
                 * receiver* : Sy.Type;    (* element tpDesc *)
                 * formals*  : Id.ParSeq;  (* formal params  *)
                 * retN*,argN* : INTEGER;
                 * ----------------------------------------- *)
                   bndRec- : Record;
                 END;

(* ============================================================ *)
(*               Predicates on Type extensions                  *)
(* ============================================================ *)

  PROCEDURE (t : Base)isBooleanType*() : BOOLEAN;
  BEGIN RETURN t.tpOrd = boolN END isBooleanType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isNumType*() : BOOLEAN;
  BEGIN 
    RETURN 
     (t.tpOrd <= realN) & (t.tpOrd >= byteN) OR (t.tpOrd =  uBytN);
  END isNumType;

  PROCEDURE (t : Enum)isNumType*() : BOOLEAN;
  BEGIN RETURN TRUE END isNumType;


(* -------------------------------------------- *)

  PROCEDURE (t : Base)isBaseType*() : BOOLEAN;
  BEGIN RETURN TRUE END isBaseType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isIntType*() : BOOLEAN;
  BEGIN 
    RETURN 
      (t.tpOrd <= lIntN) & (t.tpOrd >= byteN) OR (t.tpOrd = uBytN);
  END isIntType;

  PROCEDURE (t : Enum)isIntType*() : BOOLEAN;
  BEGIN RETURN TRUE END isIntType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isScalarType*() : BOOLEAN;
  BEGIN RETURN t.tpOrd # anyRec END isScalarType;

  PROCEDURE (t : Enum)isScalarType*() : BOOLEAN;
  BEGIN RETURN TRUE END isScalarType;

  PROCEDURE (t : Array)isScalarType*() : BOOLEAN, EXTENSIBLE;
  BEGIN RETURN FALSE END isScalarType;

  PROCEDURE (t : Vector)isScalarType*() : BOOLEAN;
  BEGIN RETURN TRUE END isScalarType;

  PROCEDURE (t : Record)isScalarType*() : BOOLEAN;
  BEGIN RETURN FALSE END isScalarType;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)isImportedType*() : BOOLEAN;
  BEGIN
    IF t.bindTp # NIL THEN
      RETURN t.bindTp.isImportedType();
    ELSE
      RETURN (t.idnt # NIL) & (t.idnt.dfScp # NIL) & t.idnt.dfScp.isImport();
    END;
  END isImportedType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isSetType*() : BOOLEAN;
  BEGIN RETURN t.tpOrd = setN END isSetType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isRealType*() : BOOLEAN;
  BEGIN RETURN (t.tpOrd = realN) OR (t.tpOrd = sReaN) END isRealType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isCharType*() : BOOLEAN;
  BEGIN RETURN (t.tpOrd = charN) OR (t.tpOrd = sChrN) END isCharType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isNativeObj*() : BOOLEAN;
  BEGIN RETURN (t = anyRecTp) OR (t = anyPtrTp) END isNativeObj;

(* -------------------------------------------- *)

  PROCEDURE (t : Pointer)isNativeObj*() : BOOLEAN;
  BEGIN RETURN t = CSt.ntvObj END isNativeObj;

  PROCEDURE (t : Pointer)isNativeStr*() : BOOLEAN;
  BEGIN RETURN t = CSt.ntvStr END isNativeStr;

  PROCEDURE (t : Pointer)isNativeExc*() : BOOLEAN;
  BEGIN RETURN t = CSt.ntvExc END isNativeExc;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)isNativeObj*() : BOOLEAN;
  BEGIN RETURN t = CSt.ntvObj(Pointer).boundTp END isNativeObj;

  PROCEDURE (t : Record)isNativeStr*() : BOOLEAN;
  BEGIN RETURN t = CSt.ntvStr(Pointer).boundTp END isNativeStr;

  PROCEDURE (t : Record)isNativeExc*() : BOOLEAN;
  BEGIN RETURN t = CSt.ntvExc(Pointer).boundTp END isNativeExc;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isStringType*() : BOOLEAN;
  BEGIN RETURN (t.tpOrd = strN) OR (t.tpOrd = sStrN) END isStringType;

  PROCEDURE (t : Pointer)isStringType*() : BOOLEAN;
  BEGIN RETURN t = CSt.ntvStr END isStringType;

  PROCEDURE (t : Record)isStringType*() : BOOLEAN;
  BEGIN RETURN t = CSt.ntvStr(Pointer).boundTp END isStringType;

(* -------------------------------------------- *)

  PROCEDURE (t : Pointer)nativeCompat*() : BOOLEAN;
  BEGIN RETURN (t = CSt.ntvStr) OR (t = CSt.ntvObj) END nativeCompat;

(* -------------------------------------------- *)

  PROCEDURE (t : Array)isCharArrayType*() : BOOLEAN;
  BEGIN RETURN (t.elemTp # NIL) & t.elemTp.isCharType() END isCharArrayType;

(* -------------------------------------------- *)

  PROCEDURE (t : Pointer)isDynamicType*() : BOOLEAN;
  (** A type is dynamic if it is a pointer to any     *
   *  record. Overrides isDynamicType method in Symbols.Type. *)
  BEGIN RETURN (t.boundTp # NIL) & (t.boundTp IS Record) END isDynamicType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isPointerType*() : BOOLEAN;
  BEGIN RETURN t.tpOrd = anyPtr END isPointerType;

(* -------------------------------------------- *)

  PROCEDURE (t : Pointer)isPointerType*() : BOOLEAN;
  BEGIN RETURN TRUE END isPointerType;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)isRecordType*() : BOOLEAN;
  BEGIN RETURN TRUE END isRecordType;

(* -------------------------------------------- *)

  PROCEDURE (t : Procedure)isProcType*() : BOOLEAN;
  BEGIN RETURN TRUE END isProcType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isProcType*() : BOOLEAN;
  BEGIN RETURN t.tpOrd = anyPtr END isProcType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isDynamicType*() : BOOLEAN;
  BEGIN RETURN t.tpOrd = anyPtr END isDynamicType;

(* -------------------------------------------- *)

  PROCEDURE (t : Procedure)isProperProcType*() : BOOLEAN;
  BEGIN RETURN t.retType = NIL END isProperProcType;

(* -------------------------------------------- *)

  PROCEDURE (t : Procedure)returnType*() : Sy.Type;
  BEGIN RETURN t.retType END returnType;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)isAbsRecType*() : BOOLEAN;
  (** A record is absolute if it is declared to be an absolute  *
   *  record, or is a compound type.                            *
   *  Overrides isAbsRecType method in Symbols.Type.            *)
  BEGIN
    RETURN (isAbs = t.recAtt) OR
           (iFace = t.recAtt) OR
           (cmpnd = t.recAtt);
  END isAbsRecType;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)isExtnRecType*() : BOOLEAN;
  (** A record is extensible if declared absolute or extensible *
   *  record. Overrides isExtnRecType method in Symbols.Type.   *)
  BEGIN
    RETURN (extns = t.recAtt) OR (isAbs = t.recAtt);
  END isExtnRecType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isExtnRecType*() : BOOLEAN;
  (** A base type is extensible if it is ANYREC or ANYPTR * 
   *  Overrides isExtnRecType method in Symbols.Type.     *)
  BEGIN
    RETURN t.tpOrd = anyRec;
  END isExtnRecType;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)isLimRecType*() : BOOLEAN;
  (** A record is limited if it is declared to be limited *
   *  record. Overrides isLimRec method in Symbols.Type.  *)
  BEGIN RETURN limit = t.recAtt END isLimRecType;

(* -------------------------------------------- *)

  PROCEDURE (t : Array)isOpenArrType*() : BOOLEAN;
  BEGIN RETURN (t.kind = arrTp) & (t.length = 0) END isOpenArrType;

  PROCEDURE (t : Vector)isVectorType*() : BOOLEAN;
  BEGIN RETURN TRUE END isVectorType;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)needsInit*() : BOOLEAN;
  BEGIN RETURN FALSE END needsInit;

(* -------------------------------------------- *)

  PROCEDURE (t : Base)isLongType*() : BOOLEAN;
  BEGIN RETURN (t.tpOrd = realN) OR (t.tpOrd = lIntN) END isLongType;

(* -------------------------------------------- *)

  PROCEDURE (r : Record)compoundCompat*(e : Sy.Type) : BOOLEAN, NEW;
  (** Returns TRUE iff e is a type that could possibly be assignment
   *   compatible with the compound type r (i.e. r := e).
   *  If e is an extensible record type, it is sufficient that
   *   its base type is a subtype of the base type of r.
   *   Because the type is extensible, compatibility may not
   *   be determinable statically. Assertions may need to be
   *   inserted to determine compatibility at runtime.
   *  If e is not extensible then it must be a subtype of the
   *   base type of r and implement all of the interfaces of
   *   r. *)
  BEGIN
    ASSERT(r.isCompoundType());
    (* Not compatible if r is not a base of e *)
    IF (~r.isBaseOf(e)) THEN RETURN FALSE END;
    (* Dynamically compatible if e is an extensible record type *)
    IF (e.isExtnRecType()) THEN RETURN TRUE END;
    (* d is not extensible. It must support all of r's interfaces
     * statically *)
    RETURN e.implementsAll(r);
  END compoundCompat;

(* -------------------------------------------- *)

  PROCEDURE (b : Base)includes*(x : Sy.Type) : BOOLEAN;
    VAR xBas : Base;
        xOrd : INTEGER;
        bOrd : INTEGER;
  BEGIN
    IF x IS Enum THEN x := integerT;
    ELSIF ~(x IS Base) THEN RETURN FALSE;
    END;

    xBas := x(Base);
    xOrd := xBas.tpOrd;
    bOrd := b.tpOrd;
    CASE bOrd OF
    | uBytN, byteN, sChrN : (* only equality here *)
        RETURN xOrd = bOrd;
(*
 *  | byteN : (* only equality here *)
 *      RETURN xOrd = bOrd; 
 *  | uBytN, sChrN : (* only equality here *)
 *      RETURN (xOrd = uBytN) OR (xOrd = sChrN);
 *)
    | charN :
        RETURN (xOrd = charN) OR (xOrd = sChrN) OR (xOrd = uBytN);
    | sIntN .. realN :
        RETURN (xOrd <= bOrd) & (xOrd >= byteN) OR (xOrd =  uBytN);
    ELSE
        RETURN FALSE;
    END;
  END includes;

  PROCEDURE (b : Enum)includes*(x : Sy.Type) : BOOLEAN;
    VAR xBas : Base;
  BEGIN
    RETURN integerT.includes(x);
  END includes;

(* -------------------------------------------- *)

  PROCEDURE (b : Base)isBaseOf*(e : Sy.Type) : BOOLEAN;
  (** Find if e is an extension of base type b.   *
   *  Overrides the isBaseOf method in Symbols.Type   *)
  BEGIN
    IF    e.kind = recTp THEN RETURN b.tpOrd = anyRec;
    ELSIF e.kind = ptrTp THEN RETURN b.tpOrd = anyPtr;
    ELSE (* all others *)     RETURN b = e;
    END;
  END isBaseOf;

(* -------------------------------------------- *)

  PROCEDURE (b : Record)isBaseOf*(e : Sy.Type) : BOOLEAN;
  (** Find if e is an extension of record type b.   *
   *  Overrides the isBaseOf method in Symbols.Type   *)
    VAR ext : Record;
        i   : INTEGER;
  BEGIN
    IF e # NIL THEN e := e.boundRecTp() END;

    IF (e = NIL) OR (e.kind # recTp) THEN RETURN FALSE;
    ELSIF e = b THEN RETURN TRUE;               (* Trivially! *)
    END;                                        (* Not a record *)
    ext := e(Record);                           (* Cast to Rec. *)

    (* Compound type test: returns true if b is
     * a compound type and its base is a base of
     * e *)
    IF b.isCompoundType() THEN
      RETURN b.baseTp.isBaseOf(e);
    END;

    RETURN b.isBaseOf(ext.baseTp);              (* Recurse up!  *)
  END isBaseOf;

(* -------------------------------------------- *)

  PROCEDURE (b : Pointer)isBaseOf*(e : Sy.Type) : BOOLEAN;
  (** Find if e is an extension of pointer type b.    *
   *  Overrides the isBaseOf method in Symbols.Type   *)
    VAR ext : Pointer;
  BEGIN
    IF (e = NIL) OR (e.kind # ptrTp)  THEN RETURN FALSE;
    ELSIF (e = b) OR (b = CSt.ntvObj) THEN RETURN TRUE;   (* Trivially! *)
    END;
    ext := e(Pointer);                        (* Cast to Ptr. *)
    RETURN (b.boundTp # NIL)                  (* Go to bnd-tp *)
          & b.boundTp.isBaseOf(ext.boundTp);  (* for decision *)
  END isBaseOf;

(* -------------------------------------------- *)

  PROCEDURE (s : Array)isRefSurrogate*() : BOOLEAN;
  BEGIN RETURN TRUE END isRefSurrogate;

(* -------------------------------------------- *)

  PROCEDURE (s : Record)isRefSurrogate*() : BOOLEAN;
  BEGIN
    RETURN (Sy.clsTp IN s.xAttr) OR CSt.targetIsJVM();
  END isRefSurrogate;

(* -------------------------------------------- *)

  PROCEDURE (lhT : Array)arrayCompat*(rhT : Sy.Type) : BOOLEAN;
  BEGIN
    IF lhT.length = 0 THEN      (* An open array type *)
      IF rhT.kind = arrTp THEN
        RETURN lhT.elemTp.arrayCompat(rhT(Array).elemTp);
      ELSE
        RETURN lhT.isCharArrayType() & rhT.isStringType();
      END;
    ELSE
      RETURN FALSE;
    END;
  END arrayCompat;

(* -------------------------------------------- *)

  PROCEDURE (lhT : Enum)equalType*(rhT : Sy.Type) : BOOLEAN;
  BEGIN
    IF lhT = rhT THEN RETURN TRUE END;
    WITH rhT : Base DO
      RETURN rhT = integerT;
    ELSE
      RETURN FALSE;
    END;
  END equalType;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)isForeign*() : BOOLEAN;
  BEGIN
    RETURN Sy.isFn IN t.xAttr;
  END isForeign;

  PROCEDURE (t : Pointer)isForeign*() : BOOLEAN;
  BEGIN
    RETURN t.boundTp.isForeign();
  END isForeign;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)isCompoundType*() : BOOLEAN;
  (* Returns true iff the record is a compound type *)
  BEGIN
    RETURN t.recAtt = cmpnd;
  END isCompoundType;

(* -------------------------------------------- *)

  PROCEDURE (t : Pointer)isCompoundType*() : BOOLEAN;
  (* Returns true iff the pointer points to a compound type *)
  BEGIN
    RETURN (t.boundTp # NIL) & t.boundTp.isCompoundType();
  END isCompoundType;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)ImplementationType*() : Sy.Type;
  (* For compound types, this returns the base type of the compound
   * unless it is ANNYREC in which case it returns the first
   * interface in the list *)
  BEGIN
    IF t.isCompoundType() THEN
      IF t.baseTp # anyRecTp THEN
         RETURN t.baseTp(Record).bindTp;
      ELSE
         RETURN t.interfaces.a[0];
      END;
    ELSE
      RETURN t;
    END;
  END ImplementationType;

(* -------------------------------------------- *)

  PROCEDURE (t : Record)valCopyOK*() : BOOLEAN;
  BEGIN
    RETURN ~(Sy.noCpy IN t.xAttr);
  END valCopyOK;

  PROCEDURE (t : Array)valCopyOK*() : BOOLEAN;
  BEGIN
    RETURN t.elemTp.valCopyOK();
  END valCopyOK;

(* ============================================ *)

  PROCEDURE (t : Record)isInterfaceType*() : BOOLEAN;
  BEGIN
    RETURN (t.recAtt = iFace) OR
           ( (t.recAtt = cmpnd) &
             ( (t.baseTp = NIL) OR (t.baseTp = anyRecTp) ) );
  END isInterfaceType;

(* -------------------------------------------- *)

  PROCEDURE (t : Pointer)isInterfaceType*() : BOOLEAN;
  BEGIN
    RETURN (t.boundTp # NIL) & t.boundTp.isInterfaceType();
  END isInterfaceType;

(* ============================================ *)

  PROCEDURE (t : Event)isEventType*() : BOOLEAN;
  BEGIN RETURN TRUE END isEventType;

(* ============================================ *)

  PROCEDURE (t : Record)implements*(x : Sy.Type) : BOOLEAN;
   (* Assert: x.isInterfaceType is true *)
    VAR i : INTEGER; d : Sy.Type;
  BEGIN
    FOR i := 0 TO t.interfaces.tide - 1 DO
      d := t.interfaces.a[i];
      IF (d # NIL) &
   ((d = x) OR d.implements(x)) THEN RETURN TRUE END;
    END;
   (* else ... *)
    RETURN (t.baseTp # NIL) & t.baseTp.implements(x);
  END implements;

(* -------------------------------------------- *)

  PROCEDURE (t : Pointer)implements*(x : Sy.Type) : BOOLEAN;
  BEGIN
    RETURN (t.boundTp # NIL) & t.boundTp.implements(x);
  END implements;

(* ============================================ *)

  PROCEDURE (r : Record)implementsAll*(x : Sy.Type) : BOOLEAN;
  (* Returns true iff r implements all of the interfaces of x.*)
    VAR
      i: INTEGER;
  BEGIN
    WITH x : Pointer DO
            RETURN r.implementsAll(x.boundTp);
         | x : Record DO
            FOR i := 0 TO x.interfaces.tide - 1 DO
              IF ~r.implements(x.interfaces.a[i]) THEN RETURN FALSE END;
            END;
            RETURN TRUE;
         ELSE
            RETURN FALSE;
    END;
    RETURN FALSE;
  END implementsAll;

(* -------------------------------------------- *)

  PROCEDURE (i : Pointer)implementsAll*(x : Sy.Type) : BOOLEAN;
  (* Returns true iff p implements all of the interfaces of x.*)
  BEGIN
    RETURN i.boundTp.implementsAll(x);
  END implementsAll;

(* ============================================ *)

  PROCEDURE (lhsT : Procedure)formsMatch(rhsT : Procedure) : BOOLEAN,NEW;
    VAR index : INTEGER;
        lP,rP : Id.ParId;
  BEGIN
    IF lhsT.formals.tide # rhsT.formals.tide THEN RETURN FALSE;
    ELSE
      FOR index := 0 TO lhsT.formals.tide-1 DO
        lP := lhsT.formals.a[index];
        rP := rhsT.formals.a[index];
        IF (lP.type # NIL) & ~lP.type.equalType(rP.type) THEN RETURN FALSE END;
        IF lP.parMod # rP.parMod THEN RETURN FALSE END;
      END;
    END;
    RETURN TRUE;
  END formsMatch;

(* -------------------------------------------- *)

  PROCEDURE (lT : Array)equalOpenOrVector*(r : Sy.Type) : BOOLEAN, EXTENSIBLE;
    VAR rT : Array;
  BEGIN
    IF ~(r IS Array) THEN RETURN FALSE;
    ELSE
      rT := r(Array);
      RETURN (lT.length = 0) & (rT.length = 0) &
              lT.elemTp.equalType(rT.elemTp);
    END;
  END equalOpenOrVector;

(* -------------------------------------------- *)

  PROCEDURE (lT : Vector)equalOpenOrVector*(rT : Sy.Type) : BOOLEAN;
  BEGIN
    WITH rT : Vector DO
      RETURN lT.elemTp.equalType(rT.elemTp);
    ELSE 
      RETURN FALSE;
    END;
  END equalOpenOrVector;

(* -------------------------------------------- *)

  PROCEDURE (lT : Pointer)equalPointers*(r : Sy.Type) : BOOLEAN;
    VAR rT : Pointer;
        rO : Opaque;
  BEGIN
    IF r IS Opaque THEN
      rO := r(Opaque);
      IF rO.resolved # NIL THEN r := rO.resolved END;
    END;
    IF ~(r IS Pointer) THEN RETURN FALSE;
    ELSE
      rT := r(Pointer);
      RETURN lT.boundTp.equalType(rT.boundTp);
    END;
  END equalPointers;

(* -------------------------------------------- *)

  PROCEDURE (i : Record)InstantiateCheck*(tok : S.Token),NEW;
  BEGIN
    IF i.recAtt = isAbs THEN
      S.SemError.Report(90, tok.lin, tok.col);
    ELSIF i.recAtt = iFace THEN
      S.SemError.Report(131, tok.lin, tok.col);
    ELSIF (i.recAtt = limit) & i.isImportedType() THEN
      S.SemError.Report(71, tok.lin, tok.col);
    ELSIF (Sy.clsTp IN i.xAttr) & (Sy.noNew IN i.xAttr) THEN
      S.SemError.Report(155, tok.lin, tok.col);
    END;
  END InstantiateCheck;

(* -------------------------------------------- *)

  PROCEDURE (lhsT : Procedure)procMatch*(rT : Sy.Type) : BOOLEAN;
    VAR rhsT : Procedure;
  BEGIN
    IF ~(rT IS Procedure) THEN RETURN FALSE;
    ELSE
      rhsT := rT(Procedure);
      IF (lhsT.retType = NIL) # (rhsT.retType = NIL) THEN RETURN FALSE END;
      IF (lhsT.retType # NIL) &
         ~lhsT.retType.equalType(rhsT.retType) THEN RETURN FALSE END;
      RETURN lhsT.formsMatch(rhsT);
    END;
  END procMatch;

(* -------------------------------------------- *)

  PROCEDURE (lhsT : Procedure)namesMatch*(rT : Sy.Type) : BOOLEAN;
    VAR rhsT  : Procedure;
        index : INTEGER;
  BEGIN
    IF ~(rT IS Procedure) THEN  RETURN FALSE;
    ELSE
      rhsT := rT(Procedure);
      IF lhsT.formals.tide # rhsT.formals.tide THEN RETURN FALSE END;
      FOR index := 0 TO lhsT.formals.tide-1 DO
        IF lhsT.formals.a[index].hash #
           rhsT.formals.a[index].hash THEN RETURN FALSE END;
      END;
      RETURN TRUE;
    END;
  END namesMatch;

(* -------------------------------------------- *)

  PROCEDURE (lhsT : Procedure)sigsMatch*(rT : Sy.Type) : BOOLEAN;
    VAR rhsT : Procedure;
  BEGIN
    IF ~(rT IS Procedure) THEN
      RETURN FALSE;
    ELSE
      rhsT := rT(Procedure);
      RETURN lhsT.formsMatch(rhsT);
    END;
  END sigsMatch;

(* -------------------------------------------- *)

  PROCEDURE (oldT : Procedure)CheckCovariance*(newI : Sy.Idnt);
  (* When a method is overidden, the formals must match, except *
   * that the return type may vary covariantly with the recvr.  *)
    VAR newT : Procedure;
  BEGIN
    IF newI IS Id.Procs THEN
      newT := newI.type(Procedure);
      IF (oldT.retType = NIL) # (newT.retType = NIL) THEN newI.IdError(116);
      ELSIF ~newT.formsMatch(oldT) THEN
        newI.IdError(160);
      ELSIF (oldT.retType # NIL) & (oldT.retType # newT.retType) THEN
        IF ~oldT.retType.isBaseOf(newT.retType) THEN
          Sy.RepTypesErrTok(116, oldT, newT, newI.token);
        ELSIF newI IS Id.MthId THEN
          INCL(newI(Id.MthId).mthAtt, Id.covar);
        END;
      END;
    END;
  END CheckCovariance;

(* -------------------------------------------- *)

  PROCEDURE (desc : Procedure)CheckEmptyOK*();
    VAR idx : INTEGER;
        frm : Id.ParId;
  BEGIN
    FOR idx := 0 TO desc.formals.tide - 1 DO
      frm := desc.formals.a[idx];
      IF frm.parMod = Sy.out THEN frm.IdError(114) END;
    END;
    IF desc.retType # NIL THEN desc.TypeError(115) END;
  END CheckEmptyOK;

(* -------------------------------------------- *)

  PROCEDURE (rec : Record)defBlk() : Id.BlkId, NEW;
    VAR scp : Sy.Scope;
  BEGIN
    scp := NIL;
    IF rec.idnt # NIL THEN 
      scp := rec.idnt.dfScp;
    ELSIF rec.bindTp # NIL THEN
      IF rec.bindTp.idnt # NIL THEN scp := rec.bindTp.idnt.dfScp END;
    END;
    IF scp # NIL THEN
      WITH scp : Id.BlkId DO RETURN scp ELSE RETURN NIL END;
    ELSE
      RETURN NIL;
    END;
  END defBlk;

(* -------------------------------------------- *)

  PROCEDURE^ (recT : Record)bindField*(hash : INTEGER) : Sy.Idnt,NEW;

  PROCEDURE (recT : Record)interfaceBind(hash : INTEGER) : Sy.Idnt,NEW;
    VAR idnt : Sy.Idnt;
        intT : Sy.Type;
        indx : INTEGER;
  BEGIN
    FOR indx := 0 TO recT.interfaces.tide-1 DO
      intT := recT.interfaces.a[indx].boundRecTp();
      idnt := intT(Record).bindField(hash);
      IF idnt # NIL THEN RETURN idnt END;
    END;
    RETURN NIL;
  END interfaceBind;

  PROCEDURE AddIndirectImport(id : Sy.Idnt);
    VAR dBlk : Id.BlkId;
        rTyp : Record;
  BEGIN
    IF id = NIL THEN RETURN END;
   (*
    *  This additional code checks for indirectly imported modules.
    *  For the .NET framework references to inherited fields of
    *  objects name the defining class.  If that class comes from
    *  an assembly that is not explicitly imported into the CP, 
    *  then the IL must nevertheless make an explicit reference 
    *  to that assembly.
    *)
    WITH id : Id.FldId DO
          rTyp := id.recTyp(Record);
          dBlk := rTyp.defBlk();
          IF Sy.weak IN rTyp.xAttr THEN
            IF CSt.verbose THEN
              Console.WriteString(rTyp.name());
              Console.Write(".");
              Console.WriteString(Sy.getName.ChPtr(id));
              Console.WriteString(
                        ": defining module of field imported only indirectly");
              Console.WriteLn;
            END;
            INCL(dBlk.xAttr, Sy.need);
            EXCL(rTyp.xAttr, Sy.weak);
            Sy.AppendScope(CSt.impSeq, dBlk);
          END;
    | id : Id.MthId DO
          rTyp := id.bndType(Record);
          dBlk := rTyp.defBlk();
          IF Sy.weak IN rTyp.xAttr THEN
            IF CSt.verbose THEN
              Console.WriteString(rTyp.name());
              Console.Write(".");
              Console.WriteString(Sy.getName.ChPtr(id));
              Console.WriteString(
                       ": defining module of method imported only indirectly");
              Console.WriteLn;
            END;
            INCL(dBlk.xAttr, Sy.need);
            EXCL(rTyp.xAttr, Sy.weak);
            Sy.AppendScope(CSt.impSeq, dBlk);
          END;
    | id : Id.OvlId DO
          IF (id.dfScp # NIL) & 
             (id.dfScp IS Id.BlkId) THEN
            dBlk := id.dfScp(Id.BlkId);
            IF Sy.weak IN dBlk.xAttr THEN
              IF CSt.verbose THEN
                Console.WriteString(Sy.getName.ChPtr(dBlk));
                Console.Write(".");
                Console.WriteString(Sy.getName.ChPtr(id));
                Console.WriteString(
                        ": defining module of field imported only indirectly");
                Console.WriteLn;
              END;
              INCL(dBlk.xAttr, Sy.need);
              Sy.AppendScope(CSt.impSeq, dBlk);
            END;
          END;
    ELSE (* skip *)
    END;
  END AddIndirectImport;

  PROCEDURE (recT : Record)bindField*(hash : INTEGER) : Sy.Idnt,NEW;
    VAR idnt : Sy.Idnt;
        base : Sy.Type;
  BEGIN
    idnt := recT.symTb.lookup(hash);
    IF (idnt = NIL) &
       (recT.recAtt = iFace) &
       (recT.interfaces.tide > 0) THEN idnt := recT.interfaceBind(hash);
    END;
    WHILE (idnt = NIL) &                (* while not found yet *)
          (recT.baseTp # NIL) &         (* while base is known *)
          (recT.baseTp # anyRecTp) DO   (* while base # ANYREC *)
      base := recT.baseTp;
      WITH base : Record DO
        recT := base;
        idnt := base.symTb.lookup(hash);
      ELSE
        recT.baseTp := base.boundRecTp();
      END;
    END;
    AddIndirectImport(idnt);
    RETURN idnt;
  END bindField;

(* -------------------------------------------- *)

  PROCEDURE (desc : Procedure)OutCheck*(v : VarSets.VarSet);
    VAR idx : INTEGER;
        frm : Id.ParId;
        msg : POINTER TO FileNames.NameString;
  BEGIN
    msg := NIL;
    FOR idx := 0 TO desc.formals.tide - 1 DO
      frm := desc.formals.a[idx];
      IF (frm.parMod = Sy.out) & ~v.includes(frm.varOrd) THEN
        IF msg = NIL THEN
          NEW(msg);
          Sy.getName.Of(frm, msg);
        ELSE
          GPText.Assign(msg^ + "," + Sy.getName.ChPtr(frm)^, msg);
        END;
      END;
    END;
    IF msg # NIL THEN desc.TypeErrStr(139, msg) END;
  END OutCheck;

(* ============================================================ *)
(*       Record error reporting methods   *)
(* ============================================================ *)

  PROCEDURE (ty : Record)TypeError*(n : INTEGER);
  BEGIN
    IF ty.bindTp # NIL THEN
      ty.bindTp.TypeError(n);
    ELSE
      ty.TypeError^(n);
    END;
  END TypeError;

(* -------------------------------------------- *)

  PROCEDURE (ty : Record)TypeErrStr*(n : INTEGER;
          IN s : ARRAY OF CHAR);
  BEGIN
    IF ty.bindTp # NIL THEN
      ty.bindTp.TypeErrStr(n,s);
    ELSE
      ty.TypeErrStr^(n,s);
    END;
  END TypeErrStr;

(* ============================================================ *)
(*      Constructor methods     *)
(* ============================================================ *)

  PROCEDURE newBasTp*() : Base;
    VAR rslt : Base;
  BEGIN
    NEW(rslt);
    rslt.SetKind(basTp);
    RETURN rslt;
  END newBasTp;

(* ---------------------------- *)

  PROCEDURE newNamTp*() : Opaque;
    VAR rslt : Opaque;
  BEGIN
    NEW(rslt);
    rslt.SetKind(namTp);
    RETURN rslt;
  END newNamTp;

(* ---------------------------- *)

  PROCEDURE newTmpTp*() : Opaque;
    VAR rslt : Opaque;
  BEGIN
    NEW(rslt);
    rslt.SetKind(tmpTp);
    RETURN rslt;
  END newTmpTp;

(* ---------------------------- *)

  PROCEDURE newArrTp*() : Array;
    VAR rslt : Array;
  BEGIN
    NEW(rslt);
    rslt.SetKind(arrTp);
    RETURN rslt;
  END newArrTp;

  PROCEDURE mkArrayOf*(e : Sy.Type) : Array;
    VAR rslt : Array;
  BEGIN
    NEW(rslt);
    rslt.SetKind(arrTp);
    rslt.elemTp := e;
    RETURN rslt;
  END mkArrayOf;

(* ---------------------------- *)

  PROCEDURE newVecTp*() : Vector;
    VAR rslt : Vector;
  BEGIN
    NEW(rslt);
    rslt.SetKind(vecTp);
    RETURN rslt;
  END newVecTp;

  PROCEDURE mkVectorOf*(e : Sy.Type) : Vector;
    VAR rslt : Vector;
  BEGIN
    NEW(rslt);
    rslt.SetKind(vecTp);
    rslt.elemTp := e;
    RETURN rslt;
  END mkVectorOf;

(* ---------------------------- *)

  PROCEDURE newRecTp*() : Record;
    VAR rslt : Record;
  BEGIN
    NEW(rslt);
    rslt.SetKind(recTp);
    RETURN rslt;
  END newRecTp;

(* ---------------------------- *)

  PROCEDURE newEnuTp*() : Enum;
    VAR rslt : Enum;
  BEGIN
    NEW(rslt);
    rslt.SetKind(enuTp);
    RETURN rslt;
  END newEnuTp;

(* ---------------------------- *)

  PROCEDURE newPtrTp*() : Pointer;
    VAR rslt : Pointer;
  BEGIN
    NEW(rslt);
    rslt.SetKind(ptrTp);
    RETURN rslt;
  END newPtrTp;

  PROCEDURE mkPtrTo*(e : Sy.Type) : Pointer;
    VAR rslt : Pointer;
  BEGIN
    NEW(rslt);
    rslt.SetKind(ptrTp);
    rslt.boundTp := e;
    RETURN rslt;
  END mkPtrTo;

(* ---------------------------- *)

  PROCEDURE newEvtTp*() : Procedure;
    VAR rslt : Event;
  BEGIN
    NEW(rslt);
    rslt.SetKind(evtTp);
    rslt.bndRec := newRecTp();
    rslt.bndRec.bindTp := rslt;
    rslt.bndRec.baseTp := CSt.ntvEvt;
    RETURN rslt;
  END newEvtTp;

(* ---------------------------- *)

  PROCEDURE newPrcTp*() : Procedure;
    VAR rslt : Procedure;
  BEGIN
    NEW(rslt);
    rslt.SetKind(prcTp);
    RETURN rslt;
  END newPrcTp;

(* ---------------------------- *)

  PROCEDURE newOvlTp*() : Overloaded;
    VAR rslt : Overloaded;
  BEGIN
    NEW(rslt);
    rslt.SetKind(ovlTp);
    RETURN rslt;
  END newOvlTp;

(* ============================================================ *)
(*          Some Helper procedures      *)
(* ============================================================ *)

  PROCEDURE baseRecTp*(rec : Record) : Record;
  VAR
    base : Sy.Type;
  BEGIN
    IF (rec.baseTp = NIL) OR (rec.baseTp = anyRecTp) THEN RETURN NIL; END;
    base := rec.baseTp;
    WITH base : Record DO
      RETURN base;
    ELSE
      RETURN base.boundRecTp()(Record);
    END;
  END baseRecTp;

(* ---------------------------- *)

  PROCEDURE newOvlIdent*(id : Sy.Idnt; rec : Record) : Id.OvlId;
  VAR
    oId : Id.OvlId;
  BEGIN
    oId := Id.newOvlId();
    oId.type := newOvlTp();
    oId.hash := id.hash;
    oId.dfScp := id.dfScp;
    oId.type.idnt := oId;
    oId.rec := rec;
    WITH id : Id.Procs DO
      Id.AppendProc(oId.list,id);
    ELSE
      oId.fld := id;
    END;
    RETURN oId;
  END newOvlIdent;

(* ---------------------------- *)

  PROCEDURE needOvlId*(id : Id.Procs; rec : Record) : BOOLEAN;
  VAR
     ident : Sy.Idnt;
     base : Sy.Type;
  BEGIN
    rec := baseRecTp(rec);
    WHILE (rec # NIL) DO
      ident := rec.symTb.lookup(id.hash);
      IF ident # NIL THEN
        IF ident IS Id.OvlId THEN RETURN TRUE; END;
        IF ident IS Id.Procs THEN
          RETURN ~id.type(Procedure).formsMatch(ident.type(Procedure));
        END;
        (* allow declaration of new overloaded method *)
      END;
      rec := baseRecTp(rec);
    END;
    RETURN FALSE;
  END needOvlId;

(* ---------------------------- *)

  PROCEDURE GetInheritedFeature*(hsh : INTEGER; 
                             OUT id  : Sy.Idnt;
                             VAR rec : Record);
  BEGIN
    id := rec.symTb.lookup(hsh);
    WHILE (id = NIL) & (rec.baseTp # NIL) &
          (rec.baseTp # anyRecTp) & (rec.baseTp # anyPtrTp) DO
      rec := baseRecTp(rec);
      IF rec = NIL THEN RETURN; END;
      id := rec.symTb.lookup(hsh);
    END;
  END GetInheritedFeature;

(* ---------------------------- *)

  PROCEDURE findOverriddenProc*(proc : Id.Procs) : Id.Procs;
  VAR
    id : Sy.Idnt;
    rec : Record;
    ty : Sy.Type;
    pId : Id.Procs;
  BEGIN
    ty := proc.type.boundRecTp();
    IF ty = NIL THEN RETURN NIL; END;
    rec := baseRecTp(ty(Record));
    WHILE (rec # NIL) & (rec # anyRecTp) & (rec # anyPtrTp) DO
      id := rec.symTb.lookup(proc.hash);
      WITH id : Id.OvlId DO
        pId := id.findProc(proc);
        IF pId # NIL THEN RETURN pId; END;
      | id : Id.Procs DO
        IF proc.type.sigsMatch(id.type) THEN RETURN id; END;
        RETURN NIL;
      ELSE
        RETURN NIL;
      END;
      IF (rec.baseTp = NIL) THEN
        rec := NIL;
      ELSE
        rec := baseRecTp(rec);
      END;
    END;
    RETURN NIL;
  END findOverriddenProc;

(* ---------------------------- *)

  PROCEDURE AddToOvlIdent(id : Sy.Idnt; oId : Id.OvlId; doKindCheck : BOOLEAN;
                          VAR ok : BOOLEAN);
  BEGIN
    ok := TRUE;
    WITH id : Id.Procs DO
      Id.AppendProc(oId.list,id);
    ELSE
      IF oId.fld = NIL THEN 
        oId.fld := id; 
      ELSE
        ok := (doKindCheck & (oId.fld.kind = id.kind));
      END;
    END;
  END AddToOvlIdent;

(* ---------------------------- *)

  PROCEDURE isBoxedStruct*(ptr : Sy.Type; dst : Sy.Type) : BOOLEAN;
  BEGIN
    RETURN ptr.isNativeObj() & dst.isRecordType() & ~dst.isExtnRecType();
  END isBoxedStruct;

(* ---------------------------- *)

  PROCEDURE InsertInRec*(id  : Sy.Idnt; 
                         rec : Record; 
                         doKindCheck : BOOLEAN; 
                     OUT oId : Id.OvlId; 
                     OUT ok  : BOOLEAN); 
  VAR
    existingId : Sy.Idnt;
    recScp : Record;

  BEGIN
    oId := NIL;
    ok  := TRUE;
    recScp := rec;
    GetInheritedFeature(id.hash, existingId, recScp);
   (*
    *  If existingId = NIL (the usual case) all is ok.
    *)
    IF (Sy.isFn IN rec.xAttr) & (existingId # NIL) THEN
     (*
      *  This is a foreign record, so that different rules
      *  apply.  Overloading is ok, and obscuring of 
      *  inherited field by local fields is allowed.
      *)
      IF recScp = rec THEN
       (* 
        *  The ident is for the same scope :
        *  - if it is a method, and has same params then ... ok,
        *  - else this is an overload, and must be marked,
        *  - else if this is the same kind, then ... ok,
        *  - else this is an error.
        *)
        WITH existingId : Id.Procs DO
            IF ~existingId.type.sigsMatch(id.type) THEN
              oId := newOvlIdent(existingId,rec);
              AddToOvlIdent(id,oId,doKindCheck,ok);
              rec.symTb.Overwrite(oId.hash,oId);
            END; (* and ok stays true! *)
(*
 *      | existingId : Id.FldId DO
 *)
        | existingId : Id.AbVar DO
            oId := newOvlIdent(existingId,rec);
            AddToOvlIdent(id,oId,doKindCheck,ok);
            rec.symTb.Overwrite(oId.hash,oId);
        | existingId : Id.OvlId DO
            oId := existingId;
            AddToOvlIdent(id,existingId,doKindCheck,ok);
        ELSE
         (*
          *  Check if this is actually the same feature
          *)
          IF existingId.type IS Opaque THEN existingId.type := id.type;
          ELSIF id.type IS Opaque THEN id.type := existingId.type;
          END;
          ok := (existingId.kind = id.kind) &
                 existingId.type.equalType(id.type);
        
        END;
      ELSE 
       (* 
        *  The ident is from enclosing scope : 
        *  - if it is a field ID then ... ok,
        *  - if it is a method, and has same params then ... ok,
        *  - else this is an overload, and must be marked.
        *)
        WITH existingId : Id.FldId DO
            ok := rec.symTb.enter(id.hash, id);
        | existingId : Id.Procs DO
            IF existingId.type.sigsMatch(id.type) THEN
              ok := rec.symTb.enter(id.hash, id);
            ELSE
              oId := newOvlIdent(id,rec);
              ok := rec.symTb.enter(oId.hash,oId);
            END;
        | existingId : Id.OvlId DO
            oId := existingId;
            AddToOvlIdent(id,existingId,doKindCheck,ok);
        ELSE (* must be a field *)
            ok := rec.symTb.enter(id.hash, id);
        END;
      END;
    ELSIF ~rec.symTb.enter(id.hash, id) THEN
      existingId := rec.symTb.lookup(id.hash);
      ok := doKindCheck & (existingId.kind = id.kind); 
    END;
  END InsertInRec;

(* ---------------------------- *)

  PROCEDURE Error145(start : Sy.Type);
    VAR sccTab : Sy.SccTable;
  BEGIN
    NEW(sccTab);
    sccTab.target := start;
    start.SccTab(sccTab);
    start.TypeErrStr(145, Sy.dumpList(sccTab.symTab));
  END Error145;

(* ============================================================ *)
(*    Implementation of Abstract methods    *)
(* ============================================================ *)

  PROCEDURE (i : Base)resolve*(d : INTEGER) : Sy.Type;
  BEGIN RETURN i END resolve;

(* ---------------------------- *)

  PROCEDURE (i : Enum)resolve*(d : INTEGER) : Sy.Type;
  BEGIN RETURN i END resolve;

(* ---------------------------- *)

  PROCEDURE (i : Opaque)resolve*(d : INTEGER) : Sy.Type;
    VAR newTpId : Sy.Idnt;
        oldTpId : Sy.Idnt;
  BEGIN
    IF i.depth = initialMark THEN
     (*
      *  If i.kind=tmpTp, this is a forward type, or
      *  a sym-file temporary.  If we cannot resolve
      *  this to a real type, it is an error.
      *
      *  If i.kind=namTp, this is a named opaque type,
      *  we must look it up in the symTab.  If we
      *  do not find it, the type just stays opaque.
      *)
      i.depth := finishMark;
      oldTpId := i.idnt;
      newTpId := oldTpId.dfScp.symTb.lookup(oldTpId.hash);
      IF newTpId = NIL THEN
        oldTpId.IdError(2);
      ELSIF newTpId.kind # Id.typId THEN
        oldTpId.IdError(5);
      ELSIF newTpId.type # NIL THEN
       (*
        *   This particular method might recurse, even for
        *   correct programs, such as
        * TYPE A = POINTER TO B;
        * TYPE B = RECORD c : C END;
        * TYPE C = RECORD(A) ... END;
        *   Thus we must not recurse until we have set the
        *   resolved field, since we have now set the depth
        *   mark and will not reenter the binding code again.
        *)
        i.resolved := newTpId.type;
        i.resolved := newTpId.type.resolve(d);  (* Recurse! *)
        IF i.kind = tmpTp THEN
          IF i.resolved = i THEN oldTpId.IdError(125) END;
        ELSIF i.kind = namTp THEN
          IF (i.resolved = NIL) OR
             (i.resolved.kind = namTp) THEN i.resolved := i END;
        END;
      END;
    END;
    RETURN i.resolved;
  END resolve;

(* ---------------------------- *)

  PROCEDURE (i : Array)resolve*(d : INTEGER) : Sy.Type, EXTENSIBLE;
    VAR e137,e145 : BOOLEAN;
  BEGIN
    IF i.depth = initialMark THEN
      e145 := FALSE;
      e137 := FALSE;
      i.depth := d;
      IF i.elemTp # NIL THEN i.elemTp := i.elemTp.resolve(d) END;
      IF (i.length # 0) &
         (i.elemTp # NIL) &
          i.elemTp.isOpenArrType() THEN
        i.TypeError(69);
      END;
      IF i.depth = errorMark THEN
        IF i.elemTp = i THEN e137 := TRUE ELSE e145 := TRUE END;
        i.TypeError(126);
      END;
      i.depth := finishMark;
      IF    e145 THEN Error145(i);
      ELSIF e137 THEN i.TypeError(137);
      END;
    ELSIF i.depth = d THEN (* recursion through value types *)
      i.depth := errorMark;
    END;
    RETURN i;
  END resolve;

(* ---------------------------- *)

  PROCEDURE (i : Vector)resolve*(d : INTEGER) : Sy.Type;
    VAR e137,e145 : BOOLEAN;
  BEGIN
    IF i.depth = initialMark THEN
      IF i.elemTp # NIL THEN i.elemTp := i.elemTp.resolve(d) END;
      i.depth := finishMark;
    END;
    RETURN i;
  END resolve;

(* ---------------------------- *)

  PROCEDURE (x: Record)CopyFieldsOf(b : Sy.Type),NEW; (* final *)
    VAR bRecT : Record;
        nextF : Sy.Idnt;
        index : INTEGER;
  BEGIN
    IF (b # anyRecTp) & (b.depth # errorMark) THEN
      bRecT := b.boundRecTp()(Record);
     (*
      *   First get the fields of the higher ancestors.
      *)
      IF bRecT.baseTp # NIL THEN x.CopyFieldsOf(bRecT.baseTp) END;
     (*
      *   Now add the fields of the immediate base type
      *)
      FOR index := 0 TO bRecT.fields.tide-1 DO
        nextF := bRecT.fields.a[index];
        IF ~x.symTb.enter(nextF.hash, nextF) & ~(Sy.isFn IN bRecT.xAttr) THEN
          x.symTb.lookup(nextF.hash).IdError(82);
        END;
      END;
    END;
  END CopyFieldsOf;

(* ---------------------------- *)

  PROCEDURE (i : Record)resolve*(d : INTEGER) : Sy.Type;
  (** Resolve this type, and any used in this type *)
    VAR baseT : Record;
        field : Sy.Idnt;
        index : INTEGER;
        hashN : INTEGER;
        nameS : Lv.CharOpen;
        ident : Sy.Idnt;
        intId : Sy.Idnt;
        intTp : Sy.Type;
        recId : Sy.Idnt;
        dBlk  : Id.BlkId;
		ntvNm : RTS.NativeString;
        e137,e145 : BOOLEAN;
   (* ----------------------------------------- *)
    PROCEDURE refInNET(t : Sy.Type) : BOOLEAN;
     (*
      *  This predicate is used for the .NET
      *  platform, to set the "clsTp" attribute.
      *  It implies that this type will have a
      *  reference representation in .NET
      *)
    BEGIN
      IF t = NIL THEN
          RETURN FALSE;     (* Actually we don't care here. *)
      ELSE 
        WITH t : Record DO
          RETURN Sy.clsTp IN t.xAttr;
        | t : Array DO
          RETURN TRUE;      (* arrays are references in NET *)
        | t : Event DO
          RETURN TRUE;      (* events are references in NET *)
        ELSE RETURN FALSE;  (* all others are value types.  *)
        END;
      END;
    END refInNET;
   (* ----------------------------------------- *)
  BEGIN (* resolve *)
    IF i.depth = initialMark THEN

	  IF CSt.verbose THEN
  	    IF i.idnt # NIL THEN
	      ntvNm := Sy.getName.NtStr(i.idnt);
        ELSIF (i.bindTp # NIL) & (i.bindTp.idnt # NIL) THEN
	      ntvNm := Sy.getName.NtStr(i.bindTp.idnt);
        END;
      END;
      i.depth := d;
      e145 := FALSE;
      e137 := FALSE;
     (*
      *  First: resolve the base type, if any,
      *  or set the base type to the type ANYREC.
      *)
      baseT := NIL;
      IF i.baseTp = NIL THEN
        i.baseTp := anyRecTp;
      ELSIF i.baseTp = anyPtrTp THEN
        i.baseTp := anyRecTp;
     (*
      *  Special case of baseTp of POINTER TO RTS.NativeObject ...
      *)
      ELSIF i.baseTp.isNativeObj() THEN
        IF i.baseTp IS Pointer THEN i.baseTp := i.baseTp.boundRecTp() END;
      ELSE (* the normal case *)
        i.baseTp := i.baseTp.resolve(d);
       (*
        *  There is a special case here. If the base type
        *  is an unresolved opaque from an unimported module
        *  then leave well alone.
        *)
        IF i.baseTp # NIL THEN
          IF i.baseTp IS Opaque THEN 
            i.baseTp := anyRecTp;
          ELSE 
            i.baseTp := i.baseTp.boundRecTp();
            IF i.baseTp IS Record THEN baseT := i.baseTp(Record) END;
            IF i.baseTp = NIL THEN i.TypeError(14) END; (* not rec or ptr *)
            IF i.depth = errorMark THEN
              IF i.baseTp = i THEN e137 := TRUE ELSE e145 := TRUE END;
              i.TypeError(123);
            END;
          END;
        END;
        IF baseT # NIL THEN
         (*
          *  Base is resolved, now check some semantic constraints.
          *)
          IF (isAbs = i.recAtt) &
             ~baseT.isAbsRecType() &
             ~(Sy.isFn IN baseT.xAttr) THEN
            i.TypeError(102); (* abstract record must have abstract base *)
          ELSIF baseT.isExtnRecType() THEN
            i.CopyFieldsOf(baseT);
            IF Sy.noNew IN baseT.xAttr THEN INCL(i.xAttr, Sy.noNew) END;
(* ----- Code for extensible limited records ----- *)
          ELSIF baseT.isLimRecType() THEN
            IF ~i.isLimRecType() THEN
              i.TypeError(234); (* abstract record must have abstract base *)
            ELSIF i.isImportedType() # baseT.isImportedType() THEN
              i.TypeError(235); (* abstract record must have abstract base *)
            END;
(* --- End code for extensible limited records --- *)
          ELSIF baseT.isInterfaceType() THEN
            i.TypeErrStr(154, baseT.name()); (* cannot extend interfaces *)
          ELSE
            i.TypeError(16);  (* base type is not an extensible record   *)
          END;
          IF (iFace = i.recAtt) &
             ~baseT.isNativeObj() THEN i.TypeError(156) END;
         (*
          *  Propagate no-block-copy attribute to extensions.
          *  Note the special case here: in .NET extensions 
          *  of System.ValueType may be copied freely.
          *)
          IF (Sy.noCpy IN baseT.xAttr) &
             (baseT # CSt.ntvVal) THEN INCL(i.xAttr, Sy.noCpy) END;
        END;
      END;
     (*
      *   Interface types must be exported.
      *)
      IF i.recAtt = iFace THEN
        IF i.idnt # NIL THEN
          IF i.idnt.vMod = Sy.prvMode THEN i.TypeError(215) END;
        ELSIF (i.bindTp # NIL) & (i.bindTp.idnt # NIL) THEN
          IF i.bindTp.idnt.vMod = Sy.prvMode THEN i.TypeError(215) END;
        ELSE
          i.TypeError(214);
        END;
      END;
     (*
      *   Now check semantics of interface implementation.
      *)
      IF (i.interfaces.tide > 0) & (baseT # NIL) THEN
(*
 *      (* Use this code to allow only direct foreign types. *)
 *      IF ~(Sy.isFn IN baseT.xAttr) &
 *         ~i.isImportedType() THEN i.TypeErrStr(157, baseT.name()) END;
 *)

(*
 *      (* Use this code to allow only extensions of foreign types. *)
 *      IF ~(Sy.noCpy IN baseT.xAttr) &
 *         ~i.isImportedType() THEN i.TypeErrStr(157, baseT.name()) END;
 *)
        (* Remove both to allow all code to define interfaces *)

        FOR index := 0 TO i.interfaces.tide-1 DO
          intTp := i.interfaces.a[index].resolve(d);
          IF intTp # NIL THEN
            intTp := intTp.boundRecTp();
            IF (intTp # NIL) &
               ~intTp.isInterfaceType() THEN
              i.TypeErrStr(158, intTp.name());
            END;
          END;
        END;
      END;
      i.depth := d;
     (*
      *  Next: set basis of no-block-copy flag
      *)
      IF (Sy.isFn IN i.xAttr) &
         (Sy.clsTp IN i.xAttr) THEN INCL(i.xAttr, Sy.noCpy);
      END;
     (*
      *  Next: resolve all field types.
      *)
      FOR index := 0 TO i.fields.tide-1 DO
        field := i.fields.a[index];
        IF field.type # NIL THEN field.type := field.type.resolve(d) END;
        IF i.depth = errorMark THEN
          IF field.type = i THEN e137 := TRUE ELSE e145 := TRUE END;
          field.IdError(124);
          i.depth := d;
        END;
        IF refInNET(field.type) THEN INCL(i.xAttr,Sy.clsTp) END;
        IF field.type IS Event THEN Sy.AppendIdnt(i.events, field) END;
      END;

     (*
      *  Next: resolve all method types.       NEW!
      *)
      FOR index := 0 TO i.methods.tide-1 DO
        field := i.methods.a[index];
        IF field.type # NIL THEN field.type := field.type.resolve(d) END;
      END;

     (*
      *  Next: resolve types of all static members.
      *)
      FOR index := 0 TO i.statics.tide-1 DO
        field := i.statics.a[index];
        IF field.type # NIL THEN field.type := field.type.resolve(d) END;
      END;

      i.depth := finishMark;
      IF    e145 THEN Error145(i);
      ELSIF e137 THEN i.TypeError(137);
      END;
    ELSIF i.depth = d THEN (* recursion through value types *)
      i.depth := errorMark;
    END;
   (* ##### *)
    dBlk := i.defBlk();
    IF (dBlk # NIL) & (Sy.weak IN dBlk.xAttr) THEN INCL(i.xAttr, Sy.weak) END;
   (* ##### *)
    RETURN i;
  END resolve;

(* ---------------------------- *)

  PROCEDURE (i : Record)FixDefScope*(s : Sy.Scope),NEW;
    VAR idx : INTEGER;
        idD : Sy.Idnt;
  BEGIN
    FOR idx := 0 TO i.methods.tide-1 DO
      idD := i.methods.a[idx];
      IF idD.dfScp # s THEN
        idD.dfScp := s;
        IF CSt.verbose THEN
           Console.WriteString("Fixing method module:");
           Console.WriteString(Sy.getName.ChPtr(idD));
           Console.WriteLn;
        END;
      ELSE
        RETURN
      END;
    END;
    FOR idx := 0 TO i.statics.tide-1 DO
      idD := i.statics.a[idx];
      IF idD.dfScp # s THEN
        idD.dfScp := s;
        IF CSt.verbose THEN
          Console.WriteString("Fixing static module:");
          Console.WriteString(Sy.getName.ChPtr(idD));
          Console.WriteLn;
        END;
      ELSE
        RETURN
      END;
    END;
  END FixDefScope;

(* ---------------------------- *)

  PROCEDURE (i : Pointer)resolve*(d : INTEGER) : Sy.Type;
    VAR bndT : Sy.Type;
  BEGIN
    IF i.depth = initialMark THEN
      i.depth := d;
      bndT := i.boundTp;
      IF (bndT # NIL) &           (*==> bound type is OK  *)
         (bndT.idnt = NIL) THEN   (*==> anon. bound type  *)
        WITH bndT : Record DO
          IF bndT.bindTp = NIL THEN
            INCL(bndT.xAttr, Sy.clsTp);
            INCL(bndT.xAttr, Sy.anon);
            IF i.idnt # NIL THEN    (*==> named ptr type  *)
             (*
              *  The anon record should have the same name as the
              *  pointer type.  The record is marked so that the
              *  synthetic name <ptrName>"^" can be derived.
              *  The visibility mode is the same as the pointer.
              *)
              bndT.bindTp := i;
            END;
          END;
          IF bndT.isForeign() THEN bndT.FixDefScope(i.idnt.dfScp) END;
        ELSE (* skip pointers to arrays *)
        END;
      END;
      IF bndT # NIL THEN 
        i.boundTp := bndT.resolve(d+1);
        IF (i.boundTp # NIL) &
           ~(i.boundTp IS Array) &
           ~(i.boundTp IS Record) THEN i.TypeError(140) END;
      END;
      i.depth := finishMark;
    END;
    RETURN i;
  END resolve;

(* ---------------------------- *)

  PROCEDURE (i : Procedure)resolve*(d : INTEGER) : Sy.Type;
    VAR idx : INTEGER;
        frm : Sy.Idnt;
  BEGIN
    IF i.depth = initialMark THEN
      i.depth := d;
      FOR idx := 0 TO i.formals.tide-1 DO
        frm := i.formals.a[idx];
        IF frm.type # NIL THEN frm.type := frm.type.resolve(d+1) END;
      END;

      IF i.retType # NIL THEN i.retType := i.retType.resolve(d+1) END;
      i.depth := finishMark;
    END;
    RETURN i
  END resolve;

(* ---------------------------- *)

  PROCEDURE (i : Overloaded)resolve*(d : INTEGER) : Sy.Type;
  BEGIN
    ASSERT(FALSE);
    RETURN NIL;
  END resolve;

(* ---------------------------- *)

  PROCEDURE (i : Opaque)elaboration*() : Sy.Type;
  BEGIN
    IF i.resolved # NIL THEN RETURN i.resolved ELSE RETURN i END;
  END elaboration;

(* ============================================================ *)

  PROCEDURE (i : Base)TypeErase*() : Sy.Type;
  BEGIN RETURN i END TypeErase;

(* ---------------------------- *)

  PROCEDURE (i : Enum)TypeErase*() : Sy.Type;
  BEGIN RETURN i END TypeErase;

(* ---------------------------- *)

  PROCEDURE (i : Opaque)TypeErase*() : Sy.Type;
  BEGIN RETURN i END TypeErase;

(* ---------------------------- *)

  PROCEDURE (i : Array)TypeErase*() : Sy.Type;
  BEGIN RETURN i END TypeErase;

(* ---------------------------- *)

  PROCEDURE (i : Record)TypeErase*() : Sy.Type;
  (* If the Record type is a compound type, return
   * its implementation type, otherwise erase the types
   * from the fields and methods of the record *)
    VAR
      index : INTEGER;
      id    : Sy.Idnt;
    BEGIN
      IF i.isCompoundType() THEN
        RETURN i.ImplementationType();
      END;

      (* Process the fields *)
      FOR index := 0 TO i.fields.tide-1 DO
        id := i.fields.a[index];
        IF id.type # NIL THEN
          i.fields.a[index].type := id.type.TypeErase();
        END;
      END;

      (* Process the methods *)
      FOR index := 0 TO i.methods.tide-1 DO
        id := i.methods.a[index];
        IF id.type # NIL THEN
          i.methods.a[index].type := id.type.TypeErase();
        END;
      END;

      RETURN i;
  END TypeErase;

(* ---------------------------- *)

  PROCEDURE (i : Pointer)TypeErase*() : Sy.Type;
    (* Erase the bound type *)
    VAR bndT : Sy.Type;
  BEGIN
    bndT := i.boundTp;
    IF (bndT # NIL) THEN
      i.boundTp := bndT.TypeErase();
    END;
    RETURN i;
  END TypeErase;

(* ---------------------------- *)

  PROCEDURE (i : Procedure)TypeErase*() : Sy.Type;
  (* Erase the types of the formals *)
    VAR
      index : INTEGER;
      id    : Sy.Idnt;
    BEGIN
      (* Process the fields *)
      FOR index := 0 TO i.formals.tide-1 DO
        id := i.formals.a[index];
        IF id.type # NIL THEN
          i.formals.a[index].type := id.type.TypeErase();
        END;
      END;
    RETURN i
  END TypeErase;

(* ---------------------------- *)

  PROCEDURE (i : Overloaded)TypeErase*() : Sy.Type;
  BEGIN RETURN i END TypeErase;

(* ============================================================ *)

  PROCEDURE Insert(VAR s : Sy.SymbolTable; t : Sy.Type);
    VAR junk : BOOLEAN;
  BEGIN
    IF t.idnt # NIL THEN junk := s.enter(t.idnt.hash, t.idnt) END;
  END Insert;

(* ---------------------------------------------------- *)

  PROCEDURE (i : Array)SccTab*(t : Sy.SccTable);
  BEGIN
    i.depth := initialMark;
    t.reached := FALSE;
    IF i.elemTp # NIL THEN
      IF i.elemTp = t.target THEN
        t.reached := TRUE;
      ELSIF i.elemTp.depth # initialMark THEN
        t.reached := FALSE;
        i.elemTp.SccTab(t);
      END;
      IF t.reached THEN Insert(t.symTab, i) END;
    END;
    i.depth := finishMark;
  END SccTab;

(* ---------------------------------------------------- *)

  PROCEDURE (i : Record)SccTab*(t : Sy.SccTable);
    VAR index : INTEGER;
        found : BOOLEAN;
        field : Sy.Idnt;
        fldTp : Sy.Type;
  BEGIN
    i.depth := initialMark;
    found := FALSE;
    IF i.baseTp # NIL THEN
      fldTp := i.baseTp;
      IF fldTp = t.target THEN
        t.reached := TRUE;
      ELSIF fldTp.depth # initialMark THEN
        t.reached := FALSE;
        fldTp.SccTab(t);
      END;
      IF t.reached THEN found := TRUE END;
    END;
    FOR index := 0 TO i.fields.tide-1 DO
      field := i.fields.a[index];
      fldTp := field.type;
      IF fldTp # NIL THEN
        IF fldTp = t.target THEN
          t.reached := TRUE;
        ELSIF fldTp.depth # initialMark THEN
          t.reached := FALSE;
          fldTp.SccTab(t);
        END;
        IF t.reached THEN found := TRUE END;
      END;
    END;
    IF found THEN Insert(t.symTab, i); t.reached := TRUE END;
    i.depth := finishMark;
  END SccTab;

(* ---------------------------------------------------- *)

  PROCEDURE (i : Base)SccTab*(t : Sy.SccTable);
  BEGIN (* skip *) END SccTab;

(* ---------------------------------------------------- *)

  PROCEDURE (i : Opaque)SccTab*(t : Sy.SccTable);
  BEGIN (* skip *) END SccTab;

(* ---------------------------------------------------- *)

  PROCEDURE (i : Pointer)SccTab*(t : Sy.SccTable);
  BEGIN (* skip *) END SccTab;

(* ---------------------------------------------------- *)

  PROCEDURE (i : Enum)SccTab*(t : Sy.SccTable);
  BEGIN (* skip *) END SccTab;

(* ---------------------------------------------------- *)

  PROCEDURE (i : Procedure)SccTab*(t : Sy.SccTable);
  BEGIN (* skip *) END SccTab;

(* ---------------------------------------------------- *)

  PROCEDURE (i : Overloaded)SccTab*(t : Sy.SccTable);
  BEGIN ASSERT(FALSE); END SccTab;

(* ============================================================ *)

  PROCEDURE update*(IN a : Sy.TypeSeq; t : Sy.Type) : Sy.Type;
  BEGIN
    IF t.dump-Sy.tOffset >= a.tide THEN
      Console.WriteInt(t.dump,0);
      Console.WriteInt(a.tide+Sy.tOffset,0);
      Console.WriteLn;
    END;
    IF t.kind = tmpTp THEN RETURN a.a[t.dump - Sy.tOffset] ELSE RETURN t END;
  END update;

(* ============================================================ *)

  PROCEDURE (t : Base)TypeFix*(IN a : Sy.TypeSeq);
  BEGIN END TypeFix;

(* ---------------------------- *)

  PROCEDURE (t : Enum)TypeFix*(IN a : Sy.TypeSeq);
  BEGIN END TypeFix;

(* ---------------------------- *)

  PROCEDURE (t : Opaque)TypeFix*(IN a : Sy.TypeSeq);
  BEGIN END TypeFix;

(* ---------------------------- *)

  PROCEDURE (t : Array)TypeFix*(IN a : Sy.TypeSeq);
  BEGIN
    t.elemTp := update(a, t.elemTp);
  END TypeFix;

(* ---------------------------- *)

  PROCEDURE (t : Record)TypeFix*(IN a : Sy.TypeSeq);
    VAR i : INTEGER;
        f : Sy.Idnt;
        m : Id.MthId;
        b : Sy.Type;
  BEGIN
    IF t.baseTp # NIL THEN
      IF t.baseTp IS Pointer THEN t.baseTp := t.baseTp.boundRecTp() END;
      t.baseTp := update(a, t.baseTp);
    END;
    FOR i := 0 TO t.interfaces.tide - 1 DO
      b := t.interfaces.a[i];
      t.interfaces.a[i] := update(a, b);
    END;
    FOR i := 0 TO t.fields.tide - 1 DO
      f := t.fields.a[i];
      f.type := update(a, f.type);
    END;
    FOR i := 0 TO t.methods.tide - 1 DO
      f := t.methods.a[i];
      m := f(Id.MthId);
      m.bndType := update(a, m.bndType);
      b := update(a, m.rcvFrm.type);
      m.rcvFrm.type := b;
      f.type.TypeFix(a);  (* recurse to param-types etc.  *)
    END;
    FOR i := 0 TO t.statics.tide - 1 DO
      f := t.statics.a[i];
      f.type := update(a, f.type);
      IF f.type IS Procedure THEN f.type.TypeFix(a) END;
    END;
  END TypeFix;

(* ---------------------------- *)

  PROCEDURE (t : Pointer)TypeFix*(IN a : Sy.TypeSeq);
    VAR bndT : Sy.Type;
  BEGIN
    bndT := update(a, t.boundTp);
    t.boundTp := bndT;
    IF  bndT.idnt = NIL THEN
      WITH bndT : Record DO
        INCL(bndT.xAttr, Sy.clsTp);
        INCL(bndT.xAttr, Sy.anon);
        IF bndT.bindTp = NIL THEN bndT.bindTp := t END;
      ELSE (* ptr to array : skip *)
      END;
    END;
  END TypeFix;

(* ---------------------------- *)

  PROCEDURE (t : Procedure)TypeFix*(IN a : Sy.TypeSeq);
    VAR i : INTEGER;
        f : Id.ParId;
  BEGIN
    IF t.retType # NIL  THEN t.retType := update(a, t.retType) END;
    IF t.receiver # NIL THEN t.receiver := update(a, t.receiver) END;
    FOR i := 0 TO t.formals.tide - 1 DO
      f := t.formals.a[i];
      f.type := update(a, f.type);
    END;
  END TypeFix;

(* ---------------------------- *)

  PROCEDURE (t : Overloaded)TypeFix*(IN a : Sy.TypeSeq);
  BEGIN
    ASSERT(FALSE);
  END TypeFix;

(* ============================================================ *)
(*  A type is "forced", i.e. must have its type     *)
(*  structure emitted to the symbol file if it is any of ...  *)
(*  i   : a local type with an exported TypId,    *)
(*  ii  : an imported type with an exported local alias,  *)
(*  iii : the base-type of a forced record,     *)
(*      iv  : a type with value semantics.      *)
(*  Partly forced types have structure but not methods emitted. *)
(* ============================================================ *)

  PROCEDURE MarkModule(ty : Sy.Type);
  BEGIN
    IF (ty.idnt # NIL) & (ty.idnt.dfScp # NIL) THEN
      INCL(ty.idnt.dfScp(Id.BlkId).xAttr, Sy.need);
    END;
  END MarkModule;

(* ---------------------------- *)

  PROCEDURE (i : Base)ConditionalMark*();
  BEGIN END ConditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Enum)ConditionalMark*();
  BEGIN END ConditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Opaque)ConditionalMark*();
  BEGIN
    MarkModule(i);
  END ConditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Pointer)ConditionalMark*();
  BEGIN
    IF i.force = Sy.noEmit THEN
      IF ~i.isImportedType() THEN
        i.force := Sy.forced;
        i.boundTp.ConditionalMark();
      ELSE
        MarkModule(i);
      END;
    END;
  END ConditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Record)ConditionalMark*();
    VAR idx : INTEGER;
        fTp : Sy.Type;
   (* ---------------------------- *)
    PROCEDURE blockOf(r : Record) : Id.BlkId;
    BEGIN
      IF r.bindTp # NIL THEN
        RETURN r.bindTp.idnt.dfScp(Id.BlkId);
      ELSE
        RETURN r.idnt.dfScp(Id.BlkId);
      END;
    END blockOf;
   (* ---------------------------- *)
    PROCEDURE ForceInterfaces(r : Record);
      VAR i : INTEGER;
          p : Sy.Type;
    BEGIN
      FOR i := 0 TO r.interfaces.tide-1 DO
        p := r.interfaces.a[i];
        p.force := Sy.forced;
(*
 *      WITH p : Pointer DO p.boundTp.force := Sy.forced END;
 *)
        WITH p : Pointer DO p.boundTp.force := Sy.forced ELSE END;
      END;
    END ForceInterfaces;
   (* ---------------------------- *)
  BEGIN
    IF (i.force = Sy.noEmit) THEN
      IF i.isImportedType() THEN
        i.force := Sy.partEmit;
(*
 *      IF ~CSt.special THEN i.force := Sy.partEmit END;
 *)
        INCL(blockOf(i).xAttr, Sy.need);
        IF i.bindTp # NIL THEN i.bindTp.ConditionalMark() END;
        IF (i.baseTp # NIL) &
           ~(i.baseTp IS Base) THEN i.baseTp.ConditionalMark() END;
      ELSE
        i.force := Sy.forced;
        IF i.bindTp # NIL THEN i.bindTp.UnconditionalMark() END;

        IF (i.baseTp # NIL) & ~(i.baseTp IS Base) THEN 
          i.baseTp.UnconditionalMark();
(*
 *        IF CSt.special THEN
 *          i.baseTp.ConditionalMark();
 *        ELSE
 *          i.baseTp.UnconditionalMark();
 *        END;
 *)
        END;

(*
        IF (i.baseTp # NIL) &
           ~(i.baseTp IS Base) THEN i.baseTp.UnconditionalMark() END;
 *)
        IF (i.interfaces.tide > 0) &
            i.isInterfaceType() THEN ForceInterfaces(i) END;
      END;
      FOR idx := 0 TO i.fields.tide-1 DO
        fTp := i.fields.a[idx].type;
        fTp.ConditionalMark();
      END;
    END;
  END ConditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Array)ConditionalMark*();
  BEGIN
    IF (i.force = Sy.noEmit) THEN
      IF i.isImportedType() THEN
        INCL(i.idnt.dfScp(Id.BlkId).xAttr, Sy.need);
      ELSE
        i.force := Sy.forced;
        i.elemTp.ConditionalMark();
      END;
    END;
  END ConditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Procedure)ConditionalMark*();
  BEGIN
  END ConditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Overloaded)ConditionalMark*();
  BEGIN
    ASSERT(FALSE);
  END ConditionalMark;

(* ============================================================ *)
(*   Rules for unconditional marking don't care about imports.  *)
(* ============================================================ *)

  PROCEDURE (i : Base)UnconditionalMark*();
  BEGIN END UnconditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Opaque)UnconditionalMark*();
  BEGIN
    MarkModule(i);
  END UnconditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Enum)UnconditionalMark*();
  BEGIN
    MarkModule(i);
  END UnconditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Pointer)UnconditionalMark*();
  BEGIN
    i.boundTp.ConditionalMark();
    IF (i.force # Sy.forced) THEN
      i.force := Sy.forced;
      i.boundTp.ConditionalMark();
      MarkModule(i);
    END;
  END UnconditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Record)UnconditionalMark*();
    VAR idx : INTEGER;
        fTp : Sy.Type;
  BEGIN
    IF (i.force # Sy.forced) THEN
      i.force := Sy.forced;
      IF i.baseTp # NIL THEN i.baseTp.UnconditionalMark() END;
      IF i.bindTp # NIL THEN i.bindTp.UnconditionalMark() END;
      FOR idx := 0 TO i.fields.tide-1 DO
        fTp := i.fields.a[idx].type;
        fTp.ConditionalMark();
      END;
      MarkModule(i);
    END;
  END UnconditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Array)UnconditionalMark*();
  BEGIN
    IF (i.force # Sy.forced) THEN
      i.force := Sy.forced;
      i.elemTp.ConditionalMark();
      MarkModule(i);
    END;
  END UnconditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Procedure)UnconditionalMark*();
  BEGIN
  END UnconditionalMark;

(* ---------------------------- *)

  PROCEDURE (i : Overloaded)UnconditionalMark*();
  BEGIN
    ASSERT(FALSE);
  END UnconditionalMark;

(* ============================================================ *)

  PROCEDURE (i : Pointer)superType*() : Sy.Type;
  BEGIN
    IF i.boundTp = NIL THEN RETURN NIL ELSE RETURN i.boundTp.superType() END;
  END superType;

(* ---------------------------- *)

  PROCEDURE (i : Record)superType*() : Record;
    VAR valRec : BOOLEAN;
        baseT  : Sy.Type;
        baseR  : Record;
  BEGIN
    valRec := ~(Sy.clsTp IN i.xAttr);
    baseR := NIL;
    baseT := i.baseTp;
    IF valRec THEN
      baseR := CSt.ntvVal(Record);
    ELSIF ~baseT.isNativeObj() THEN
      WITH baseT : Record DO
        baseR := baseT;
      ELSE (* skip *)
      END;
    END; 
    RETURN baseR;
  END superType;

(* ---------------------------- *)

  PROCEDURE (i : Procedure)superType*() : Sy.Type;
  BEGIN
    RETURN NIL (* for the moment *)
  END superType;

(* ============================================================ *)

  PROCEDURE (i : Pointer)boundRecTp*() : Sy.Type;
  BEGIN
    IF i.boundTp = NIL THEN RETURN NIL ELSE RETURN i.boundTp.boundRecTp() END;
  END boundRecTp;

(* ---------------------------- *)

  PROCEDURE (i : Record)boundRecTp*() : Sy.Type;
  BEGIN
    RETURN i;
  END boundRecTp;

(* ---------------------------- *)

  PROCEDURE (i : Event)boundRecTp*() : Sy.Type;
  BEGIN
    RETURN i.bndRec;
  END boundRecTp;

(* ---------------------------- *)

  PROCEDURE (i : Opaque)boundRecTp*() : Sy.Type;
  BEGIN
    IF (i.resolved = NIL) OR
       (i.resolved IS Opaque) THEN
      RETURN NIL;
    ELSE
      RETURN i.resolved.boundRecTp();
    END;
  END boundRecTp;

(* ============================================================ *)

  PROCEDURE (rec : Record)InsertMethod*(m : Sy.Idnt);
    VAR fwd : Sy.Idnt;
        mth : Id.MthId;
        ovl : Id.OvlId;
  BEGIN
    mth := m(Id.MthId);
    IF ~rec.symTb.enter(m.hash, m) THEN (* refused *)
      fwd := rec.symTb.lookup(m.hash);
      IF fwd IS Id.OvlId THEN
        ovl := fwd(Id.OvlId);
        fwd := ovl.findProc(mth);
        IF fwd = NIL THEN fwd := ovl; END;
      END;
      IF fwd.kind = Id.fwdMth THEN
        mth.CheckElab(fwd);
        rec.symTb.Overwrite(m.hash, m);
      ELSIF fwd.kind = Id.fwdPrc THEN
        fwd.IdError(63);
      ELSIF fwd IS Id.OvlId THEN
        ovl := fwd(Id.OvlId);
        (* currently disallow declaration of new overloaded method *)
        (* for name which is already overloaded *)
        fwd := findOverriddenProc(mth);
        IF fwd # NIL THEN
          Id.AppendProc(fwd(Id.OvlId).list,mth);
        ELSE
          m.IdErrorStr(207, rec.name());
        END;
      (* currently disallow declaration of new overloaded method *)
      (* for name which is NOT currently overloaded *)
      ELSE
        m.IdErrorStr(207, rec.name());
      END;
    ELSIF (Sy.noCpy IN rec.xAttr) & needOvlId(mth,rec) THEN
      ovl := newOvlIdent(mth,rec);
      rec.symTb.Overwrite(ovl.hash, ovl);
    END;
   (* 
    *  Special attribute processing for implement-only methods.
    *)
    IF (mth.kind = Id.conMth) &
       (*  (mth.vMod = Sy.rdoMode) & *)
       ~(Id.newBit IN mth.mthAtt) THEN
      fwd := rec.inheritedFeature(mth);
     (*
      * Console.WriteString("Checking callable ");
      * Console.WriteString(rec.name());
      * Console.WriteString("::");
      * Console.WriteString(Sy.getName.ChPtr(mth));
      * Console.WriteLn;
      *)
      IF (fwd # NIL) & fwd(Id.MthId).callForbidden() THEN
        INCL(mth.mthAtt, Id.noCall);
       (*
        * Console.WriteString("Marking noCall on ");
        * Console.WriteString(rec.name());
        * Console.WriteString("::");
        * Console.WriteString(Sy.getName.ChPtr(mth));
        * Console.WriteLn;
        *)
      END;
    END;
    Sy.AppendIdnt(rec.methods, m);
  END InsertMethod;

(* ---------------------------- *)

  PROCEDURE (bas : Record)superCtor*(pTp : Procedure) : Id.PrcId,NEW;
    VAR inx : INTEGER;
        stI : Sy.Idnt;
  BEGIN
    FOR inx := 0 TO bas.statics.tide-1 DO
      stI := bas.statics.a[inx];
      IF (stI.kind = Id.ctorP) &
         pTp.formsMatch(stI.type(Procedure)) THEN RETURN stI(Id.PrcId) END;
    END;
    RETURN NIL;
  END superCtor;


  PROCEDURE (rec : Record)AppendCtor*(p : Sy.Idnt);
    VAR prc : Id.Procs;
   (* ----------------------------- *)
    PROCEDURE onList(IN lst : Sy.IdSeq; proc : Id.Procs) : BOOLEAN;
      VAR inx : INTEGER;
          stI : Sy.Idnt;
          pTp : Procedure;
    BEGIN
      pTp := proc.type(Procedure);
     (*
      *  Return true if the proc is already on the list.
      *  Signal error if a different matching proc exists.
      *
      *  The matching constructor in the list could
      *  have any name.  So we simply search the list
      *  looking for *any* constructor which matches.
      *)
      FOR inx := 0 TO lst.tide-1 DO
        stI := lst.a[inx];
        IF (stI.kind = Id.ctorP) & pTp.formsMatch(stI.type(Procedure)) THEN 
          IF stI = proc THEN RETURN TRUE ELSE proc.IdError(148) END;
        END;
      END;
      RETURN FALSE;
    END onList;
   (* ----------------------------- *)
    PROCEDURE mustList(recT : Record; proc : Id.Procs) : BOOLEAN;
      VAR prcT : Procedure;
          prcN : INTEGER;
          base : Sy.Type;
          list : BOOLEAN;
    BEGIN
      prcT := proc.type(Procedure);
      base := recT.baseTp;
     (*
      *  Check for duplicate constructors with same signature
      *)
      list := onList(recT.statics, proc);
      IF (proc.basCll = NIL) OR 
         (proc.basCll.actuals.tide = 0) THEN 
       (*
        *  Trying to call the noarg constructor
        *  of the super type.
        *)
        prcN := prcT.formals.tide;
        WITH base : Record DO
           (*
            *  This is allowed, unless the noNew flag is set 
            *  in the supertype.
            *)
            IF Sy.noNew IN base.xAttr THEN proc.IdError(203) END;
            RETURN ~list & (prcN # 0); (* never list a no-arg constructor *)
        | base : Base DO
           (*
            *  This record extends the ANYREC type.  As
            *  a concession we allow no-arg constructors. 
            *)
            RETURN ~list & (prcN # 0); (* never list a no-arg constructor *)
        END;
      ELSE 
       (*
        *  This calls an explicit constructor.
        *)
        RETURN ~list & (proc.basCll.sprCtor # NIL);
      END;
    END mustList;
   (* ----------------------------- *)
  BEGIN
    prc := p(Id.Procs);
   (*
    *  First, we must check that there is a super
    *  constructor with the correct signature.
    *)
    IF mustList(rec, prc) THEN Sy.AppendIdnt(rec.statics, p) END;
    IF prc.body # NIL THEN prc.body.StmtAttr(prc) END;;
    IF prc.rescue # NIL THEN prc.rescue.StmtAttr(prc) END;;
  END AppendCtor;

(* ---------------------------- *)

  PROCEDURE (i : Procedure)boundRecTp*() : Sy.Type, EXTENSIBLE;
  BEGIN
    IF i.receiver = NIL THEN RETURN NIL ELSE RETURN i.receiver.boundRecTp() END
  END boundRecTp;

(* ============================================================ *)

  PROCEDURE (i : Record)inheritedFeature*(id : Sy.Idnt) : Sy.Idnt;
  VAR
    rec : Record;
    idnt : Sy.Idnt;
  BEGIN
    rec := i; idnt := NIL;
    rec := baseRecTp(rec);
    WHILE (idnt = NIL) & (rec # NIL) DO
      idnt := rec.symTb.lookup(id.hash);
      IF (idnt # NIL) & (idnt IS Id.OvlId) & (id IS Id.Procs) THEN
        idnt := idnt(Id.OvlId).findProc(id(Id.Procs));
      END;
      rec := baseRecTp(rec);
    END;
    RETURN idnt;
  END inheritedFeature;

(* ============================================================ *)
(*                     Diagnostic methods                       *)
(* ============================================================ *)

  PROCEDURE (s : Base)Diagnose*(i : INTEGER);
  BEGIN
    s.SuperDiag(i);
  END Diagnose;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Opaque)Diagnose*(i : INTEGER);
    VAR name : Lv.CharOpen;
  BEGIN
    s.SuperDiag(i);
    IF s.resolved # NIL THEN
      name := s.resolved.name();
      H.Indent(i+2); Con.WriteString("alias of " + name^);
      s.resolved.SuperDiag(i+2);
    ELSE
      H.Indent(i+2); Con.WriteString("opaque not resolved"); Con.WriteLn;
    END;
  END Diagnose;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Array)Diagnose*(i : INTEGER);
  BEGIN
    s.SuperDiag(i);
    H.Indent(i+2); Con.WriteString("Element type");
    IF s.elemTp # NIL THEN
      Con.WriteLn;
      s.elemTp.Diagnose(i+2);
    ELSE
      Con.WriteString(" NIL"); Con.WriteLn;
    END;
  END Diagnose;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Record)Diagnose*(i : INTEGER);
    VAR ix : INTEGER;
        id : Sy.Idnt;
        nm : FileNames.NameString;
  BEGIN
    s.SuperDiag(i);
    CASE s.recAtt OF
    | isAbs : Console.WriteString(" ABSTRACT"); Console.WriteLn;
    | limit : Console.WriteString(" LIMITED");  Console.WriteLn;
    | extns : Console.WriteString(" EXTENSIBLE"); Console.WriteLn;
    | iFace : Console.WriteString(" INTERFACE");  Console.WriteLn;
    ELSE
    END;
    IF Sy.fnInf IN s.xAttr THEN
      Console.WriteString(" [foreign-interface]"); Console.WriteLn;
    ELSIF Sy.isFn IN s.xAttr THEN
      Console.WriteString(" [foreign-class]"); Console.WriteLn;
    END;
    H.Indent(i); Console.WriteString("fields"); Console.WriteLn;
    FOR ix := 0 TO s.fields.tide-1 DO
      id := s.fields.a[ix];
      IF id # NIL THEN id.Diagnose(i+4) END;
    END;
    IF CSt.verbose THEN
      H.Indent(i); Console.WriteString("methods"); Console.WriteLn;
      FOR ix := 0 TO s.methods.tide-1 DO
        id := s.methods.a[ix];
        IF id # NIL THEN id.Diagnose(i+4) END;
      END;
      H.Indent(i); Console.WriteString("names"); Console.WriteLn;
      s.symTb.Dump(i+4);
    END;
    IF s.baseTp # NIL THEN
      H.Indent(i); Console.WriteString("base type"); Console.WriteLn;
      s.baseTp.Diagnose(i+4);
    END;
    Sy.DoXName(i, s.xName);
    Sy.DoXName(i, s.extrnNm);
    Sy.DoXName(i, s.scopeNm);
  END Diagnose;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Enum)Diagnose*(i : INTEGER);
    VAR ix : INTEGER;
        id : Sy.Idnt;
        nm : FileNames.NameString;
  BEGIN
    s.SuperDiag(i);
    H.Indent(i); Console.WriteString("consts"); Console.WriteLn;
    FOR ix := 0 TO s.statics.tide-1 DO
      id := s.statics.a[ix];
      IF id # NIL THEN id.Diagnose(i+4) END;
    END;
    Sy.DoXName(i, s.xName);
  END Diagnose;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Pointer)Diagnose*(i : INTEGER);
  BEGIN
    s.SuperDiag(i);
    H.Indent(i+2); Con.WriteString("Bound type");
    IF s.boundTp # NIL THEN
      Con.WriteLn;
      s.boundTp.Diagnose(i+2);
    ELSE
      Con.WriteString(" NIL"); Con.WriteLn;
    END;
    Sy.DoXName(i, s.xName);
  END Diagnose;

(* ---------------------------------------------------- *)
  PROCEDURE^ qualname(id : Sy.Idnt) : Lv.CharOpen;
(* ---------------------------------------------------- *)

  PROCEDURE (s : Procedure)DiagFormalType*(i : INTEGER);
    VAR ix : INTEGER;
        nm : FileNames.NameString;
  BEGIN
    IF s.formals.tide = 0 THEN
      Console.WriteString("()");
    ELSE
      Console.Write("(");
      Console.WriteLn;
      FOR ix := 0 TO s.formals.tide-1 DO
        H.Indent(i+4);
        s.formals.a[ix].DiagPar();
        IF ix < s.formals.tide-1 THEN Console.Write(";"); Console.WriteLn END;
      END;
      Console.Write(")");
    END;
    IF s.retType # NIL THEN
      Console.WriteString(" : ");
      Console.WriteString(qualname(s.retType.idnt));
    END;
  END DiagFormalType;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Procedure)Diagnose*(i : INTEGER);
    VAR ix : INTEGER;
  BEGIN
    H.Indent(i);
    IF s.receiver # NIL THEN
      Console.Write("(");
      Console.WriteString(s.name());
      Console.Write(")");
    END;
    Console.WriteString("PROC");
    s.DiagFormalType(i+4);
    Console.WriteLn;
    Sy.DoXName(i, s.xName);
  END Diagnose;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Overloaded)Diagnose*(i : INTEGER);
  BEGIN
    H.Indent(i);
    Console.WriteString("Overloaded Type");
    Console.WriteLn;
  END Diagnose;

(* ---------------------------------------------------- *)
(* ---------------------------------------------------- *)

  PROCEDURE qualname(id : Sy.Idnt) : Lv.CharOpen;
  BEGIN
    IF id = NIL THEN
      RETURN nilStr;
    ELSIF (id.dfScp = NIL) OR (id.dfScp.kind = Id.modId) THEN
      RETURN Sy.getName.ChPtr(id);
    ELSE
      RETURN Lv.strToCharOpen
    (Sy.getName.ChPtr(id.dfScp)^ + "." + Sy.getName.ChPtr(id)^);
    END;
  END qualname;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Base)name*() : Lv.CharOpen;
  BEGIN
    IF s.idnt = NIL THEN
      RETURN Lv.strToCharOpen("Anon-base-type");
    ELSE
      RETURN Sy.getName.ChPtr(s.idnt);
    END;
  END name;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Enum)name*() : Lv.CharOpen;
  BEGIN
    IF s.idnt = NIL THEN
      RETURN Lv.strToCharOpen("Anon-enum-type");
    ELSE
      RETURN Sy.getName.ChPtr(s.idnt);
    END;
  END name;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Opaque)name*() : Lv.CharOpen;
  BEGIN
    IF s.idnt = NIL THEN
      IF s.kind = namTp THEN
        RETURN Lv.strToCharOpen("Anon-opaque");
      ELSE
        RETURN Lv.strToCharOpen("Anon-temporary");
      END;
    ELSE
      RETURN qualname(s.idnt);
    END;
  END name;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Array)name*() : Lv.CharOpen, EXTENSIBLE;
    VAR elNm : Lv.CharOpen;
  BEGIN
    IF s.idnt = NIL THEN
      IF s.elemTp = NIL THEN elNm := nilStr ELSE elNm := s.elemTp.name() END;
      IF s.length = 0 THEN
        RETURN Lv.strToCharOpen("ARRAY OF " + elNm^);
      ELSE
        RETURN Lv.strToCharOpen("ARRAY " + 
                                Lv.intToCharOpen(s.length)^ +
                                " OF " + 
                                elNm^);
      END;
    ELSE
      RETURN qualname(s.idnt);
    END;
  END name;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Vector)name*() : Lv.CharOpen;
    VAR elNm : Lv.CharOpen;
  BEGIN
    IF s.idnt = NIL THEN
      IF s.elemTp = NIL THEN elNm := nilStr ELSE elNm := s.elemTp.name() END;
      RETURN Lv.strToCharOpen("VECTOR OF " + elNm^);
    ELSE
      RETURN qualname(s.idnt);
    END;
  END name;

(* ---------------------------------------------------- *)

  PROCEDURE cmpndName(s : Record) : Lv.CharOpen;
  (* Returns the name of a compound type as a list
   * of its (optional) class and its interfaces *)
    VAR
      itfList : Lv.CharOpen;
      i : INTEGER;
  BEGIN
    itfList := Lv.strToCharOpen("(");
    IF s.baseTp # NIL THEN
      itfList := Lv.strToCharOpen(itfList^ + s.baseTp.name()^ + ",");
    END;
    FOR i := 0 TO s.interfaces.tide - 1 DO
      itfList := Lv.strToCharOpen(itfList^ + s.interfaces.a[i].name()^);
      IF i # s.interfaces.tide - 1 THEN
        itfList := Lv.strToCharOpen(itfList^ + ",");
      END;
    END;
    RETURN Lv.strToCharOpen(itfList^ + ")");
  END cmpndName;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Record)name*() : Lv.CharOpen;
  BEGIN
    IF s.bindTp # NIL THEN
      RETURN Lv.strToCharOpen(s.bindTp.name()^ + "^");
    ELSIF s.idnt = NIL THEN
      IF s.recAtt = cmpnd THEN
        RETURN cmpndName(s);
      ELSE
        RETURN Lv.strToCharOpen("Anon-record");
      END;
    ELSE
      RETURN qualname(s.idnt);
    END;
  END name;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Pointer)name*() : Lv.CharOpen;
    VAR elNm : Lv.CharOpen;
  BEGIN
    IF s.idnt = NIL THEN
      IF s.boundTp = NIL THEN elNm := nilStr ELSE elNm := s.boundTp.name() END;
      RETURN Lv.strToCharOpen("POINTER TO " + elNm^);
    ELSE
      RETURN qualname(s.idnt);
    END;
  END name;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Procedure)name*() : Lv.CharOpen;
  BEGIN
    IF s.idnt = NIL THEN
      RETURN Lv.strToCharOpen("Anon-opaque-type");
    ELSE
      RETURN qualname(s.idnt);
    END;
  END name;

(* ---------------------------------------------------- *)

  PROCEDURE (s : Overloaded)name*() : Lv.CharOpen;
  BEGIN
    RETURN Lv.strToCharOpen("Overloaded-type");
  END name;

(* ============================================================ *)
BEGIN (* ====================================================== *)
  NEW(anyRecTp);
  NEW(anyPtrTp);
  nilStr := Lv.strToCharOpen("<nil>");
END TypeDesc. (* ============================================== *)
(* ============================================================ *)
