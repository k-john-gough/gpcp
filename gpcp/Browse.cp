MODULE Browse;

  IMPORT 
        RTS,
        Console,
        Error,
        CPmain,
        GPFiles,
        GPBinFiles,
        LitValue,
        ProgArgs,
        Symbols,
        IdDesc,
        GPText,
        GPTextFiles,
        GPCPcopyright,
        FileNames;

(* ========================================================================= *
// Collected syntax ---
// 
// SymFile    = Header [String (falSy | truSy | <other attribute>)]
//		{Import | Constant | Variable | Type | Procedure} 
//		TypeList Key.
//	-- optional String is external name.
//	-- falSy ==> Java class
//	-- truSy ==> Java interface
//	-- others ...
// Header     = magic modSy Name.
// Import     = impSy Name [String] Key.
//	-- optional string is explicit external name of class
// Constant   = conSy Name Literal.
// Variable   = varSy Name TypeOrd.
// Type       = typSy Name TypeOrd.
// Procedure  = prcSy Name [String] FormalType.
//	-- optional string is explicit external name of procedure
// Method     = mthSy Name byte byte TypeOrd [String][Name] FormalType.
//	-- optional string is explicit external name of method
// FormalType = [retSy TypeOrd] frmSy {parSy byte TypeOrd [String]} endFm.
//	-- optional phrase is return type for proper procedures
// TypeOrd    = ordinal.
// TypeHeader = tDefS Ord [fromS Ord Name].
//	-- optional phrase occurs if:
//	-- type not from this module, i.e. indirect export
// TypeList   = start { Array | Record | Pointer | ProcType | 
//                      NamedType | Enum | Vector } close.
// Array      = TypeHeader arrSy TypeOrd (Byte | Number | <empty>) endAr.
//	-- nullable phrase is array length for fixed length arrays
// Vector     = TypeHeader arrSy basSy TypeOrd endAr.
// Pointer    = TypeHeader ptrSy TypeOrd.
// Event      = TypeHeader evtSy FormalType.
// ProcType   = TypeHeader pTpSy FormalType.
// Record     = TypeHeader recSy recAtt [truSy | falSy] 
//		[basSy TypeOrd] [iFcSy {basSy TypeOrd}]
//		{Name TypeOrd} {Method} {Statics} endRc.
//	-- truSy ==> is an extension of external interface
//	-- falSy ==> is an extension of external class
// 	-- basSy option defines base type, if not ANY / j.l.Object
// NamedType  = TypeHeader.
// Statics    = ( Constant | Variable | Procedure ).
// Enum       = TypeHeader eTpSy { Constant } endRc.
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
        symExt  = ".cps";
        broExt  = ".bro";
        htmlExt = ".html";


(* ============================================================ *)

  TYPE
    CharOpen = POINTER TO ARRAY OF CHAR;

(* ============================================================ *)

  TYPE
    Desc = POINTER TO ABSTRACT RECORD
             name   : CharOpen;
             access : INTEGER;
           END;

    DescList = RECORD
                 list : POINTER TO ARRAY OF Desc;
                 tide : INTEGER;
               END;

    AbsValue = POINTER TO ABSTRACT RECORD
               END;

    NumValue = POINTER TO RECORD (AbsValue)
                 numVal : LONGINT;
               END;

    SetValue = POINTER TO RECORD (AbsValue)
                 setVal : SET;
               END;

    StrValue = POINTER TO RECORD (AbsValue)
                 strVal : CharOpen;
               END;

    FltValue = POINTER TO RECORD (AbsValue)
                 fltVal : REAL;
               END;

    BoolValue = POINTER TO RECORD (AbsValue)
                  boolVal : BOOLEAN;
                END;

    ChrValue = POINTER TO RECORD (AbsValue)
                 chrVal : CHAR;
               END;

    Type = POINTER TO ABSTRACT RECORD
             declarer : Desc;             
             importedFrom : Module;
             importedName : CharOpen;
           END;

    TypeList = POINTER TO ARRAY OF Type;

    Named = POINTER TO RECORD (Type)
            END;

    Basic = POINTER TO EXTENSIBLE RECORD (Type)
              name : CharOpen;
            END;

    Enum = POINTER TO EXTENSIBLE RECORD (Type)
             ids : DescList; 
           END;

    Pointer = POINTER TO EXTENSIBLE RECORD (Type)
                baseNum : INTEGER;
                isAnonPointer : BOOLEAN;
                baseType : Type;
              END;

    Record = POINTER TO EXTENSIBLE RECORD (Type)
               recAtt    : INTEGER;
               baseType  : Type;
               ptrType   : Pointer;
               isAnonRec : BOOLEAN;
               baseNum   : INTEGER;
               intrFaces : DescList; 
               fields    : DescList; 
               methods   : DescList; 
               statics   : DescList;
             END;
    
    Array = POINTER TO EXTENSIBLE RECORD (Type)
              size : INTEGER;
              elemType : Type;
              elemTypeNum : INTEGER;
            END;

    Vector = POINTER TO EXTENSIBLE RECORD (Type)
               elemType : Type;
               elemTypeNum : INTEGER;
             END;

    Par = POINTER TO RECORD
            typeNum : INTEGER;
            type    : Type;
            opNm    : CharOpen; (* Optional *)
            mode    : INTEGER;
           END;

    ParList = RECORD
                list : POINTER TO ARRAY OF Par;
                tide : INTEGER;
              END;

    Proc = POINTER TO EXTENSIBLE RECORD (Type)
             fName         : CharOpen;
             retType       : Type;
             retTypeNum    : INTEGER;
             noModes       : BOOLEAN;
             isConstructor : BOOLEAN;
             pars          : ParList;
           END;

    Event   = POINTER TO RECORD (Proc) END;

    Meth = POINTER TO EXTENSIBLE RECORD (Proc)
             receiver   : Type;
             recName    : CharOpen; (* Optional *)
             recTypeNum : INTEGER;
             attr       : INTEGER;
             recMode    : INTEGER;
           END;

    
    ImportDesc = POINTER TO RECORD (Desc)
                 END;
                 
    ConstDesc = POINTER TO RECORD  (Desc)
                  val : AbsValue;
                END;

    TypeDesc = POINTER TO EXTENSIBLE RECORD (Desc)
                type : Type;
                typeNum : INTEGER;
              END;

    UserTypeDesc = POINTER TO RECORD (TypeDesc)
                   END;

    VarDesc = POINTER TO RECORD (TypeDesc)
              END;

    ProcDesc = POINTER TO RECORD (Desc)
                 pType : Proc; 
               END;

    ModList = RECORD
                tide : INTEGER;
                list : POINTER TO ARRAY OF Module;
              END;

    Module = POINTER TO RECORD
               name      : CharOpen;
               symName   : CharOpen;
               fName     : CharOpen;
               pathName  : GPFiles.FileNameArray;
               imports   : ModList;
               consts    : DescList;
               vars      : DescList;
               types     : DescList;
               procs     : DescList;
               systemMod : BOOLEAN;
               progArg   : BOOLEAN;
               print     : BOOLEAN;
               strongNm  : POINTER TO ARRAY 6 OF INTEGER;
             END;
    
(* ============================================================ *)

  TYPE
    
    Output = POINTER TO EXTENSIBLE RECORD
               thisMod : Module;
             END;

    FileOutput = POINTER TO EXTENSIBLE RECORD (Output)
                   file : GPTextFiles.FILE;
                 END;

    HtmlOutput = POINTER TO RECORD (FileOutput)
                 END;
                    
(* ============================================================ *)

  VAR
    args, argNo  : INTEGER;
    fileName, modName  : CharOpen;
    printFNames, doAll, verbatim, verbose, hexCon, alpha : BOOLEAN;
    file  : GPBinFiles.FILE;
    sSym  : INTEGER;
    cAtt  : CHAR;
    iAtt  : INTEGER;
    lAtt  : LONGINT;
    rAtt  : REAL;
    sAtt  : CharOpen;
    typeList : TypeList;
    accArray : ARRAY 4 OF CHAR;
    outExt  : ARRAY 6 OF CHAR;
    output : Output;
    module : Module;
    modList : ModList;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE QuickSortDescs(lo, hi : INTEGER; dLst : DescList);
    VAR i,j : INTEGER;
        dsc : Desc;
	tmp : Desc;
   (* -------------------------------------------------- *)
    PROCEDURE canonLT(l,r : ARRAY OF CHAR) : BOOLEAN;
      VAR i : INTEGER;
    BEGIN
      FOR i := 0 TO LEN(l) - 1 DO l[i] := CAP(l[i]) END;
      FOR i := 0 TO LEN(r) - 1 DO r[i] := CAP(r[i]) END;
      RETURN l < r;
    END canonLT;
   (* -------------------------------------------------- *)
   (* -------------------------------------------------- *)
    PROCEDURE canonGT(l,r : ARRAY OF CHAR) : BOOLEAN;
      VAR i : INTEGER;
    BEGIN
      FOR i := 0 TO LEN(l) - 1 DO l[i] := CAP(l[i]) END;
      FOR i := 0 TO LEN(r) - 1 DO r[i] := CAP(r[i]) END;
      RETURN l > r;
    END canonGT;
   (* -------------------------------------------------- *)
  BEGIN
    i := lo; j := hi;
    dsc := dLst.list[(lo+hi) DIV 2];
    REPEAT
   (*
    * WHILE dLst.list[i].name < dsc.name DO INC(i) END;
    * WHILE dLst.list[j].name > dsc.name DO DEC(j) END;
    *)
      WHILE canonLT(dLst.list[i].name$, dsc.name$) DO INC(i) END;
      WHILE canonGT(dLst.list[j].name$, dsc.name$) DO DEC(j) END;
      IF i <= j THEN
        tmp := dLst.list[i]; dLst.list[i] := dLst.list[j]; dLst.list[j] := tmp; 
	INC(i); DEC(j);
      END;
    UNTIL i > j;
    IF lo < j THEN QuickSortDescs(lo, j,  dLst) END;
    IF i < hi THEN QuickSortDescs(i,  hi, dLst) END;
  END QuickSortDescs;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE GetModule(name : CharOpen) : Module;
  VAR
    i : INTEGER;
    tmp : POINTER TO ARRAY OF Module;
    mod : Module;
  BEGIN
    ASSERT(modList.list # NIL);
    FOR i := 0 TO modList.tide-1 DO
      IF modList.list[i].name^ = name^ THEN RETURN modList.list[i] END;
    END;
    IF modList.tide >= LEN(modList.list) THEN
      tmp := modList.list;
      NEW(modList.list,modList.tide*2);
      FOR i := 0 TO modList.tide-1 DO
        modList.list[i] := tmp[i]; 
      END;
    END; 
    NEW(mod);
    mod.systemMod := FALSE;
    mod.progArg := FALSE;
    mod.name := name;
    mod.symName := BOX(name^ + symExt);
    modList.list[modList.tide] := mod;
    INC(modList.tide);
    RETURN mod;
  END GetModule;

  PROCEDURE AddMod (VAR mList : ModList; m : Module);
  VAR
    tmp : POINTER TO ARRAY OF Module;
    i : INTEGER;
  BEGIN
    IF mList.list = NIL THEN
      NEW(mList.list,10);
      mList.tide := 0;
    ELSIF mList.tide >= LEN(mList.list) THEN 
      tmp := mList.list;
      NEW(mList.list,LEN(tmp)*2);
      FOR i := 0 TO mList.tide-1 DO
        mList.list[i] := tmp[i];
      END;
    END;
    mList.list[mList.tide] := m;
    INC(mList.tide);
  END AddMod;

(* ============================================================ *)

  PROCEDURE AddDesc (VAR dList : DescList; d : Desc);
  VAR
    tmp : POINTER TO ARRAY OF Desc;
    i : INTEGER;
  BEGIN
    IF dList.list = NIL THEN
      NEW(dList.list,10);
      dList.tide := 0;
    ELSIF dList.tide >= LEN(dList.list) THEN 
      tmp := dList.list;
      NEW(dList.list,LEN(tmp)*2);
      FOR i := 0 TO dList.tide-1 DO
        dList.list[i] := tmp[i];
      END;
    END;
    dList.list[dList.tide] := d;
    INC(dList.tide);
  END AddDesc;

  PROCEDURE AddPar (VAR pList : ParList; p : Par);
  VAR
    tmp : POINTER TO ARRAY OF Par;
    i : INTEGER;
  BEGIN
    IF pList.list = NIL THEN
      NEW(pList.list,10);
      pList.tide := 0;
    ELSIF pList.tide >= LEN(pList.list) THEN 
      tmp := pList.list;
      NEW(pList.list,LEN(tmp)*2);
      FOR i := 0 TO pList.tide-1 DO
        pList.list[i] := tmp[i];
      END;
    END;
    pList.list[pList.tide] := p;
    INC(pList.tide);
  END AddPar;

  PROCEDURE AddType (VAR tList : TypeList; t : Type; pos : INTEGER);
  VAR
    tmp : POINTER TO ARRAY OF Type;
    i : INTEGER;
  BEGIN
    ASSERT(tList # NIL);
    IF pos >= LEN(tList) THEN 
      tmp := tList;
      NEW(tList,LEN(tmp)*2);
      FOR i := 0 TO LEN(tmp)-1 DO
        tList[i] := tmp[i];
      END;
    END;
    tList[pos] := t;
  END AddType;

(* ============================================================ *)
(* ========	Various reading utility procedures	======= *)
(* ============================================================ *)

  PROCEDURE read() : INTEGER;
  BEGIN
    RETURN GPBinFiles.readByte(file);
  END read;

(* ======================================= *)

  PROCEDURE readUTF() : CharOpen; 
    CONST
      bad = "Bad UTF-8 string";
    VAR num : INTEGER;
      bNm : INTEGER;
      len : INTEGER;
      idx : INTEGER;
      chr : INTEGER;
      buff : CharOpen;
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
(* ========		Symbol File Reader		======= *)
(* ============================================================ *)
(*
  PROCEDURE DiagnoseSymbol();
    VAR arg : ARRAY 24 OF CHAR;
  BEGIN
    CASE sSym OF
    | ORD('H') : Console.WriteString("MODULE "); RETURN;
    | ORD('0') : Console.WriteString("FALSE");
    | ORD('1') : Console.WriteString("TRUE");
    | ORD('I') : Console.WriteString("IMPORT "); RETURN;
    | ORD('C') : Console.WriteString("CONST");
    | ORD('T') : Console.WriteString("TYPE "); RETURN;
    | ORD('P') : Console.WriteString("PROCEDURE "); RETURN;
    | ORD('M') : Console.WriteString("MethodSymbol");
    | ORD('V') : Console.WriteString("VAR "); RETURN;
    | ORD('p') : Console.WriteString("ParamSymbol");
    | ORD('&') : Console.WriteString("StartSymbol");
    | ORD('!') : Console.WriteString("CloseSymbol");
    | ORD('{') : Console.WriteString("StartRecord");
    | ORD('}') : Console.WriteString("EndRecord");
    | ORD('(') : Console.WriteString("StartFormals");
    | ORD('@') : Console.WriteString("FROM "); RETURN;
    | ORD(')') : Console.WriteString("EndFormals");
    | ORD('[') : Console.WriteString("StartArray");
    | ORD(']') : Console.WriteString("EndArray");
    | ORD('%') : Console.WriteString("ProcType");
    | ORD('^') : Console.WriteString("POINTER");
    | ORD('e') : Console.WriteString("EnumType");
    | ORD('~') : Console.WriteString("InterfaceType");
    | ORD('v') : Console.WriteString("EventType");
    | ORD('*') : Console.WriteString("VectorType");
    | ORD('\') : Console.WriteString("BYTE "); Console.WriteInt(iAtt,1);
    | ORD('c') : Console.WriteString("CHAR "); Console.Write(cAtt);
    | ORD('S') : Console.WriteString("SetSymbol 0x"); Console.WriteHex(iAtt,1);
    | ORD('K') : Console.WriteString("KeySymbol 0x"); Console.WriteHex(iAtt,1);
    | ORD('t') : Console.WriteString("TypeDef t#"); Console.WriteInt(iAtt,1);
    | ORD('+') : Console.WriteString("BaseType t#"); Console.WriteInt(iAtt,1);
    | ORD('R') : Console.WriteString("RETURN t#"); Console.WriteInt(iAtt,1);
    | ORD('#') :
	    RTS.LongToStr(lAtt, arg); 
	    Console.WriteString("Number "); 
		Console.WriteString(arg$);
    | ORD('$') : 
        Console.WriteString("NameSymbol #");
        Console.WriteInt(iAtt,1); 
        Console.Write(' ');
        Console.WriteString(sAtt);
    | ORD('s') : 
        Console.WriteString("String '");
        Console.WriteString(sAtt);
        Console.Write("'");
    | ORD('r') : 
        RTS.RealToStrInvar(rAtt, arg);
        Console.WriteString("Real "); 
        Console.WriteString(arg$);
    ELSE 
	    Console.WriteString("Bad Symbol ");
		Console.WriteInt(sSym, 1);
	    Console.WriteString(" in File");
    END;
    Console.WriteLn;
  END DiagnoseSymbol;
*)
(* ============================================================ *)

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
    (* DiagnoseSymbol(); *)
  END GetSym;

(* ======================================= *)

  PROCEDURE ReadPast(sym : INTEGER);
  BEGIN
    IF sSym # sym THEN 
      Console.WriteString("Expected ");
      Console.Write(CHR(sym));
      Console.WriteString(" got ");
      Console.Write(CHR(sSym));
      Console.WriteLn;
      RTS.Throw("Bad symbol file format"); 
    END;
    GetSym();
  END ReadPast;

(* ============================================ *)

  PROCEDURE GetLiteral(VAR lit : AbsValue);
  VAR
    b : BoolValue;
    n : NumValue;
    c : ChrValue;
    f : FltValue;
    s : SetValue;
    st : StrValue;
  BEGIN
    CASE sSym OF
    | truSy : NEW(b); b.boolVal := TRUE; lit := b;
    | falSy : NEW(b); b.boolVal := FALSE; lit := b;
    | numSy : NEW(n); n.numVal := lAtt; lit := n;
    | chrSy : NEW(c); c.chrVal := cAtt; lit := c;
    | fltSy : NEW(f); f.fltVal := rAtt; lit := f;
    | setSy : NEW(s); s.setVal := BITS(iAtt); lit := s;
    | strSy : NEW(st); st.strVal := sAtt; lit := st;
    END;
    GetSym();						(* read past value  *)
  END GetLiteral;

(* ============================================ *)

  PROCEDURE GetFormalType(p : Proc);
  (*
  // FormalType = [retSy TypeOrd] frmSy {parSy byte TypeOrd} endFm.
  //	-- optional phrase is return type for proper procedures
  *)
    VAR 
         par : Par;
        byte : INTEGER;
  BEGIN
    p.noModes := TRUE;
    IF sSym = retSy THEN 
      p.retTypeNum := iAtt;
      GetSym();
    ELSE
      p.retTypeNum := 0; 
    END;
    ReadPast(frmSy);
    WHILE sSym = parSy DO
      NEW(par);
      par.mode := read();
      IF par.mode > 0 THEN p.noModes := FALSE; END;
      par.typeNum := readOrd();
      GetSym();
      IF sSym = strSy THEN
        par.opNm := sAtt;
        GetSym();
      END;
      AddPar(p.pars,par);
    END;
    ReadPast(endFm);
  END GetFormalType;

(* ============================================ *)

  PROCEDURE pointerType() : Pointer;
  (* Assert: the current symbol is ptrSy.	*)
  (* Pointer   = TypeHeader ptrSy TypeOrd.	*)
    VAR 
      ptr : Pointer;
  BEGIN
    NEW(ptr);
    ptr.baseNum := readOrd();
    ptr.isAnonPointer := FALSE;
    GetSym();
    RETURN ptr;
  END pointerType;

(* ============================================ *)

  PROCEDURE eventType() : Proc;
  (* Assert: the current symbol is evtSy.	*)
  (* Event   = TypeHeader evtSy FormalType.	*)
    VAR p : Event;
  BEGIN
    NEW(p);
    GetSym();		(* read past evtSy *)
    GetFormalType(p);
    RETURN p;
  END eventType;

(* ============================================ *)

  PROCEDURE procedureType() : Proc;
  (* Assert: the current symbol is pTpSy.	*)
  (* ProcType  = TypeHeader pTpSy FormalType.	*)
  VAR
    p : Proc;
  BEGIN
    NEW(p);
    GetSym();		(* read past pTpSy *)
    GetFormalType(p);
    RETURN p;
  END procedureType;

(* ============================================ *)

  PROCEDURE^ GetConstant() : ConstDesc;

  PROCEDURE enumType() : Enum;
  (* Assert: the current symbol is eTpSy.	  *)
  (* Enum  = TypeHeader eTpSy { Constant } endRc. *)
  VAR
    e : Enum;
  BEGIN
    NEW(e);
    GetSym();
    WHILE (sSym = conSy) DO
      AddDesc(e.ids,GetConstant());
    END;
    ReadPast(endRc);
    RETURN e;
  END enumType;

(* ============================================ *)

  PROCEDURE arrayType() : Type;
  (* Assert: at entry the current symbol is arrSy.		     *)
  (* Array      = TypeHeader arrSy TypeOrd (Byte | Number | ) endAr. *)
  (*	-- nullable phrase is array length for fixed length arrays   *)
    VAR 
        arr : Array;
  BEGIN
    NEW(arr); 
    arr.elemTypeNum := readOrd();
    GetSym();
    IF sSym = bytSy THEN
      arr.size := iAtt;
      GetSym();
    ELSIF sSym = numSy THEN
      arr.size := SHORT(lAtt);
      GetSym();
    ELSE 
      arr.size := 0 
    END;
    ReadPast(endAr);
    RETURN arr;
  END arrayType;

(* ============================================ *)

  PROCEDURE vectorType() : Type;
  (* Assert: at entry the current symbol is vecSy.		     *)
  (* Vector   = TypeHeader vecSy TypeOrd endAr.                      *)
    VAR 
        vec : Vector;
  BEGIN
    NEW(vec); 
    vec.elemTypeNum := readOrd();
    GetSym();
    ReadPast(endAr);
    RETURN vec;
  END vectorType;

(* ============================================ *)

  PROCEDURE^ GetProc() : ProcDesc;
  PROCEDURE^ GetVar() : VarDesc;

  PROCEDURE recordType(recNum : INTEGER) : Record;
  (* Assert: at entry the current symbol is recSy.			*)
  (* Record     = TypeHeader recSy recAtt [truSy | falSy | <others>] 	*)
  (*	[basSy TypeOrd] [iFcSy {basSy TypeOrd}]				*)
  (*	{Name TypeOrd} {Method} {Statics} endRc.			*)
    VAR 
        rec  : Record;
        f : VarDesc;
        t : TypeDesc;
        m : ProcDesc;
        mth : Meth;
  BEGIN
    NEW(rec);
    rec.recAtt := read();
    rec.isAnonRec := FALSE;
    GetSym();				(* Get past recSy rAtt	*)
    IF (sSym = falSy) OR (sSym = truSy) THEN
      GetSym();
    END;
    IF sSym = basSy THEN
      rec.baseNum := iAtt;
      GetSym();
    ELSE
      rec.baseNum := 0;
    END;
    IF sSym = iFcSy THEN
      GetSym();
      WHILE sSym = basSy DO
(* *
 * *	Console.WriteString("got interface $T");
 * *	Console.WriteInt(iAtt,1);
 * *	Console.WriteLn;
 * *)
        NEW(t);
        t.typeNum := iAtt;
	GetSym();
	AddDesc(rec.intrFaces,t);
      END;
    END;
    WHILE sSym = namSy DO
      NEW(f);
      f.name := sAtt;
      f.access := iAtt;
      f.typeNum := readOrd();
      GetSym();
      AddDesc(rec.fields,f);
    END;
   (* Method     = mthSy Name byte byte TypeOrd [String] FormalType. *)
    WHILE sSym = mthSy DO
      NEW(m);
      NEW(mth);
      mth.importedFrom := NIL;
      mth.isConstructor := FALSE;
      m.pType := mth;
      GetSym();
      IF (sSym # namSy) THEN RTS.Throw("Bad symbol file format"); END;
      m.name := sAtt;
      m.access := iAtt;
      mth.declarer := m;
     (* byte1 is the method attributes  *)
      mth.attr := read();
     (* byte2 is param form of receiver *)
      mth.recMode := read();
     (* next 1 or 2 bytes are rcv-type  *)
      mth.recTypeNum := readOrd();
      GetSym();
      IF sSym = strSy THEN 
        mth.fName := sAtt;
        GetSym(); 
      ELSE
        mth.fName := NIL;
      END;
      IF sSym = namSy THEN 
        mth.recName := sAtt;
        GetSym(); 
      END;
      GetFormalType(mth);
      AddDesc(rec.methods,m);
    END;
    WHILE (sSym = conSy) OR (sSym = prcSy) OR (sSym = varSy) DO
      IF sSym = conSy THEN
        AddDesc(rec.statics,GetConstant());
      ELSIF sSym = prcSy THEN
        AddDesc(rec.statics,GetProc());
      ELSE
        AddDesc(rec.statics,GetVar());
      END;
    END;
    ReadPast(endRc); 
    RETURN rec;
  END recordType;

(* ============================================ *)

  PROCEDURE ResolveProc(p : Proc);
  VAR
    i : INTEGER;
  BEGIN
    p.retType := typeList[p.retTypeNum];
    IF p.retTypeNum = 0 THEN ASSERT(p.retType = NIL); END;
    IF p IS Meth THEN
      p(Meth).receiver := typeList[p(Meth).recTypeNum];
    END;
    FOR i := 0 TO p.pars.tide-1 DO
      p.pars.list[i].type := typeList[p.pars.list[i].typeNum];
    END; 
  END ResolveProc;

(* ============================================ *)

  PROCEDURE ReadTypeList(mod : Module);
  (* TypeList   = start { Array | Record | Pointer 	*)
  (*		| ProcType | NamedType | Enum } close.  *)
  (* TypeHeader = tDefS Ord [fromS Ord Name].		*)
    VAR modOrd : INTEGER;
	typOrd : INTEGER;
        typ    : Type;
        namedType : Named;
        f : VarDesc;
        rec : Record;
        impName : CharOpen;
        i,j : INTEGER;
  BEGIN
    GetSym();
    typOrd := 0;
    WHILE sSym = tDefS DO
      typOrd := iAtt;
      ASSERT(typOrd # 0);
      ReadPast(tDefS);
      modOrd := -1;
      impName := BOX("");
     (*
      *  The fromS symbol appears if the type is imported.
      *)
      IF sSym = fromS THEN
        modOrd := iAtt;
        GetSym();
        impName := sAtt;
        ReadPast(namSy);
      END;
     (* Get type info. *)
      CASE sSym OF 
      | arrSy : typ := arrayType();
      | vecSy : typ := vectorType();
      | recSy : typ := recordType(typOrd);
      | ptrSy : typ := pointerType();
      | evtSy : typ := eventType();
      | pTpSy : typ := procedureType();
      | eTpSy : typ := enumType();
      ELSE 
        NEW(namedType);
	typ := namedType;
      END;
      IF typ # NIL THEN
        AddType(typeList,typ,typOrd);
        IF modOrd > -1 THEN      
          typ.importedFrom := mod.imports.list[modOrd]; 
          typ.importedName := impName;
        END;
      END;
    END;
    ReadPast(close);
    FOR i := Symbols.tOffset TO typOrd DO
      typ := typeList[i];
      IF typ IS Array THEN
        typ(Array).elemType := typeList[typ(Array).elemTypeNum];
      ELSIF typ IS Vector THEN
        typ(Vector).elemType := typeList[typ(Vector).elemTypeNum];
      ELSIF typ IS Record THEN
        rec := typ(Record);
        IF (rec.baseNum > 0) THEN
          rec.baseType := typeList[rec.baseNum];
        END;
        FOR j := 0 TO rec.fields.tide-1 DO
          f := rec.fields.list[j](VarDesc);
          f.type := typeList[f.typeNum];
        END;
        FOR j := 0 TO rec.methods.tide-1 DO
          ResolveProc(rec.methods.list[j](ProcDesc).pType);
        END;
        FOR j := 0 TO rec.statics.tide-1 DO
          IF rec.statics.list[j] IS ProcDesc THEN
            ResolveProc(rec.statics.list[j](ProcDesc).pType);
          ELSIF rec.statics.list[j] IS VarDesc THEN
            f := rec.statics.list[j](VarDesc);
            f.type := typeList[f.typeNum];
          END;
        END;
      ELSIF typ IS Pointer THEN
        typ(Pointer).baseType := typeList[typ(Pointer).baseNum];
      ELSIF typ IS Proc THEN
        ResolveProc(typ(Proc));
      END;
    END;
  END ReadTypeList;

(* ============================================ *)

  PROCEDURE ResolveAnonRecs();
  VAR r : Record;
      typ : Type;
      ch0 : CHAR;
      i,j,k : INTEGER;
  BEGIN
    FOR i := Symbols.tOffset TO LEN(typeList)-1 DO
      typ := typeList[i];
      IF ~verbatim & (typ # NIL) & (typ.declarer # NIL) THEN
        ch0 := typ.declarer.name[0];
        IF (ch0 = "@") OR (ch0 = "$") THEN typ.declarer := NIL END;
      END;
      IF typ IS Record THEN 
        r := typ(Record);
        FOR j := 0 TO r.intrFaces.tide - 1 DO
          k := r.intrFaces.list[j](TypeDesc).typeNum;
          r.intrFaces.list[j](TypeDesc).type := typeList[k];
        END;
        IF typ.declarer = NIL THEN (* anon record *)
          typ(Record).isAnonRec := TRUE;
        END;
      ELSIF (typ IS Pointer) & (typ(Pointer).baseType IS Record) THEN
        IF (typ.declarer = NIL) & (typ.importedFrom = NIL) THEN 
          typ(Pointer).isAnonPointer := TRUE; 
        END;
        r := typ(Pointer).baseType(Record);
        IF (r.declarer = NIL) THEN  (* anon record *)
          r.isAnonRec := TRUE;
          r.ptrType := typ(Pointer);
        END;
      END;
    END;
  END ResolveAnonRecs;

(* ============================================ *)
  
  PROCEDURE GetType() : UserTypeDesc;
  (* Type       = typSy Name TypeOrd. *)
  VAR
    typeDesc : UserTypeDesc;
  BEGIN
    GetSym();
    NEW (typeDesc);
    typeDesc.name := sAtt;
    typeDesc.access := iAtt;
    typeDesc.typeNum := readOrd();
    GetSym();
    RETURN typeDesc;
  END GetType;

(* ============================================ *)

  PROCEDURE GetImport() : Module;
  (* Import     = impSy Name [String] Key.  *)
  VAR
    impMod : Module;
  BEGIN
    GetSym();
    IF doAll THEN
      impMod := GetModule(sAtt); 
    ELSE
      NEW(impMod);
      impMod.name := sAtt;
      impMod.systemMod := FALSE;
      impMod.progArg := FALSE;
    END;
    GetSym();
    IF sSym = strSy THEN impMod.fName := sAtt; GetSym(); END;
    ReadPast(keySy);
    RETURN impMod;
  END GetImport;

(* ============================================ *)

  PROCEDURE GetConstant() : ConstDesc;
  (*  Constant   = conSy Name Literal.  *)
  VAR
    constDesc : ConstDesc;
  BEGIN
    GetSym();
    NEW(constDesc);
    constDesc.name := sAtt;
    constDesc.access := iAtt;
    GetSym();
    GetLiteral(constDesc.val);
    RETURN constDesc;
  END GetConstant;

(* ============================================ *)

  PROCEDURE GetVar() : VarDesc;
  (*  Variable   = varSy Name TypeOrd.  *)
  VAR
    varDesc : VarDesc;
  BEGIN
    GetSym();
    NEW(varDesc);
    varDesc.name := sAtt;
    varDesc.access := iAtt;
    varDesc.typeNum := readOrd();
    GetSym();
    RETURN varDesc;
  END GetVar;

(* ============================================ *)

  PROCEDURE GetProc() : ProcDesc;
  (* Procedure  = prcSy Name [String] [trySy] FormalType.  *)
  VAR
    procDesc : ProcDesc;
  BEGIN
    GetSym();
    NEW(procDesc);
    procDesc.name := sAtt;
    procDesc.access := iAtt;
    GetSym();
    NEW(procDesc.pType);
    IF sSym = strSy THEN 
      IF sAtt^ = "<init>" THEN
        procDesc.pType.fName := BOX("< init >");  
      ELSE
        procDesc.pType.fName := sAtt;
      END;
      GetSym(); 
    ELSE
      procDesc.pType.fName := NIL;
    END;
    IF sSym = truSy THEN
      procDesc.pType.isConstructor := TRUE;
      GetSym();
    ELSE
      procDesc.pType.isConstructor := FALSE;
    END;
    procDesc.pType.importedFrom := NIL;
    procDesc.pType.declarer := procDesc;
    GetFormalType(procDesc.pType);
    RETURN procDesc;
  END GetProc;

(* ============================================ *)

  PROCEDURE SymFile(mod : Module);
   (*
   // SymFile    = Header [String (falSy | truSy | <others>)]
   //		{Import | Constant | Variable | Type | Procedure} 
   //		TypeList Key.
   // Header     = magic modSy Name.
   //
   //  magic has already been recognized.
   *)
  VAR 
    i,j,k : INTEGER;
    typeDesc : UserTypeDesc;
    varDesc  : VarDesc;
    procDesc : ProcDesc;
    thisType : Type;
  BEGIN
    AddMod(mod.imports,mod);
    ReadPast(modSy);
    IF sSym = namSy THEN (* do something with f.sAtt *)
      IF mod.name^ # sAtt^ THEN
        Error.WriteString("Wrong name in symbol file. Expected <");
        Error.WriteString(mod.name^ + ">, found <");
        Error.WriteString(sAtt^ + ">"); 
	    Error.WriteLn;
        HALT(1);
      END;
      GetSym();
    ELSE RTS.Throw("Bad symfile header");
    END;
    IF sSym = strSy THEN (* optional name *)
      mod.fName := sAtt;
      GetSym();
      IF (sSym = falSy) OR (sSym = truSy) THEN 
        GetSym();
      ELSE RTS.Throw("Bad explicit name");
      END; 
    ELSE
      mod.fName := NIL;
    END; 
   (*
    *  Optional strong name info.
    *)
    IF sSym = numSy THEN 
      NEW(mod.strongNm);   (* POINTER TO ARRAY 6 OF INTEGER *)
      mod.strongNm[0] := RTS.hiInt(lAtt);
      mod.strongNm[1] := RTS.loInt(lAtt);
      GetSym();
      mod.strongNm[2] := RTS.hiInt(lAtt);
      mod.strongNm[3] := RTS.loInt(lAtt);
      GetSym();
      mod.strongNm[4] := RTS.hiInt(lAtt);
      mod.strongNm[5] := RTS.loInt(lAtt);
      GetSym();
    END;
    (* end optional strong name information *)
    LOOP
      CASE sSym OF
      | start : EXIT;
      | typSy : AddDesc(mod.types,GetType());
      | impSy : AddMod(mod.imports,GetImport());
      | conSy : AddDesc(mod.consts,GetConstant());
      | varSy : AddDesc(mod.vars,GetVar());
      | prcSy : AddDesc(mod.procs,GetProc());
      ELSE RTS.Throw("Bad object");
      END;
    END;
    ReadTypeList(mod);
    IF sSym # keySy THEN
      RTS.Throw("Missing keySy");
    END; 
    FOR i := 0 TO mod.types.tide-1 DO
      typeDesc := mod.types.list[i](UserTypeDesc);
      thisType := typeList[typeDesc.typeNum];
      typeDesc.type := thisType;
      typeDesc.type.declarer := typeDesc;
    END;
    FOR i := 0 TO mod.vars.tide-1 DO
      varDesc := mod.vars.list[i](VarDesc);
      varDesc.type := typeList[varDesc.typeNum];
    END;
    FOR i := 0 TO mod.procs.tide-1 DO
      procDesc := mod.procs.list[i](ProcDesc);
      ResolveProc(mod.procs.list[i](ProcDesc).pType);
    END;
    ResolveAnonRecs();
  END SymFile;

(* ============================================================ *)

  PROCEDURE GetSymAndModNames(VAR symName : CharOpen;
                              OUT modName : CharOpen);
  VAR i,j : INTEGER;
      ok : BOOLEAN; 
  BEGIN
    modName := BOX(symName^);
    i := 0;
    WHILE ((i < LEN(symName)) & (symName[i] # '.') & 
           (symName[i] # 0X)) DO INC(i); END;
    IF (i >= LEN(symName)) OR (symName[i] # '.') THEN 
      symName := BOX(symName^ + symExt);
    ELSE
      modName[i] := 0X;
    END;
  END GetSymAndModNames;

  PROCEDURE Parse();
  VAR 
    marker,modIx,i   : INTEGER;
    mod : Module;
  BEGIN
    modIx := 0;
    WHILE (modIx < modList.tide) DO
      mod := modList.list[modIx];
      INC(modIx);
      mod.print := FALSE;
      file := GPBinFiles.findLocal(mod.symName);
      IF file = NIL THEN
        file := GPBinFiles.findOnPath("CPSYM", mod.symName);
        IF (file = NIL) OR (mod.progArg) THEN
          Error.WriteString("File <" + mod.symName^ + "> not found"); 
          Error.WriteLn;
          HALT(1);
        END;
        mod.pathName := GPBinFiles.getFullPathName(file);
        i := 0;
        WHILE (i < LEN(mod.pathName)) & (mod.pathName[i] # ".") DO INC(i); END;
        mod.pathName[i] := 0X;
      ELSE 
        marker := readInt();
        IF marker = RTS.loInt(magic) THEN
        (* normal case, nothing to do *)
        ELSIF marker = RTS.loInt(syMag) THEN
          mod.systemMod := TRUE;
        ELSE
          Error.WriteString("File <" + fileName^ + "> is not a valid symbol file"); 
          Error.WriteLn;
          RETURN;
        END;
        mod.print := TRUE;
        GetSym();
        IF verbose THEN
          Error.WriteString("Reading " + mod.name^); Error.WriteLn;
        END;
        SymFile(mod);
        GPBinFiles.CloseFile(file);
      END;
    END;
  RESCUE (x)
    Error.WriteString("Error in Parse()"); Error.WriteLn;
    Error.WriteString(RTS.getStr(x)); Error.WriteLn;
  END Parse;

(* ===================================================================== *)

PROCEDURE (o : Output) WriteStart(mod : Module),NEW,EMPTY;

PROCEDURE (o : Output) WriteEnd(),NEW,EMPTY;

PROCEDURE (o : Output) Write(ch : CHAR),NEW,EXTENSIBLE;
BEGIN
  Console.Write(ch);
END Write;

PROCEDURE (o : Output) WriteIdent(str : ARRAY OF CHAR),NEW,EXTENSIBLE;
BEGIN
  Console.WriteString(str);
END WriteIdent;

PROCEDURE (o : Output) WriteImport(impMod : Module),NEW,EXTENSIBLE;
BEGIN
  Console.WriteString(impMod.name);
END WriteImport;

PROCEDURE (o : Output) WriteString(str : ARRAY OF CHAR),NEW,EXTENSIBLE;
BEGIN
  Console.WriteString(str);
END WriteString;

PROCEDURE (o : Output) WriteLn(),NEW,EXTENSIBLE;
BEGIN
  Console.WriteLn;
END WriteLn;

PROCEDURE (o : Output) WriteInt(i : INTEGER),NEW,EXTENSIBLE;
BEGIN
  Console.WriteInt(i,1);
END WriteInt;

PROCEDURE (o : Output) WriteLong(l : LONGINT),NEW,EXTENSIBLE;
VAR
  str : ARRAY 30 OF CHAR;
BEGIN
  IF (l > MAX(INTEGER)) OR (l < MIN(INTEGER)) THEN
    RTS.LongToStr(l,str);
    Console.WriteString(str);
  ELSE
    Console.WriteInt(SHORT(l),1);
  END;
END WriteLong;

PROCEDURE (o : Output) WriteKeyword(str : ARRAY OF CHAR),NEW,EXTENSIBLE;
BEGIN
  Console.WriteString(str);
END WriteKeyword;

PROCEDURE (o : Output) Indent(i : INTEGER),NEW,EXTENSIBLE;
BEGIN
  WHILE i > 0 DO
    Console.Write(' ');
    DEC(i);
  END;
END Indent;

PROCEDURE (o : Output) WriteImportedTypeName(impMod : Module;
                                             tName : ARRAY OF CHAR),NEW,EXTENSIBLE;
BEGIN
  Console.WriteString(impMod.name^ + "." + tName);
END WriteImportedTypeName;

PROCEDURE (o : Output) WriteTypeName(tName : ARRAY OF CHAR),NEW,EXTENSIBLE;
BEGIN
  Console.WriteString(tName);
END WriteTypeName;

PROCEDURE (o : Output) WriteTypeDecl(tName : ARRAY OF CHAR),NEW,EXTENSIBLE;
BEGIN
  Console.WriteString(tName);
END WriteTypeDecl;

(* FIXME *)
PROCEDURE (o : Output) MethRef(IN nam : ARRAY OF CHAR),NEW,EMPTY;
PROCEDURE (o : Output) MethAnchor(IN nam : ARRAY OF CHAR),NEW,EMPTY;
(* FIXME *)

(* ------------------------------------------------------------------- *)

PROCEDURE (f : FileOutput) Write(ch : CHAR),EXTENSIBLE;
BEGIN
  GPText.Write(f.file,ch);
END Write;

PROCEDURE (f : FileOutput) WriteIdent(str : ARRAY OF CHAR),EXTENSIBLE;
BEGIN
  GPText.WriteString(f.file,str);
END WriteIdent;

PROCEDURE (f : FileOutput) WriteImport(impMod : Module),EXTENSIBLE;
BEGIN
  GPText.WriteString(f.file,impMod.name);
END WriteImport;

PROCEDURE (f : FileOutput) WriteString(str : ARRAY OF CHAR),EXTENSIBLE;
BEGIN
  GPText.WriteString(f.file,str);
END WriteString;

PROCEDURE (f : FileOutput) WriteLn(),EXTENSIBLE;
BEGIN
  GPText.WriteLn(f.file);
END WriteLn;

PROCEDURE (f : FileOutput) WriteInt(i : INTEGER),EXTENSIBLE;
BEGIN
  GPText.WriteInt(f.file,i,1);
END WriteInt;

PROCEDURE (f : FileOutput) WriteLong(l : LONGINT),EXTENSIBLE;
BEGIN
  GPText.WriteLong(f.file,l,1);
END WriteLong;

PROCEDURE (f : FileOutput) WriteKeyword(str : ARRAY OF CHAR),EXTENSIBLE;
BEGIN
  GPText.WriteString(f.file,str);
END WriteKeyword;

PROCEDURE (f : FileOutput) Indent(i : INTEGER),EXTENSIBLE;
BEGIN
  WHILE i > 0 DO
    GPText.Write(f.file,' ');
    DEC(i);
  END;
END Indent;

PROCEDURE (f : FileOutput) WriteImportedTypeName(impMod : Module;
                                                 tName : ARRAY OF CHAR),EXTENSIBLE;
BEGIN
  GPText.WriteString(f.file,impMod.name^ + "." + tName);
END WriteImportedTypeName;

PROCEDURE (f : FileOutput) WriteTypeName(tName : ARRAY OF CHAR),EXTENSIBLE;
BEGIN
  GPText.WriteString(f.file,tName);
END WriteTypeName;

PROCEDURE (f : FileOutput) WriteTypeDecl(tName : ARRAY OF CHAR),EXTENSIBLE;
BEGIN
  GPText.WriteString(f.file,tName);
END WriteTypeDecl;

(* ------------------------------------------------------------------- *)

PROCEDURE (h : HtmlOutput) WriteStart(mod : Module);
BEGIN
  GPText.WriteString(h.file,"<html><head><title>");
  GPText.WriteString(h.file,mod.name);
  GPText.WriteString(h.file,"</title></head>");
  GPText.WriteLn(h.file);
  GPText.WriteString(h.file,'<body bgcolor="white">');
  GPText.WriteLn(h.file);
  GPText.WriteString(h.file,"<hr><pre>");
  GPText.WriteLn(h.file);
END WriteStart;

PROCEDURE (h : HtmlOutput) WriteEnd();
BEGIN
  GPText.WriteString(h.file,"</font></pre></hr></body></html>");
  GPText.WriteLn(h.file);
END WriteEnd;

PROCEDURE (h : HtmlOutput) Write(ch : CHAR);
BEGIN
  GPText.Write(h.file,ch);
END Write;

PROCEDURE (h : HtmlOutput) WriteImport(impMod : Module);
BEGIN
  GPText.WriteString(h.file,'<a href="');
  IF impMod.pathName = NIL THEN
    GPText.WriteString(h.file,impMod.name);
  ELSE
    GPText.WriteString(h.file,impMod.pathName);
  END;
  GPText.WriteString(h.file,'.html">');
  GPText.WriteString(h.file,impMod.name);
  GPText.WriteString(h.file,'</a>');
END WriteImport;

PROCEDURE (h : HtmlOutput) WriteIdent(str : ARRAY OF CHAR);
BEGIN
  GPText.WriteString(h.file,'<font color="#cc0033">');
  GPText.WriteString(h.file,str);
  GPText.WriteString(h.file,"</font>");
END WriteIdent;

PROCEDURE (h : HtmlOutput) WriteString(str : ARRAY OF CHAR);
BEGIN
  GPText.WriteString(h.file,str);
END WriteString;

PROCEDURE (h : HtmlOutput) WriteLn();
BEGIN
  GPText.WriteLn(h.file);
END WriteLn;

PROCEDURE (h : HtmlOutput) WriteInt(i : INTEGER );
BEGIN
  GPText.WriteInt(h.file,i,1);
END WriteInt;

PROCEDURE (h : HtmlOutput) WriteLong(l : LONGINT);
BEGIN
  GPText.WriteLong(h.file,l,1);
END WriteLong;

PROCEDURE (h : HtmlOutput) WriteKeyword(str : ARRAY OF CHAR);
BEGIN
  GPText.WriteString(h.file,"<b>" + str + "</b>");
END WriteKeyword;

PROCEDURE (h : HtmlOutput) Indent(i : INTEGER);
BEGIN
  WHILE i > 0 DO
    GPText.Write(h.file,' ');
    DEC(i);
  END;
END Indent;

PROCEDURE (h : HtmlOutput) WriteImportedTypeName(impMod : Module;
                                                 tName : ARRAY OF CHAR);
BEGIN
  GPText.WriteString(h.file,'<a href="');
  IF impMod.pathName = NIL THEN
    GPText.WriteString(h.file,impMod.name);
  ELSE
    GPText.WriteString(h.file,impMod.pathName);
  END;
  GPText.WriteString(h.file,'.html#type-');;
  GPText.WriteString(h.file,tName);
  GPText.WriteString(h.file,'">');
  GPText.WriteString(h.file,impMod.name^ + "." + tName);
  GPText.WriteString(h.file,'</a>');
END WriteImportedTypeName;

PROCEDURE (h : HtmlOutput) WriteTypeName(tName : ARRAY OF CHAR);
BEGIN
  GPText.WriteString(h.file,'<a href="#type-');;
  GPText.WriteString(h.file,tName);
  GPText.WriteString(h.file,'">');
  GPText.WriteString(h.file,tName);
  GPText.WriteString(h.file,'</a>');
END WriteTypeName;

PROCEDURE (h : HtmlOutput) WriteTypeDecl(tName : ARRAY OF CHAR);
BEGIN
  GPText.WriteString(h.file,'<a name="type-');
  GPText.WriteString(h.file,tName);
  GPText.WriteString(h.file,'"></a>');
  GPText.WriteString(h.file,'<font color="#cc0033">');
  GPText.WriteString(h.file,tName);
  GPText.WriteString(h.file,"</font>");
END WriteTypeDecl;

(* FIXME *)
PROCEDURE (h : HtmlOutput) MethRef(IN nam : ARRAY OF CHAR);
BEGIN
  GPText.WriteString(h.file, '    <a href="#meths-');;
  GPText.WriteString(h.file, nam);
  GPText.WriteString(h.file, '">');
  GPText.WriteString(h.file, '<font color="#cc0033">');
  GPText.WriteString(h.file, "(* Typebound Procedures *)");
  GPText.WriteString(h.file, "</font>");
  GPText.WriteString(h.file, '</a>');
END MethRef;

PROCEDURE (h : HtmlOutput) MethAnchor(IN nam : ARRAY OF CHAR);
BEGIN
  GPText.WriteString(h.file, '<a name="meths-');
  GPText.WriteString(h.file, nam);
  GPText.WriteString(h.file, '"></a>');
END MethAnchor;
(* FIXME *)

(* ==================================================================== *)
(*				Format Helpers				*)
(* ==================================================================== *)

  PROCEDURE qStrOf(str : CharOpen) : CharOpen;
    VAR len : INTEGER;
	idx : INTEGER;
	ord : INTEGER;
        rslt : LitValue.CharVector;
    (* -------------------------------------- *)
    PROCEDURE hexDigit(d : INTEGER) : CHAR;
    BEGIN
      IF d < 10 THEN RETURN CHR(d + ORD('0')) 
      ELSE RETURN CHR(d-10 + ORD('a'));
      END;
    END hexDigit;
    (* -------------------------------------- *)
    PROCEDURE AppendHex2D(r : LitValue.CharVector; o : INTEGER);
    BEGIN
      APPEND(r, '\');
      APPEND(r, 'x');
      APPEND(r, hexDigit(o DIV 16 MOD 16));
      APPEND(r, hexDigit(o        MOD 16));
    END AppendHex2D;
    (* -------------------------------------- *)
    PROCEDURE AppendUnicode(r : LitValue.CharVector; o : INTEGER);
    BEGIN
      APPEND(r, '\');
      APPEND(r, 'u');
      APPEND(r, hexDigit(o DIV 1000H MOD 16));
      APPEND(r, hexDigit(o DIV  100H MOD 16));
      APPEND(r, hexDigit(o DIV   10H MOD 16));
      APPEND(r, hexDigit(o           MOD 16));
    END AppendUnicode;
    (* -------------------------------------- *)
  BEGIN
   (*
    *  Translate the string into ANSI-C like
    *  for human, rather than machine consumption.
    *)
    NEW(rslt, LEN(str) * 2);
    APPEND(rslt, '"');
    FOR idx := 0 TO LEN(str) - 2 DO
      ord := ORD(str[idx]);
      CASE ord OF
      |  0 : APPEND(rslt, '\');
             APPEND(rslt, '0');
      |  9 : APPEND(rslt, '\');
             APPEND(rslt, 't');
      | 10 : APPEND(rslt, '\');
             APPEND(rslt, 'n');
      | 12 : APPEND(rslt, '\');
             APPEND(rslt, 'r');
      | ORD('"') :
             APPEND(rslt, '/');
             APPEND(rslt, '"');
      ELSE
        IF ord > 0FFH THEN AppendUnicode(rslt, ord);
        ELSIF (ord > 07EH) OR (ord < ORD(' ')) THEN AppendHex2D(rslt, ord);
        ELSE APPEND(rslt, CHR(ord));
        END;
      END;
    END;
    APPEND(rslt, '"');
    APPEND(rslt, 0X);
    RETURN LitValue.chrVecToCharOpen(rslt);
  END qStrOf;

  PROCEDURE hexOf(ch : CHAR) : CharOpen;
    VAR res : CharOpen;
	idx : INTEGER;
	ord : INTEGER;
    (* -------------------------------------- *)
    PROCEDURE hexDigit(d : INTEGER) : CHAR;
    BEGIN
      IF d < 10 THEN RETURN CHR(d + ORD('0')) 
      ELSE RETURN CHR(d-10 + ORD('A'));
      END;
    END hexDigit;
    (* -------------------------------------- *)
  BEGIN
    ord := ORD(ch);
    IF ord <= 7FH THEN
      NEW(res, 4); res[3] := 0X; res[2] := "X";
      res[1] := hexDigit(ord MOD 16);
      res[0] := hexDigit(ord DIV 16);
    ELSIF ord <= 0FFH THEN
      NEW(res, 5); res[4] := 0X; res[3] := "X";
      res[2] := hexDigit(ord MOD 16);
      res[1] := hexDigit(ord DIV 16);
      res[0] := "0";
    ELSIF ord <= 07FFFH THEN
      NEW(res, 10); res[9] := 0X; res[8] := "X";
      FOR idx := 7 TO 0 BY -1 DO
        res[idx] := hexDigit(ord MOD 16); ord := ord DIV 16;
      END;
    ELSE
      NEW(res, 11); res[10] := 0X; res[9] := "X";
      FOR idx := 8 TO 0 BY -1 DO
        res[idx] := hexDigit(ord MOD 16); ord := ord DIV 16;
      END;
    END;
    RETURN res;
  END hexOf;

(* ==================================================================== *)

  PROCEDURE LongToHex(n : LONGINT) : CharOpen;
    VAR arr : ARRAY 40 OF CHAR;
        idx : INTEGER;
   (* -------------------------------------- *)
    PROCEDURE hexDigit(d : INTEGER) : CHAR;
    BEGIN
      IF d < 10 THEN RETURN CHR(d + ORD('0')) 
      ELSE RETURN CHR(d-10 + ORD('a'));
      END;
    END hexDigit;
   (* -------------------------------------- *)
    PROCEDURE DoDigit(n : LONGINT;
                  VAR a : ARRAY OF CHAR; 
                  VAR i : INTEGER);
    BEGIN
      ASSERT(n >= 0);
      IF n > 15 THEN 
        DoDigit(n DIV 16, a, i);
        a[i] := hexDigit(SHORT(n MOD 16)); INC(i);
      ELSIF n > 9 THEN
        a[0] := '0';
        a[1] := hexDigit(SHORT(n)); i := 2;
      ELSE
        a[0] := hexDigit(SHORT(n)); i := 1;
      END;
    END DoDigit;
   (* -------------------------------------- *)
  BEGIN
    idx := 0;
    DoDigit(n, arr, idx);
    arr[idx] := 'H'; INC(idx); arr[idx] := 0X;
    RETURN BOX(arr);
  END LongToHex;

(* ==================================================================== *)

  PROCEDURE Length(a : ARRAY OF CHAR) : INTEGER;
  VAR i : INTEGER;
  BEGIN
    i := 0;
    WHILE (a[i] # 0X) & (i < LEN(a)) DO INC(i); END;
    RETURN i; 
  END Length;

  PROCEDURE (v : AbsValue) Print(),NEW,EMPTY;

  PROCEDURE (n : NumValue) Print();
  BEGIN
    IF hexCon & (n.numVal >= 0) THEN
      output.WriteString(LongToHex(n.numVal));
    ELSE
      output.WriteLong(n.numVal);
    END;
  END Print;

  PROCEDURE (f : FltValue) Print();
  VAR
    str : ARRAY 30 OF CHAR;
  BEGIN
    RTS.RealToStr(f.fltVal,str);
    output.WriteString(str);
  END Print;

  PROCEDURE (s : SetValue) Print();
  VAR
    i,j,k : INTEGER;
    first : BOOLEAN;
    inSet : BOOLEAN;
   (* ----------------------------------- *)
    PROCEDURE WriteRange(j,k:INTEGER; VAR f : BOOLEAN);
    BEGIN
      IF f THEN f := FALSE ELSE output.Write(',') END;
      output.WriteInt(j);
      CASE k-j OF
      | 0 : (* skip *)
      | 1 : output.Write(','); 
	    output.WriteInt(k);
      ELSE  output.WriteString('..');
            output.WriteInt(k);
      END;
    END WriteRange;
   (* ----------------------------------- *)
  BEGIN  (* this is an FSA with two states *)
    output.Write("{");
    first := TRUE;  inSet := FALSE; j := 0; k := 0;
    FOR i := 0 TO MAX(SET) DO
      IF inSet THEN
	IF i IN s.setVal THEN k := i;
	ELSE inSet := FALSE; WriteRange(j,k,first);
	END;
      ELSE
	IF i IN s.setVal THEN inSet := TRUE; j := i; k := i END;
      END;
    END;
    IF k = MAX(SET) THEN WriteRange(j,k,first) END;
    output.Write("}");
  END Print;

  PROCEDURE (c : ChrValue) Print();
  BEGIN
    IF (c.chrVal <= " ") OR (c.chrVal > 7EX) THEN
      output.WriteString(hexOf(c.chrVal));
    ELSE
      output.Write("'");
      output.Write(c.chrVal);
      output.Write("'");
    END;
  END Print;

  PROCEDURE (s : StrValue) Print();
  BEGIN
    output.WriteString(qStrOf(s.strVal));
  END Print;

  PROCEDURE (b : BoolValue) Print();
  BEGIN
    IF b.boolVal THEN
      output.WriteString("TRUE");
    ELSE
      output.WriteString("FALSE");
    END;
  END Print;

  PROCEDURE (t : Type) PrintType(indent : INTEGER),NEW,EMPTY;

  PROCEDURE (t : Type) Print(indent : INTEGER;details : BOOLEAN),NEW,EXTENSIBLE;
  BEGIN
    IF t.importedFrom # NIL THEN
      IF t.importedFrom = output.thisMod THEN
        output.WriteKeyword(t.importedName);
      ELSE
        output.WriteImportedTypeName(t.importedFrom, t.importedName);
      END;
      RETURN;
    END;

    IF ~details & (t.declarer # NIL) THEN
      output.WriteTypeName(t.declarer.name); 
    ELSE
      t.PrintType(indent);
    END;
  END Print;

  PROCEDURE (b : Basic) Print(indent : INTEGER; details : BOOLEAN);
  BEGIN
    output.WriteString(b.name);
  END Print;

  PROCEDURE^ PrintList(indent : INTEGER; dl : DescList; xLine : BOOLEAN);

  PROCEDURE (e : Enum) PrintType(indent : INTEGER),EXTENSIBLE;
  VAR
    i : INTEGER;
  BEGIN
    output.WriteKeyword("ENUM");  output.WriteLn;
    PrintList(indent+2,e.ids,FALSE);
    output.Indent(indent);
    output.WriteKeyword("END");
  END PrintType;

  PROCEDURE printBaseType(r : Record) : BOOLEAN;
  VAR
    pType : Pointer;
  BEGIN
    IF r.intrFaces.tide # 0 THEN RETURN TRUE END;
    IF (r.baseType # NIL) & ~(r.baseType IS Basic) THEN
      IF (r.baseType IS Pointer) THEN
        RETURN ~r.baseType(Pointer).isAnonPointer;
      END; 
      IF (r.baseType IS Record) & (r.baseType(Record).isAnonRec) THEN
        pType := r.baseType(Record).ptrType;
        IF (pType = NIL) OR (pType.isAnonPointer) THEN
          RETURN FALSE;
        END;
      END;
      RETURN TRUE;
    ELSE RETURN FALSE;
    END;
  END printBaseType;

  PROCEDURE (r : Record) PrintType(indent : INTEGER),EXTENSIBLE;
  CONST
    eStr = "EXTENSIBLE ";
    aStr = "ABSTRACT ";
    lStr = "LIMITED ";
    iStr = "INTERFACE ";
    vStr = "(* vlCls *) ";
    nStr = "(* noNew *) ";
  VAR
    rStr : ARRAY 12 OF CHAR; 
    iTyp : Type;
    i    : INTEGER;
    fLen : INTEGER;
    fNum : INTEGER;
    sLen : INTEGER;

    PROCEDURE maxFldLen(r : Record) : INTEGER;
      VAR j,l,m : INTEGER;
    BEGIN
      m := 0;
      FOR j := 0 TO r.fields.tide-1 DO
        l := LEN(r.fields.list[j].name$);
        m := MAX(l,m);
      END;
      RETURN m;
    END maxFldLen;

    PROCEDURE fieldNumber(VAR lst : DescList) : INTEGER;
      VAR count : INTEGER;
    BEGIN
      count := 0;
      FOR count := 0 TO lst.tide - 1 DO
        IF lst.list[count] IS ProcDesc THEN RETURN count END;
      END;
      RETURN lst.tide;
    END fieldNumber;

  BEGIN
    CASE r.recAtt MOD 8 OF
    | 1 : rStr := aStr;
    | 2 : rStr := lStr;
    | 3 : rStr := eStr;
    | 4 : rStr := iStr;
    ELSE  rStr := "";
    END;
    IF printFNames THEN
      IF r.recAtt DIV 8 = 1 THEN output.WriteString(nStr);
      ELSIF r.recAtt DIV 16 = 1 THEN output.WriteString(vStr);
      END;
    END;
    output.WriteKeyword(rStr + "RECORD"); 
    IF printBaseType(r) THEN
      output.WriteString(" (");
      IF (r.baseType IS Record) & (r.baseType(Record).ptrType # NIL) THEN
        r.baseType(Record).ptrType.Print(0,FALSE);
      ELSIF r.baseType = NIL THEN
        output.WriteString("ANYPTR");
      ELSE
        r.baseType.Print(0,FALSE); 
      END;
     (* ##### *)
      FOR i := 0 TO r.intrFaces.tide-1 DO
        output.WriteString(" + ");
	iTyp := r.intrFaces.list[i](TypeDesc).type;
        IF (iTyp IS Record) & (iTyp(Record).ptrType # NIL) THEN
          iTyp(Record).ptrType.Print(0,FALSE);
        ELSE
          iTyp.Print(0,FALSE); 
        END;
      END;
     (* ##### *)
      output.WriteString(")"); 
    END;

(* FIXME *)
    IF r.methods.tide > 0 THEN
      IF r.declarer # NIL THEN 
        output.MethRef(r.declarer.name);
      ELSIF (r.ptrType # NIL) & (r.ptrType.declarer # NIL) THEN
        output.MethRef(r.ptrType.declarer.name);
      END;
    END;
(* FIXME *)

    output.WriteLn;
    fLen := maxFldLen(r);
    FOR i := 0 TO r.fields.tide-1 DO
      output.Indent(indent+2);
      output.WriteIdent(r.fields.list[i].name);
      output.Write(accArray[r.fields.list[i].access]);
      output.Indent(fLen - LEN(r.fields.list[i].name$));
      output.WriteString(" : ");
      r.fields.list[i](VarDesc).type.Print(indent + fLen + 6, FALSE);
      output.Write(';'); output.WriteLn;
    END;
    IF r.statics.tide > 0 THEN
      IF alpha THEN
        sLen := r.statics.tide - 1;
        fNum := fieldNumber(r.statics);
        IF fNum > 1    THEN QuickSortDescs(0, fNum-1, r.statics) END;
        IF fNum < sLen THEN QuickSortDescs(fNum, sLen, r.statics) END;
      END;
      output.Indent(indent);
      output.WriteKeyword("STATIC"); output.WriteLn;
      PrintList(indent+2, r.statics, FALSE);
    END;
    output.Indent(indent);
    output.WriteKeyword("END"); 
  END PrintType;

  PROCEDURE (a : Array) PrintType(indent : INTEGER),EXTENSIBLE;
  BEGIN
    output.WriteKeyword("ARRAY ");
    IF a.size > 0 THEN output.WriteInt(a.size);  output.Write(' '); END;
    output.WriteKeyword("OF ");
    a.elemType.Print(indent,FALSE);
  END PrintType;

  PROCEDURE (a : Vector) PrintType(indent : INTEGER),EXTENSIBLE;
  BEGIN
    output.WriteKeyword("VECTOR ");
    output.WriteKeyword("OF ");
    a.elemType.Print(indent,FALSE);
  END PrintType;

  PROCEDURE PrintPar(p : Par; num, indent, pLen : INTEGER; noModes : BOOLEAN);
    VAR extra : INTEGER;
  BEGIN
    extra := pLen+3;
    output.Indent(indent);
    IF ~noModes THEN 
      INC(extra, 4);
      CASE p.mode OF
      | 1 : output.WriteString("IN  ");
      | 2 : output.WriteString("OUT ");
      | 3 : output.WriteString("VAR ");
      ELSE  output.WriteString("    ");
      END;
    END;
    IF p.opNm = NIL THEN
      output.WriteString("p");
      output.WriteInt(num);
      IF num > 9 THEN output.Indent(pLen-3) ELSE output.Indent(pLen-2) END;
    ELSE
      output.WriteString(p.opNm);
      output.Indent(pLen - LEN(p.opNm$));
    END;
    output.WriteString(" : ");
    p.type.Print(indent+extra,FALSE); 
  END PrintPar;

  PROCEDURE PrintFormals(p : Proc; indent : INTEGER);
  VAR
    i    : INTEGER;
    pLen : INTEGER;

    PROCEDURE maxParLen(p : Proc) : INTEGER;
      VAR j,l,m : INTEGER;
    BEGIN
      m := 0;
      FOR j := 0 TO p.pars.tide-1 DO
        IF p.pars.list[j].opNm # NIL THEN
          l := LEN(p.pars.list[j].opNm$);
        ELSIF j > 9 THEN 
          l := 3;
        ELSE  
          l := 2;
        END;
        m := MAX(m,l);
      END;
      RETURN m;
    END maxParLen;

  BEGIN
    output.Write('('); 
    IF p.pars.tide > 0 THEN
      pLen := maxParLen(p);
      PrintPar(p.pars.list[0],0,0, pLen, p.noModes);
      FOR i := 1 TO p.pars.tide-1 DO
        output.Write(';');
        output.WriteLn; 
        PrintPar(p.pars.list[i], i, indent+1, pLen, p.noModes);
      END;
    END;
    output.Write(')'); 
    IF p.retType # NIL THEN
      output.WriteString(' : ');
      p.retType.Print(indent,FALSE);
    END;
  END PrintFormals;

 (* -----------------------------------------------------------	*)

  PROCEDURE (p : Proc) PrintType(indent : INTEGER),EXTENSIBLE;
  BEGIN
    output.WriteKeyword("PROCEDURE");
    PrintFormals(p, indent+9);
  END PrintType;

 (* -----------------------------------------------------------	*)

  PROCEDURE (p : Proc) PrintProc(indent : INTEGER),NEW;
  BEGIN
    output.Indent(indent);
    output.WriteKeyword("PROCEDURE ");
    output.WriteIdent(p.declarer.name);
    output.Write(accArray[p.declarer.access]);
    IF printFNames & (p.fName # NIL) THEN
      output.WriteString('["' + p.fName^ + '"]');
      INC(indent,Length(p.fName)+4);
    END; 
    PrintFormals(p,indent+11+Length(p.declarer.name));
    IF p.isConstructor THEN output.WriteKeyword(",CONSTRUCTOR"); END;
    output.WriteString(";"); output.WriteLn;
  END PrintProc;

 (* -----------------------------------------------------------	*)

  PROCEDURE (m : Meth) PrintType(indent : INTEGER),EXTENSIBLE;
  BEGIN
    output.WriteLn;
    output.WriteKeyword("PROCEDURE ");
    output.Write("(");
    IF m.recMode = 1 THEN 
      output.WriteString("IN "); 
      INC(indent,3);
    ELSIF m.recMode = 3 THEN 
      output.WriteString("VAR "); 
      INC(indent,4);
    END;
    IF m.recName = NIL THEN
      output.WriteString("self"); 
      INC(indent,4);
    ELSE
      output.WriteString(m.recName); 
      INC(indent,LEN(m.recName$));
    END;
    output.WriteString(":"); 
    ASSERT(m.receiver.importedFrom = NIL);
    output.WriteString(m.receiver.declarer.name);
    output.WriteString(") ");
    output.WriteIdent(m.declarer.name);
    output.Write(accArray[m.declarer.access]);
    IF printFNames & (m.fName # NIL) THEN
      output.WriteString('["' + m.fName^ + '"]');
      INC(indent,Length(m.fName)+4);
    END; 
    PrintFormals(m, indent + 15 + 
                    Length(m.declarer.name)+
                    Length(m.receiver.declarer.name));
  
    CASE m.attr OF
    | 1 : output.WriteKeyword(",NEW");
    | 2 : output.WriteKeyword(",ABSTRACT");
    | 3 : output.WriteKeyword(",NEW,ABSTRACT");
    | 4 : output.WriteKeyword(",EMPTY");
    | 5 : output.WriteKeyword(",NEW,EMPTY");
    | 6 : output.WriteKeyword(",EXTENSIBLE");
    | 7 : output.WriteKeyword(",NEW,EXTENSIBLE");
    ELSE  (* nothing *)
    END;
    output.WriteString(";"); output.WriteLn;
  END PrintType;

  PROCEDURE (p : Pointer) PrintType(indent : INTEGER),EXTENSIBLE;
  BEGIN
    output.WriteKeyword("POINTER TO ");
    p.baseType.Print(indent,FALSE);
  END PrintType;

  PROCEDURE (p : Event) PrintType(indent : INTEGER);
  BEGIN
    output.WriteKeyword("EVENT");
    PrintFormals(p, indent+5);
  END PrintType;

  PROCEDURE PrintList(indent : INTEGER; dl : DescList; xLine : BOOLEAN);
  VAR
    i : INTEGER;
    d : Desc;
    m : INTEGER;
   (* ----------------------------------------------- *)
    PROCEDURE notHidden(d : Desc) : BOOLEAN;
    BEGIN
      RETURN verbatim OR ((d.name[0] # "@") & (d.name[0] # "$"));
    END notHidden;
   (* ----------------------------------------------- *)
    PROCEDURE maxNamLen(dl : DescList) : INTEGER;
      VAR j,l,m : INTEGER; 
          d     : Desc;
    BEGIN
      m := 0;
      FOR j := 0 TO dl.tide-1 DO
        d := dl.list[j];
        IF notHidden(d) THEN m := MAX(m, LEN(d.name$)) END;
      END;
      RETURN m;
    END maxNamLen;
   (* ----------------------------------------------- *)
  BEGIN
    m := maxNamLen(dl);
    FOR i := 0 TO dl.tide -1 DO 
      d := dl.list[i];
      IF ~notHidden(d) THEN 
        (* skip *)
      ELSIF d IS ProcDesc THEN 
        d(ProcDesc).pType.PrintProc(indent);
        IF xLine THEN output.WriteLn; END;
      ELSE
        output.Indent(indent);
        IF d IS TypeDesc THEN 
          output.WriteTypeDecl(d.name); 
        ELSE
          output.WriteIdent(d.name);
        END;
        output.Write(accArray[d.access]);

        IF (d IS VarDesc) OR (d IS ConstDesc) THEN 
          output.Indent(m - LEN(d.name$));
        END;

        WITH d : ConstDesc DO
            output.WriteString(" = ");
            d.val.Print();
        | d : TypeDesc DO
          IF d IS VarDesc THEN
            output.WriteString(" : ");
          ELSE
            output.WriteString(" = ");
          END;
          d.type.Print(Length(d.name)+6, d IS UserTypeDesc);
        END;
        output.Write(";");
        output.WriteLn;
        IF xLine THEN output.WriteLn; END;
      END;
    END;
  END PrintList;

(* ==================================================================== *)

  PROCEDURE PrintDigest(i0,i1 : INTEGER);
    VAR buffer : ARRAY 17 OF CHAR;
        index  : INTEGER;
   (* ------------------------------------ *)
    PROCEDURE hexRep(i : INTEGER) : CHAR;
    BEGIN
      i := ORD(BITS(i) * {0..3});
      IF i <= 9 THEN RETURN CHR(ORD("0") + i);
      ELSE RETURN CHR(ORD("A") - 10 + i);
      END;
    END hexRep;
   (* ------------------------------------ *)
  BEGIN
    IF (i0 = 0) & (i1 = 0) THEN RETURN END;
    output.Write(" "); output.Write("[");
    FOR index := 7 TO 0 BY -1 DO
      buffer[index] := hexRep(i0); i0 := i0 DIV 16;
    END;
    FOR index := 15 TO 8 BY -1 DO
      buffer[index] := hexRep(i1); i1 := i1 DIV 16;
    END;
    buffer[16] := 0X;
    output.WriteString(buffer);
    output.Write("]");
  END PrintDigest;

(* ==================================================================== *)

  PROCEDURE PrintModule(mod : Module);
  VAR
    i,j : INTEGER;
    ty : Type;
    rec : Record;
    first : BOOLEAN;
    heading : ARRAY 20 OF CHAR;
    (* --------------------------- *)
    PROCEDURE WriteOptionalExtras(impMod : Module);
    BEGIN
      IF impMod.fName # NIL THEN
        IF printFNames THEN
          output.WriteString(' (* "' + impMod.fName^ + '" *)');
        ELSE
          output.WriteString(' := "' + impMod.fName^ + '"');
        END; 
      END; 
    END WriteOptionalExtras;
    (* --------------------------- *)
  BEGIN

    IF (mod.types.tide > 0) & alpha THEN 
      QuickSortDescs(0, mod.types.tide-1, mod.types);
    END;

    output.WriteStart(mod);
    IF mod.systemMod THEN
      heading := "SYSTEM ";
    ELSIF mod.fName # NIL THEN
      heading := "FOREIGN ";
    ELSE
      heading := "";
    END;
    heading := heading + "MODULE ";
    output.WriteKeyword(heading);
    output.WriteIdent(mod.name);
    IF printFNames & (mod.fName # NIL) THEN 
      output.WriteString(' ["' + mod.fName^ + '"]'); 
    END;
    output.Write(';'); 
   (*
    *  Optional strong name goes here.
    *)
    IF mod.strongNm # NIL THEN
      output.WriteLn; 
      output.WriteString("    (* version ");
      output.WriteInt(mod.strongNm[0]); output.Write(":");
      output.WriteInt(mod.strongNm[1]); output.Write(":");
      output.WriteInt(mod.strongNm[2]); output.Write(":");
      output.WriteInt(mod.strongNm[3]); 
      PrintDigest(mod.strongNm[4], mod.strongNm[5]);
      output.WriteString(" *)");
    END;
   (*  end optional strong name.  *)
    output.WriteLn; output.WriteLn;
    IF mod.imports.tide > 1 THEN
      output.WriteKeyword("IMPORT"); output.WriteLn;
      output.Indent(4);
      output.WriteImport(mod.imports.list[1]); 
      WriteOptionalExtras(mod.imports.list[1]);
      FOR i := 2 TO mod.imports.tide -1 DO 
        output.Write(','); output.WriteLn; 
        output.Indent(4);
        output.WriteImport(mod.imports.list[i]); 
        WriteOptionalExtras(mod.imports.list[i]);
      END;
      output.Write(';'); output.WriteLn;
      output.WriteLn;
    END;
    IF mod.consts.tide > 0 THEN 
      output.WriteKeyword("CONST"); output.WriteLn; 
      PrintList(2,mod.consts,FALSE);
      output.WriteLn;
    END;
    IF mod.types.tide > 0 THEN 
      output.WriteKeyword("TYPE");  
      output.WriteLn; output.WriteLn;
      PrintList(2,mod.types,TRUE);
      output.WriteLn;
    END;
    IF mod.vars.tide > 0 THEN 
      output.WriteKeyword("VAR"); output.WriteLn; 
      PrintList(2,mod.vars,FALSE);
      output.WriteLn;
    END;
    FOR i := 0 TO mod.procs.tide -1 DO 
      output.WriteLn;
      mod.procs.list[i](ProcDesc).pType.PrintProc(0);
    END;
    output.WriteLn;
    FOR i := 0 TO mod.types.tide -1 DO 
      ty := mod.types.list[i](UserTypeDesc).type;
      IF ty IS Pointer THEN ty := ty(Pointer).baseType; END;
      IF ty IS Record THEN
        rec := ty(Record);

        IF (rec.methods.tide > 0) & alpha THEN 
          QuickSortDescs(0, rec.methods.tide-1, rec.methods);
        END;

(* FIXME *)
        IF rec.methods.tide > 0 THEN
          IF rec.declarer # NIL THEN 
            output.MethAnchor(rec.declarer.name);
          ELSIF (rec.ptrType # NIL) & (rec.ptrType.declarer # NIL) THEN
            output.MethAnchor(rec.ptrType.declarer.name);
          END;
        END;
(* FIXME *)

        FOR j := 0 TO rec.methods.tide -1 DO
          rec.methods.list[j](ProcDesc).pType.PrintType(0);
        END;
      END;
    END;
    output.WriteLn;
    output.WriteKeyword("END ");
    output.WriteIdent(mod.name);
    output.Write("."); output.WriteLn; 
    output.WriteEnd();
  END PrintModule;

(* ============================================================ *)

  PROCEDURE InitTypes();
  VAR
    t : Basic;
  BEGIN
    NEW(typeList,50);
    typeList[0] := NIL;
    NEW(t); t.name := BOX("BOOLEAN"); typeList[1] := t;
    NEW(t); t.name := BOX("SHORTCHAR"); typeList[2] := t;
    NEW(t); t.name := BOX("CHAR"); typeList[3] := t;
    NEW(t); t.name := BOX("BYTE"); typeList[4] := t;
    NEW(t); t.name := BOX("SHORTINT"); typeList[5] := t;
    NEW(t); t.name := BOX("INTEGER"); typeList[6] := t;
    NEW(t); t.name := BOX("LONGINT"); typeList[7] := t;
    NEW(t); t.name := BOX("SHORTREAL"); typeList[8] := t;
    NEW(t); t.name := BOX("REAL"); typeList[9] := t;
    NEW(t); t.name := BOX("SET"); typeList[10] := t;
    NEW(t); t.name := BOX("ANYREC"); typeList[11] := t;
    NEW(t); t.name := BOX("ANYPTR"); typeList[12] := t;
    NEW(t); t.name := BOX("ARRAY OF CHAR"); typeList[13] := t;
    NEW(t); t.name := BOX("ARRAY OF SHORTCHAR"); typeList[14] := t;
    NEW(t); t.name := BOX("UBYTE"); typeList[15] := t;
(*
 *  NEW(t); t.name := "SPECIAL"; typeList[16] := t;
 *)
  END InitTypes;
 
  PROCEDURE InitAccArray();
  BEGIN
    accArray[0] := ' ';
    accArray[1] := '*';
    accArray[2] := '-';
    accArray[3] := '!';
  END InitAccArray;

(* ============================================================ *)

PROCEDURE Usage();
BEGIN
  Console.WriteString("gardens point Browse: " + GPCPcopyright.verStr);
  Console.WriteLn;
  IF RTS.defaultTarget = "net" THEN
    Console.WriteString("Usage:  Browse [options] <ModuleName>");
    Console.WriteLn;
    Console.WriteString("Browse Options ... ");
    Console.WriteLn;
    Console.WriteString(" /all ==> browse this and all imported modules");
    Console.WriteLn;
    Console.WriteString(" /file ==> write output to a file <ModuleName>.bro ");
    Console.WriteLn;
    Console.WriteString(" /full ==> display explicit foreign names ");
    Console.WriteLn;
    Console.WriteString(" /help ==> display this usage message");
    Console.WriteLn;
    Console.WriteString(" /hex  ==> use hexadecimal for short literals"); 
    Console.WriteLn;
    Console.WriteString(
                    " /html ==> write html output to file <ModuleName>.html");
    Console.WriteLn;
    Console.WriteString(" /sort ==> sort procedures and types alphabetically");
    Console.WriteLn;
    Console.WriteString(" /verbatim ==> display anonymous public type names");
    Console.WriteLn;
  ELSE			(* RTS.defaultTarget = "jvm" *)
    Console.WriteString("Usage: cprun Browse [options] <ModuleName>");
    Console.WriteLn;
    Console.WriteString("Browse Options ... ");
    Console.WriteLn;
    Console.WriteString(" -all ==> browse this and all imported modules");
    Console.WriteLn;
    Console.WriteString(" -file ==> write output to a file <ModuleName>.bro ");
    Console.WriteLn;
    Console.WriteString(" -full ==> display explicit foreign names ");
    Console.WriteLn;
    Console.WriteString(" -help ==> display this usage message");
    Console.WriteLn;
    Console.WriteString(" -hex  ==> use hexadecimal for short literals"); 
    Console.WriteLn;
    Console.WriteString(
	" -html ==> write html output to file <ModuleName>.html");
    Console.WriteLn;
    Console.WriteString(" -sort ==> sort procedures and types alphabetically");
    Console.WriteLn;
    Console.WriteString(" -verbatim ==> display anonymous public type names");
    Console.WriteLn;
  END;
  HALT(1);
END Usage;

PROCEDURE BadOption(optStr : ARRAY OF CHAR);
BEGIN
  Console.WriteString("Unrecognised option: " + optStr);
  Console.WriteLn;
END BadOption;

PROCEDURE ParseOptions() : INTEGER;
VAR
  argNo : INTEGER;
  option : FileNames.NameString;
  fOutput : FileOutput;
  hOutput : HtmlOutput; 
  fileOutput, htmlOutput : BOOLEAN;
BEGIN
  printFNames := FALSE;
  fileOutput := FALSE;
  htmlOutput := FALSE;
  verbatim := FALSE;
  hexCon := FALSE;
  doAll := FALSE;
  alpha := FALSE;
  argNo := 0;
  ProgArgs.GetArg(argNo,option); 
  WHILE (option[0] = '-') OR (option[0] = GPFiles.optChar) DO
    INC(argNo);
    option[0] := '-';
    IF option[1] = 'f' THEN
      IF option = "-full" THEN
        printFNames := TRUE;
      ELSIF option = "-file" THEN
        IF htmlOutput THEN 
          Console.WriteString("Cannot have html and file output");
          Console.WriteLn;
        ELSE
          fileOutput := TRUE;
          NEW(fOutput);
          output := fOutput;
        END;
      ELSE
        BadOption(option);
      END;
    ELSIF option[1] = 'v' THEN
      IF option = "-verbatim" THEN
        verbatim := TRUE;
      ELSIF option = "-verbose" THEN
        verbose := TRUE;
      ELSE
        BadOption(option);
      END;
    ELSIF option = "-all" THEN
      doAll := TRUE;
    ELSIF option = "-hex" THEN
      hexCon := TRUE;
    ELSIF option = "-html" THEN
      IF fileOutput THEN 
        Console.WriteString("Cannot have html and file output");
        Console.WriteLn;
      ELSE
        htmlOutput := TRUE;
        NEW(hOutput);
        output := hOutput;
      END;
    ELSIF option = "-sort" THEN
      alpha := TRUE;
    ELSIF option = "-help" THEN
      Usage();
    ELSE
      BadOption(option);
    END;
    IF argNo < args THEN ProgArgs.GetArg(argNo,option) ELSE RETURN argNo END;
  END;
  RETURN argNo;
END ParseOptions;

PROCEDURE Print();
VAR
  i : INTEGER;
BEGIN
  FOR i := 0 TO modList.tide-1 DO
    IF modList.list[i].print THEN
      output.thisMod := modList.list[i];
      IF output IS FileOutput THEN
        output(FileOutput).file := 
            GPTextFiles.createFile(modList.list[i].name^ + outExt);
      END;
      PrintModule(modList.list[i]); 
      IF output IS FileOutput THEN
        GPTextFiles.CloseFile(output(FileOutput).file);
      END;
    END;
  END;
RESCUE (x)
  Error.WriteString("Error in Parse()"); Error.WriteLn;
  Error.WriteString(RTS.getStr(x)); Error.WriteLn;
END Print;

BEGIN
  NEW(fileName, 256);
  NEW(modName, 256);
  InitTypes();
  InitAccArray();
  modList.tide := 0;
  NEW(modList.list,5);
  NEW(output);
  args := ProgArgs.ArgNumber();
  IF (args < 1) THEN Usage(); END; 
  argNo := ParseOptions(); 
  IF (output IS FileOutput) THEN
    IF (output IS HtmlOutput) THEN 
      outExt := htmlExt;
    ELSE
      outExt := broExt;
    END; 
  END;
  WHILE (argNo < args) DO
    ProgArgs.GetArg(argNo,fileName);
    GetSymAndModNames(fileName,modName);
    module := GetModule(modName);
    module.symName := fileName;
    module.progArg := TRUE;
    INC(argNo);
  END;
  Parse();
  Print();
END Browse.

(* ============================================================ *)
