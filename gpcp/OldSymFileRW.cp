(* ==================================================================== *)
(*									*)
(*  SymFileRW:  Symbol-file reading and writing for GPCP.		*)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*									*)
(* ==================================================================== *)

MODULE OldSymFileRW;

  IMPORT 
        GPCPcopyright,
        RTS,
        Error,
        Console,
        GF := GPFiles,
        BF := GPBinFiles,
        Id := IdDesc,
        D  := Symbols,
        LitValue,
        Visitor,
        ExprDesc,
        Ty := TypeDesc,
        B  := Builtin,
        S  := CPascalS,
        G  := CompState,
        Nh := NameHash,
        FileNames;

(* ========================================================================= *
// Collected syntax ---
// 
// SymFile    = Header [String (falSy | truSy | <other attribute>)]
//              [ VersionName ]
//		{Import | Constant | Variable | Type | Procedure} 
//		TypeList Key.
//	-- optional String is external name.
//	-- falSy ==> Java class
//	-- truSy ==> Java interface
//	-- others ...
// Header     = magic modSy Name.
// VersionName= numSy longint numSy longint numSy longint.
//      --            mj# mn#       bld rv#    8xbyte extract
// Import     = impSy Name [String] Key.
//	-- optional string is explicit external name of class
// Constant   = conSy Name Literal.
// Variable   = varSy Name TypeOrd.
// Type       = typSy Name TypeOrd.
// Procedure  = prcSy Name [String] FormalType.
//	-- optional string is explicit external name of procedure
// Method     = mthSy Name byte byte TypeOrd [String] [Name] FormalType.
//	-- optional string is explicit external name of method
// FormalType = [retSy TypeOrd] frmSy {parSy byte TypeOrd [String]} endFm.
//	-- optional phrase is return type for proper procedures
// TypeOrd    = ordinal.
// TypeHeader = tDefS Ord [fromS Ord Name].
//	-- optional phrase occurs if:
//	-- type not from this module, i.e. indirect export
// TypeList   = start { Array | Record | Pointer | ProcType | 
//                Enum | Vector | NamedType } close.
// Array      = TypeHeader arrSy TypeOrd (Byte | Number | <empty>) endAr.
//	-- nullable phrase is array length for fixed length arrays
// Vector     = TypeHeader vecSy TypeOrd endAr.
// Pointer    = TypeHeader ptrSy TypeOrd.
// Event      = TypeHeader evtSy FormalType.
// ProcType   = TypeHeader pTpSy FormalType.
// Record     = TypeHeader recSy recAtt [truSy | falSy] 
//		[basSy TypeOrd] [iFcSy {basSy TypeOrd}]
//		{Name TypeOrd} {Method} {Statics} endRc.
//	-- truSy ==> is an extension of external interface
//	-- falSy ==> is an extension of external class
// 	-- basSy option defines base type, if not ANY / j.l.Object
// Statics    = ( Constant | Variable | Procedure ).
// Enum       = TypeHeader eTpSy { Constant } endRc.
// NamedType  = TypeHeader.
// Name	      = namSy byte UTFstring.
// Literal    = Number | String | Set | Char | Real | falSy | truSy.
// Byte       = bytSy byte.
// String     = strSy UTFstring.
// Number     = numSy longint.
// Real       = fltSy ieee-double.
// Set        = setSy integer.
// Key        = keySy integer..
// Char       = chrSy unicode character.
//
// Notes on the syntax:
// All record types must have a Name field, even though this is often
// redundant.  The issue is that every record type (including those that
// are anonymous in CP) corresponds to a IR class, and the definer 
// and the user of the class _must_ agree on the IR name of the class.
// The same reasoning applies to procedure types, which must have equal
// interface names in all modules.
// ======================================================================== *)

  CONST
        modSy = ORD('H'); namSy = ORD('$'); bytSy = ORD('\');
        numSy = ORD('#'); chrSy = ORD('c'); strSy = ORD('s');
        fltSy = ORD('r'); falSy = ORD('0'); truSy = ORD('1');
        impSy = ORD('I'); setSy = ORD('S'); keySy = ORD('K');
        conSy = ORD('C'); typSy = ORD('T'); tDefS = ORD('t');
        prcSy = ORD('P'); retSy = ORD('R'); mthSy = ORD('M');
        varSy = ORD('V'); parSy = ORD('p'); start = ORD('&');
        close = ORD('!'); recSy = ORD('{'); endRc = ORD('}');
        frmSy = ORD('('); fromS = ORD('@'); endFm = ORD(')');
        arrSy = ORD('['); endAr = ORD(']'); pTpSy = ORD('%');
        ptrSy = ORD('^'); basSy = ORD('+'); eTpSy = ORD('e');
        iFcSy = ORD('~'); evtSy = ORD('v'); vecSy = ORD('*');

  CONST
        magic   = 0DEADD0D0H;
        syMag   = 0D0D0DEADH;
        dumped* = -1;

(* ============================================================ *)

  TYPE
        SymFile = POINTER TO RECORD 
        	    file : BF.FILE;
        	    cSum : INTEGER;
        	    modS : Id.BlkId;
        	    iNxt : INTEGER;
        	    oNxt : INTEGER;
        	    work : D.TypeSeq;
        	  END;

  TYPE
        SymFileReader* = POINTER TO RECORD
        	    file  : BF.FILE;
        	    modS  : Id.BlkId;
        	    impS  : Id.BlkId;
        	    sSym  : INTEGER;
        	    cAtt  : CHAR;
        	    iAtt  : INTEGER;
        	    lAtt  : LONGINT;
        	    rAtt  : REAL;
        	    sAtt  : FileNames.NameString;
                    oArray : D.IdSeq;
        	    sArray : D.ScpSeq;		(* These two sequences	*)
  		    tArray : D.TypeSeq;		(* must be private as   *)
        	  END;				(* file parses overlap. *)

(* ============================================================ *)

  TYPE	TypeLinker*  = POINTER TO RECORD (D.SymForAll) sym : SymFileReader END;
  TYPE	SymFileSFA*  = POINTER TO RECORD (D.SymForAll) sym : SymFile END;

(* ============================================================ *)

  VAR   lastKey : INTEGER;	(* private state for CPMake *)
        fSepArr : ARRAY 2 OF CHAR;

(* ============================================================ *)
(* ========	     Import Stack Implementation	======= *)
(* ============================================================ *)

   VAR	stack	: ARRAY 32 OF Id.BlkId;
        topIx	: INTEGER;

   PROCEDURE InitStack;
   BEGIN
     topIx := 0; G.impMax := 0;
   END InitStack;

   PROCEDURE PushStack(b : Id.BlkId);
   BEGIN
     stack[topIx] := b; 
     INC(topIx);
     IF topIx > G.impMax THEN G.impMax := topIx END; 
   END PushStack;

   PROCEDURE PopStack;
   BEGIN
     DEC(topIx);
   END PopStack;

(* ============================================================ *)

  PROCEDURE GetLastKeyVal*() : INTEGER;
  BEGIN
    RETURN lastKey;
  END GetLastKeyVal;

(* ============================================================ *)
(* ========	Various writing utility procedures	======= *)
(* ============================================================ *)

  PROCEDURE newSymFile(mod : Id.BlkId) : SymFile;
    VAR new : SymFile;
  BEGIN
    NEW(new);
   (*
    *  Initialization: cSum starts at zero. Since impOrd of
    *  the module is zero, impOrd of the imports starts at 1.
    *)
    new.cSum := 0;
    new.iNxt := 1;
    new.oNxt := D.tOffset;
    new.modS := mod;
    D.InitTypeSeq(new.work, 32);
    RETURN new;
  END newSymFile;

(* ======================================= *)

  PROCEDURE (f : SymFile)Write(chr : INTEGER),NEW;
    VAR tmp : INTEGER;
  BEGIN [UNCHECKED_ARITHMETIC]
   (* need to turn off overflow checking here *)
    tmp := f.cSum * 2 + chr;
    IF f.cSum < 0 THEN INC(tmp) END;
    f.cSum := tmp;
    BF.WriteByte(f.file, chr);
  END Write;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteStrUTF(IN nam : ARRAY OF CHAR),NEW;
    VAR buf : ARRAY 256 OF INTEGER;
        num : INTEGER;
        idx : INTEGER;
        chr : INTEGER;
  BEGIN
    num := 0;
    idx := 0;
    chr := ORD(nam[idx]);
    WHILE chr # 0H DO
      IF    chr <= 7FH THEN 		(* [0xxxxxxx] *)
        buf[num] := chr; INC(num);
      ELSIF chr <= 7FFH THEN 		(* [110xxxxx,10xxxxxx] *)
        buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num  ] := 0C0H + chr; INC(num, 2);
      ELSE 				(* [1110xxxx,10xxxxxx,10xxxxxxx] *)
        buf[num+2] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num  ] := 0E0H + chr; INC(num, 3);
      END;
      INC(idx); chr := ORD(nam[idx]);
    END;
    f.Write(num DIV 256);
    f.Write(num MOD 256);
    FOR idx := 0 TO num-1 DO f.Write(buf[idx]) END;
  END WriteStrUTF;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteOpenUTF(chOp : LitValue.CharOpen),NEW;
    VAR buf : ARRAY 256 OF INTEGER;
        num : INTEGER;
        idx : INTEGER;
        chr : INTEGER;
  BEGIN
    num := 0;
    idx := 0;
    chr := ORD(chOp[0]);
    WHILE chr # 0H DO
      IF    chr <= 7FH THEN 		(* [0xxxxxxx] *)
        buf[num] := chr; INC(num);
      ELSIF chr <= 7FFH THEN 		(* [110xxxxx,10xxxxxx] *)
        buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num  ] := 0C0H + chr; INC(num, 2);
      ELSE 				(* [1110xxxx,10xxxxxx,10xxxxxxx] *)
        buf[num+2] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
        buf[num  ] := 0E0H + chr; INC(num, 3);
      END;
      INC(idx);
      chr := ORD(chOp[idx]);
    END;
    f.Write(num DIV 256);
    f.Write(num MOD 256);
    FOR idx := 0 TO num-1 DO f.Write(buf[idx]) END;
  END WriteOpenUTF;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteString(IN nam : ARRAY OF CHAR),NEW;
  BEGIN
    f.Write(strSy); 
    f.WriteStrUTF(nam);
  END WriteString;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteName(idD : D.Idnt),NEW;
  BEGIN
    f.Write(namSy); 
    f.Write(idD.vMod); 
    f.WriteOpenUTF(Nh.charOpenOfHash(idD.hash));
  END WriteName;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteChar(chr : CHAR),NEW;
    CONST mask = {0 .. 7};
    VAR   a,b,int : INTEGER;
  BEGIN
    f.Write(chrSy);
    int := ORD(chr);
    b := ORD(BITS(int) * mask); int := ASH(int, -8);
    a := ORD(BITS(int) * mask); 
    f.Write(a); 
    f.Write(b); 
  END WriteChar;

