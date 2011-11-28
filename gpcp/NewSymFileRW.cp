
(* ==================================================================== *)
(*									*)
(*  SymFileRW:  Symbol-file reading and writing for GPCP.		*)
(*	Copyright (c) John Gough 1999 -- 2011.				*)
(*									*)
(* ==================================================================== *)

MODULE NewSymFileRW;

  IMPORT 
        GPCPcopyright,
        RTS,
        Error,
        Console,
        GF := GPFiles,
        BF := GPBinFiles,
        Id := IdDesc,
        D  := Symbols,
        Lt := LitValue,
        Visitor,
        ExprDesc,
        Ty := TypeDesc,
        B  := Builtin,
        S  := CPascalS,
        CSt:= CompState,
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
//
// Notes on the fine print about UTFstring --- November 2011 clarification.
// The character sequence in the symbol file is modified UTF-8, that is
// it may represent CHR(0), U+0000, by the bytes 0xC0, 0x80. String
// constants may thus contain embedded nulls. 
// 
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
        buffDefault = 1024;

(* ============================================================ *)

  TYPE
        SymFile = POINTER TO RECORD 
        	    file : BF.FILE;
        	    cSum : INTEGER;
        	    modS : Id.BlkId;
        	    iNxt : INTEGER;
        	    oNxt : INTEGER;
        	    work : D.TypeSeq;
                    (* Recycled scratch area *)
                    buff : POINTER TO ARRAY OF UBYTE; 
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
                    rScp  : ImpResScope;
        	    strLen : INTEGER;
        	    strAtt : Lt.CharOpen;
                    oArray : D.IdSeq;
        	    sArray : D.ScpSeq;		(* These two sequences	*)
  		    tArray : D.TypeSeq;		(* must be private as   *)
        	  END;				(* file parses overlap. *)

(* ============================================================ *)

  TYPE ImpResScope   = POINTER TO RECORD
                         work : D.ScpSeq;       (* Direct and ind imps. *)
                         host : Id.BlkId;       (* Compilation module.  *)
                       END;

(* ============================================================ *)

  TYPE	TypeLinker*  = POINTER TO RECORD (D.SymForAll) sym : SymFileReader END;
  TYPE	SymFileSFA*  = POINTER TO RECORD (D.SymForAll) sym : SymFile END;
  TYPE	ResolveAll*  = POINTER TO RECORD (D.SymForAll) END;

