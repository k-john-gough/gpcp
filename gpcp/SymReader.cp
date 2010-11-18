MODULE SymReader;
(* ========================================================================= *)
(*                                                                           *)
(*  Symbol file reading module for the .NET to Gardens Point Component       *)
(*  Pascal Symbols tool.                                                     *)
(*      Copyright (c) Siu-Yuen Chan 2001.                                    *)
(*                                                                           *)
(*  This module reads Gardens Point Component Pascal (GPCP) symbol files     *)
(*  and stores all meta information read into METASTORE (defined by          *)
(*  MetaStore module).                                                       *)
(* ========================================================================= *)

IMPORT
    Error,
    GPFiles,
    GF := GPBinFiles,
    MS := MetaStore,
    MP := MetaParser,
    ST := AscString,
    RTS;

(* ========================================================================= *
// Collected syntax ---
// 
// SymFile    = Header [String (falSy | truSy | <other attribute>)]
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
// Pointer    = TypeHeader ptrSy TypeOrd.
// Event      = TypeHeader evtSy FormalType.
// ProcType   = TypeHeader pTpSy FormalType.
// Record     = TypeHeader recSy recAtt [truSy | falSy]
//              [basSy TypeOrd] [iFcSy {basSy TypeOrd}]
//              {Name TypeOrd} {Method} endRc.
//      -- truSy ==> is an extension of external interface
//      -- falSy ==> is an extension of external class
//      -- basSy option defines base type, if not ANY / j.l.Object
// NamedType  = TypeHeader
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
    modSy = ORD('H');  namSy = ORD('$');  bytSy = ORD('\');
    numSy = ORD('#');  chrSy = ORD('c');  strSy = ORD('s');
    fltSy = ORD('r');  falSy = ORD('0');  truSy = ORD('1');
    impSy = ORD('I');  setSy = ORD('S');  keySy = ORD('K');
    conSy = ORD('C');  typSy = ORD('T');  tDefS = ORD('t');
    prcSy = ORD('P');  retSy = ORD('R');  mthSy = ORD('M');
    varSy = ORD('V');  parSy = ORD('p');  start = ORD('&');
    close = ORD('!');  recSy = ORD('{');  endRc = ORD('}');
    frmSy = ORD('(');  fromS = ORD('@');  endFm = ORD(')');
    arrSy = ORD('[');  endAr = ORD(']');  pTpSy = ORD('%');
    ptrSy = ORD('^');  basSy = ORD('+');  eTpSy = ORD('e');
    iFcSy = ORD('~');  evtSy = ORD('v');

CONST
    tOffset*  = 16; (* backward compatibility with JavaVersion *)
    iOffset   = 1;

CONST
    magic   = 0DEADD0D0H;
    syMag   = 0D0D0DEADH;
    dumped* = -1;

CONST (* record attributes *)
    noAtt* = ORD(MS.noAtt);  (* no attribute *)
    abstr* = ORD(MS.Rabstr); (* Is ABSTRACT *)
    limit* = ORD(MS.Rlimit); (* Is LIMIT *)
    extns* = ORD(MS.Rextns); (* Is EXTENSIBLE *)
    iFace* = ORD(MS.RiFace); (* Is INTERFACE *) 
    nnarg* = ORD(MS.Rnnarg); (* Has NO NoArg Constructor ( cannot use NEW() ) *)
    valTp* = ORD(MS.RvalTp); (* ValueType *)


TYPE
(*
    CharOpen* = POINTER TO ARRAY OF CHAR;
*)
    CharOpen* = ST.CharOpen;

    TypeSeq = POINTER TO
        RECORD 
            tide: INTEGER;
            high: INTEGER;
            a: POINTER TO ARRAY OF MS.Type;
        END;

    ScopeSeq = POINTER TO
        RECORD 
            tide: INTEGER;
            high: INTEGER;
            a: POINTER TO ARRAY OF MS.Namespace;
        END;

    Reader* = POINTER TO 
        RECORD
            file: GF.FILE;
            fasb: MS.Assembly;
            fns : MS.Namespace;
            sSym  : INTEGER;            (* the symbol read in *)
            cAtt  : CHAR;               (* character attribute *)
            iAtt  : INTEGER;            (* integer attribute *)
            lAtt  : LONGINT;            (* long attribute *)
            rAtt  : REAL;               (* real attribute *)
            sAtt  : ARRAY 128 OF CHAR;  (* string attribute *)
            sArray: ScopeSeq;
            tArray: TypeSeq;
            tNxt  : INTEGER;
        END;

    (* for building temporary formal list *)
    FmlList = POINTER TO
        RECORD
            fml: MS.Formal;
            nxt: FmlList;
        END;



PROCEDURE InitTypeSeq(seq: TypeSeq; capacity : INTEGER); 
BEGIN
    NEW(seq.a, capacity);
    seq.high := capacity-1;
    seq.tide := 0;
END InitTypeSeq;


PROCEDURE AppendType(VAR seq : TypeSeq; elem : MS.Type);
VAR
    temp : POINTER TO ARRAY OF MS.Type;
    i    : INTEGER;
BEGIN
    IF seq.a = NIL THEN 
      InitTypeSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END; (* IF *)
    seq.a[seq.tide] := elem; INC(seq.tide); 
END AppendType;


PROCEDURE InitScopeSeq(seq: ScopeSeq; capacity : INTEGER); 
BEGIN
    NEW(seq.a, capacity);
    seq.high := capacity-1;
    seq.tide := 0;
END InitScopeSeq;


PROCEDURE AppendScope(VAR seq : ScopeSeq; elem : MS.Namespace);
VAR
    temp : POINTER TO ARRAY OF MS.Namespace;
    i    : INTEGER;
BEGIN
    IF seq.a = NIL THEN 
      InitScopeSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END; (* IF *)
    seq.a[seq.tide] := elem; INC(seq.tide); 
END AppendScope;


PROCEDURE (rd: Reader) Read(): INTEGER, NEW;
BEGIN
    RETURN GF.readByte(rd.file);
END Read;


PROCEDURE (rd: Reader) ReadChar(): CHAR, NEW;
BEGIN
    RETURN CHR(rd.Read() * 256 + rd.Read());
END ReadChar;


PROCEDURE (rd: Reader) ReadInt(): INTEGER, NEW;
BEGIN [UNCHECKED_ARITHMETIC]
    (* overflow checking off here *)
    RETURN ((rd.Read() * 256 + rd.Read()) * 256 + rd.Read()) * 256 + rd.Read();
END ReadInt;


PROCEDURE (rd: Reader) ReadLong(): LONGINT, NEW;
VAR
    result : LONGINT;
    index  : INTEGER;
BEGIN [UNCHECKED_ARITHMETIC]
    (* overflow checking off here *)
    result := rd.Read();
    FOR index := 1 TO 7 DO
      result := result * 256 + rd.Read();
    END; (* FOR *)
    RETURN result;
END ReadLong;


PROCEDURE (rd: Reader) ReadReal(): REAL, NEW;
VAR
    result : LONGINT;
BEGIN
    result := rd.ReadLong();
    RETURN RTS.longBitsToReal(result);
END ReadReal;


PROCEDURE (rd: Reader) ReadOrd(): INTEGER, NEW;
VAR
    chr : INTEGER;
BEGIN
    chr := rd.Read();
    IF chr <= 07FH THEN RETURN chr;
    ELSE
        DEC(chr, 128);
        RETURN chr + rd.Read() * 128;
    END; (* IF *)
END ReadOrd;


PROCEDURE (rd: Reader) ReadUTF(OUT nam : ARRAY OF CHAR), NEW;
CONST
    bad = "Bad UTF-8 string";
VAR
    num : INTEGER;
    bNm : INTEGER;
    idx : INTEGER;
    chr : INTEGER;
BEGIN
    num := 0;
    bNm := rd.Read() * 256 + rd.Read();
    FOR idx := 0 TO bNm-1 DO
        chr := rd.Read();
        IF chr <= 07FH THEN                   (* [0xxxxxxx] *)
            nam[num] := CHR(chr); INC(num);
        ELSIF chr DIV 32 = 06H THEN           (* [110xxxxx,10xxxxxx] *)
            bNm := chr MOD 32 * 64;
            chr := rd.Read();
            IF chr DIV 64 = 02H THEN
                nam[num] := CHR(bNm + chr MOD 64); INC(num);
            ELSE
                RTS.Throw(bad);
            END; (* IF *)
        ELSIF chr DIV 16 = 0EH THEN           (* [1110xxxx,10xxxxxx,10xxxxxx] *)
            bNm := chr MOD 16 * 64;
            chr := rd.Read();
            IF chr DIV 64 = 02H THEN
                bNm := (bNm + chr MOD 64) * 64; 
                chr := rd.Read();
                IF chr DIV 64 = 02H THEN
                    nam[num] := CHR(bNm + chr MOD 64); INC(num);
                ELSE 
                    RTS.Throw(bad);
                END; (* IF *)
            ELSE
                RTS.Throw(bad);
            END; (* IF *)
        ELSE
            RTS.Throw(bad);
        END; (* IF *)
    END; (* FOR *)
    nam[num] := 0X;
END ReadUTF;


PROCEDURE (rd: Reader) GetSym(), NEW;
BEGIN
    rd.sSym := rd.Read();
    CASE rd.sSym OF
    | namSy : 
        rd.iAtt := rd.Read(); rd.ReadUTF(rd.sAtt);
    | strSy : 
        rd.ReadUTF(rd.sAtt);
    | retSy, fromS, tDefS, basSy :
        rd.iAtt := rd.ReadOrd();
    | bytSy :
        rd.iAtt := rd.Read();
    | keySy, setSy :
        rd.iAtt := rd.ReadInt();
    | numSy :
        rd.lAtt := rd.ReadLong();
    | fltSy :
        rd.rAtt := rd.ReadReal();
    | chrSy :
        rd.cAtt := rd.ReadChar();
    ELSE (* nothing to do *)
    END; (* CASE *)
END GetSym;


PROCEDURE (rd: Reader) Abandon(), NEW;
BEGIN
    RTS.Throw(ST.StrCat(ST.ToChrOpen("Bad symbol file format - "),
                        ST.ToChrOpen(GF.getFullPathName(rd.file))));
END Abandon;


PROCEDURE (rd: Reader) ReadPast(sym : INTEGER), NEW;
BEGIN
    IF rd.sSym # sym THEN rd.Abandon(); END;
    rd.GetSym();
END ReadPast;


PROCEDURE NewReader*(file: GF.FILE) : Reader;
VAR
    new: Reader;
BEGIN
    NEW(new);
    NEW(new.tArray);
    NEW(new.sArray);
    new.file := file;
    InitTypeSeq(new.tArray, 8);
    InitScopeSeq(new.sArray, 8);
    new.tNxt := tOffset;
    RETURN new;
END NewReader;


PROCEDURE (rd: Reader) TypeOf(ord : INTEGER): MS.Type, NEW;
VAR
    newT : MS.TempType;
    indx : INTEGER;
    rslt : MS.Type;
BEGIN
    IF ord < tOffset THEN                               (* builtin type *)
        rslt := MS.baseTypeArray[ord];
        IF rslt = NIL THEN
            rslt := MS.MakeDummyPrimitive(ord);
        END; (* IF *)
        RETURN rslt;
    ELSIF ord - tOffset < rd.tArray.tide THEN           (* type already read *)
        RETURN rd.tArray.a[ord - tOffset];
    ELSE 
        indx := rd.tArray.tide + tOffset;
        REPEAT
            (* create types and append to tArray until ord is reached *)
            (* details of these types are to be fixed later *)
            newT := MS.NewTempType();
            newT.SetTypeOrd(indx); INC(indx);
            AppendType(rd.tArray, newT);
        UNTIL indx > ord;
        RETURN newT;
    END; (* IF *)
END TypeOf;


PROCEDURE (rd: Reader) GetTypeFromOrd(): MS.Type, NEW;
VAR
    ord : INTEGER;
BEGIN
    ord := rd.ReadOrd();
    rd.GetSym();
    RETURN rd.TypeOf(ord);
END GetTypeFromOrd;


PROCEDURE (rd: Reader) GetHeader(modname: CharOpen), NEW;
VAR
    marker: INTEGER;
    idx1, idx2: INTEGER;
    scopeNm: CharOpen;
    str: CharOpen;
BEGIN
    marker := rd.ReadInt();
    IF marker = RTS.loInt(magic) THEN
        (* normal case, nothing to do *)
    ELSIF marker = RTS.loInt(syMag) THEN
        (* should never reach here for foreign module *)
    ELSE
        (* Error *)
        Error.WriteString("File <");
        Error.WriteString(GF.getFullPathName(rd.file));
        Error.WriteString("> wrong format"); Error.WriteLn;
        RETURN;
    END; (* IF *)
    rd.GetSym();
    rd.ReadPast(modSy);
    IF rd.sSym = namSy THEN
        IF modname^ # ST.ToChrOpen(rd.sAtt)^ THEN
            Error.WriteString("Wrong name in symbol file. Expected <");
            Error.WriteString(modname); Error.WriteString(">, found <");
            Error.WriteString(rd.sAtt); Error.WriteString(">"); Error.WriteLn;
            HALT(1);
        END; (* IF *)
        rd.GetSym();
    ELSE
        RTS.Throw("Bad symfile header");
    END; (* IF *)
    IF rd.sSym = strSy THEN (* optional name *)
        (* non-GPCP module *)
        scopeNm := ST.ToChrOpen(rd.sAtt);
        idx1 := ST.StrChr(scopeNm, '['); idx2 := ST.StrChr(scopeNm, ']');
        str := ST.SubStr(scopeNm,idx1+1, idx2-1);
        rd.fasb := MS.GetAssemblyByName(ST.StrSubChr(str,'.','_'));
        ASSERT(rd.fasb # NIL);
        str := ST.SubStr(scopeNm, idx2+1, LEN(scopeNm)-1);
        rd.fns  := rd.fasb.GetNamespace(str);
        ASSERT(rd.fns # NIL);

        rd.GetSym();
        IF (rd.sSym = falSy) OR (rd.sSym = truSy) THEN
            rd.GetSym();
        ELSE
            RTS.Throw("Bad explicit name");
        END; (* IF *)
    ELSE
        (* GPCP module *)
        rd.fasb := MS.GetAssemblyByName(modname);
        ASSERT(rd.fasb # NIL);
        rd.fns  := rd.fasb.GetNamespace(modname);
        ASSERT(rd.fns # NIL);
    END; (* IF *)
END GetHeader;


PROCEDURE (rd: Reader) GetVersionName(), NEW;
VAR
    i: INTEGER;
    version: MS.Version;
    token: MS.PublicKeyToken;
BEGIN
    (* get the assembly version *)
    ASSERT(rd.sSym = numSy); NEW(version);
    version[MS.Major] := RTS.loShort(RTS.hiInt(rd.lAtt));
    version[MS.Minor] := RTS.loShort(RTS.loInt(rd.lAtt));
    rd.GetSym();
    version[MS.Build] := RTS.loShort(RTS.hiInt(rd.lAtt));
    version[MS.Revis] := RTS.loShort(RTS.loInt(rd.lAtt));
    rd.fasb.SetVersion(version);
    (* get the assembly public key token *)
    rd.sSym := rd.Read();
    ASSERT(rd.sSym = numSy); NEW(token);
    FOR i := 0 TO 7 DO
        token[i] := RTS.loByte(RTS.loShort(rd.Read()));
    END;
    rd.fasb.SetPublicKeyToken(token);
    (* get next symbol *)
    rd.GetSym();
END GetVersionName;


PROCEDURE (rd: Reader)GetLiteral(): MS.Literal, NEW;
VAR
    lit: MS.Literal;
BEGIN
    CASE rd.sSym OF
    | truSy :
        lit := MS.MakeBoolLiteral(TRUE);
    | falSy :
        lit := MS.MakeBoolLiteral(FALSE);
    | numSy :
        lit := MS.MakeLIntLiteral(rd.lAtt);
    | chrSy :
        lit := MS.MakeCharLiteral(rd.cAtt);
    | fltSy :
        lit := MS.MakeRealLiteral(rd.rAtt);
    | setSy :
        lit := MS.MakeSetLiteral(BITS(rd.iAtt));
    | strSy :
        lit := MS.MakeStrLiteral(ST.ToChrOpen(rd.sAtt));        (* implicit rd.sAtt^ *)
    ELSE
        RETURN NIL;
    END; (* CASE *)
    rd.GetSym();                                        (* read past value  *)
    RETURN lit;
END GetLiteral;


PROCEDURE (rd: Reader) Import, NEW;
VAR
    mname: CharOpen;
    asbname: CharOpen;
    asbfile: CharOpen;
    nsname: CharOpen;
    scopeNm: CharOpen;
    idx1, idx2: INTEGER;
    len: INTEGER;
    asb: MS.Assembly;
    ns: MS.Namespace;
BEGIN
    rd.ReadPast(namSy);
    mname := ST.ToChrOpen(rd.sAtt);
    IF rd.sSym = strSy THEN
        (* non-GPCP module *)
        scopeNm := ST.ToChrOpen(rd.sAtt);
        idx1 := ST.StrChr(scopeNm, '['); idx2 := ST.StrChr(scopeNm, ']');
        asbfile := ST.SubStr(scopeNm,idx1+1, idx2-1);
        nsname := ST.SubStr(scopeNm, idx2+1, LEN(scopeNm)-1);
        rd.GetSym();
    ELSE
        (* possible GPCP module *)
        len := LEN(mname);
        IF mname[len-2] = '_' THEN mname := ST.SubStr(mname, 0, len-3); END;
        asbfile := mname;
        nsname  := mname;  (* or it can be assigned as MS.NULLSPACE *)
    END; (* IF *)
    (* need to get the assembly real name here *)
    asbname := MP.GetAssemblyRealName(asbfile);
    asb := MS.InsertAssembly(asbname, asbfile);
    ns := asb.InsertNamespace(nsname);
    AppendScope(rd.sArray, ns);
    rd.ReadPast(keySy);
END Import;


PROCEDURE (rd: Reader) ParseType, NEW;
VAR
    typ: MS.TempType;
    ord: INTEGER;
BEGIN
    typ := MS.NewTempType();    (* this is a temporay type, not the final type *)
    typ.SetName(ST.ToChrOpen(rd.sAtt));
    typ.SetFullName(ST.StrCat(ST.StrCatChr(rd.fns.GetName(),'.'),typ.GetName()));
    typ.SetVisibility(rd.iAtt);
    ord := rd.ReadOrd();
    IF ord >= tOffset THEN
        ASSERT(rd.tNxt = ord);
        typ.SetTypeOrd(ord);
        AppendType(rd.tArray, typ); INC(rd.tNxt);
        typ.SetNamespace(rd.fns);
    ELSE
        (* primitive types *)
    END; (* IF *)
    rd.GetSym();
END ParseType;


PROCEDURE (rd: Reader) GetFormalTypes(): MS.FormalList, NEW;
(*
// FormalType = [retSy TypeOrd] frmSy {parSy byte TypeOrd} endFm.
//      -- optional phrase is return type for proper procedures
 *)
CONST
    FNAME = "arg";
VAR
    rslt: MS.FormalList;
    ftype: MS.Type;
    fmode: INTEGER;
    count: INTEGER;
    temp: FmlList;
    head: FmlList;
    last: FmlList;
    fml: MS.Formal;
    pos: INTEGER;
    str: CharOpen;
    nametype: MS.Type;
    unresolved: INTEGER;
BEGIN
    head := NIL; last := NIL; count := 0; ftype := NIL; NEW(str,3); unresolved := 0;
    rd.ReadPast(frmSy);
    WHILE rd.sSym = parSy DO
        fmode := rd.Read();
        ftype := rd.GetTypeFromOrd();
        RTS.IntToStr(count, str);
        WITH ftype: MS.NamedType DO
            fml := MS.MakeFormal(ST.StrCat(ST.ToChrOpen(FNAME),str), ftype, fmode);
        | ftype: MS.TempType DO
            fml := MS.MakeFormal(ST.StrCat(ST.ToChrOpen(FNAME),str), MS.dmyTyp, fmode);
            (* collect reference if TempType/NamedType *)
            ftype.AddReferenceFormal(fml);
            INC(unresolved);
        ELSE
            fml := MS.MakeFormal(ST.StrCat(ST.ToChrOpen(FNAME),str), ftype, fmode);
        END; (* WITH *)

        (* add the formal to a temporary formals linkedlist *)
        NEW(temp); temp.nxt := NIL; temp.fml := fml;
        IF last # NIL THEN last.nxt := temp; last := temp; ELSE last := temp; head := temp; END;
        INC(count);
    END; (* WHILE *)
    rd.ReadPast(endFm);

    (* now I know how many formals for the method *)
    rslt := MS.CreateFormalList(count);
    temp := head; pos := 0;
    WHILE temp # NIL DO
        rslt.AddFormal(temp.fml, pos);
        temp := temp.nxt; INC(pos);
    END; (* WHILE *)
    rslt.ostd := unresolved;
    RETURN rslt;
END GetFormalTypes;


PROCEDURE FixProcTypes(rec: MS.RecordType; newM: MS.Method; fl: MS.FormalList; rtype: MS.Type);
VAR
    newF: MS.Method;
BEGIN
    IF MS.WithoutMethodNameMangling() THEN
        newF := newM;
        WITH newF: MS.Function DO
            WITH rtype: MS.TempType DO
                (* not a concrete return type *)
                WITH rtype: MS.NamedType DO
                    (* return type name is resolved *)
                    IF fl.ostd = 0 THEN
                        (* no unresolved formal types names *)
                        newM.FixSigCode();               (* fix the sigcode of newM *)
                        newM := rec.AddMethod(newM);
                    ELSE
                        (* need to AddMethod after formal type names resolved *)
                    END; (* IF *)
                ELSE
                    (* return type name is unresolved *)
                    INC(newF.ostd);
                    (* need to AddMethod after return type name and formal type names resolved *)
                END; (* IF *)

                (* collect reference if TempType/NamedType *)
                rtype.AddReferenceFunction(newF);
            ELSE
                (* concrete return type ==> type name is solved *)
                IF fl.ostd = 0 THEN
                    (* no unresolved formal types names *)
                    newM.FixSigCode();               (* fix the sigcode of newM *)
                    newM := rec.AddMethod(newM);
                ELSE
                    (* need to AddMethod after formal type names resolved *)
                END; (* IF *)
            END; (* WITH *)
        ELSE
            (* not a function *)
            IF fl.ostd = 0 THEN
                (* no unresolved formal types names *)
                newM.FixSigCode();               (* fix the sigcode of newM *)
                newM := rec.AddMethod(newM);
            ELSE
                (* need to AddMethod after formal type names resolved *)
            END; (* IF *)
        END; (* WITH *)
    ELSE
        newM.FixSigCode();               (* fix the sigcode of newM *)
        newM := rec.AddMethod(newM);
        WITH newM: MS.Function DO
            WITH rtype: MS.TempType DO
                (* collect reference if TempType/NamedType *)
                rtype.AddReferenceFunction(newM);
            ELSE
            END; (* WITH *)
        ELSE
        END; (* IF *)
    END; (* IF *)
END FixProcTypes;


PROCEDURE (rd: Reader) ParseMethod(rec: MS.RecordType), NEW;
VAR
    newM: MS.Method;
    newF: MS.Method;
    mAtt: SET;
    vMod: INTEGER;
    rFrm: INTEGER;
    fl: MS.FormalList;
    rtype: MS.Type;
    rectyp: MS.Type;
    mname: CharOpen;
    ovlname: CharOpen;
BEGIN
    NEW(newM);
    mname := ST.ToChrOpen(rd.sAtt);
    vMod := rd.iAtt;
    (* byte1 is the method attributes *)
    mAtt := BITS(rd.Read());
    (* byte2 is param form of receiver *)
    rFrm := rd.Read();
    (* next 1 or 2 bytes are rcv-type  *)
    rectyp := rd.TypeOf(rd.ReadOrd());
    rd.GetSym();
    ovlname := NIL;

    IF ~MS.WithoutMethodNameMangling() THEN
        IF rd.sSym = strSy THEN
            (* optional invoking method name *)
            ovlname := mname;
            mname := ST.ToChrOpen(rd.sAtt);
            rd.GetSym();
        END; (* IF *)
    END; (* IF *)

    rtype := NIL;
    IF rd.sSym = retSy THEN
        rtype := rd.TypeOf(rd.iAtt);
        rd.GetSym();
    END; (* IF *)
    fl := rd.GetFormalTypes();

    newM := rec.MakeMethod(mname, MS.Mnonstatic, rtype, fl);
    IF (rectyp # NIL) & (rectyp # rec) THEN newM.SetDeclaringType(rectyp); END;

    IF MS.WithoutMethodNameMangling() THEN
    ELSE
        IF ovlname # NIL THEN
            newM.SetOverload(ovlname);    (* fix the sigcode of newM *)
        ELSE
        END;
    END; (* IF *)

    newM.SetVisibility(vMod);
    newM.InclAttributes(mAtt);
    FixProcTypes(rec, newM, fl, rtype);
END ParseMethod;


PROCEDURE (rd: Reader) ParseProcedure(rec: MS.RecordType), NEW;
VAR
    newP: MS.Method;
    newF: MS.Method;
    vMod: INTEGER;
    rFrm: INTEGER;
    fl: MS.FormalList;
    rtype: MS.Type;
    rectyp: MS.Type;
    pname: CharOpen;
    ivkname: CharOpen;
    ovlname: CharOpen;
    isCtor: BOOLEAN;
    idx: INTEGER;
BEGIN
    NEW(newP);
    pname := ST.ToChrOpen(rd.sAtt);
    vMod := rd.iAtt;

    rd.ReadPast(namSy);
    ivkname := NIL; ovlname := NIL; isCtor := FALSE;

    IF rd.sSym = strSy THEN
        (* optional string of invoke name if overloaded method OR Constructor *)
        ivkname := ST.ToChrOpen(rd.sAtt);
        rd.GetSym();

        IF rd.sSym = truSy THEN
            (* optional truSy shows that procedure is a constructor *)
            isCtor := TRUE;
            IF LEN(pname) > LEN(MS.replCtor) THEN
                (* overload constructor name is in the form of "init_..." *)
                ovlname := pname;
                idx := ST.StrChr(ovlname,'_');
                IF idx # ST.NotExist THEN
                    pname := ST.SubStr(ovlname, 0, idx-1);
                ELSE
                    ASSERT(FALSE);
                END; (* IF *)
            ELSE
                (* constructor is not overloaded *)
            END; (* IF *)
            rd.GetSym();
        ELSE
            (* not a constructor *)
            ovlname := pname;
            pname := ivkname;
        END; (* IF *)
    END; (* IF *)

    rtype := NIL;
    IF rd.sSym = retSy THEN
        rtype := rd.TypeOf(rd.iAtt);
        rd.GetSym();
    END; (* IF *)
    fl := rd.GetFormalTypes();

    newP := rec.MakeMethod(pname, MS.Mstatic, rtype, fl);
    IF isCtor THEN
        newP.SetConstructor();
        newP.SetInvokeName(ivkname);
    END; (* IF *)

    IF MS.WithoutMethodNameMangling() THEN
    ELSE
        IF ovlname # NIL THEN
            newP.SetOverload(ovlname);    (* fix the sigcode of newM *)
        END;
    END; (* IF *)

    newP.SetVisibility(vMod);
    FixProcTypes(rec, newP, fl, rtype);
END ParseProcedure;


PROCEDURE (rd: Reader) ParseRecordField(rec: MS.RecordType), NEW;
VAR
    fldname: CharOpen;
    fvmod: INTEGER;
    ftyp: MS.Type;
    fld: MS.Field;
BEGIN
    fldname := ST.ToChrOpen(rd.sAtt);
    fvmod := rd.iAtt;
    ftyp := rd.TypeOf(rd.ReadOrd());

    WITH ftyp: MS.NamedType DO
        fld := rec(MS.ValueType).MakeField(fldname, ftyp, FALSE);
    | ftyp: MS.TempType DO
        fld := rec(MS.ValueType).MakeField(fldname, MS.dmyTyp, FALSE);
        (* collect reference if TempType/NamedType *)
        ftyp.AddReferenceField(fld);
    ELSE
        fld := rec(MS.ValueType).MakeField(fldname, ftyp, FALSE);
    END; (* WITH *)

    fld.SetVisibility(fvmod);
    WITH rec: MS.PrimType DO            (* for IntPtr and UIntPtr, otherwise StrucType *)
        ASSERT(rec.AddField(fld, FALSE));
    ELSE  (* IntfcType should not has data member *)
        ASSERT(FALSE);
    END; (* WITH *)
END ParseRecordField;


PROCEDURE (rd: Reader) ParseStaticVariable(rec: MS.RecordType), NEW;
(* Variable   = varSy Name TypeOrd.             *)
VAR
    varname: CharOpen;
    vvmod: INTEGER;
    vtyp: MS.Type;
    newV : MS.Field;
BEGIN
    varname := ST.ToChrOpen(rd.sAtt);
    vvmod := rd.iAtt;
    vtyp := rd.TypeOf(rd.ReadOrd());

    WITH vtyp: MS.NamedType DO
        newV := rec(MS.ValueType).MakeField(varname, vtyp, FALSE);
    | vtyp: MS.TempType DO
        newV := rec(MS.ValueType).MakeField(varname, MS.dmyTyp, FALSE);
        (* collect reference if TempType/NamedType *)
        vtyp.AddReferenceField(newV);
    ELSE
        newV := rec(MS.ValueType).MakeField(varname, vtyp, FALSE);
    END; (* WITH *)

    newV.SetVisibility(vvmod);
    WITH rec: MS.PrimType DO            (* for IntPtr and UIntPtr, otherwise StrucType *)
        ASSERT(rec.AddField(newV, TRUE));
    ELSE  (* IntfcType should not has data member *)
        ASSERT(FALSE);
    END; (* WITH *)
    rd.GetSym();
END ParseStaticVariable;


PROCEDURE (rd: Reader) ParseConstant(rec: MS.RecordType), NEW;
(* Constant   = conSy Name Literal.             *)
(* Assert: f.sSym = namSy.                      *)
VAR
    cname: CharOpen;
    cvmod: INTEGER;
    ctyp: MS.Type;
    cvalue: MS.Literal;
    newC : MS.Field;
    tord: INTEGER;
BEGIN
    cname := ST.ToChrOpen(rd.sAtt);
    cvmod := rd.iAtt;
    rd.ReadPast(namSy);
    cvalue := rd.GetLiteral();

    IF cvalue IS MS.BoolLiteral THEN
        tord := MS.boolN;
    ELSIF cvalue IS MS.LIntLiteral THEN
        tord := MS.lIntN;
    ELSIF cvalue IS MS.CharLiteral THEN
        tord := MS.charN;
    ELSIF cvalue IS MS.RealLiteral THEN
        tord := MS.realN;
    ELSIF cvalue IS MS.SetLiteral THEN
        tord := MS.setN;
    ELSIF cvalue IS MS.StrLiteral THEN
        tord := MS.strN;
    ELSE
        tord := MS.unCertain;
    END; (* IF *)
    ctyp := MS.baseTypeArray[tord];
    IF ctyp = NIL THEN
        ctyp := MS.MakeDummyPrimitive(tord);
    END; (* IF *)
    newC := rec(MS.ValueType).MakeConstant(cname, ctyp, cvalue);

    newC.SetVisibility(cvmod);
    WITH rec: MS.ValueType DO
        ASSERT(rec.AddField(newC, TRUE));
    ELSE  (* IntfcType should not has data member *)
        ASSERT(FALSE);
    END; (* WITH *)

END ParseConstant;


PROCEDURE (rd: Reader) ParsePointerType(old: MS.Type): MS.Type, NEW;
VAR
    indx: INTEGER;
    rslt: MS.PointerType;
    junk: MS.Type;
    ns: MS.Namespace;
    tname: CharOpen;
    ftname: CharOpen;
    target: MS.Type;
BEGIN
    (* read the target type ordinal *)
    indx := rd.ReadOrd();
    WITH old: MS.PointerType DO
        rslt := old;
        (*
         *  Check if there is space in the tArray for this
         *  element, otherwise expand using typeOf().
         *)
        IF indx - tOffset >= rd.tArray.tide THEN
            junk := rd.TypeOf(indx);
        END; (* IF *)
        rd.tArray.a[indx-tOffset] := rslt.GetTarget();
    | old: MS.TempType DO
        ns := old.GetNamespace();
	IF ns = NIL THEN
	    (* it is an anonymous pointer to array type *)
            old.SetAnonymous();
            target := rd.TypeOf(indx);
            rslt := MS.MakeAnonymousPointerType(target);
	ELSE
            tname := old.GetName();
            ftname := old.GetFullName();
            target := rd.TypeOf(indx);
            target.SetNamespace(ns);        (* the the default namespace of the target *)
            rslt := ns.InsertPointer(tname,ftname,target);
            rslt.SetVisibility(old.GetVisibility());
        END; (* IF *)

        (* changed from TempType to PointerType, so fix all references to the type *)
        MS.FixReferences(old, rslt);

        IF target.GetName() = NIL THEN
            target.SetAnonymous();
            target.SetVisibility(MS.Vprivate);
            (* collect reference if TempType/NamedType *)
            target(MS.TempType).AddSrcPointerType(rslt);                (* <== should that be for all TempType target?? *)
        ELSE
        END; (* IF *)
    ELSE
        ASSERT(FALSE); rslt := NIL;
    END; (* WITH *)
    rd.GetSym();
    RETURN rslt;
END ParsePointerType;


PROCEDURE (rd: Reader) ParseArrayType(tpTemp: MS.Type): MS.Type, NEW;
VAR
    rslt: MS.Type;
    ns: MS.Namespace;
    elemTp: MS.Type;
    length: INTEGER;
    tname: CharOpen;
    ftname: CharOpen;
    sptr: MS.PointerType;
    sptrname: CharOpen;
    typOrd: INTEGER;
BEGIN
    typOrd := rd.ReadOrd();
    elemTp := rd.TypeOf(typOrd);
    ns := tpTemp.GetNamespace();
    IF ns = NIL THEN
        (* its name (currently "DummyType") can only be fixed after its element type is determined *)
        tpTemp.SetAnonymous();
        IF typOrd < tOffset THEN
            (* element type is primitive, and was already create by TypeOf() calling MakeDummyPrimitive() *)
            tname := elemTp.GetName();
            tname := ST.StrCat(tname, MS.anonArr);             (* append "_arr" *)
            ns := elemTp.GetNamespace();                       (* []SYSTEM  - for dummy primitives *)
            ftname := ST.StrCatChr(ns.GetName(), '.');
            ftname := ST.StrCat(ftname, tname);
        ELSE
            ns := elemTp.GetNamespace();
            IF ns # NIL THEN
                (* the anonymous array element is already known *)
                tname := elemTp.GetName();
                tname := ST.StrCat(tname, MS.anonArr);             (* append "_arr" *)
                ftname := ST.StrCatChr(ns.GetName(), '.');
                ftname := ST.StrCat(ftname, tname);
            ELSE
                (* cannot insert this type as its element type is still unknown, and so is its namespace ??? *)
                tname := ST.NullString;
                ftname := tname;
            END; (* IF *)
        END; (* IF *)
    ELSE
        IF ~tpTemp.IsAnonymous() THEN
            tname := tpTemp.GetName();
            ftname := tpTemp.GetFullName();
        ELSE
            (* if array is anonymous and has namespace, 
               then either its element type has been parsed (ARRAY OF ParsedElement), 
                        or it has a src pointer type (Arr1AnonymousArray = POINTER TO ARRAY OF something) *)
            tname := elemTp.GetName();
            IF tname # NIL THEN
                tname := ST.StrCat(tname, MS.anonArr);             (* append "_arr" *)
            ELSE
                sptr := tpTemp(MS.TempType).GetNonAnonymousPTCrossRef();
                sptrname := sptr.GetName();
                tname := ST.SubStr(sptrname, 4, LEN(sptrname)-1);  (* get rid of "Arr1" *)
                tname := ST.StrCat(tname, MS.anonArr);             (* append "_arr" *)
            END; (* IF *)
            ftname := ST.StrCatChr(ns.GetName(), '.');
            ftname := ST.StrCat(ftname, tname);
        END; (* IF *)
    END; (* IF *)
    rd.GetSym();
    IF rd.sSym = bytSy THEN
        length := rd.iAtt;
        rd.GetSym();
    ELSIF rd.sSym = numSy THEN
        length := SHORT(rd.lAtt);
        rd.GetSym();
    ELSE
        length := 0;
    END; (* IF *)

    IF ns # NIL THEN
        rslt := ns.InsertArray(tname, ftname, 1, length, elemTp);
        rslt.SetVisibility(tpTemp.GetVisibility());

        (* changed from TempType to ArrayType, so fix all references to the type *)
        MS.FixReferences(tpTemp, rslt);

        IF tpTemp.IsAnonymous() THEN
            rslt.SetAnonymous();
        ELSE
            rslt.NotAnonymous();
        END; (* IF *)
    ELSE
        (* add this to defer anonymous array insertion list*)
        tpTemp(MS.TempType).SetDimension(1);
        tpTemp(MS.TempType).SetLength(length);
        elemTp(MS.TempType).AddAnonymousArrayType(tpTemp(MS.TempType));
        rslt := tpTemp;
    END; (* IF *)

    rd.ReadPast(endAr);
    RETURN rslt;
END ParseArrayType;


PROCEDURE (rd: Reader) ParseRecordType(old: MS.Type; typIdx: INTEGER): MS.RecordType, NEW;
(* Assert: at entry the current symbol is recSy.                        *)
(* Record     = TypeHeader recSy recAtt [truSy | falSy | <others>]      *)
(*      [basSy TypeOrd] [iFcSy {basSy TypeOrd}]                         *)
(*      {Name TypeOrd} {Method} {Statics} endRc.                        *)
VAR
    rslt: MS.RecordType;
    recAtt: INTEGER;
    oldS: INTEGER;
    fldD: MS.Field;
    mthD: MS.Method;
    conD: MS.Constant;
    isValueType: BOOLEAN; (* is ValueType *)
    hasNarg: BOOLEAN;     (* has noarg constructor ( can use NEW() ) *)
    ns: MS.Namespace;
    tname: CharOpen;
    ftname: CharOpen;
    attr: MS.Attribute;
    tt: INTEGER;
    sptr: MS.PointerType;
    base: MS.Type;
    itfc: MS.Type;
    tord: INTEGER;    (* temporary type storage *)
    ttyp: MS.Type;    (* temporary type storage *)
    fldname: CharOpen;
    fvmod: INTEGER;
BEGIN
    WITH old: MS.RecordType DO
        rslt := old;
        recAtt := rd.Read();    (* record attribute *)                          (* <==== *)
        rd.GetSym();            (* falSy *)
        rd.GetSym();            (* optional basSy *)
        IF rd.sSym = basSy THEN rd.GetSym() END;
    | old: MS.TempType DO

        ns := old.GetNamespace();
        IF ~old.IsAnonymous() THEN
            tname := old.GetName();
            ftname := old.GetFullName();
        ELSE
            (* if record is anonymous, it has only one src pointer type *)
            sptr := old(MS.TempType).GetFirstPTCrossRef();
            tname := ST.StrCat(sptr.GetName(), MS.anonRec);
            ftname := ST.StrCatChr(ns.GetName(), '.');
            ftname := ST.StrCat(ftname, tname);
        END; (* IF *)

        recAtt := rd.Read();                                                    (* <==== *)
        (* check for ValueType *)
        IF recAtt >= valTp THEN
            isValueType := TRUE; recAtt := recAtt MOD valTp;
        ELSE
            isValueType := FALSE;
        END; (* IF *)

        (* check for no NOARG constructor *)
        IF recAtt >= nnarg THEN
            hasNarg := FALSE; recAtt := recAtt MOD nnarg;
        ELSE
            hasNarg := TRUE;
        END; (* IF *)

        (* Record default to Struct, change to Class if found to be ClassType later (when it has event?) *)
        tt := MS.Struct;
        IF recAtt = iFace THEN tt := MS.Interface; END;

        rd.GetSym();
        IF rd.sSym = falSy THEN
        ELSIF rd.sSym = truSy THEN
        END; (* IF *)

        rslt := ns.InsertRecord(tname, ftname, tt);
        rslt.SetVisibility(old.GetVisibility());

        IF isValueType THEN rslt.InclAttributes(MS.RvalTp); END;
        IF hasNarg THEN rslt.SetHasNoArgConstructor(); END;

        CASE recAtt OF
          abstr : rslt.InclAttributes(MS.Rabstr);
        | limit : (* foreign has no LIMITED attribute *)
        | extns : rslt.InclAttributes(MS.Rextns);
        ELSE
            (* noAtt *)
        END; (* CASE *)

        rd.GetSym();
        IF rd.sSym = basSy THEN
            base := rd.TypeOf(rd.iAtt);
            WITH base: MS.NamedType DO
                rslt.SetBaseType(base);
            | base: MS.TempType DO
                (* base is a temp type *)
                (* collect reference if TempType/NamedType *)
                base(MS.TempType).AddDeriveRecordType(rslt);
            ELSE
                (* base has already been parsed *)
                rslt.SetBaseType(base);
            END; (* WITH *)
            rd.GetSym();
        END; (* IF *)

        IF rd.sSym = iFcSy THEN
            rd.GetSym();
            WHILE rd.sSym = basSy DO

                itfc := rd.TypeOf(rd.iAtt);
                WITH itfc: MS.NamedType DO
                    (* add to interface list of rslt *)
                     rslt.AddInterface(itfc);
                | itfc: MS.TempType DO
                    (* itfc is a temp type *)
                    (* collect reference *)
                    itfc(MS.TempType).AddImplRecordType(rslt);
                ELSE
                    (* itfc has already been parsed *)
                    (* add to interface list of rslt *)
                     rslt.AddInterface(itfc);
                END; (* WITH *)
                rd.GetSym();
            END; (* WHILE *)
        END; (* IF *)

        (* changed from TempType to RecordType, so fix all references to the type *)
        MS.FixReferences(old, rslt);
        (* need to be here as its methods, fields, etc. may reference to this new type *)
        rd.tArray.a[typIdx] := rslt;
    ELSE
        ASSERT(FALSE); rslt := NIL;
    END; (* WITH *)

    WHILE rd.sSym = namSy DO
        (* check for record fields *)
        rd.ParseRecordField(rslt);
        rd.GetSym();
        (* insert the field to the record's field list *)
    END; (* WHILE *)

    WHILE (rd.sSym = mthSy) OR (rd.sSym = prcSy) OR
          (rd.sSym = varSy) OR (rd.sSym = conSy) DO
        oldS := rd.sSym; rd.GetSym();
        IF oldS = mthSy THEN
            rd.ParseMethod(rslt);
        ELSIF oldS = prcSy THEN
            rd.ParseProcedure(rslt);
        ELSIF oldS = varSy THEN
            rd.ParseStaticVariable(rslt);
        ELSIF oldS = conSy THEN
            rd.ParseConstant(rslt);
        ELSE
            rd.Abandon();
        END; (* IF *)
    END; (* WHILE *)
    rd.ReadPast(endRc);
    RETURN rslt;
END ParseRecordType;


PROCEDURE (rd: Reader) ParseEnumType(tpTemp: MS.Type): MS.Type, NEW;
VAR
    rslt: MS.EnumType;
    const: MS.Constant;
    ns: MS.Namespace;
    tname: CharOpen;
    ftname: CharOpen;
BEGIN
    rslt := NIL;
    ns := tpTemp.GetNamespace();
    tname := tpTemp.GetName();
    ftname := tpTemp.GetFullName();
    rslt := ns.InsertRecord(tname, ftname, MS.Enum)(MS.EnumType);
    rslt.SetVisibility(tpTemp.GetVisibility());

    (* changed from TempType to EnumType, so fix all references to the type *)
    MS.FixReferences(tpTemp, rslt);

    rd.GetSym();
    WHILE rd.sSym = conSy DO
        rd.GetSym();
        rd.ParseConstant(rslt);
    END; (* WHILE *)
    rd.ReadPast(endRc);
    RETURN rslt;
END ParseEnumType;


PROCEDURE (rd: Reader) ParseDelegType(old: MS.Type; isMul: BOOLEAN): MS.Type, NEW;
VAR
    rslt: MS.PointerType;
    ns: MS.Namespace;
    tname: CharOpen;
    ftname: CharOpen;
    ttname: CharOpen;
    tftname: CharOpen;
    target: MS.RecordType;
    rtype: MS.Type;
    fl: MS.FormalList;
    newM: MS.Method;
    newF: MS.Method;
BEGIN
    (* create the pointer *)
    WITH old: MS.PointerType DO
        rslt := old;
    | old: MS.TempType DO
        ns := old.GetNamespace();

        (* pointer name *)
        tname := old.GetName();
        ftname := old.GetFullName();

        (* target name *)
        ttname := ST.StrCat(tname, MS.anonRec);
        tftname := ST.StrCatChr(ns.GetName(), '.');
        tftname := ST.StrCat(tftname, ttname);

        (* create the target record *)
        target := ns.InsertRecord(ttname, tftname, MS.Delegate);
        target.SetNamespace(ns);        (* the the default namespace of the target *)
        target.SetAnonymous();
        IF isMul THEN target.SetMulticast() END;

        (* target visibility *)
        target.SetVisibility(MS.Vprivate);
        (* Delegate is not value type *)
        (* Delegate has no noarg constructor *)
        (* Delegate is neither abstract, nor extensible *)
        (* lost information on base type of Delegate *)
        (* lost information on interface implemented by Delegate *)

        rslt := ns.InsertPointer(tname,ftname,target);
        rslt.SetVisibility(old.GetVisibility());

        (* changed from TempType to PointerType, so fix all references to the type *)
        MS.FixReferences(old, rslt);

        (* the "Invoke" method of delegate *)
        rd.GetSym();

        rtype := NIL;
        IF rd.sSym = retSy THEN
            rtype := rd.TypeOf(rd.iAtt);
            rd.GetSym();
        END; (* IF *)

        fl := rd.GetFormalTypes();

        newM := target.MakeMethod(ST.ToChrOpen("Invoke"), MS.Mnonstatic, rtype, fl);
        newM.SetVisibility(MS.Vpublic);                 (* "Invoke" method has Public visiblilty *)

        (* "Invoke" method has final {} attribute (or should it has NEW attribute) *) 
        (* newM.InclAttributes(MS.Mnew); *)

        FixProcTypes(target, newM, fl, rtype);
    ELSE
        ASSERT(FALSE); rslt := NIL;
    END; (* WITH *)

    RETURN rslt;
END ParseDelegType;


PROCEDURE (rd: Reader) ParseTypeList*(), NEW;
(* TypeList   = start { Array | Record | Pointer        *)
(*                | ProcType } close.                   *)
(* TypeHeader = tDefS Ord [fromS Ord Name].             *)
VAR
    typOrd: INTEGER;
    typIdx: INTEGER;
    tpTemp : MS.Type;
    modOrd : INTEGER;
    impMod : MS.Namespace;
    tpDesc : MS.Type;
BEGIN
    impMod := NIL;
    WHILE rd.sSym = tDefS DO
        (* Do type header *)
        typOrd := rd.iAtt;
        typIdx := typOrd - tOffset;
        tpTemp := rd.tArray.a[typIdx];

        rd.ReadPast(tDefS);
        (* The fromS symbol appears if the type is imported *)
        IF rd.sSym = fromS THEN
            modOrd := rd.iAtt;
            impMod := rd.sArray.a[modOrd-1];
            rd.GetSym();
            (* With the strict ordering of the imports,
             * it may be unnecessary to create this object
             * in case the other module has been fully read
             * already?
             * It is also possible that the type has 
             * been imported already, but just as an opaque.
             *)
            tpTemp.SetNamespace(impMod);
            rd.ReadPast(namSy);


            IF tpTemp.GetName() = NIL THEN
                tpTemp.SetName(ST.ToChrOpen(rd.sAtt));
            ELSE
            END; (* IF *)
            tpTemp.SetFullName(ST.StrCat(ST.StrCatChr(impMod.GetName(),'.'), tpTemp.GetName()));
        END; (* IF *)

        (* GetTypeinfo *)
        CASE rd.sSym OF 
        | arrSy :
                  tpDesc := rd.ParseArrayType(tpTemp);
                  rd.tArray.a[typIdx] := tpDesc;
        | recSy :
                  tpDesc := rd.ParseRecordType(tpTemp, typIdx);
                  rd.tArray.a[typIdx] := tpDesc;
        | ptrSy :
                  tpDesc := rd.ParsePointerType(tpTemp);
                  rd.tArray.a[typIdx] := tpDesc;
        | evtSy :
                  tpDesc := rd.ParseDelegType(tpTemp, TRUE);
                  rd.tArray.a[typIdx] := tpDesc;
        | pTpSy :
                  tpDesc := rd.ParseDelegType(tpTemp, FALSE);
                  rd.tArray.a[typIdx] := tpDesc;
        | eTpSy :
                  tpDesc := rd.ParseEnumType(tpTemp);
                  rd.tArray.a[typIdx] := tpDesc;
        ELSE 
            (* NamedTypes come here *)
            IF impMod = NIL THEN impMod := rd.fns; END;
            (* the outcome could be a PointerType, ArrayType or RecordType if it already exist *)
            tpDesc := impMod.InsertNamedType(tpTemp.GetName(), tpTemp.GetFullName());
            rd.tArray.a[typIdx] := tpDesc;
            (* changed from TempType to NamedType, so fix all references to the type *)
            MS.FixReferences(tpTemp, tpDesc);
        END; (* CASE *)
    END; (* WHILE *)
    rd.ReadPast(close);
END ParseTypeList;


PROCEDURE (rd: Reader) InsertMainClass(): MS.PointerType, NEW;
VAR
    tname  : ST.CharOpen;
    tgtname: ST.CharOpen;
    target : MS.RecordType;
    rslt   : MS.PointerType;
    base   : MS.Type;
    ns     : MS.Namespace;
    asb    : MS.Assembly;
BEGIN
    ASSERT(ST.StrCmp(rd.fasb.GetName(), rd.fns.GetName()) = ST.Equal);
    tname := rd.fns.GetName();
    tgtname := ST.StrCat(tname, MS.anonRec);
    target := rd.fns.InsertRecord(tgtname, tgtname, MS.Struct);
    target.SetVisibility(MS.Vpublic);
    target.SetHasNoArgConstructor();
    base := MS.GetTypeByName(ST.ToChrOpen("mscorlib"),ST.ToChrOpen("System"),ST.ToChrOpen("Object"));
    ASSERT(base # NIL);              (* mscorlib_System.Object should always exist *)
    target.SetBaseType(base);
    rslt := rd.fns.InsertPointer(tname,tname,target);
    rslt.SetVisibility(MS.Vpublic);
    RETURN rslt;
END InsertMainClass;


PROCEDURE ParseSymbolFile*(symfile: GF.FILE; modname: CharOpen);
VAR
    rd: Reader;
    oldS: INTEGER;
    class: MS.PointerType;
    rec: MS.Type;
BEGIN
    rec := NIL;
    rd := NewReader(symfile);
    rd.GetHeader(modname);
    IF rd.sSym = numSy THEN rd.GetVersionName(); END; (* optional strong name info. *)
    LOOP
        oldS := rd.sSym;
        rd.GetSym();
        CASE oldS OF
        | start : EXIT;
        | impSy : rd.Import();
        | typSy : rd.ParseType();
        | conSy : (* a global variable belongs to an GPCP module, e.g. ["[GPFiles]GPFiles"] *)
                  IF rec = NIL THEN
                      class := rd.InsertMainClass();
                      rec := class.GetTarget();
                  END; (* IF *)
                  WITH rec: MS.RecordType DO
                      rd.ParseConstant(rec);
                  ELSE
                      ASSERT(FALSE);
                  END; (* WITH *)
        | prcSy : (* a global variable belongs to an GPCP module, e.g. ["[GPFiles]GPFiles"] *)
                  IF rec = NIL THEN
                      class := rd.InsertMainClass();
                      rec := class.GetTarget();
                  END; (* IF *)
                  WITH rec: MS.RecordType DO
                      rd.ParseProcedure(rec);
                  ELSE
                      ASSERT(FALSE);
                  END; (* WITH *)
        | varSy : (* a global variable belongs to an GPCP module, e.g. ["[GPFiles]GPFiles"] *)
                  IF rec = NIL THEN
                      class := rd.InsertMainClass();
                      rec := class.GetTarget();
                  END; (* IF *)
                  WITH rec: MS.RecordType DO
                      rd.ParseStaticVariable(rec);
                  ELSE
                      ASSERT(FALSE);
                  END; (* WITH *)
        ELSE
            RTS.Throw("Bad object");
        END; (* CASE *)
    END; (* LOOP *)
    rd.ParseTypeList();
    IF rd.sSym # keySy THEN RTS.Throw("Missing keySy"); END;
END ParseSymbolFile;


END SymReader.