(* ======================================= *)

  PROCEDURE (f : SymFile)Write4B(int : INTEGER),NEW;
    CONST mask = {0 .. 7};
    VAR   a,b,c,d : INTEGER;
  BEGIN
    d := ORD(BITS(int) * mask); int := ASH(int, -8);
    c := ORD(BITS(int) * mask); int := ASH(int, -8);
    b := ORD(BITS(int) * mask); int := ASH(int, -8);
    a := ORD(BITS(int) * mask); 
    f.Write(a); 
    f.Write(b); 
    f.Write(c); 
    f.Write(d); 
  END Write4B;

(* ======================================= *)

  PROCEDURE (f : SymFile)Write8B(val : LONGINT),NEW;
  BEGIN
    f.Write4B(RTS.hiInt(val));
    f.Write4B(RTS.loInt(val));
  END Write8B;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteNum(val : LONGINT),NEW;
  BEGIN
    f.Write(numSy);
    f.Write8B(val);
  END WriteNum;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteReal(flt : REAL),NEW;
    VAR rslt : LONGINT;
  BEGIN
    f.Write(fltSy);
    rslt := RTS.realToLongBits(flt);
    f.Write8B(rslt);
  END WriteReal;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteOrd(ord : INTEGER),NEW;
  BEGIN
    IF ord <= 7FH THEN 
      f.Write(ord);
    ELSIF ord <= 7FFFH THEN
      f.Write(128 + ord MOD 128);	(* LS7-bits first *)
      f.Write(ord DIV 128);		(* MS8-bits next  *)
    ELSE
      ASSERT(FALSE);
    END;
  END WriteOrd;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitTypeOrd(t : D.Type),NEW;
  (*
   *  This proceedure facilitates the naming rules
   *  for records and (runtime) classes: -
   *
   *  (1)  Classes derived from named record types have
   *       names synthesized from the record typename.
   *  (2)  If a named pointer is bound to an anon record
   *       the class takes its name from the pointer name.
   *  (3)  If both the pointer and the record types have
   *       names, the class is named from the record.
   *)
    VAR recT : Ty.Record;
   (* ------------------------------------ *)
    PROCEDURE AddToWorklist(syF :SymFile; tyD : D.Type);
    BEGIN
      tyD.dump := syF.oNxt; INC(syF.oNxt);
      D.AppendType(syF.work, tyD);
      IF tyD.idnt = NIL THEN 
        tyD.idnt := Id.newSfAnonId(tyD.dump);
        tyD.idnt.type := tyD;
      END;
    END AddToWorklist;
   (* ------------------------------------ *)
  BEGIN
    IF t.dump = 0 THEN (* type is not dumped yet *)
      WITH t : Ty.Record DO
       (*
        *   We wish to ensure that anonymous records are
        *   never emitted before their binding pointer
        *   types.  This ensures that we do not need to
        *   merge types when reading the files.
        *)
        IF (t.bindTp # NIL) & 
           (t.bindTp.dump = 0) THEN 
          AddToWorklist(f, t.bindTp);		(* First the pointer...  *)
        END;
        AddToWorklist(f, t);			(* Then this record type *)
      | t : Ty.Pointer DO
       (*
        *  If a pointer to record is being emitted, and 
        *  the pointer is NOT anonymous, then the class
        *  is known by the name of the record.  Thus the
        *  record name must be emitted, at least opaquely.
        *  Furthermore, we must indicate the binding
        *  relationship between the pointer and record.
        *  (It is possible that DCode need record size.)
        *)
        AddToWorklist(f, t);			(* First this pointer... *)
        IF (t.boundTp # NIL) & 
           (t.boundTp.dump = 0) &
           (t.boundTp IS Ty.Record) THEN
          recT := t.boundTp(Ty.Record);
          IF recT.bindTp = NIL THEN
            t.force := D.forced;
            AddToWorklist(f, t.boundTp);	(* Then the record type  *)
          END;
        END;
      ELSE (* All others *)
        AddToWorklist(f, t);			(* Just add the type.    *)
      END;
    END;
    f.WriteOrd(t.dump);
  END EmitTypeOrd;

(* ============================================================ *)
(* ========	    Various writing procedures		======= *)
(* ============================================================ *)

  PROCEDURE (f : SymFile)FormalType(t : Ty.Procedure),NEW;
  (*
  ** FormalType = [retSy TypeOrd] frmSy {parSy Byte TypeOrd [String]} endFm.
  *)
    VAR indx : INTEGER;
        parI : Id.ParId;
  BEGIN
    IF t.retType # NIL THEN
      f.Write(retSy);
      f.EmitTypeOrd(t.retType);
     (*
      *   The structure of this type must be
      *   emitted, unless it is an imported type.
      *)
      t.retType.ConditionalMark();
    END;
    f.Write(frmSy);
    FOR indx := 0 TO t.formals.tide-1 DO
      parI := t.formals.a[indx];
      f.Write(parSy);
      f.Write(parI.parMod);
      f.EmitTypeOrd(parI.type);
     (*
      *   Emit Optional Parameter name 
      *)
      IF ~G.legacy & (parI.hash # 0) THEN
        f.WriteString(Nh.charOpenOfHash(parI.hash));
      END;
     (*
      *   The structure of this type must be
      *   emitted, unless it is an imported type.
      *)
      parI.type.ConditionalMark();
    END;
    f.Write(endFm);
  END FormalType;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitConstId(id : Id.ConId),NEW;
    VAR conX : ExprDesc.LeafX;
        cVal : LitValue.Value;
        sVal : INTEGER;
  (*
  ** Constant = conSy Name Literal.
  ** Literal  = Number | String | Set | Char | Real | falSy | truSy.
  *)
  BEGIN
    conX := id.conExp(ExprDesc.LeafX);
    cVal := conX.value;
    f.Write(conSy);
    f.WriteName(id);
    CASE conX.kind OF
    | ExprDesc.tBool  : f.Write(truSy);
    | ExprDesc.fBool  : f.Write(falSy);
    | ExprDesc.numLt  : f.WriteNum(cVal.long());
    | ExprDesc.charLt : f.WriteChar(cVal.char());
    | ExprDesc.realLt : f.WriteReal(cVal.real());
    | ExprDesc.strLt  : f.WriteString(cVal.chOpen());
    | ExprDesc.setLt  : 
        f.Write(setSy); 
        IF cVal # NIL THEN sVal := cVal.int() ELSE sVal := 0 END;
        f.Write4B(sVal);
    END;
  END EmitConstId;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitTypeId(id : Id.TypId),NEW;
  (*
  **  Type = TypeSy Name TypeOrd.
  *)
  BEGIN
    f.Write(typSy);
    f.WriteName(id);
    f.EmitTypeOrd(id.type);
   (*
    *   The structure of this type must be
    *   emitted, even if it is an imported type.
    *)
    id.type.UnconditionalMark();
  END EmitTypeId;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitVariableId(id : Id.VarId),NEW;
  (*
  ** Variable = varSy Name TypeOrd.
  *)
  BEGIN
    f.Write(varSy);
    f.WriteName(id);
    f.EmitTypeOrd(id.type);
   (*
    *   The structure of this type must be
    *   emitted, unless it is an imported type.
    *)
    id.type.ConditionalMark();
  END EmitVariableId;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitImportId(id : Id.BlkId),NEW;
  (*
  ** Import = impSy Name.
  *)
  BEGIN
    IF D.need IN id.xAttr THEN
      f.Write(impSy);
      f.WriteName(id);
      IF id.scopeNm # NIL THEN f.WriteString(id.scopeNm) END;
      f.Write(keySy);
      f.Write4B(id.modKey);
      id.impOrd := f.iNxt; INC(f.iNxt);
    END;
  END EmitImportId;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitProcedureId(id : Id.PrcId),NEW;
  (*
  ** Procedure = prcSy Name FormalType.
  *)
  BEGIN
    f.Write(prcSy);
    f.WriteName(id);
    IF id.prcNm # NIL THEN f.WriteString(id.prcNm) END;
    IF id.kind = Id.ctorP THEN f.Write(truSy) END;
    f.FormalType(id.type(Ty.Procedure));
  END EmitProcedureId;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitMethodId(id : Id.MthId),NEW;
  (*
  ** Method = mthSy Name Byte Byte TypeOrd [strSy ] FormalType.
  *)
  BEGIN
    IF id.kind = Id.fwdMth THEN id := id.resolve(Id.MthId) END;
    f.Write(mthSy);
    f.WriteName(id);
    f.Write(ORD(id.mthAtt));
    f.Write(id.rcvFrm.parMod);
    f.EmitTypeOrd(id.rcvFrm.type);
    IF id.prcNm # NIL THEN f.WriteString(id.prcNm) END;
    IF ~G.legacy & (id.rcvFrm.hash # 0) THEN f.WriteName(id.rcvFrm) END;
    f.FormalType(id.type(Ty.Procedure));
  END EmitMethodId;

(* ======================================= *)

  PROCEDURE moduleOrd(tpId : D.Idnt) : INTEGER;
    VAR impM : Id.BlkId;
  BEGIN
    IF  (tpId = NIL) OR 
        (tpId.dfScp = NIL) OR 
        (tpId.dfScp.kind = Id.modId) THEN
      RETURN 0;
    ELSE
      impM := tpId.dfScp(Id.BlkId);
      IF impM.impOrd = 0 THEN RETURN -1 ELSE RETURN impM.impOrd END;
    END;
  END moduleOrd;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitTypeHeader(t : D.Type),NEW;
  (*
  **  TypeHeader = typSy Ord [fromS Ord Name].
  *)
    VAR mod : INTEGER;
        idt : D.Idnt;
   (* =================================== *)
    PROCEDURE warp(id : D.Idnt) : D.Idnt;
    BEGIN
      IF    id.type = G.ntvObj THEN RETURN G.objId;
      ELSIF id.type = G.ntvStr THEN RETURN G.strId;
      ELSIF id.type = G.ntvExc THEN RETURN G.excId;
      ELSIF id.type = G.ntvTyp THEN RETURN G.clsId;
      ELSE  RETURN NIL;
      END;
    END warp;
   (* =================================== *)
  BEGIN
    WITH t : Ty.Record DO
      IF t.bindTp = NIL THEN 
        idt := t.idnt;
      ELSIF t.bindTp.dump = 0 THEN
        ASSERT(FALSE);
        idt := NIL;
      ELSE
        idt := t.bindTp.idnt;
      END;
    ELSE
      idt := t.idnt;
    END;
    mod := moduleOrd(t.idnt);
    f.Write(tDefS);
    f.WriteOrd(t.dump);
   (*
    *  Convert native types back to RTS.nativeXXX, if necessary.
    *  That is ... if the native module is not explicitly imported.
    *)
    IF mod = -1 THEN idt := warp(idt); mod := moduleOrd(idt) END;
    IF mod # 0 THEN
      f.Write(fromS);
      f.WriteOrd(mod);
      f.WriteName(idt);
    END;
  END EmitTypeHeader;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitArrOrVecType(t : Ty.Array),NEW;
  BEGIN
    f.EmitTypeHeader(t);
    IF t.force # D.noEmit THEN	(* Don't emit structure unless forced *)
      IF t.kind = Ty.vecTp THEN f.Write(vecSy) ELSE f.Write(arrSy) END;
      f.EmitTypeOrd(t.elemTp);
      IF t.length > 127 THEN
        f.Write(numSy);
        f.Write8B(t.length);
      ELSIF t.length > 0 THEN
        f.Write(bytSy);
        f.Write(t.length);
      END;
      f.Write(endAr);
    END;
  END EmitArrOrVecType;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitRecordType(t : Ty.Record),NEW;
    VAR index  : INTEGER;
        field  : D.Idnt;
        method : D.Idnt;
  (*
  **  Record = TypeHeader recSy recAtt [truSy | falSy | <others>] 
  **		[basSy TypeOrd] [iFcSy {basSy TypeOrd}]
  **		{Name TypeOrd} {Method} {Statics} endRc.
  *)
  BEGIN
    f.EmitTypeHeader(t);
    IF t.force # D.noEmit THEN	(* Don't emit structure unless forced *)
      f.Write(recSy);
      index := t.recAtt; 
      IF D.noNew IN t.xAttr THEN INC(index, Ty.noNew) END;
      IF D.clsTp IN t.xAttr THEN INC(index, Ty.clsRc) END;
      f.Write(index);
   (* ########## *)
      IF t.recAtt = Ty.iFace THEN
  	f.Write(truSy);
      ELSIF G.special OR (D.isFn IN t.xAttr) THEN  
        f.Write(falSy);
      END;
   (* ########## *)
      IF t.baseTp # NIL THEN			(* this is the parent type *)
        f.Write(basSy);
        f.EmitTypeOrd(t.baseTp);
      END;
   (* ########## *)
      IF t.interfaces.tide > 0 THEN
        f.Write(iFcSy);
        FOR index := 0 TO t.interfaces.tide-1 DO	(* any interfaces  *)
          f.Write(basSy);
          f.EmitTypeOrd(t.interfaces.a[index]);
        END;
      END;
   (* ########## *)
      FOR index := 0 TO t.fields.tide-1 DO
        field := t.fields.a[index];
        IF field.vMod # D.prvMode THEN
          f.WriteName(field);
          f.EmitTypeOrd(field.type);
        END;
      END;
      IF t.force = D.forced THEN  (* Don't emit methods unless forced *)
        FOR index := 0 TO t.methods.tide-1 DO
          method := t.methods.a[index];
          IF method.vMod # D.prvMode THEN
            f.EmitMethodId(method(Id.MthId));
          END;
        END;
(*
 *      IF G.special THEN  (* we might need to emit static stuff *)
 *
 *  From 1.2.0 this provides for contructors that do not
 *  extend imported foreign record types.
 *)
          FOR index := 0 TO t.statics.tide-1 DO
            field := t.statics.a[index];
            IF field.vMod # D.prvMode THEN
              CASE field.kind OF
              | Id.conId  : f.EmitConstId(field(Id.ConId));
              | Id.varId  : f.EmitVariableId(field(Id.VarId));
              | Id.ctorP,
                Id.conPrc : f.EmitProcedureId(field(Id.PrcId));
              END;
            END;
          END;
        END;
(*
 *    END;
 *)
      f.Write(endRc);
    END;
    D.AppendType(f.modS.expRecs, t);
  END EmitRecordType;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitEnumType(t : Ty.Enum),NEW;
    VAR index  : INTEGER;
        const  : D.Idnt;
  (*
  **  Enum = TypeHeader eTpSy { constant } endRc.
  *)
  BEGIN
    f.EmitTypeHeader(t);
    f.Write(eTpSy);
    FOR index := 0 TO t.statics.tide-1 DO
      const := t.statics.a[index];
      IF const.vMod # D.prvMode THEN f.EmitConstId(const(Id.ConId)) END;
    END;
    f.Write(endRc);
    (* D.AppendType(f.modS.expRecs, t); *)
  END EmitEnumType;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitOpaqueType(t : Ty.Opaque),NEW;
  BEGIN
    f.EmitTypeHeader(t);
  END EmitOpaqueType;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitPointerType(t : Ty.Pointer),NEW;
  BEGIN
    f.EmitTypeHeader(t);
    IF (t.force # D.noEmit) OR 			(* Only emit structure if *)
       (t.boundTp.force # D.noEmit) THEN	(* ptr or boundTp forced. *)
      f.Write(ptrSy);
      f.EmitTypeOrd(t.boundTp);
    END;
  END EmitPointerType;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitProcedureType(t : Ty.Procedure),NEW;
  BEGIN
    f.EmitTypeHeader(t);
    IF t.isEventType() THEN f.Write(evtSy) ELSE f.Write(pTpSy) END;
    f.FormalType(t);
    D.AppendType(f.modS.expRecs, t);
  END EmitProcedureType;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitTypeList(),NEW;
    VAR indx : INTEGER;
        type : D.Type;
  BEGIN
   (*
    *   We cannot use a FOR loop here, as the tide changes
    *   during evaluation, as a result of reaching new types.
    *)
    indx := 0;
    WHILE indx < f.work.tide DO
      type := f.work.a[indx];

      WITH type : Ty.Array     DO f.EmitArrOrVecType(type);
      |    type : Ty.Record    DO f.EmitRecordType(type);
      |    type : Ty.Opaque    DO f.EmitOpaqueType(type);
      |    type : Ty.Pointer   DO f.EmitPointerType(type);
      |    type : Ty.Procedure DO f.EmitProcedureType(type);
      |    type : Ty.Enum      DO f.EmitEnumType(type);
      END;
      INC(indx);
    END;
  END EmitTypeList;

(* ======================================= *)

  PROCEDURE EmitSymfile*(m : Id.BlkId);

    VAR symVisit : SymFileSFA;
        symfile  : SymFile;
        marker   : INTEGER;
(*
 *      fileName : FileNames.NameString;
 *)
        fNamePtr : LitValue.CharOpen;
  (* ----------------------------------- *)
    PROCEDURE mkPathName(m : D.Idnt) : LitValue.CharOpen;
      VAR str : LitValue.CharOpen;
    BEGIN
      str := BOX(G.symDir);
      IF str[LEN(str) - 2] = GF.fileSep THEN
        str := BOX(str^ + D.getName.ChPtr(m)^ + ".cps");
      ELSE
        str := BOX(str^ + fSepArr + D.getName.ChPtr(m)^ + ".cps");
      END;
      RETURN str;
    END mkPathName;
  (* ----------------------------------- *)
  (*
  ** SymFile = Header [String (falSy | truSy | <others>)]
  **            [ VersionName]
  **		{Import | Constant | Variable
  **                 | Type | Procedure | Method} TypeList.
  ** Header = magic modSy Name.
  ** VersionName= numSy longint numSy longint numSy longint.
  **      --            mj# mn#       bld rv#        8xbyte extract
  *)
  BEGIN
   (*
    *  Create the SymFile structure, and open the output file.
    *)
    symfile := newSymFile(m);
   (* Start of alternative gpcp1.2 code *)
    IF G.symDir # "" THEN
      fNamePtr := mkPathName(m);
      symfile.file := BF.createPath(fNamePtr);
    ELSE
      fNamePtr := BOX(D.getName.ChPtr(m)^ + ".cps");
      symfile.file := BF.createFile(fNamePtr);
    END;
    IF symfile.file = NIL THEN
      S.SemError.Report(177, 0, 0);
      Error.WriteString("Cannot create file <" + fNamePtr^ + ">"); 
      Error.WriteLn;
      RETURN;
    ELSE
     (*
      *  Emit the symbol file header
      *)
      IF G.verbose THEN G.Message("Created " + fNamePtr^) END;
     (* End of alternative gpcp1.2 code *)
      IF D.rtsMd IN m.xAttr THEN
        marker := RTS.loInt(syMag);	(* ==> a system module *)
      ELSE
        marker := RTS.loInt(magic);	(* ==> a normal module *)
      END;
      symfile.Write4B(RTS.loInt(marker));
      symfile.Write(modSy);
      symfile.WriteName(m);
      IF m.scopeNm # NIL THEN (* explicit name *)
        symfile.WriteString(m.scopeNm);
        symfile.Write(falSy);
      END;
     (*
      *  Emit the optional TypeName, if required.
      *
      *  VersionName= numSy longint numSy longint numSy longint.
      *       --            mj# mn#       bld rv#        8xbyte extract
      *)
      IF m.verNm # NIL THEN
        symfile.WriteNum(m.verNm[0] * 100000000L + m.verNm[1]);
        symfile.WriteNum(m.verNm[2] * 100000000L + m.verNm[3]);
        symfile.WriteNum(m.verNm[4] * 100000000L + m.verNm[5]);
      END;
     (*
      *  Create the symbol table visitor, an extension of 
      *  Symbols.SymForAll type.  Emit symbols from the scope.
      *)
      NEW(symVisit);
      symVisit.sym := symfile;
      symfile.modS.symTb.Apply(symVisit); 
     (*
      *  Now emit the types on the worklist.
      *)
      symfile.Write(start);
      symfile.EmitTypeList();
      symfile.Write(close);
     (*
      *  Now emit the accumulated checksum key symbol.
      *)
      symfile.Write(keySy);
      lastKey := symfile.cSum;
      IF G.special THEN symfile.Write4B(0) ELSE symfile.Write4B(lastKey) END;
      BF.CloseFile(symfile.file);
    END;
  END EmitSymfile;

(* ============================================================ *)
(* ========	Various reading utility procedures	======= *)
(* ============================================================ *)

  PROCEDURE read(f : BF.FILE) : INTEGER;
  BEGIN
    RETURN BF.readByte(f);
  END read;

(* ======================================= *)

  PROCEDURE ReadUTF(f : BF.FILE; OUT nam : ARRAY OF CHAR);
    CONST
        bad = "Bad UTF-8 string";
    VAR num : INTEGER;
        bNm : INTEGER;
        idx : INTEGER;
        chr : INTEGER;
  BEGIN
    num := 0;
    bNm := read(f) * 256 + read(f);
    FOR idx := 0 TO bNm-1 DO
      chr := read(f);
      IF chr <= 07FH THEN		(* [0xxxxxxx] *)
        nam[num] := CHR(chr); INC(num);
      ELSIF chr DIV 32 = 06H THEN	(* [110xxxxx,10xxxxxx] *)
        bNm := chr MOD 32 * 64;
        chr := read(f);
        IF chr DIV 64 = 02H THEN
          nam[num] := CHR(bNm + chr MOD 64); INC(num);
        ELSE
          RTS.Throw(bad);
        END;
      ELSIF chr DIV 16 = 0EH THEN	(* [1110xxxx,10xxxxxx,10xxxxxxx] *)
        bNm := chr MOD 16 * 64;
        chr := read(f);
        IF chr DIV 64 = 02H THEN
          bNm := (bNm + chr MOD 64) * 64; 
          chr := read(f);
          IF chr DIV 64 = 02H THEN
            nam[num] := CHR(bNm + chr MOD 64); INC(num);
          ELSE 
            RTS.Throw(bad);
          END;
        ELSE
          RTS.Throw(bad);
        END;
      ELSE
        RTS.Throw(bad);
      END;
    END;
    nam[num] := 0X;
  END ReadUTF;

(* ======================================= *)

  PROCEDURE readChar(f : BF.FILE) : CHAR;
  BEGIN
    RETURN CHR(read(f) * 256 + read(f));
  END readChar;

(* ======================================= *)

  PROCEDURE readInt(f : BF.FILE) : INTEGER;
  BEGIN [UNCHECKED_ARITHMETIC]
    (* overflow checking off here *)
    RETURN ((read(f) * 256 + read(f)) * 256 + read(f)) * 256 + read(f);
  END readInt;

(* ======================================= *)

  PROCEDURE readLong(f : BF.FILE) : LONGINT;
    VAR result : LONGINT;
        index  : INTEGER;
  BEGIN [UNCHECKED_ARITHMETIC]
    (* overflow checking off here *)
    result := read(f);
    FOR index := 1 TO 7 DO
      result := result * 256 + read(f);
    END;
    RETURN result;
  END readLong;

(* ======================================= *)

  PROCEDURE readReal(f : BF.FILE) : REAL;
    VAR result : LONGINT;
  BEGIN
    result := readLong(f);
    RETURN RTS.longBitsToReal(result);
  END readReal;

(* ======================================= *)

  PROCEDURE readOrd(f : BF.FILE) : INTEGER;
    VAR chr : INTEGER;
  BEGIN
    chr := read(f);
    IF chr <= 07FH THEN RETURN chr;
    ELSE
      DEC(chr, 128);
      RETURN chr + read(f) * 128;
    END;
  END readOrd;

(* ============================================================ *)
(* ========		Symbol File Reader		======= *)
(* ============================================================ *)

  PROCEDURE newSymFileReader*(mod : Id.BlkId) : SymFileReader;
    VAR new : SymFileReader;
  BEGIN
    NEW(new);
    new.modS := mod;
    D.InitIdSeq(new.oArray, 4);
    D.InitTypeSeq(new.tArray, 8);
    D.InitScpSeq(new.sArray, 8);
    RETURN new;
  END newSymFileReader;

(* ======================================= *)
  PROCEDURE^ (f : SymFileReader)SymFile(IN nm : ARRAY OF CHAR),NEW;
  PROCEDURE^ WalkThisImport(imp, mod : Id.BlkId);
(* ======================================= *)

  PROCEDURE Abandon(f : SymFileReader);
  BEGIN
    RTS.Throw("Bad symbol file format" + 
              Nh.charOpenOfHash(f.impS.hash)^); 
  END Abandon;

(* ======================================= *)

  PROCEDURE (f : SymFileReader)GetSym(),NEW;
    VAR file : BF.FILE;
  BEGIN
    file := f.file;
    f.sSym := read(file);
    CASE f.sSym OF
    | namSy : 
        f.iAtt := read(file); ReadUTF(file, f.sAtt);
    | strSy : 
        ReadUTF(file, f.sAtt);
    | retSy, fromS, tDefS, basSy :
        f.iAtt := readOrd(file);
    | bytSy :
        f.iAtt := read(file);
    | keySy, setSy :
        f.iAtt := readInt(file);
    | numSy :
        f.lAtt := readLong(file);
    | fltSy :
        f.rAtt := readReal(file);
    | chrSy :
        f.cAtt := readChar(file);
    ELSE (* nothing to do *)
    END;
  END GetSym;

(* ======================================= *)

  PROCEDURE (f : SymFileReader)ReadPast(sym : INTEGER),NEW;
  BEGIN
    IF f.sSym # sym THEN Abandon(f) END;
    f.GetSym();
  END ReadPast;

(* ======================================= *)

  PROCEDURE (f : SymFileReader)Parse*(scope : Id.BlkId;
                                      filNm : FileNames.NameString),NEW;
    VAR fileName : FileNames.NameString;
        marker   : INTEGER;
        token    : S.Token;
  BEGIN
    token := scope.token;

    f.impS := scope;
    D.AppendScope(f.sArray, scope);
    fileName := filNm + ".cps";
    f.file := BF.findOnPath("CPSYM", fileName);
   (* #### *)
    IF f.file = NIL THEN
      fileName := "__" + fileName;
      f.file := BF.findOnPath("CPSYM", fileName);
      IF f.file # NIL THEN
        S.SemError.RepSt2(309, filNm, fileName, token.lin, token.col);
        filNm := "__" + filNm;
        scope.clsNm := LitValue.strToCharOpen(filNm);
      END;
    END;
   (* #### *)
    IF f.file = NIL THEN
      S.SemError.Report(129, token.lin, token.col); RETURN;
    ELSE
      IF G.verbose THEN G.Message("Opened " + fileName) END;
      marker := readInt(f.file);
      IF marker = RTS.loInt(magic) THEN
        (* normal case, nothing to do *)
      ELSIF marker = RTS.loInt(syMag) THEN
        INCL(scope.xAttr, D.rtsMd);
      ELSE
        S.SemError.Report(130, token.lin, token.col); RETURN;
      END;
      f.GetSym();
      f.SymFile(filNm);
      IF G.verbose THEN 
        G.Message("Ended " + fileName + ", Key: " 
        		+ LitValue.intToCharOpen(f.impS.modKey)^);
      END;
      BF.CloseFile(f.file);
    END;
  END Parse;

(* ============================================ *)

  PROCEDURE testInsert(id : D.Idnt; sc : D.Scope) : D.Idnt;
    VAR ident : D.Idnt;

    PROCEDURE Report(i,s : D.Idnt);
      VAR iS, sS : FileNames.NameString;
    BEGIN
      D.getName.Of(i, iS);
      D.getName.Of(s, sS);
      S.SemError.RepSt2(172, iS, sS, S.line, S.col);
    END Report;

  BEGIN
    IF sc.symTb.enter(id.hash, id) THEN
      ident := id;
    ELSE
      ident := sc.symTb.lookup(id.hash);	(* Warp the return Idnt	*)
      IF ident.kind # id.kind THEN Report(id, sc); ident := id END;
    END;
    RETURN ident;
  END testInsert;

(* ============================================ *)

  PROCEDURE Insert(id : D.Idnt; VAR tb : D.SymbolTable);
    VAR ident : D.Idnt;

    PROCEDURE Report(i : D.Idnt);
      VAR iS : FileNames.NameString;
    BEGIN
      D.getName.Of(i, iS);
      S.SemError.RepSt1(172, iS, 1, 1);
    END Report;

  BEGIN
    IF ~tb.enter(id.hash, id) THEN
      ident := tb.lookup(id.hash);		(* and test isForeign? *)
      IF ident.kind # id.kind THEN Report(id) END;
    END;
  END Insert;

(* ============================================ *)
 
  PROCEDURE InsertInRec(id : D.Idnt; rec : Ty.Record; sfr : SymFileReader);
  (* insert, taking into account possible overloaded methods. *)
  VAR
    ok : BOOLEAN;
    oId : Id.OvlId;

    PROCEDURE Report(i : D.Idnt; IN s : ARRAY OF CHAR);
      VAR iS, sS : FileNames.NameString;
    BEGIN
      D.getName.Of(i, iS);
(*
 *    D.getName.Of(s, sS);
 *    S.SemError.RepSt2(172, iS, sS, S.line, S.col);
 *)
      S.SemError.RepSt2(172, iS, s, S.line, S.col);
    END Report;

  BEGIN
    Ty.InsertInRec(id,rec,TRUE,oId,ok);
    IF oId # NIL THEN D.AppendIdnt(sfr.oArray,oId); END;
(*
    IF ~ok THEN Report(id,rec.idnt); END;
 *)
    IF ~ok THEN Report(id, rec.name()) END;
  END InsertInRec;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)getLiteral() : D.Expr,NEW;
    VAR expr : D.Expr;
  BEGIN
    CASE f.sSym OF
    | truSy : expr := ExprDesc.mkTrueX();
    | falSy : expr := ExprDesc.mkFalseX();
    | numSy : expr := ExprDesc.mkNumLt(f.lAtt);
    | chrSy : expr := ExprDesc.mkCharLt(f.cAtt);
    | fltSy : expr := ExprDesc.mkRealLt(f.rAtt);
    | setSy : expr := ExprDesc.mkSetLt(BITS(f.iAtt));
    | strSy : expr := ExprDesc.mkStrLt(f.sAtt);		(* implicit f.sAtt^ *)
    END;
    f.GetSym();						(* read past value  *)
    RETURN expr;
  END getLiteral;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)typeOf(ord : INTEGER) : D.Type,NEW;
    VAR newT : D.Type;
        indx : INTEGER;
  BEGIN
    IF ord < D.tOffset THEN				(* builtin type	*)	
      RETURN B.baseTypeArray[ord];
    ELSIF ord - D.tOffset < f.tArray.tide THEN
      RETURN f.tArray.a[ord - D.tOffset];
    ELSE 
      indx := f.tArray.tide + D.tOffset;
      REPEAT
        newT := Ty.newTmpTp();
        newT.dump := indx; INC(indx);
        D.AppendType(f.tArray, newT);
      UNTIL indx > ord;
      RETURN newT;
    END;
  END typeOf;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)getTypeFromOrd() : D.Type,NEW;
    VAR ord : INTEGER;
  BEGIN
    ord := readOrd(f.file);
    f.GetSym();
    RETURN f.typeOf(ord);
  END getTypeFromOrd;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)getFormalType(rslt : Ty.Procedure;
        				     indx : INTEGER) : D.Type,NEW;
  (*
  ** FormalType = [retSy TypeOrd] frmSy {parSy Byte TypeOrd [String]} endFm.
  //	-- optional phrase is return type for proper procedures
  *)
    VAR parD : Id.ParId;
        byte : INTEGER;
  BEGIN
    IF f.sSym = retSy THEN 
      rslt.retType := f.typeOf(f.iAtt);
      f.GetSym();
    END;
    f.ReadPast(frmSy);
    WHILE f.sSym = parSy DO
      byte := read(f.file);
      parD := Id.newParId();
      parD.parMod := byte;
      parD.varOrd := indx; 
      parD.type := f.getTypeFromOrd();
     (* Skip over optional parameter name string *)
      IF f.sSym = strSy THEN (* parD.hash := Nh.enterStr(f.sAtt); *)
        f.GetSym;
      END;
      Id.AppendParam(rslt.formals, parD);
      INC(indx);
    END;
    f.ReadPast(endFm);
    RETURN rslt;
  END getFormalType;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)pointerType(old : D.Type) : D.Type,NEW;
  (* Assert: the current symbol ptrSy 		*)
  (* Pointer   = TypeHeader ptrSy TypeOrd.	*)
    VAR rslt : Ty.Pointer;
        indx : INTEGER;
        junk : D.Type;
        isEvt: BOOLEAN;
  BEGIN
    isEvt := (f.sSym = evtSy);
    indx := readOrd(f.file);
    WITH old : Ty.Pointer DO
      rslt := old;
     (*
      *  Check if there is space in the tArray for this
      *  element, otherwise expand using typeOf().
      *)
      IF indx - D.tOffset >= f.tArray.tide THEN
        junk := f.typeOf(indx);
      END;
      f.tArray.a[indx - D.tOffset] := rslt.boundTp;
    ELSE
      rslt := Ty.newPtrTp();
      rslt.boundTp := f.typeOf(indx);
      IF isEvt THEN rslt.SetKind(Ty.evtTp) END;
    END;
    f.GetSym();
    RETURN rslt;
  END pointerType;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)procedureType() : D.Type,NEW;
  (* Assert: the current symbol is pTpSy.	*)
  (* ProcType  = TypeHeader pTpSy FormalType.	*)
  BEGIN
    f.GetSym();		(* read past pTpSy *)
    RETURN f.getFormalType(Ty.newPrcTp(), 0);
  END procedureType;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)eventType() : D.Type,NEW;
  (* Assert: the current symbol is evtSy.	*)
  (* EventType = TypeHeader evtSy FormalType.	*)
  BEGIN
    f.GetSym();		(* read past evtSy *)
    RETURN f.getFormalType(Ty.newEvtTp(), 0);
  END eventType;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)arrayType() : Ty.Array,NEW;
  (* Assert: at entry the current symbol is arrSy.		     *)
  (* Array      = TypeHeader arrSy TypeOrd (Byte | Number | ) endAr. *)
  (*	-- nullable phrase is array length for fixed length arrays   *)
    VAR rslt : Ty.Array;
        eTyp : D.Type;
  BEGIN
    rslt := Ty.newArrTp();
    rslt.elemTp := f.typeOf(readOrd(f.file));
    f.GetSym();
    IF f.sSym = bytSy THEN
      rslt.length := f.iAtt;
      f.GetSym();
    ELSIF f.sSym = numSy THEN
      rslt.length := SHORT(f.lAtt);
      f.GetSym();
    (* ELSE length := 0 *)
    END;
    f.ReadPast(endAr);
    RETURN rslt;
  END arrayType;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)vectorType() : Ty.Vector,NEW;
  (* Assert: at entry the current symbol is vecSy.                   *)
  (* Vector     = TypeHeader vecSy TypeOrd endAr.                    *)
    VAR rslt : Ty.Vector;
        eTyp : D.Type;
  BEGIN
    rslt := Ty.newVecTp();
    rslt.elemTp := f.typeOf(readOrd(f.file));
    f.GetSym();
    f.ReadPast(endAr);
    RETURN rslt;
  END vectorType;

(* ============================================ *)
  PROCEDURE^ (f : SymFileReader)procedure() : Id.PrcId,NEW;
  PROCEDURE^ (f : SymFileReader)method()    : Id.MthId,NEW;
  PROCEDURE^ (f : SymFileReader)constant()  : Id.ConId,NEW;
  PROCEDURE^ (f : SymFileReader)variable()  : Id.VarId,NEW;
(* ============================================ *)

  PROCEDURE (f : SymFileReader)recordType(old  : D.Type) : D.Type,NEW;
  (* Assert: at entry the current symbol is recSy.			*)
  (* Record     = TypeHeader recSy recAtt [truSy | falSy | <others>] 	*)
  (*	[basSy TypeOrd] [iFcSy {basSy TypeOrd}]				*)
  (*	{Name TypeOrd} {Method} {Statics} endRc.			*)
    CONST 
        vlTp = Ty.valRc;
    VAR rslt : Ty.Record;
        fldD : Id.FldId;
        varD : Id.VarId;
        mthD : Id.MthId;
        conD : Id.ConId;
        prcD : Id.PrcId;
        typD : Id.TypId;
        oldS : INTEGER;
        attr : INTEGER;
        mskd : INTEGER;
  BEGIN
    WITH old : Ty.Record DO rslt := old ELSE rslt := Ty.newRecTp() END;
    attr := read(f.file);
    mskd := attr MOD 8;
   (*
    *  The recAtt field has two other bits piggy-backed onto it.
    *  The noNew Field of xAttr is just added on in the writing 
    *  and is stripped off here.  The valRc field is used to lock
    *  in foreign value classes, even though they have basTp # NIL.
    *)
(*
 *  IF mskd # Ty.noAtt THEN INCL(rslt.xAttr, D.clsTp) END;
 *  IF attr >= noNw THEN DEC(attr, noNw); INCL(rslt.xAttr, D.noNew) END;
 *)
    IF attr >= Ty.clsRc THEN DEC(attr,Ty.clsRc); INCL(rslt.xAttr,D.clsTp) END;
    IF attr >= Ty.noNew THEN DEC(attr,Ty.noNew); INCL(rslt.xAttr,D.noNew) END;

    rslt.recAtt := attr;
    f.GetSym();				(* Get past recSy rAtt	*)
    IF f.sSym = falSy THEN
      INCL(rslt.xAttr, D.isFn);
      f.GetSym();
    ELSIF f.sSym = truSy THEN
      INCL(rslt.xAttr, D.isFn);
      INCL(rslt.xAttr, D.fnInf);
      INCL(rslt.xAttr, D.noCpy);
      f.GetSym();
    END;
    IF f.impS.scopeNm # NIL THEN rslt.extrnNm := f.impS.scopeNm END;

    IF f.sSym = basSy THEN
      rslt.baseTp := f.typeOf(f.iAtt);
      IF f.iAtt # Ty.anyRec THEN INCL(rslt.xAttr, D.clsTp) END;
      f.GetSym();
    END;
    IF f.sSym = iFcSy THEN
      f.GetSym();
      WHILE f.sSym = basSy DO
        typD := Id.newSfAnonId(f.iAtt);
        typD.type := f.typeOf(f.iAtt);
        D.AppendType(rslt.interfaces, typD.type);
        f.GetSym();
      END;
    END;
    WHILE f.sSym = namSy DO
      fldD := Id.newFldId();
      fldD.SetMode(f.iAtt);
      fldD.hash := Nh.enterStr(f.sAtt);
      fldD.type := f.typeOf(readOrd(f.file));
      fldD.recTyp := rslt;
      f.GetSym();
      IF rslt.symTb.enter(fldD.hash, fldD) THEN 
        D.AppendIdnt(rslt.fields, fldD);
      END;
    END;

    WHILE (f.sSym = mthSy) OR
          (f.sSym = prcSy) OR
          (f.sSym = varSy) OR
          (f.sSym = conSy) DO
      oldS := f.sSym; f.GetSym();
      IF oldS = mthSy THEN
        mthD := f.method();
        mthD.bndType := rslt;
        mthD.type(Ty.Procedure).receiver := rslt;
        InsertInRec(mthD,rslt,f);
        D.AppendIdnt(rslt.methods, mthD);
      ELSIF oldS = prcSy THEN
        prcD := f.procedure();
        prcD.bndType := rslt;
        InsertInRec(prcD,rslt,f);
        D.AppendIdnt(rslt.statics, prcD);
      ELSIF oldS = varSy THEN
        varD := f.variable();
        varD.recTyp := rslt;
        InsertInRec(varD,rslt,f);
        D.AppendIdnt(rslt.statics, varD);
      ELSIF oldS = conSy THEN
        conD := f.constant();
        conD.recTyp := rslt;
        InsertInRec(conD,rslt,f);
      ELSE
        Abandon(f);
      END;
    END;
(* #### *)
    IF attr >= Ty.valRc THEN 
      DEC(attr, Ty.valRc); 
      EXCL(rslt.xAttr, D.clsTp);
      EXCL(rslt.xAttr, D.noCpy);
    END;
(* #### *)
    f.ReadPast(endRc); 
    RETURN rslt;
  END recordType;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)enumType() : D.Type,NEW;
  (* Assert: at entry the current symbol is eTpSy.			*)
  (* Enum  = TypeHeader eTpSy { Constant} endRc.			*)
    VAR rslt : Ty.Enum;
        cnst : D.Idnt;
  BEGIN
    rslt := Ty.newEnuTp();
    f.GetSym();				(* Get past recSy 	*)
    WHILE f.sSym = conSy DO
      f.GetSym();
      cnst := f.constant();
      Insert(cnst, rslt.symTb);
      D.AppendIdnt(rslt.statics, cnst);
    END;
    f.ReadPast(endRc); 
    RETURN rslt;
  END enumType;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)Type(),NEW;
  (* Type       = typSy Name TypeOrd.		*)
    VAR newI : Id.TypId;
        oldI : D.Idnt;
        type : D.Type;
  BEGIN
   (* 
    * Post: every previously unknown typId id
    *	has the property:  id.type.idnt = id.
    *   If oldI # newT, then the new typId has
    *   newT.type.idnt = oldI.
    *)
    newI := Id.newTypId(NIL);
    newI.SetMode(f.iAtt);
    newI.hash := Nh.enterStr(f.sAtt);
    newI.type := f.getTypeFromOrd(); 
    newI.dfScp := f.impS;
    oldI := testInsert(newI, f.impS);

    IF oldI # newI THEN 
      f.tArray.a[newI.type.dump - D.tOffset] := oldI.type;
    END;

    IF newI.type.idnt = NIL THEN newI.type.idnt := oldI END;
  END Type;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)Import(),NEW;
  (* Import     = impSy Name [String] Key.	*)
  (*	-- optional string is external name	*)
  (* first symbol should be namSy here.		*)
    VAR impD : Id.BlkId;
        oldS : Id.BlkId;
        oldD : D.Idnt;
  BEGIN
    impD := Id.newImpId();
    impD.dfScp := impD;			(* ImpId define their own scope *)

    INCL(impD.xAttr, D.weak);
    impD.SetMode(f.iAtt);
    impD.hash := Nh.enterStr(f.sAtt);
    f.ReadPast(namSy); 
    IF impD.hash = f.modS.hash THEN	(* Importing own imp indirectly	*)
        				(* Shouldn't this be an error?  *)
      D.AppendScope(f.sArray, f.modS);
      IF f.sSym = strSy THEN 
        (* probably don't need to do anything here ... *)
        f.GetSym();
      END;
    ELSE				(* Importing some other module.	*)
      oldD := testInsert(impD, f.modS);
      IF f.sSym = strSy THEN 
        impD.scopeNm := LitValue.strToCharOpen(f.sAtt);
        f.GetSym();
      END;
      IF (oldD # impD) & (oldD.kind = Id.impId) THEN
        oldS := oldD(Id.BlkId);
        D.AppendScope(f.sArray, oldS);
        IF (oldS.modKey # 0) & (f.iAtt # oldS.modKey) THEN
          S.SemError.RepSt1(133,		(* Detected bad KeyVal	*)
        	Nh.charOpenOfHash(impD.hash)^, 
        	S.line, S.col);
        END;
(* should not be necessary anymore *)
        IF ~(D.weak IN oldS.xAttr) &
           ~(D.fixd IN oldS.xAttr) THEN
         (*
          *   This recursively reads the symbol files for 
          *   any imports of this file which are on the
          *   list to be imported later anyhow.
          *)
          WalkThisImport(oldS, f.modS);
        END;
      ELSE
        D.AppendScope(f.sArray, impD);
      END;
      impD.modKey := f.iAtt;
    END;
    f.ReadPast(keySy);
  END Import;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)constant() : Id.ConId,NEW;
  (* Constant   = conSy Name Literal.		*)
  (* Assert: f.sSym = namSy.			*)
    VAR newC : Id.ConId;
        anyI : D.Idnt;
  BEGIN
    newC := Id.newConId();
    newC.SetMode(f.iAtt);
    newC.hash := Nh.enterStr(f.sAtt);
    newC.dfScp := f.impS;
    f.ReadPast(namSy);
    newC.conExp := f.getLiteral();
    newC.type := newC.conExp.type;
    RETURN newC;
  END constant;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)variable() : Id.VarId,NEW;
  (* Variable   = varSy Name TypeOrd.		*)
    VAR newV : Id.VarId;
        anyI : D.Idnt;
  BEGIN
    newV := Id.newVarId();
    newV.SetMode(f.iAtt);
    newV.hash := Nh.enterStr(f.sAtt);
    newV.type := f.getTypeFromOrd();
    newV.dfScp := f.impS;
    RETURN newV;
  END variable;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)procedure() : Id.PrcId,NEW;
  (* Procedure  = prcSy Name[String]FormalType. *)
  (* This is a static proc, mths come with Recs *)
    VAR newP : Id.PrcId;
        anyI : D.Idnt;
  BEGIN
    newP := Id.newPrcId();
    newP.setPrcKind(Id.conPrc);
    newP.SetMode(f.iAtt);
    newP.hash := Nh.enterStr(f.sAtt);
    newP.dfScp := f.impS;
    f.ReadPast(namSy);
    IF f.sSym = strSy THEN 
      newP.prcNm := LitValue.strToCharOpen(f.sAtt);
     (* and leave scopeNm = NIL *)
      f.GetSym();
    END;
    IF f.sSym = truSy THEN	(* ### this is a constructor ### *)
      f.GetSym();
      newP.setPrcKind(Id.ctorP);
    END;			(* ### this is a constructor ### *)
    newP.type := f.getFormalType(Ty.newPrcTp(), 0);
    (* IF this is a java module, do some semantic checks *)
    (* ... *)
    RETURN newP;
  END procedure;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)method() : Id.MthId,NEW;
  (* Method     = mthSy Name byte byte TypeOrd [String][Name] FormalType. *)
    VAR newM : Id.MthId;
        rcvD : Id.ParId;
        rFrm : INTEGER;
        mAtt : SET;
  BEGIN
    newM := Id.newMthId();
    newM.SetMode(f.iAtt);
    newM.setPrcKind(Id.conMth);
    newM.hash := Nh.enterStr(f.sAtt);
    newM.dfScp := f.impS;
    rcvD := Id.newParId();
    rcvD.varOrd := 0;
   (* byte1 is the method attributes  *)
    mAtt := BITS(read(f.file));
   (* byte2 is param form of receiver *)
    rFrm := read(f.file);
   (* next 1 or 2 bytes are rcv-type  *)
    rcvD.type := f.typeOf(readOrd(f.file));
    f.GetSym();
    rcvD.parMod := rFrm;
    IF f.sSym = strSy THEN 
      newM.prcNm := LitValue.strToCharOpen(f.sAtt);
     (* and leave scopeNm = NIL *)
      f.GetSym();
    END;
   (* Skip over optional receiver name string *)
    IF f.sSym = namSy THEN (* rcvD.hash := Nh.enterString(f.sAtt); *)
      f.GetSym();
    END;
   (* End skip over optional receiver name *)
    newM.type := f.getFormalType(Ty.newPrcTp(), 1);
    newM.mthAtt := mAtt;
    newM.rcvFrm := rcvD;
   (* IF this is a java module, do some semantic checks *)
    RETURN newM;
  END method;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)TypeList(),NEW;
  (* TypeList   = start { Array | Record | Pointer      *)
  (*		  | ProcType | Vector} close.           *)
  (* TypeHeader = tDefS Ord [fromS Ord Name].           *)
    VAR modOrd : INTEGER;
        typOrd : INTEGER;
        typIdx : INTEGER;
        tpDesc : D.Type;
        tpTemp : D.Type;
        tpIdnt : Id.TypId;
        prevId : D.Idnt;
        prevTp : D.Type;
        impScp : D.Scope;
        linkIx : INTEGER;
        bndTyp : D.Type;
        typeFA : TypeLinker;
  BEGIN
    WHILE f.sSym = tDefS DO
      linkIx := 0;
      tpIdnt := NIL;
     (* Do type header *)
      typOrd := f.iAtt;
      typIdx := typOrd - D.tOffset;
      tpTemp := f.tArray.a[typIdx];
      impScp := NIL;
      f.ReadPast(tDefS);
     (*
      *  The [fromS modOrd typNam] appears if the type is imported.
      *  There are two cases:
      *     this is the first time that "mod.typNam" has been 
      *     seen during this compilation 
      *                   ==> insert a new typId descriptor in mod.symTb
      *     this name is already in the mod.symTb table
      *                   ==> fetch the previous descriptor
      *)
      IF f.sSym = fromS THEN
        modOrd := f.iAtt;
        impScp := f.sArray.a[modOrd];
        f.GetSym();
        tpIdnt := Id.newTypId(NIL);
        tpIdnt.SetMode(f.iAtt);
        tpIdnt.hash := Nh.enterStr(f.sAtt);
        tpIdnt.dfScp := impScp;
        tpIdnt := testInsert(tpIdnt, impScp)(Id.TypId);
        f.ReadPast(namSy);
      END;

     (* Get type info. *)
      CASE f.sSym OF
      | arrSy : tpDesc := f.arrayType();
      | vecSy : tpDesc := f.vectorType();
      | recSy : tpDesc := f.recordType(tpTemp);
      | pTpSy : tpDesc := f.procedureType();
      | evtSy : tpDesc := f.eventType();
      | eTpSy : tpDesc := f.enumType();
      | ptrSy : tpDesc := f.pointerType(tpTemp);
                IF tpDesc # NIL THEN 
                  bndTyp := tpDesc(Ty.Pointer).boundTp;
                  IF (bndTyp # NIL) & 
                     (bndTyp.kind = Ty.tmpTp) THEN
                    linkIx := bndTyp.dump - D.tOffset;
                  END;
                END;
      ELSE 
        tpDesc := Ty.newNamTp();
      END;
      IF tpIdnt # NIL THEN
       (*
        *  A name has been declared for this type, tpIdnt is
        *  the (possibly previously known) id descriptor, and
        *  tpDesc is the newly parsed descriptor of the type.
        *) 
        IF  tpIdnt.type = NIL THEN 
         (*
          *  Case #1: no previous type.
          *  This is the first time the compiler has seen this type
          *)
          tpIdnt.type := tpDesc;
          tpDesc.idnt := tpIdnt;
        ELSIF tpDesc IS Ty.Opaque THEN
         (* 
          *  Case #2: previous type exists, new type is opaque.
          *  Throw away the newly parsed opaque type desc, and
          *  use the previously known type *even* if it is opaque!
          *)
          tpDesc := tpIdnt.type;
        ELSIF tpIdnt.type IS Ty.Opaque THEN
         (*
          *  Case #3: previous type is opaque, new type is non-opaque.
          *  This type had been seen opaquely, but now has a 
          *  non-opaque definition
          *)
          tpIdnt.type(Ty.Opaque).resolved := tpDesc;
          tpIdnt.type := tpDesc;
          tpDesc.idnt := tpIdnt;
        ELSE
         (*
          *  Case #4: previous type is non-opaque, new type is non-opaque.
          *  This type already has a non-opaque descriptor.
          *  We shall keep the original copy.
          *)
          tpDesc := tpIdnt.type;
        END;
       (*
        *  Normally, imported types cannot be anonymous.
        *  However, there is one special case here. Anon
        *  records can be record base types, but are always 
        *  preceeded by the binding pointer type. A typical
        *  format of output from SymDec might be ---
        *
        *       T18 = SomeMod.BasePtr
        *             POINTER TO T19;
        *       T19 = EXTENSIBLE RECORD  (T11) ... END;
        *
        *  in this case T19 is an anon record from SomeMod,
        *  not the current module.
        *
        *  Thus we pre-override the future record declaration
        *  by the bound type of the pointer.  This ensures
        *  uniqueness of the record descriptor, even if it is
        *  imported indirectly multiple times.
        *)
        WITH tpDesc : Ty.Pointer DO
          IF linkIx # 0 THEN f.tArray.a[linkIx] := tpDesc.boundTp END;
        ELSE (* skip *)
        END;
        f.tArray.a[typIdx] := tpDesc;
      ELSE  
       (* 
        *  tpIdnt is NIL ==> type is from this import,
        *  except for the special case above.  In the usual
        *  case we replace the tmpTp by tpDesc. In the special
        *  case the tmpTp has been already been overridden by 
        *  the previously imported bound type.
        *)
        prevTp := f.tArray.a[typIdx];
        prevId := prevTp.idnt;
        IF (prevId # NIL) &
           (prevId.type.kind = Ty.namTp) THEN
          prevId.type(Ty.Opaque).resolved := tpDesc;
          prevId.type := tpDesc;
        END;
        tpDesc.idnt := prevId;
        f.tArray.a[typIdx] := tpDesc;
      END;
    END; (* while *)
   (*
    *  First we fix up all symbolic references in the
    *  the type array.  Postcondition is : no element
    *  of the type array directly or indirectly refers
    *  to a temporary type.
    *)
    FOR linkIx := 0 TO f.tArray.tide - 1 DO
      f.tArray.a[linkIx].TypeFix(f.tArray);
    END;
   (*
    *  We now fix up all references in the symbol table
    *  that still refer to temporary symbol-file types.
    *)
    NEW(typeFA);
    typeFA.sym := f;
    f.impS.symTb.Apply(typeFA); 
    f.ReadPast(close);
   (*
    *  Now check that all overloaded ids are necessary
    *)
    FOR linkIx := 0 TO f.oArray.tide - 1 DO
      f.oArray.a[linkIx].OverloadFix();
      f.oArray.a[linkIx] := NIL;
    END;
  END TypeList;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)SymFile(IN nm : ARRAY OF CHAR),NEW;
   (*
   // SymFile    = Header [String (falSy | truSy | <others>)]
   //		{Import | Constant | Variable | Type | Procedure} 
   //		TypeList Key.
   // Header     = magic modSy Name.
   //
   //  magic has already been recognized.
   *)
    VAR oldS : INTEGER;
  BEGIN
    f.ReadPast(modSy);
    IF f.sSym = namSy THEN (* do something with f.sAtt *)
      IF nm # f.sAtt THEN
        Error.WriteString("Wrong name in symbol file. Expected <");
        Error.WriteString(nm + ">, found <");
        Error.WriteString(f.sAtt + ">"); 
        Error.WriteLn;
        HALT(1);
      END;
      f.GetSym();
    ELSE RTS.Throw("Bad symfile header");
    END;
    IF f.sSym = strSy THEN (* optional name *)
      f.impS.scopeNm := LitValue.strToCharOpen(f.sAtt);
      f.GetSym();
      IF f.sSym = falSy THEN 
        INCL(f.impS.xAttr, D.isFn);
        f.GetSym();
      ELSIF f.sSym = truSy THEN
        INCL(f.impS.xAttr, D.isFn);
        INCL(f.impS.xAttr, D.fnInf);
        f.GetSym();
      ELSE RTS.Throw("Bad explicit name");
      END; 
    END; 
    IF f.sSym = numSy THEN (* optional strong name info.    *)
      NEW(f.impS.verNm);   (* POINTER TO ARRAY 6 OF INTEGER *)
      f.impS.verNm[0] := RTS.hiInt(f.lAtt);
      f.impS.verNm[1] := RTS.loInt(f.lAtt);
      f.GetSym();
      f.impS.verNm[2] := RTS.hiInt(f.lAtt);
      f.impS.verNm[3] := RTS.loInt(f.lAtt);
      f.GetSym();
      f.impS.verNm[4] := RTS.hiInt(f.lAtt);
      f.impS.verNm[5] := RTS.loInt(f.lAtt);
      f.GetSym();
      IF G.verbose THEN
        Console.WriteString("version:"); 
        Console.WriteInt(f.impS.verNm[0],1); Console.Write(".");
        Console.WriteInt(f.impS.verNm[1],1); Console.Write(".");
        Console.WriteInt(f.impS.verNm[2],1); Console.Write(".");
        Console.WriteInt(f.impS.verNm[3],1); 
        Console.WriteHex(f.impS.verNm[4],9);
        Console.WriteHex(f.impS.verNm[5],9); Console.WriteLn;
      END;
    END;
    LOOP
      oldS := f.sSym;
      f.GetSym();
      CASE oldS OF
      | start : EXIT;
      | typSy : f.Type();
      | impSy : f.Import();
      | conSy : Insert(f.constant(),  f.impS.symTb);
      | varSy : Insert(f.variable(),  f.impS.symTb);
      | prcSy : Insert(f.procedure(), f.impS.symTb);
      ELSE RTS.Throw("Bad object");
      END;
    END;
   (* Now read the typelist *)
    f.TypeList();
    IF f.sSym = keySy THEN
      IF f.impS.modKey = 0 THEN 
        f.impS.modKey := f.iAtt;
      ELSIF f.impS.modKey # f.iAtt THEN
        S.SemError.Report(173, S.line, S.col);	(* Detected bad KeyVal	*)
      END;
    ELSE RTS.Throw("Missing keySy");
    END; 
  END SymFile;

(* ============================================================ *)
(* ========	     SymFileSFA visitor method		======= *)
(* ============================================================ *)

  PROCEDURE (t : SymFileSFA)Op*(id : D.Idnt);
  BEGIN
    IF (id.kind = Id.impId) OR (id.vMod # D.prvMode) THEN
      CASE id.kind OF
      | Id.typId  : t.sym.EmitTypeId(id(Id.TypId));
      | Id.conId  : t.sym.EmitConstId(id(Id.ConId));
      | Id.impId  : t.sym.EmitImportId(id(Id.BlkId));
      | Id.varId  : t.sym.EmitVariableId(id(Id.VarId));
(* new *)
      | Id.conPrc : t.sym.EmitProcedureId(id(Id.PrcId));
(*
 * old ... we used to emit the constructor as a static method.
 *         Now it appears as a static in the bound record decl.
 *
 *    | Id.ctorP,
 *      Id.conPrc : t.sym.EmitProcedureId(id(Id.PrcId));
 *)
      ELSE (* skip *)
      END;
    END;
  END Op;

(* ============================================================ *)
(* ========	     TypeLinker visitor method		======= *)
(* ============================================================ *)

  PROCEDURE (t : TypeLinker)Op*(id : D.Idnt);
  BEGIN
    IF id.type = NIL THEN RETURN
    ELSIF id.type.kind = Ty.tmpTp THEN
      id.type := Ty.update(t.sym.tArray, id.type);
    ELSE
      id.type.TypeFix(t.sym.tArray);
    END;
    IF  (id IS Id.TypId) & 
        (id.type.idnt = NIL) THEN id.type.idnt := id END;
  END Op;

(* ============================================================ *)
(* ========	    Symbol file parser method		======= *)
(* ============================================================ *)

  PROCEDURE WalkThisImport(imp, mod : Id.BlkId);
    VAR syFil : SymFileReader;
        filNm : FileNames.NameString;
  BEGIN
    PushStack(imp);
    INCL(imp.xAttr, D.fixd);
    S.GetString(imp.token.pos, imp.token.len, filNm);
    syFil := newSymFileReader(mod);
    syFil.Parse(imp, filNm);
    PopStack;
  END WalkThisImport;
 
(* ============================================ *)

  PROCEDURE WalkImports*(IN imps : D.ScpSeq; modI : Id.BlkId);
    VAR indx : INTEGER;
        scpI : D.Scope;
        blkI : Id.BlkId;
  BEGIN
   (*
    *  The list of scopes has been constructed by
    *  the parser, while reading the import list.
    *  In the case of already known scopes the list
    *  references the original descriptor.
    *)
    InitStack;
    FOR indx := 0 TO imps.tide-1 DO
      scpI := imps.a[indx];
      blkI := scpI(Id.BlkId);
      IF blkI.kind = Id.alias THEN
        blkI.symTb  := blkI.dfScp.symTb;
      ELSIF ~(D.fixd IN blkI.xAttr) THEN 
        WalkThisImport(blkI,modI);
      END;
    END;
  END WalkImports;

(* ============================================================ *)
BEGIN
  lastKey := 0;
  fSepArr[0] := GF.fileSep;
END OldSymFileRW.
(* ============================================================ *)