(* ============================================================ *)

  VAR   lastKey : INTEGER;	(* private state for CPMake *)
        fSepArr : ARRAY 2 OF CHAR;

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
    NEW(new.buff, buffDefault);
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

 (* ======================================= *
  *  This method writes a UTF-8 byte sequence that
  *  represents the input string up to but not
  *  including the terminating null character.
  *)
  PROCEDURE (f : SymFile)WriteNameUTF(IN nam : ARRAY OF CHAR),NEW;
    VAR num : INTEGER;
        idx : INTEGER;
        chr : INTEGER;
  BEGIN
    IF LEN(nam) * 3 > LEN(f.buff) THEN 
      NEW(f.buff, LEN(nam) * 3);
    END;

    num := 0;
    idx := 0;
    chr := ORD(nam[0]);
    WHILE chr # 0H DO
      IF    chr <= 7FH THEN 		(* [0xxxxxxx] *)
        f.buff[num] := USHORT(chr); INC(num);
      ELSIF chr <= 7FFH THEN 		(* [110xxxxx,10xxxxxx] *)
        f.buff[num+1] := USHORT(080H + chr MOD 64); chr := chr DIV 64;
        f.buff[num  ] := USHORT(0C0H + chr); INC(num, 2);
      ELSE 				(* [1110xxxx,10xxxxxx,10xxxxxxx] *)
        f.buff[num+2] := USHORT(080H + chr MOD 64); chr := chr DIV 64;
        f.buff[num+1] := USHORT(080H + chr MOD 64); chr := chr DIV 64;
        f.buff[num  ] := USHORT(0E0H + chr); INC(num, 3);
      END;
      INC(idx); chr := ORD(nam[idx]);
    END;
    f.Write(num DIV 256);
    f.Write(num MOD 256);
    FOR idx := 0 TO num-1 DO f.Write(f.buff[idx]) END;
  END WriteNameUTF;


 (* ======================================= *
  *  This method writes a UTF-8 byte sequence that
  *  represents the input string up to but not
  *  including the final null character. The 
  *  string may include embedded null characters.
  *  Thus if the last meaningfull character is null
  *  there will be two nulls at the end.
  *)
  PROCEDURE (f : SymFile)WriteStringUTF(chOp : Lt.CharOpen),NEW;
    VAR num : INTEGER;
        len : INTEGER;
        idx : INTEGER;
        chr : INTEGER;
  BEGIN
    len := LEN(chOp) - 1; (* Discard "terminating" null *)
    IF len * 3 > LEN(f.buff) THEN 
      NEW(f.buff, len * 3);
    END;

    num := 0;
    FOR idx := 0 TO len - 1 DO
      chr := ORD(chOp[idx]);
      IF chr = 0 THEN         (* [11000000, 10000000] *)
        f.buff[num+1] := 080H; 
        f.buff[num  ] := 0C0H; INC(num, 2);
      ELSIF chr <= 7FH THEN 		(* [0xxxxxxx] *)
        f.buff[num  ] := USHORT(chr); INC(num);
      ELSIF chr <= 7FFH THEN 		(* [110xxxxx,10xxxxxx] *)
        f.buff[num+1] := USHORT(080H + chr MOD 64); chr := chr DIV 64;
        f.buff[num  ] := USHORT(0C0H + chr); INC(num, 2);
      ELSE 				(* [1110xxxx,10xxxxxx,10xxxxxxx] *)
        f.buff[num+2] := USHORT(080H + chr MOD 64); chr := chr DIV 64;
        f.buff[num+1] := USHORT(080H + chr MOD 64); chr := chr DIV 64;
        f.buff[num  ] := USHORT(0E0H + chr); INC(num, 3);
      END;
    END;
    f.Write(num DIV 256);
    f.Write(num MOD 256);
    FOR idx := 0 TO num-1 DO f.Write(f.buff[idx]) END;
  END WriteStringUTF;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteStringForName(nam : Lt.CharOpen),NEW;
  BEGIN
    f.Write(strSy); 
    f.WriteNameUTF(nam);
  END WriteStringForName;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteStringForLit(str : Lt.CharOpen),NEW;
  BEGIN
    f.Write(strSy); 
    f.WriteStringUTF(str);
  END WriteStringForLit;

