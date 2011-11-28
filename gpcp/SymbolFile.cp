(* ==================================================================== *)
(*                                                                      *)
(*  SymFileRW:  Symbol-file reading and writing for GPCP.               *)
(*      Copyright (c) John Gough 1999, 2000.                            *)
(*                                                                      *)
(* ==================================================================== *)

MODULE SymbolFile;

  IMPORT 
        GPCPcopyright,
        RTS,
        Error,
        GPBinFiles,
        FileNames,
        LitValue,
        CompState,
        MH := ModuleHandler;

(* ========================================================================= *
// Collected syntax ---
// 
// SymFile    = Header [String (falSy | truSy | <other attribute>)]
//              [ VersionName ]
//              {Import | Constant | Variable | Type | Procedure} 
//              TypeList Key.
//      -- optional String is external name.
//      -- falSy ==> Java class
//      -- truSy ==> Java interface
//      -- others ...
// Header     = magic modSy Name.
// VersionName= numSy longint numSy longint numSy longint.
//      --            mj# mn#       bld rv#    8xbyte extract
// Import     = impSy Name [String] Key.
//      -- optional string is explicit external name of class
// Constant   = conSy Name Literal.
// Variable   = varSy Name TypeOrd.
// Type       = typSy Name TypeOrd.
// Procedure  = prcSy Name [String] FormalType.
//      -- optional string is explicit external name of procedure
// Method     = mthSy Name byte byte TypeOrd [String] FormalType.
//      -- optional string is explicit external name of method
// FormalType = [retSy TypeOrd] frmSy {parSy byte TypeOrd} endFm.
//      -- optional phrase is return type for proper procedures
// TypeOrd    = ordinal.
// TypeHeader = tDefS Ord [fromS Ord Name].
//      -- optional phrase occurs if:
//      -- type not from this module, i.e. indirect export
// TypeList   = start { Array | Record | Pointer | ProcType } close.
// Array      = TypeHeader arrSy TypeOrd (Byte | Number | <empty>) endAr.
//      -- nullable phrase is array length for fixed length arrays
// Vector     = TypeHeader vecSy TypeOrd endAr.
// Pointer    = TypeHeader ptrSy TypeOrd.
// EventType  = TypeHeader evtSy FormalType.
// ProcType   = TypeHeader pTpSy FormalType.
// Record     = TypeHeader recSy recAtt [truSy | falSy] 
//              [basSy TypeOrd] [ iFcSy {basSy TypeOrd}]
//              {Name TypeOrd} {OtherStuff} endRc.
//      -- truSy ==> is an extension of external interface
//      -- falSy ==> is an extension of external class
//      -- basSy option defines base type, if not ANY / j.l.Object
// OtherStuff = Method | Procedure | Variable | Constant.
// Enum       = TypeHeader eTpSy { Constant } endRc.
// Name       = namSy byte UTFstring.
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

  VAR
    file* : GPBinFiles.FILE;
    fileName* : FileNames.NameString;
    sSym : INTEGER;
    cAtt : CHAR;
    iAtt : INTEGER;
    lAtt : LONGINT;
    rAtt : REAL;
    sAtt : LitValue.CharOpen;

(* ============================================================ *)
(* ========     Various reading utility procedures      ======= *)
(* ============================================================ *)

  PROCEDURE read() : INTEGER;
  BEGIN
    RETURN GPBinFiles.readByte(file);
  END read;

(* ======================================= *)

  PROCEDURE readUTF() : LitValue.CharOpen; 
    CONST
      bad = "Bad UTF-8 string";
    VAR num : INTEGER;
      bNm : INTEGER;
      len : INTEGER;
      idx : INTEGER;
      chr : INTEGER;
      buff : LitValue.CharOpen;
  BEGIN
    num := 0;
   (* 
    *  bNm is the length in bytes of the UTF8 representation 
    *)
    len := read() * 256 + read();  (* max length 65k *)
   (* 
    *  Worst case the number of chars will equal byte-number.
    *)
    NEW(buff, len + 1); 
    idx := 0;
    WHILE idx < len DO
      chr := read(); INC(idx);
      IF chr <= 07FH THEN		(* [0xxxxxxx] *)
        buff[num] := CHR(chr); INC(num);
      ELSIF chr DIV 32 = 06H THEN	(* [110xxxxx,10xxxxxx] *)
        bNm := chr MOD 32 * 64;
        chr := read(); INC(idx);
        IF chr DIV 64 = 02H THEN
          buff[num] := CHR(bNm + chr MOD 64); INC(num);
        ELSE
          RTS.Throw(bad);
        END;
      ELSIF chr DIV 16 = 0EH THEN	(* [1110xxxx,10xxxxxx,10xxxxxxx] *)
        bNm := chr MOD 16 * 64;
        chr := read(); INC(idx);
        IF chr DIV 64 = 02H THEN
          bNm := (bNm + chr MOD 64) * 64; 
          chr := read(); INC(idx);
          IF chr DIV 64 = 02H THEN
            buff[num] := CHR(bNm + chr MOD 64); INC(num);
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
    buff[num] := 0X;
    RETURN LitValue.arrToCharOpen(buff, num);
  END readUTF;

(* ======================================= *)

  PROCEDURE readChar() : CHAR;
  BEGIN
    RETURN CHR(read() * 256 + read());
  END readChar;

(* ======================================= *)

  PROCEDURE readInt() : INTEGER;
  BEGIN [UNCHECKED_ARITHMETIC]
    (* overflow checking off here *)
    RETURN ((read() * 256 + read()) * 256 + read()) * 256 + read();
  END readInt;

(* ======================================= *)

  PROCEDURE readLong() : LONGINT;
    VAR result : LONGINT;
        index  : INTEGER;
  BEGIN [UNCHECKED_ARITHMETIC]
    (* overflow checking off here *)
    result := read();
    FOR index := 1 TO 7 DO
      result := result * 256 + read();
    END;
    RETURN result;
  END readLong;

(* ======================================= *)

  PROCEDURE readReal() : REAL;
    VAR result : LONGINT;
  BEGIN
    result := readLong();
    RETURN RTS.longBitsToReal(result);
  END readReal;

(* ======================================= *)

  PROCEDURE readOrd() : INTEGER;
    VAR chr : INTEGER;
  BEGIN
    chr := read();
    IF chr <= 07FH THEN RETURN chr;
    ELSE
      DEC(chr, 128);
      RETURN chr + read() * 128;
    END;
  END readOrd;

(* ============================================================ *)
(* ========             Symbol File Reader              ======= *)
(* ============================================================ *)

  PROCEDURE SymError(IN msg : ARRAY OF CHAR);
  BEGIN
    Error.WriteString("Error in <" + fileName + "> : ");
    Error.WriteString(msg); Error.WriteLn;
  END SymError;

(* ======================================= *)

  PROCEDURE GetSym();
  BEGIN
    sSym := read();
    CASE sSym OF
    | namSy : 
        iAtt := read(); 
        sAtt := readUTF();
    | strSy : 
        sAtt := readUTF();
    | retSy, fromS, tDefS, basSy :
        iAtt := readOrd();
    | bytSy :
        iAtt := read();
    | keySy, setSy :
        iAtt := readInt();
    | numSy :
        lAtt := readLong();
    | fltSy :
        rAtt := readReal();
    | chrSy :
        cAtt := readChar();
    ELSE (* nothing to do *)
    END;
  END GetSym;

(* ======================================= *)

  PROCEDURE Check(sym : INTEGER);
  BEGIN
    IF sSym # sym THEN
      Error.WriteString("Expected " );
      Error.WriteInt(sym,0);
      Error.WriteString(" but got " );
      Error.WriteInt(sSym,0);
      Error.WriteLn;
      THROW("Bad symbol file format");
    END;
  END Check;

  PROCEDURE CheckAndGet(sym : INTEGER);
  VAR
    ok : BOOLEAN;
  BEGIN
    IF sSym # sym THEN
      Error.WriteString("Expected " );
      Error.WriteInt(sym,0);
      Error.WriteString(" but got " );
      Error.WriteInt(sSym,0);
      Error.WriteLn;
      THROW("Bad symbol file format");
    END;
    GetSym();
  END CheckAndGet;

(* ======================================= *)

  PROCEDURE OpenSymbolFile*(IN name : ARRAY OF CHAR; onPath : BOOLEAN);
  BEGIN
    fileName := name + ".cps";
    IF onPath THEN
      file := GPBinFiles.findOnPath(CompState.cpSymX, fileName);
    ELSE
      file := GPBinFiles.findLocal(fileName);
    END;
  END OpenSymbolFile;

(* ======================================= *)


  PROCEDURE SkipFormalType();
  (*
  // FormalType = [retSy TypeOrd] frmSy {parSy byte TypeOrd} endFm.
  //    -- optional phrase is return type for proper procedures
  *)
    VAR 
        byte : INTEGER;
  BEGIN
    IF sSym = retSy THEN GetSym(); END;
    CheckAndGet(frmSy);
    WHILE sSym = parSy DO
      byte := read();
      byte := readOrd();
      GetSym();
      IF sSym = strSy THEN GetSym() END;
    END;
    CheckAndGet(endFm);
  END SkipFormalType;

(* ============================================ *)

  PROCEDURE TypeList();
  (* TypeList   = start { Array | Record | Pointer | ProcType } close. *)
  (* TypeHeader = tDefS Ord [fromS Ord Name].                          *)
    VAR 
      num, oldS : INTEGER;
      tmp : INTEGER;
  BEGIN
    WHILE sSym = tDefS DO
      GetSym(); 
      IF sSym = fromS THEN
        GetSym();  (* fromS *)
        GetSym();  (* Name *)
      END;
     (* Get type info. *)
      CASE sSym OF
      | arrSy : num := readOrd();
                GetSym(); 
                IF (sSym = bytSy) OR (sSym = numSy) THEN GetSym(); END;
                CheckAndGet(endAr);
      | vecSy : num := readOrd();
                GetSym();
                CheckAndGet(endAr);
      | eTpSy : GetSym();
                WHILE sSym = conSy DO
                  GetSym();  (* read past conSy *)
                  CheckAndGet(namSy);
                  GetSym();  (* read past literal *)
                END;
                CheckAndGet(endRc);
      | recSy : num := read(); 
                GetSym();
                IF (sSym = falSy) OR (sSym = truSy) THEN GetSym(); END;
                IF (sSym = basSy) THEN GetSym(); END;
                IF sSym = iFcSy THEN
                  GetSym();
                  WHILE sSym = basSy DO GetSym() END;
                END;
                WHILE sSym = namSy DO num := readOrd(); GetSym(); END;
                WHILE (sSym = mthSy) OR (sSym = conSy) OR 
                      (sSym = prcSy) OR (sSym = varSy) DO
                  oldS := sSym; GetSym();
                  IF oldS = mthSy THEN 
                    (* mthSy Name byte byte TypeOrd [String] FormalType. *)
                     Check(namSy);
                     num := read();
                     num := read();
                     num := readOrd();
                     GetSym();
                     IF sSym = strSy THEN GetSym(); END;
                     IF sSym = namSy THEN GetSym(); END;
                     SkipFormalType();
                  ELSIF oldS = conSy THEN (* Name Literal *)
                     CheckAndGet(namSy);
                     GetSym();  
                  ELSIF oldS = prcSy THEN (* Name [String] FormalType. *)
                     CheckAndGet(namSy);   
                     IF sSym = strSy THEN GetSym(); END;
                     IF sSym = truSy THEN GetSym(); END;
                     SkipFormalType();
                  ELSE (* Name TypeOrd. *)
                    Check(namSy);
                    tmp := readOrd(); 
                    GetSym(); 
                  END;
                END;
                CheckAndGet(endRc);
      | ptrSy : num := readOrd(); GetSym(); 
      | pTpSy, evtSy : GetSym(); SkipFormalType();
      ELSE (* skip *)
      END;
    END;
    GetSym();
  END TypeList;

(* ============================================ *)

  PROCEDURE ReadSymbolFile*(mod : MH.ModInfo; addKeys : BOOLEAN);
  (*
  // SymFile    = Header [String (falSy | truSy | <others>)]
  //            {Import | Constant | Variable | Type | Procedure} 
  //            TypeList Key.
  // Header     = magic modSy Name.
  //
  *)
  VAR 
    marker   : INTEGER;
    oldS,tmp : INTEGER;
    impMod   : MH.ModInfo;
  BEGIN
    impMod := NIL;
    marker := readInt();
    IF (marker = RTS.loInt(magic)) OR (marker = RTS.loInt(syMag)) THEN
      (* normal case, nothing to do *)
    ELSE
      SymError("Bad symbol file format."); 
      RETURN;
    END;
    GetSym();
    CheckAndGet(modSy);
    Check(namSy);
    IF mod.name^ # sAtt^ THEN 
      SymError("Wrong name in symbol file. Expected <" + mod.name^ + 
                ">, found <" + sAtt^ + ">"); 
      RETURN;
    END;
    GetSym();
    IF sSym = strSy THEN (* optional name *)
      GetSym();
      IF (sSym = falSy) OR (sSym = truSy) THEN 
        GetSym();
      ELSE 
        SymError("Bad explicit name in symbol file.");
        RETURN;
      END; 
    END; 

    IF sSym = numSy THEN   (* optional strong name info.    *)
      (* ignore major, minor and get next symbol *)
      GetSym();
      (* ignore build, revision and get next symbol *)
      GetSym();
      (* ignore assembly publickeytoken and get next symbol *)
      GetSym();
    END;

    LOOP
      oldS := sSym;
      GetSym();
      CASE oldS OF
      | start : EXIT;
      | typSy, varSy : tmp := readOrd(); GetSym(); (* Name typeOrd *)
      | impSy : IF addKeys THEN impMod := MH.GetModule(sAtt); END;
                GetSym();
                IF sSym = strSy THEN GetSym(); END;
                Check(keySy);
                IF addKeys THEN MH.AddKey(mod,impMod,iAtt); END;
                GetSym();
      | conSy : GetSym(); GetSym();  (* Name Literal *)
      | prcSy : (* Name [String] FormalType *);
                GetSym(); 
                IF sSym = strSy THEN GetSym(); END;
                SkipFormalType();
      ELSE SymError("Bad symbol file format."); EXIT;
      END;
    END;
    TypeList();
    IF sSym = keySy THEN
      mod.key := iAtt;
    ELSE 
      SymError("Missing keySy");
    END; 
    GPBinFiles.CloseFile(file);
  END ReadSymbolFile;

  PROCEDURE CloseSymFile*();
  BEGIN
    IF file # NIL THEN GPBinFiles.CloseFile(file) END;
  END CloseSymFile;

(* ============================================================ *)
BEGIN
END SymbolFile.
(* ============================================================ *)