(* ======================================= *)

  PROCEDURE (f : SymFile)WriteNameForId(idD : D.Idnt),NEW;
  BEGIN
    f.Write(namSy); 
    f.Write(idD.vMod); 
    f.WriteNameUTF(Nh.charOpenOfHash(idD.hash));
  END WriteNameForId;

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
      IF ~CSt.legacy & (parI.hash # 0) THEN
        f.WriteStringForName(Nh.charOpenOfHash(parI.hash));
      END;
    END;
    f.Write(endFm);
  END FormalType;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitConstId(id : Id.ConId),NEW;
    VAR conX : ExprDesc.LeafX;
        cVal : Lt.Value;
        sVal : INTEGER;
  (*
  ** Constant = conSy Name Literal.
  ** Literal  = Number | String | Set | Char | Real | falSy | truSy.
  *)
  BEGIN
    conX := id.conExp(ExprDesc.LeafX);
    cVal := conX.value;
    f.Write(conSy);
    f.WriteNameForId(id);
    CASE conX.kind OF
    | ExprDesc.tBool  : f.Write(truSy);
    | ExprDesc.fBool  : f.Write(falSy);
    | ExprDesc.numLt  : f.WriteNum(cVal.long());
    | ExprDesc.charLt : f.WriteChar(cVal.char());
    | ExprDesc.realLt : f.WriteReal(cVal.real());
    | ExprDesc.strLt  : f.WriteStringForLit(cVal.chOpen());
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
    f.WriteNameForId(id);
    f.EmitTypeOrd(id.type);
  END EmitTypeId;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitVariableId(id : Id.VarId),NEW;
  (*
  ** Variable = varSy Name TypeOrd.
  *)
  BEGIN
    f.Write(varSy);
    f.WriteNameForId(id);
    f.EmitTypeOrd(id.type);
  END EmitVariableId;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitImportId(id : Id.BlkId),NEW;
  (*
  ** Import = impSy Name.
  *)
  BEGIN
    IF D.need IN id.xAttr THEN
      f.Write(impSy);
      f.WriteNameForId(id);
      IF id.scopeNm # NIL THEN f.WriteStringForName(id.scopeNm) END; 
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
    f.WriteNameForId(id);
    IF id.prcNm # NIL THEN f.WriteStringForName(id.prcNm) END; 
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
    f.WriteNameForId(id);
    f.Write(ORD(id.mthAtt));
    f.Write(id.rcvFrm.parMod);
    f.EmitTypeOrd(id.rcvFrm.type);
    IF id.prcNm # NIL THEN f.WriteStringForName(id.prcNm) END; 
    IF ~CSt.legacy & (id.rcvFrm.hash # 0) THEN f.WriteNameForId(id.rcvFrm) END;
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

  PROCEDURE (f : SymFile)isImportedPointer(ptr : Ty.Pointer) : BOOLEAN,NEW;
  BEGIN
    RETURN (ptr.idnt # NIL) & 
           (ptr.idnt.dfScp # NIL) & 
           (ptr.idnt.dfScp # f.modS);
  END isImportedPointer;

  PROCEDURE (f : SymFile)isImportedRecord(rec : Ty.Record) : BOOLEAN,NEW;
  BEGIN
    IF rec.bindTp # NIL THEN (* bindTp takes precedence *)
      RETURN f.isImportedPointer(rec.bindTp(Ty.Pointer));
    ELSIF rec.idnt # NIL THEN 
      RETURN (rec.idnt.dfScp # NIL) & (rec.idnt.dfScp # f.modS);
    ELSE 
      RETURN FALSE;
    END;
  END isImportedRecord;

  PROCEDURE (f : SymFile)isImportedArray(arr : Ty.Array) : BOOLEAN,NEW;
  BEGIN
    RETURN (arr.idnt # NIL) & 
           (arr.idnt.dfScp # NIL) & 
           (arr.idnt.dfScp # f.modS);
  END isImportedArray; 

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
      IF    id.type = CSt.ntvObj THEN RETURN CSt.objId;
      ELSIF id.type = CSt.ntvStr THEN RETURN CSt.strId;
      ELSIF id.type = CSt.ntvExc THEN RETURN CSt.excId;
      ELSIF id.type = CSt.ntvTyp THEN RETURN CSt.clsId;
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
(*
 *  mod := moduleOrd(t.idnt);
 *)
    mod := moduleOrd(idt);
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
      f.WriteNameForId(idt);
    END;
  END EmitTypeHeader;

(* ======================================= *)

  PROCEDURE (f : SymFile)EmitArrOrVecType(t : Ty.Array),NEW;
  BEGIN
    f.EmitTypeHeader(t);
    IF ~f.isImportedArray(t) THEN
(*
 *  IF t.force # D.noEmit THEN	(* Don't emit structure unless forced *)
 *)
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
    IF ~f.isImportedRecord(t) THEN
(*
 *  IF t.force # D.noEmit THEN	(* Don't emit structure unless forced *)
 *)
      f.Write(recSy);
      index := t.recAtt; 
      IF D.noNew IN t.xAttr THEN INC(index, Ty.noNew) END;
      IF D.clsTp IN t.xAttr THEN INC(index, Ty.clsRc) END;
      f.Write(index);
   (* ########## *)
      IF t.recAtt = Ty.iFace THEN
  	f.Write(truSy);
      ELSIF CSt.special OR (D.isFn IN t.xAttr) THEN  
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
        IF (field.vMod # D.prvMode) & (field.type # NIL) THEN
          f.WriteNameForId(field);
          f.EmitTypeOrd(field.type);
        END;
      END;
      FOR index := 0 TO t.methods.tide-1 DO
        method := t.methods.a[index];
        IF method.vMod # D.prvMode THEN
          f.EmitMethodId(method(Id.MthId));
        END;
      END;
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
    IF ~f.isImportedPointer(t) THEN
(*
 *  IF (t.force # D.noEmit) OR 			(* Only emit structure if *)
 *     (t.boundTp.force # D.noEmit) THEN	(* ptr or boundTp forced. *)
 *)
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
        fNamePtr : Lt.CharOpen;
  (* ----------------------------------- *)
    PROCEDURE mkPathName(m : D.Idnt) : Lt.CharOpen;
      VAR str : Lt.CharOpen;
    BEGIN
      str := BOX(CSt.symDir);
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
    IF CSt.symDir # "" THEN
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
      IF CSt.verbose THEN CSt.Message("Created " + fNamePtr^) END;
     (* End of alternative gpcp1.2 code *)
      IF D.rtsMd IN m.xAttr THEN
        marker := RTS.loInt(syMag);	(* ==> a system module *)
      ELSE
        marker := RTS.loInt(magic);	(* ==> a normal module *)
      END;
      symfile.Write4B(RTS.loInt(marker));
      symfile.Write(modSy);
      symfile.WriteNameForId(m);
      IF m.scopeNm # NIL THEN (* explicit name *)
        symfile.WriteStringForName(m.scopeNm); 
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
      IF CSt.special THEN symfile.Write4B(0) ELSE symfile.Write4B(lastKey) END;
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

  PROCEDURE (rdr : SymFileReader)ReadUTF(), NEW;
    CONST
        bad = "Bad UTF-8 string";
    VAR num : INTEGER;
        bNm : INTEGER;
        len : INTEGER;
        idx : INTEGER;
        chr : INTEGER;
        fil : BF.FILE;
  BEGIN
    num := 0;
    fil := rdr.file;
   (* 
    *  len is the length in bytes of the UTF8 representation 
    *)
    len := read(fil) * 256 + read(fil);  (* max length 65k *)
   (* 
    *  Worst case the number of chars will equal byte-number.
    *)
    IF LEN(rdr.strAtt) <= len THEN 
      NEW(rdr.strAtt, len + 1);
    END;

    idx := 0;
    WHILE idx < len DO
      chr := read(fil); INC(idx);
      IF chr <= 07FH THEN		(* [0xxxxxxx] *)
        rdr.strAtt[num] := CHR(chr); INC(num);
      ELSIF chr DIV 32 = 06H THEN	(* [110xxxxx,10xxxxxx] *)
        bNm := chr MOD 32 * 64;
        chr := read(fil); INC(idx);
        IF chr DIV 64 = 02H THEN
          rdr.strAtt[num] := CHR(bNm + chr MOD 64); INC(num);
        ELSE
          RTS.Throw(bad);
        END;
      ELSIF chr DIV 16 = 0EH THEN	(* [1110xxxx,10xxxxxx,10xxxxxxx] *)
        bNm := chr MOD 16 * 64;
        chr := read(fil); INC(idx);
        IF chr DIV 64 = 02H THEN
          bNm := (bNm + chr MOD 64) * 64; 
          chr := read(fil); INC(idx);
          IF chr DIV 64 = 02H THEN
            rdr.strAtt[num] := CHR(bNm + chr MOD 64); INC(num);
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
    rdr.strAtt[num] := 0X;
    rdr.strLen := num;
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
    NEW(new.strAtt, buffDefault);
    RETURN new;
  END newSymFileReader;

(* ======================================= *)
  PROCEDURE^ (f : SymFileReader)SymFile(IN nm : ARRAY OF CHAR),NEW;
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
        f.iAtt := read(file); f.ReadUTF();
    | strSy : 
        f.ReadUTF();
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

  PROCEDURE (f : SymFileReader)Parse*(scope : Id.BlkId),NEW;
    VAR filNm    : Lt.CharOpen;
        fileName : Lt.CharOpen;
        message  : Lt.CharOpen;
        marker   : INTEGER;
        token    : S.Token;
        index    : INTEGER;
        
    PROCEDURE NameAndKey(idnt : D.Scope) : Lt.CharOpen;
      VAR name : Lt.CharOpen;
          keyV : INTEGER;
    BEGIN
      WITH idnt : Id.BlkId DO
        RETURN BOX(Nh.charOpenOfHash(idnt.hash)^ + 
                   " : "  + Lt.intToCharOpen(idnt.modKey)^);
      ELSE
        RETURN BOX("bad idnt");
      END; 
    END NameAndKey;
    
  BEGIN
    message := NIL;
    token := scope.token;
    IF token = NIL THEN token := S.prevTok END;
    filNm := Nh.charOpenOfHash(scope.hash);
    
    f.impS := scope;
    D.AppendScope(f.sArray, scope);
    fileName := BOX(filNm^ + ".cps");
    f.file := BF.findOnPath(CSt.cpSymX$, fileName);
   (* #### *)
    IF f.file = NIL THEN
      fileName := BOX("__" + fileName^);
      f.file := BF.findOnPath(CSt.cpSymX$, fileName);
      IF f.file # NIL THEN
        S.SemError.RepSt2(309, filNm, fileName, token.lin, token.col);
        filNm := BOX("__" + filNm^);
        scope.clsNm := filNm;
      END;
    END;
   (* #### *)
    IF f.file = NIL THEN
      (* S.SemError.Report(129, token.lin, token.col); *)
      S.SemError.RepSt1(129, BOX(filNm^ + ".cps"), token.lin, token.col); 
      RETURN;
    ELSE
      IF CSt.verbose THEN 
        IF D.weak IN scope.xAttr THEN
          message := BOX("Implicit import " + filNm^);
        ELSE
          message := BOX("Explicit import " + filNm^);
        END;
      END;
      marker := readInt(f.file);
      IF marker = RTS.loInt(magic) THEN
        (* normal case, nothing to do *)
      ELSIF marker = RTS.loInt(syMag) THEN
        INCL(scope.xAttr, D.rtsMd);
      ELSE
        (* S.SemError.Report(130, token.lin, token.col); *)
        S.SemError.RepSt1(130, BOX(filNm^ + ".cps"), token.lin, token.col); 
        RETURN;
      END;
      f.GetSym();
      f.SymFile(filNm);
      IF CSt.verbose THEN 
        CSt.Message(message^ + ", Key: " + Lt.intToCharOpen(f.impS.modKey)^);
        FOR index := 0 TO f.sArray.tide - 1 DO
          CSt.Message("  imports " + NameAndKey(f.sArray.a[index])^);
        END;
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
    | strSy : expr := ExprDesc.mkStrLenLt(f.strAtt, f.strLen);
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
      IF f.sSym = strSy THEN (* parD.hash := Nh.enterStr(f.strAtt); *)
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
   (* 
    *  Do not override extrnNm values set
    *  by *Maker.Init for Native* types.
    *)
    IF (f.impS.scopeNm # NIL) & (rslt.extrnNm = NIL) THEN
      rslt.extrnNm := f.impS.scopeNm; 
    END;

    IF f.sSym = basSy THEN
     (* 
      *  Do not override baseTp values set
      *  by *Maker.Init for Native* types.
      *)
      IF rslt.baseTp = NIL THEN
        rslt.baseTp := f.typeOf(f.iAtt);
        IF f.iAtt # Ty.anyRec THEN INCL(rslt.xAttr, D.clsTp) END;
      END;
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
      fldD.hash := Nh.enterStr(f.strAtt);
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
    * Post: every previously unknown typId 'id'
    *	has the property:  id.type.idnt = id.
    *   If oldI # newT, then the new typId has
    *   newT.type.idnt = oldI.
    *)
    newI := Id.newTypId(NIL);
    newI.SetMode(f.iAtt);
    newI.hash := Nh.enterStr(f.strAtt);
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
    impD.hash := Nh.enterStr(f.strAtt);
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
        impD.scopeNm := Lt.arrToCharOpen(f.strAtt, f.strLen);
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
      ELSE
        D.AppendScope(f.sArray, impD);
      END;
      impD.modKey := f.iAtt;
    END;
    f.ReadPast(keySy);
  END Import;

(* ============================================ *)

  PROCEDURE (f : SymFileReader)constant() : Id.ConId,NEW;
  (* Constant = conSy Name Literal.		*)
  (* Name     = namSy byte UTFstring.           *)
  (* Assert: f.sSym = namSy.			*)
    VAR newC : Id.ConId;
        anyI : D.Idnt;
  BEGIN
    newC := Id.newConId();
    newC.SetMode(f.iAtt);
    newC.hash := Nh.enterStr(f.strAtt);
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
    newV.hash := Nh.enterStr(f.strAtt);
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
    newP.hash := Nh.enterStr(f.strAtt);
    newP.dfScp := f.impS;
    f.ReadPast(namSy);
    IF f.sSym = strSy THEN 
      newP.prcNm := Lt.arrToCharOpen(f.strAtt, f.strLen);
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
    newM.hash := Nh.enterStr(f.strAtt);
    newM.dfScp := f.impS;
    IF CSt.verbose THEN newM.SetNameFromHash(newM.hash) END;
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
      newM.prcNm := Lt.arrToCharOpen(f.strAtt, f.strLen);
     (* and leave scopeNm = NIL *)
      f.GetSym();
    END;
   (* Skip over optional receiver name string *)
    IF f.sSym = namSy THEN (* rcvD.hash := Nh.enterString(f.strAtt); *)
      f.GetSym();
    END;
   (* End skip over optional receiver name *)
    newM.type := f.getFormalType(Ty.newPrcTp(), 1);
    newM.type.idnt := newM;
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
        tpIdnt : Id.TypId;
        prevTp : D.Type;
        impScp : D.Scope;
        basBlk : Id.BlkId;
        linkIx : INTEGER;
        bndTyp : D.Type;
        typeFA : TypeLinker;

   (* ================================ *)
    PROCEDURE getDetails(f : SymFileReader; p : D.Type) : D.Type;
    BEGIN
      CASE f.sSym OF
      | arrSy : RETURN f.arrayType();
      | vecSy : RETURN f.vectorType();
      | recSy : RETURN f.recordType(p);
      | pTpSy : RETURN f.procedureType();
      | evtSy : RETURN f.eventType();
      | eTpSy : RETURN f.enumType();
      | ptrSy : RETURN f.pointerType(p);
      ELSE 
                RETURN Ty.newNamTp();
      END;
    END getDetails;
   (* ================================ *)
  BEGIN
    WHILE f.sSym = tDefS DO
      linkIx := 0;
      tpIdnt := NIL;
      impScp := NIL;

     (* Do type header *)
      typOrd := f.iAtt;
      typIdx := typOrd - D.tOffset;
      prevTp := f.tArray.a[typIdx];
      f.ReadPast(tDefS);
     (*
      *  The [fromS modOrd typNam] appears if the type is imported.
      *  There are two cases:
      *  (1) this is the first time that "mod.typNam" has been 
      *      seen during this compilation 
      *                   ==> insert a new typId descriptor in mod.symTb
      *  (2) this name is already in the mod.symTb table
      *                   ==> fetch the previous descriptor
      *)
      IF f.sSym = fromS THEN
        modOrd := f.iAtt;
        impScp := f.sArray.a[modOrd];
        f.GetSym();
        tpIdnt := Id.newTypId(NIL);
        tpIdnt.SetMode(f.iAtt);
        tpIdnt.hash := Nh.enterStr(f.strAtt);
        tpIdnt.dfScp := impScp;
        tpIdnt := testInsert(tpIdnt, impScp)(Id.TypId);
        f.ReadPast(namSy);
        tpDesc := getDetails(f, prevTp);
       (*
        *  In the new symbol table format we do not wish
        *  to include details of indirectly imported types.
        *  However, there may be a reference to the bound
        *  type of an indirectly imported pointer.  In this
        *  case we need to make sure that the otherwise
        *  bound type declaration catches the same opaque
        *  type descriptor.
        *)
        IF tpDesc # NIL THEN
          WITH tpDesc : Ty.Pointer DO
            bndTyp := tpDesc.boundTp;
            IF (bndTyp # NIL) & (bndTyp.kind = Ty.tmpTp) THEN
              linkIx := bndTyp.dump - D.tOffset;
            END;
          ELSE (* skip *)
          END;
        END;
        tpDesc := Ty.newNamTp();
        tpDesc.idnt := tpIdnt;
        IF linkIx # 0 THEN 
          ASSERT(linkIx > typIdx);
          f.tArray.a[linkIx] := tpDesc;
        END;
       (*
        *  A name has been declared for this type, tpIdnt is
        *  the (possibly previously known) id descriptor, and
        *  tpDesc is the newly parsed descriptor of the type.
        *) 
        IF tpIdnt.type = NIL THEN
          tpIdnt.type := tpDesc;
        ELSE
          tpDesc := tpIdnt.type;
        END;
        IF tpDesc.idnt = NIL THEN tpDesc.idnt := tpIdnt END;
      ELSE
        tpDesc := getDetails(f, prevTp); 
        ASSERT(tpDesc # NIL);
        IF (prevTp # NIL) &
           (prevTp.idnt # NIL) THEN
          IF (prevTp.kind = Ty.namTp) &
             (prevTp.idnt.dfScp # f.impS) THEN
           (*
            *  This is the special case of an anonymous
            *  bound type of an imported pointer.  In the
            *  new type resolver we want this to remain
            *  as an opaque type until *all* symbol files
            *  have been fully processed.
            *  So ... override the parsed type.
            *)
            tpDesc := prevTp;
          ELSE 
            prevTp.idnt.type := tpDesc;  (* override opaque *)
            tpDesc.idnt := prevTp.idnt;
          END;
        END;
       (* 
        *  This is the normal case
        *)
        WITH tpDesc : Ty.Pointer DO
          bndTyp := tpDesc.boundTp;
          IF (bndTyp # NIL) & (bndTyp.kind = Ty.tmpTp) THEN
            linkIx := bndTyp.dump - D.tOffset;
            IF linkIx # 0 THEN 
              ASSERT(linkIx > typIdx);
              f.tArray.a[linkIx] := tpDesc.boundTp;
            END;
          END;
        ELSE (* skip *)
        END;
      END;
      f.tArray.a[typIdx] := tpDesc;
    END; (* while *)
    FOR linkIx := 0 TO f.tArray.tide - 1 DO
      tpDesc := f.tArray.a[linkIx];
     (*
      *  First we fix up all symbolic references in the
      *  the type array.  Postcondition is : no element
      *  of the type array directly or indirectly refers
      *  to a temporary type.
      *)
      tpDesc.TypeFix(f.tArray);
    END;
    FOR linkIx := 0 TO f.tArray.tide - 1 DO
      tpDesc := f.tArray.a[linkIx];
     (*
      *  At this stage we want to check the base types
      *  of every defined record type.  If the base type
      *  is imported then we check. 
      *  Define 'set' := dfScp.xAttr * {weak, need}; then ...
      *
      *    set = {D.need}            ==> module is explicitly imported
      *
      *    set = {D.weak}            ==> module must be imported, but is not
      *                                   on the import worklist at this stage
      *    set = {D.weak, D.need}   ==> module must be imported, and is 
      *                                   already on the import worklist.
      *)
      IF tpDesc # NIL THEN
        WITH tpDesc : Ty.Record DO
          IF tpDesc.baseTp # NIL THEN
            prevTp := tpDesc.baseTp;
            IF (prevTp.kind = Ty.namTp) &
               (prevTp.idnt # NIL) &
               (prevTp.idnt.dfScp # NIL) THEN
              basBlk := prevTp.idnt.dfScp(Id.BlkId);
              IF basBlk.xAttr * {D.weak, D.need} = {D.weak} THEN
                INCL(basBlk.xAttr, D.need);
                D.AppendScope(f.rScp.work, prevTp.idnt.dfScp);
              END;
            END;
          END;
        ELSE (* skip *)
        END; (* with *)
      END;
    END; (* for linkIx do *)
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
    IF f.sSym = namSy THEN (* do something with f.strAtt *)
      IF nm # f.strAtt^ THEN
        Error.WriteString("Wrong name in symbol file. Expected <");
        Error.WriteString(nm + ">, found <");
        Error.WriteString(f.strAtt^ + ">"); 
        Error.WriteLn;
        HALT(1);
      END;
      f.GetSym();
    ELSE RTS.Throw("Bad symfile header");
    END;
    IF f.sSym = strSy THEN (* optional name *)
      f.impS.scopeNm := Lt.arrToCharOpen(f.strAtt, f.strLen);
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
      IF CSt.verbose THEN
        Console.WriteString("version:"); 
        Console.WriteInt(f.impS.verNm[0],1); Console.Write(".");
        Console.WriteInt(f.impS.verNm[1],1); Console.Write(".");
        Console.WriteInt(f.impS.verNm[2],1); Console.Write(".");
        Console.WriteInt(f.impS.verNm[3],1); 
        Console.WriteHex(f.impS.verNm[4],9);
        Console.WriteHex(f.impS.verNm[5],9); Console.WriteLn;
      END;
      (*
      //  The CPS format only provides for version information if
      //  there is also a strong key token. Do not propagate random
      //  junk with PeToCps from assemblies with version info only
      *)
      IF (f.impS.verNm[4] = 0) OR (f.impS.verNm[5] = 0) THEN
        f.impS := NIL;
      END;
    END;
    LOOP
      oldS := f.sSym;
      f.GetSym();
      CASE oldS OF
      | start : EXIT;
      | typSy : f.Type();                             (* Declare public tp *)
      | impSy : f.Import();                           (* Declare an import *)
      | conSy : Insert(f.constant(),  f.impS.symTb);  (* Const. definition *)
      | varSy : Insert(f.variable(),  f.impS.symTb);  (* Var. declaration  *)
      | prcSy : Insert(f.procedure(), f.impS.symTb);  (* Proc. declaration *)
      ELSE RTS.Throw("Bad object");
      END;
    END;
   (* 
    *  Now read the typelist. 
    *)
    f.TypeList();
   (* 
    *  Now check the module key.
    *)
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
      | Id.conPrc : t.sym.EmitProcedureId(id(Id.PrcId));
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
(* ========	     ResolveAll visitor method		======= *)
(* ============================================================ *)

  PROCEDURE (t : ResolveAll)Op*(id : D.Idnt);
  BEGIN
    IF id.type # NIL THEN id.type := id.type.resolve(1) END;
  END Op;

(* ============================================================ *)
(* ========	    Symbol file parser method		======= *)
(* ============================================================ *)

  PROCEDURE (res : ImpResScope)ReadThisImport(imp : Id.BlkId),NEW;
    VAR syFil : SymFileReader;
  BEGIN
    INCL(imp.xAttr, D.fixd);
    syFil := newSymFileReader(res.host);
    syFil.rScp := res;
    syFil.Parse(imp);
  END ReadThisImport;

(* ============================================ *)

  PROCEDURE WalkImports*(VAR imps : D.ScpSeq; modI : Id.BlkId);
    VAR indx : INTEGER;
        blkI : Id.BlkId;
        fScp : ImpResScope;
        rAll : ResolveAll;
  BEGIN
   (*
    *  The list of scopes has been constructed by
    *  the parser, while reading the import list.
    *  In the case of already known scopes the list
    *  references the original descriptor.
    *
    *  Unlike the previous version (SymFileRW) this
    *  routine may mutate the length of the sequence.
    *)
    NEW(fScp);
   (*
    *  Copy the incoming sequence.
    *)
    fScp.work := imps;
    fScp.host := modI;
   (*
    *  Now import modules on the list.
    *)
    indx := 0;
    WHILE indx < fScp.work.tide DO
      blkI := fScp.work.a[indx](Id.BlkId);
      IF blkI.kind = Id.alias THEN
        blkI.symTb  := blkI.dfScp.symTb;
      ELSIF ~(D.fixd IN blkI.xAttr) THEN 
        fScp.ReadThisImport(blkI);
      END;
      INC(indx);
    END;
    FOR indx := 0 TO fScp.work.tide-1 DO
      blkI := fScp.work.a[indx](Id.BlkId);
      NEW(rAll);
      blkI.symTb.Apply(rAll);
    END;
   (*
    *  Copy the (possibly mutated) sequence out.
    *)
    imps := fScp.work;
  END WalkImports;

(* ============================================================ *)
BEGIN
  lastKey := 0;
  fSepArr[0] := GF.fileSep;
END NewSymFileRW.
(* ============================================================ *)
