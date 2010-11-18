MODULE SymWriter;
(* ========================================================================= *)
(*                                                                           *)
(*  Symbol file writing module for the .NET to Gardens Point Component       *)
(*  Pascal Symbols tool.                                                     *)
(*      Copyright (c) Siu-Yuen Chan 2001.                                    *)
(*                                                                           *)
(*  This module converts all meta information inside METASTORE (defined by   *)
(*  MetaStore module) into Gardens Point Component Pascal (GPCP) recognized  *)
(*  symbols, then writes the symbols to files in GPCP symbol file format.    *)
(* ========================================================================= *)

IMPORT
    ST := AscString,
    Error,
    RTS,
    MS := MetaStore,
    GF := GPBinFiles;

CONST
    SymbolExt* = ".cps";

CONST
    (* ModulesName Types *)
    (* assembly name same as namespace name, and contains only one word,
       e.g. Accessibility.dll has only a namespace named Accessibility,
            and the module name should be:
                Accessibility_["[Accessibility]Accessibility"] *)
    SingleWord = 0;     

    (* assembly name same as namespace name, and contains multiple word,
       e.g. Microsoft.Win32.InterOp.dll has a namespace named Microsoft.Win32.InterOp,
            and the module name shoulle be:
                Microsoft_Win32_InterOp_["[Microsoft.Win32.InterOp]Microsoft.Win32.InterOp"] *)
    MultipleWord = 1;

    (* assembly name different form namespace name, contains multiple word, and 
       with namespace name includes the entire assembly name
       e.g. Microsoft.Win32.InterOp.dll has a namespace named Microsoft.Win32.InterOp.Trident,
            and the module name shoulle be:
                Microsoft_Win32_InterOp__Trident["[Microsoft.Win32.InterOp]Microsoft.Win32.InterOp.Trident"] *)
    IncludeWord = 3;

    (* assembly name different from namespace name, contains multiple word, and
       with no relationship between assembly name and namespace name
       e.g. mscorlib.dll has a namespace named System.Reflection,
            and the module name should be:
                mscorlib_System_Reflection["[mscorlib]System.Reflection"] *)
    DifferentWord = 2;
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
    magic   = 0DEADD0D0H;
    syMag   = 0D0D0DEADH;
    MAXMODULE = 64;
    MAXTYPE = 256;

CONST
    tOffset*  = 16; (* backward compatibility with JavaVersion *)

CONST (* mode-kinds *)(* should follow exactly as defined in Symbol.cp *)
      (* used in describing Type *)
    prvMode = MS.Vprivate;
    pubMode = MS.Vpublic;
    rdoMode = MS.Vreadonly;
    protect = MS.Vprotected;

CONST (* base-ordinals *)
    notBs   = MS.notBs;
    boolN*  = MS.boolN;      (* BOOLEAN *)
    sChrN*  = MS.sChrN;      (* SHORTCHAR *)
    charN*  = MS.charN;      (* CHAR *)
    uBytN*  = MS.uBytN;      (* UBYTE *)
    byteN*  = MS.byteN;      (* BYTE *)
    sIntN*  = MS.sIntN;      (* SHORTINT *)
    intN*   = MS.intN;       (* INTEGER *)
    lIntN*  = MS.lIntN;      (* LONGING *)
    sReaN*  = MS.sReaN;      (* SHORTREAL *)
    realN*  = MS.realN;      (* REAL *)
    setN*   = MS.setN;       (* SET *)
    anyRec* = MS.anyRec;     (* ANYREC *)
    anyPtr* = MS.anyPtr;     (* ANYPTR *)
    strN*   = MS.strN;       (* STRING (ARRAY OF CHAR) *)
    sStrN*  = MS.sStrN;      (* SHORTSTRING (ARRAY OF SHORTCHAR) *)
    metaN*  = MS.metaN;      (* META *)

CONST (* record attributes *)
    noAtt* = ORD(MS.noAtt);  (* no attribute *)
    abstr* = ORD(MS.Rabstr); (* Is ABSTRACT *)
    limit* = ORD(MS.Rlimit); (* Is LIMIT *)
    extns* = ORD(MS.Rextns); (* Is EXTENSIBLE *)
    iFace* = ORD(MS.RiFace); (* Is INTERFACE *)
    nnarg* = ORD(MS.Rnnarg); (* Has NO NoArg Constructor ( cannot use NEW() ) *)
    valTp* = ORD(MS.RvalTp); (* ValueType *)

CONST (* method attributes *)
    newBit* = 0;
    final*  = MS.Mfinal;
    isNew*  = MS.Mnew;
    isAbs*  = MS.Mabstr; 
    empty*  = MS.Mempty;
    isExt*  = MS.MisExt;
    mask*   = MS.Mmask;
    covar*  = MS.Mcovar;     (* ==> covariant return type *)

CONST (* param-modes *)
    val*    = MS.IsVal;      (* value parameter *)
    in*     = MS.IsIn;       (* IN parameter *)
    out*    = MS.IsOut;      (* OUT parameter *)
    var*    = MS.IsVar;      (* VAR parameter *)
    notPar* = MS.NotPar;

TYPE
(*
    CharOpen = POINTER TO ARRAY OF CHAR;
*)
    CharOpen = ST.CharOpen;

    TypeSeq = POINTER TO
        RECORD 
            tide: INTEGER;
            high: INTEGER;
            a: POINTER TO ARRAY OF MS.Type;
        END;

    ModuleSeq = POINTER TO
        RECORD 
            tide: INTEGER;
            high: INTEGER;
            a: POINTER TO ARRAY OF MS.Namespace;
        END;

    Emiter = POINTER TO
        RECORD
            asbname: CharOpen;
            asbfile: CharOpen;
            nsname: CharOpen;
            modname: CharOpen;
            version: MS.Version;
            token: MS.PublicKeyToken;
            ns: MS.Namespace;
            mnameKind: INTEGER;
            maintyp: MS.Type;
            file: GF.FILE;
            cSum: INTEGER;
            iNxt: INTEGER;    (* next IMPORT Ord *)
            oNxt: INTEGER;    (* next TypeOrd *)
            work: TypeSeq;
            impo: ModuleSeq;
         END;

VAR
    PreEmit: BOOLEAN;


PROCEDURE ^ (et: Emiter) EmitDelegate(t: MS.DelegType), NEW;


PROCEDURE MakeTypeName(typ: MS.Type): CharOpen;
(* for handling the '+' sign inside the Beta2 nested type name *)
VAR
    name: CharOpen;
    idx: INTEGER;
BEGIN
    name := typ.GetName();
    IF typ.IsNested() THEN
        idx := ST.StrChr(name, '+');
        IF idx # ST.NotExist THEN
            name[idx] := '$';
        END; (* IF *)
        ASSERT(ST.StrChr(name, '+') = ST.NotExist);
    ELSE
    END; (* IF *)
    RETURN name;
END MakeTypeName;


PROCEDURE (et: Emiter) MakeFullTypeName(typ: MS.Type): CharOpen, NEW;
VAR
    tnsname: CharOpen;
    tasbname: CharOpen;
    tmodname: CharOpen;
    tname: CharOpen;
    dim: INTEGER;
    elm: MS.Type;
BEGIN
    tnsname := typ.GetNamespaceName();
    tasbname := typ.GetAssemblyName();
    IF (tnsname^ = et.nsname^) & (tasbname^ = et.asbname^) THEN
        (* local type *)
        tname := MakeTypeName(typ);
    ELSE
        (* foreign type *)
        tmodname := MS.MakeModuleName(tasbname, tnsname);
        tmodname := ST.StrCatChr(tmodname, '.');
        tname := ST.StrCat(tmodname, MakeTypeName(typ));
    END; (* IF *)
    RETURN tname;
END MakeFullTypeName;


PROCEDURE InitTypeSeq(seq: TypeSeq; capacity : INTEGER); 
BEGIN
    NEW(seq.a, capacity);
    seq.high := capacity-1;
    seq.tide := 0;
END InitTypeSeq;


PROCEDURE InitModuleSeq(seq: ModuleSeq; capacity : INTEGER); 
BEGIN
    NEW(seq.a, capacity);
    seq.high := capacity-1;
    seq.tide := 0;
END InitModuleSeq;


PROCEDURE ResetTypeSeq(VAR seq : TypeSeq);
VAR
    i: INTEGER;
    type: MS.Type;
BEGIN
    IF seq.a = NIL THEN
        InitTypeSeq(seq, 2);
    ELSE
        FOR i := 0 TO seq.tide-1 DO
            type := seq.a[i]; seq.a[i] := NIL;
            type.ClearTypeOrd();
            type.ClearInHierarchy();
        END; (* FOR *)
        seq.tide := 0; 
    END; (* IF *)
END ResetTypeSeq;


PROCEDURE ResetModuleSeq(VAR seq : ModuleSeq);
VAR
    i: INTEGER;
    ns: MS.Namespace;
BEGIN
    IF seq.a = NIL THEN
        InitModuleSeq(seq, 2);
    ELSE
        FOR i := 0 TO seq.tide-1 DO
            ns := seq.a[i]; seq.a[i] := NIL;
            ns.ClearModuleOrd();
        END; (* FOR *)
        seq.tide := 0; 
    END; (* IF *)
END ResetModuleSeq;


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
    END;
    seq.a[seq.tide] := elem; INC(seq.tide); 
END AppendType;


PROCEDURE AppendModule(VAR seq : ModuleSeq; elem : MS.Namespace);
VAR
    temp : POINTER TO ARRAY OF MS.Namespace;
    i    : INTEGER;
BEGIN
    IF seq.a = NIL THEN 
      InitModuleSeq(seq, 2);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, (seq.high+1));
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide); 
END AppendModule;


PROCEDURE (et: Emiter) AddToImpolist(ns: MS.Namespace), NEW;
BEGIN
    IF (ns # et.ns) & ~ns.Dumped() THEN
        ns.SetModuleOrd(et.iNxt); INC(et.iNxt);
        AppendModule(et.impo, ns);
    END; (* IF *)
END AddToImpolist;


PROCEDURE NewEmiter(): Emiter;
VAR
    et: Emiter;
BEGIN
    NEW(et);
   (*
    *  Initialization: cSum starts at zero. Since impOrd of
    *  the module is zero, impOrd of the imports starts at 1.
    *)
    et.version := NIL;
    et.token := NIL;
    et.cSum := 0;
    et.iNxt := 1;
    et.oNxt := tOffset;         (* 1-15 are reserved for base types *)
    NEW(et.work);
    InitTypeSeq(et.work, MAXTYPE);
    NEW(et.impo);
    InitModuleSeq(et.impo, MAXMODULE);
    RETURN et;
END NewEmiter;


PROCEDURE (et: Emiter) Reset(), NEW;
BEGIN
    et.cSum := 0;
    et.iNxt := 1;
    et.oNxt := tOffset;         (* 1-15 are reserved for base types *)
    ResetTypeSeq(et.work);
    ResetModuleSeq(et.impo);
END Reset;

(* ================================================================ *)

PROCEDURE (et: Emiter) Write(chr: INTEGER), NEW;
VAR
    tmp: INTEGER;
BEGIN [UNCHECKED_ARITHMETIC]
   (* need to turn off overflow checking here *)
    IF ~PreEmit THEN
        tmp := et.cSum * 2 + chr;
        IF et.cSum < 0 THEN INC(tmp) END;
        et.cSum := tmp;
        GF.WriteByte(et.file, chr);
    END; (* IF *)
END Write;


PROCEDURE (et: Emiter) WriteByte(byt: INTEGER), NEW;
BEGIN
    IF ~PreEmit THEN
        ASSERT((byt <= 127) & (byt > 0));
        et.Write(bytSy);
        et.Write(byt);
    END; (* IF *)
END WriteByte;


PROCEDURE (et: Emiter) WriteChar(chr: CHAR), NEW;
CONST
    mask = {0 .. 7};
VAR
    a, b, int: INTEGER;
BEGIN
    IF ~PreEmit THEN
        et.Write(chrSy);
        int := ORD(chr);
        b := ORD(BITS(int) * mask); int := ASH(int, -8);
        a := ORD(BITS(int) * mask);
        et.Write(a); et.Write(b);
    END; (* IF *)
END WriteChar;


PROCEDURE (et: Emiter) Write4B(int: INTEGER), NEW;
CONST mask = {0 .. 7};
VAR   a,b,c,d : INTEGER;
BEGIN
    IF ~PreEmit THEN
        d := ORD(BITS(int) * mask); int := ASH(int, -8);
        c := ORD(BITS(int) * mask); int := ASH(int, -8);
        b := ORD(BITS(int) * mask); int := ASH(int, -8);
        a := ORD(BITS(int) * mask); 
        et.Write(a); 
        et.Write(b); 
        et.Write(c); 
        et.Write(d); 
    END; (* IF *)
END Write4B;


PROCEDURE (et: Emiter) Write8B(val: LONGINT), NEW;
BEGIN
    IF ~PreEmit THEN
        et.Write4B(RTS.hiInt(val));
        et.Write4B(RTS.loInt(val));
    END; (* IF *)
END Write8B;


PROCEDURE (et: Emiter) WriteNum(num: LONGINT), NEW;
BEGIN
    IF ~PreEmit THEN
        et.Write(numSy);
        et.Write8B(num);
    END; (* IF *)
END WriteNum;


PROCEDURE (et: Emiter) WriteReal(flt: REAL), NEW;
VAR
    rslt: LONGINT;
BEGIN
    IF ~PreEmit THEN
        et.Write(fltSy);
        rslt := RTS.realToLongBits(flt);
        et.Write8B(rslt);
    END; (* IF *)
END WriteReal;


PROCEDURE (et: Emiter) WriteOrd(ord: INTEGER), NEW;
BEGIN
    IF ~PreEmit THEN
        IF ord <= 7FH THEN 
            et.Write(ord);
        ELSIF ord <= 7FFFH THEN
            et.Write(128 + ord MOD 128);        (* LS7-bits first *)
            et.Write(ord DIV 128);              (* MS8-bits next  *)
        ELSE
            ASSERT(FALSE);
        END; (* IF *)
    END; (* IF *)
END WriteOrd;


PROCEDURE (et: Emiter) WriteStrUTF(IN nam: ARRAY OF CHAR), NEW;
VAR
    buf : ARRAY 256 OF INTEGER;
    num : INTEGER;
    idx : INTEGER;
    chr : INTEGER;
BEGIN
    IF ~PreEmit THEN
        num := 0;
        idx := 0;
        chr := ORD(nam[idx]);
        WHILE chr # 0H DO
            IF    chr <= 7FH THEN               (* [0xxxxxxx] *)
                buf[num] := chr; INC(num);
            ELSIF chr <= 7FFH THEN              (* [110xxxxx,10xxxxxx] *)
                buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
                buf[num  ] := 0C0H + chr; INC(num, 2);
            ELSE                                (* [1110xxxx,10xxxxxx,10xxxxxx] *)
                buf[num+2] := 080H + chr MOD 64; chr := chr DIV 64;
                buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
                buf[num  ] := 0E0H + chr; INC(num, 3);
            END; (* IF *)
            INC(idx); chr := ORD(nam[idx]);
        END; (* WHILE *)
        et.Write(num DIV 256);
        et.Write(num MOD 256);
        FOR idx := 0 TO num-1 DO et.Write(buf[idx]) END;
    END; (* IF *)
END WriteStrUTF;


PROCEDURE (et: Emiter) WriteOpenUTF(chOp: CharOpen), NEW;
VAR
    buf : ARRAY 256 OF INTEGER;
    num : INTEGER;
    idx : INTEGER;
    chr : INTEGER;
BEGIN
    IF ~PreEmit THEN
        num := 0;
        idx := 0;
        chr := ORD(chOp[0]);
        WHILE chr # 0H DO
            IF chr <= 7FH THEN                  (* [0xxxxxxx] *)
                buf[num] := chr; INC(num);
            ELSIF chr <= 7FFH THEN              (* [110xxxxx,10xxxxxx] *)
                buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
                buf[num  ] := 0C0H + chr; INC(num, 2);
            ELSE                                (* [1110xxxx,10xxxxxx,10xxxxxx] *)
                buf[num+2] := 080H + chr MOD 64; chr := chr DIV 64;
                buf[num+1] := 080H + chr MOD 64; chr := chr DIV 64;
                buf[num  ] := 0E0H + chr; INC(num, 3);
            END; (* IF *)
            INC(idx);
            chr := ORD(chOp[idx]);
        END; (* WHILE *)
        et.Write(num DIV 256);
        et.Write(num MOD 256);
        FOR idx := 0 TO num-1 DO et.Write(buf[idx]) END;
    END; (* IF *)
END WriteOpenUTF;


PROCEDURE (et: Emiter) WriteString(IN str: ARRAY OF CHAR), NEW;
BEGIN
    IF ~PreEmit THEN
        et.Write(strSy);
        et.WriteStrUTF(str);
    END; (* IF *)
END WriteString;


PROCEDURE (et: Emiter) IsTypeForeign(t: MS.Type): BOOLEAN, NEW;
VAR
    tnsname: CharOpen;
    tasbname: CharOpen;
BEGIN
    IF t.GetNamespace() # NIL THEN
        tnsname := t.GetNamespaceName();
        tasbname := t.GetAssemblyName();
        IF (tnsname^ = et.nsname^) & (tasbname^ = et.asbname^) THEN
            (* local type *)
            RETURN FALSE;
        ELSE
            RETURN TRUE;
        END; (* IF *)
    ELSE
        RETURN FALSE;
    END; (* IF *)
END IsTypeForeign;

(* ================================================================ *)

PROCEDURE (et: Emiter) EmitKey(key: INTEGER), NEW;
BEGIN
    et.Write(keySy);
    et.Write4B(key);
END EmitKey;


PROCEDURE (et: Emiter) EmitName(name: CharOpen; vMod: INTEGER), NEW;
BEGIN
    et.Write(namSy);
    et.Write(vMod);
    et.WriteOpenUTF(name);
END EmitName;


PROCEDURE (et: Emiter) EmitString(IN nam: ARRAY OF CHAR), NEW;
BEGIN
    et.Write(strSy); 
    et.WriteStrUTF(nam);
END EmitString;


PROCEDURE (et: Emiter) EmitScopeName(asbname: CharOpen; nsname: CharOpen), NEW;
VAR
    scopeNm: CharOpen;
BEGIN
    scopeNm := ST.StrCat(ST.ToChrOpen("["),asbname);
    scopeNm := ST.StrCatChr(scopeNm,"]");
    IF nsname^ # MS.NULLSPACE THEN
        scopeNm := ST.StrCat(scopeNm,nsname);
    END; (* IF *)
    et.EmitString(scopeNm);
END EmitScopeName;


PROCEDURE (et: Emiter) EmitHeader(), NEW;
BEGIN
    et.Write4B(RTS.loInt(magic));
    et.Write(modSy);
    et.EmitName(et.modname, prvMode);  (* hardcode to prvMode doesn't matter for Module *)
    et.EmitScopeName(et.asbfile, et.nsname);        (* <== should be asbfile or asbname? *)
    et.Write(falSy);
END EmitHeader;


PROCEDURE (et: Emiter) EmitVersion(), NEW;
VAR
    i: INTEGER;
BEGIN
    IF et.version # NIL THEN
        (* pack major and minor into a longint *)
        et.Write(numSy);
        et.Write4B(et.version[MS.Major]);
        et.Write4B(et.version[MS.Minor]);
        (* pack build and revision into a longint *)
        et.Write(numSy);
        et.Write4B(et.version[MS.Build]);
        et.Write4B(et.version[MS.Revis]);
        (* pack public key token into a longint *)
        IF et.token # NIL THEN
            et.Write(numSy);
            FOR i := 0 TO 7 DO et.Write(et.token[i]); END;
        ELSE
            et.WriteNum(0);
        END; (* IF *)
    END; (* IF *)
END EmitVersion;


PROCEDURE (et: Emiter) DirectImports(), NEW;
VAR
    fns: MS.Namespace;
    nstv: MS.OTraverser;
BEGIN
    IF et.ns.HasForeignSpaces() THEN
        NEW(nstv);
        nstv.Initialize(et.ns.GetForeignSpaces());
        fns := nstv.GetNextNamespace();
        WHILE fns # NIL DO
            (* assigns import modules ordinal *)
            et.AddToImpolist(fns);
            fns := nstv.GetNextNamespace();
        END; (* WHILE *)
    END; (* IF *)
END DirectImports;


PROCEDURE (et: Emiter) EmitImports(), NEW;
VAR
    indx: INTEGER;
    fns: MS.Namespace;
    fasbname: CharOpen;
    fasbfile: CharOpen;
    fnsname: CharOpen;
    fmodname: CharOpen;
BEGIN
    indx := 0;
    WHILE indx < et.impo.tide DO
        et.Write(impSy);
        fns := et.impo.a[indx];
        fnsname := fns.GetName();
        fasbfile := fns.GetAssemblyFile();
        fasbname := fns.GetAssemblyName();
        fmodname := MS.MakeModuleName(fasbname, fnsname);
        et.EmitName(fmodname, prvMode); (* hardcode vMode to prvMode
                                           doesn't matter for Imports *)
        IF (ST.StrChr(fnsname,'.') # ST.NotExist) OR
           (fasbname^ # fnsname^) THEN
            et.EmitScopeName(fasbfile, fnsname);
        END; (* IF *)
        et.EmitKey(0);  (* key is zero for foreigns *)
        INC(indx);
    END; (* WHILE *)
END EmitImports;


PROCEDURE (et: Emiter) AddToWorklist(typ: MS.Type), NEW;
BEGIN
    typ.SetTypeOrd(et.oNxt); INC(et.oNxt);
    AppendType(et.work, typ);
END AddToWorklist;


PROCEDURE (et: Emiter) EmitTypeOrd(t: MS.Type), NEW;
BEGIN
    IF ~t.Dumped() THEN et.AddToWorklist(t); END;
    et.WriteOrd(t.GetTypeOrd());
END EmitTypeOrd;


PROCEDURE (et: Emiter) EmitLocalTypeName(typ: MS.Type), NEW;
VAR
    tname: CharOpen;
BEGIN
    typ.SetInHierarchy();
    tname := et.MakeFullTypeName(typ);
    et.Write(typSy);
    et.EmitName(tname, pubMode);
    et.EmitTypeOrd(typ);
END EmitLocalTypeName;


PROCEDURE (et: Emiter) EmitLocalTypes(), NEW;
VAR
    tv: MS.OTraverser;
    t: MS.Type;
    tname: CharOpen;
    tord: INTEGER;
    ntv: MS.OTraverser;
    nt: MS.Type;
BEGIN
    NEW(tv); tv.Initialize(et.ns.GetTypes());
    t := tv.GetNextType();
    WHILE t # NIL DO
        IF t.IsExported() THEN
            IF (et.mnameKind = SingleWord) & (t.GetName()^ = et.nsname^) THEN
                IF t.IsInterface() THEN
                    (* if 't' is POINTER TO INTERFACE, it cannot be main type *)
                    et.EmitLocalTypeName(t);
                ELSE
                    (* a gpcp module main type, don't emit this type *)
                    et.maintyp := t;
                END; (* IF *)
            ELSE
                et.EmitLocalTypeName(t);
            END; (* IF *)
        END; (* IF *)
        t := tv.GetNextType();
    END; (* WHILE *)
END EmitLocalTypes;


PROCEDURE (et: Emiter) EmitTypes(), NEW;
BEGIN
    et.EmitLocalTypes();
END EmitTypes;


PROCEDURE (et: Emiter) EmitTypeHeader(t: MS.Type), NEW;
BEGIN
    et.Write(tDefS);
    et.WriteOrd(t.GetTypeOrd());
    IF et.IsTypeForeign(t) & ~t.IsAnonymous() THEN
        et.Write(fromS);
        et.WriteOrd(t.GetNamespace().GetModuleOrd());
        et.EmitName(MakeTypeName(t), pubMode);
    END; (* IF *)
END EmitTypeHeader;


PROCEDURE (et: Emiter) EmitNamedType(t: MS.Type), NEW;
BEGIN
    et.EmitTypeHeader(t);
END EmitNamedType;


PROCEDURE (et: Emiter) EmitArrayType(t: MS.ArrayType), NEW;
VAR
    elm: MS.Type;
    len: INTEGER;
BEGIN
    et.EmitTypeHeader(t);
    et.Write(arrSy);
    et.EmitTypeOrd(t.GetElement());
    len := t.GetLength();
    IF len > 127 THEN
        et.WriteNum(len);
    ELSIF len > 0 THEN
        et.WriteByte(len);
    ELSE
    END; (* IF *)
    et.Write(endAr);
END EmitArrayType;


PROCEDURE (et: Emiter) EmitPointerType(t: MS.PointerType), NEW;
VAR
    tgt: MS.Type;
BEGIN
    IF t.IsDelegate() THEN
        tgt := t.GetTarget();
        WITH tgt: MS.DelegType DO
            tgt.SetTypeOrd(t.GetTypeOrd());
            et.EmitDelegate(tgt);
            tgt.ClearTypeOrd();
        ELSE
            ASSERT(FALSE);
        END; (* WITH *)

    ELSE
        et.EmitTypeHeader(t);
        et.Write(ptrSy);
        tgt := t.GetTarget();
        IF t.IsInHierarchy() THEN
            tgt.SetInHierarchy();
        ELSE
        END; (* IF *)
        et.EmitTypeOrd(tgt);
    END; (* IF *)
END EmitPointerType;


PROCEDURE (et: Emiter) EmitMethodAttribute(m: MS.Method), NEW;
VAR
    mthAtt: SET;
    dt: MS.Type;
BEGIN
    mthAtt := {};
    IF m.IsNew() THEN
        mthAtt := isNew;
    END; (* IF *)
    dt := m.GetDeclaringType();
    IF m.IsAbstract() THEN
        mthAtt := mthAtt + isAbs;
    ELSIF (dt.IsAbstract() OR dt.IsExtensible()) & m.IsExtensible() THEN
        mthAtt := mthAtt + isExt;
    END; (* IF *)
    et.Write(ORD(mthAtt));
END EmitMethodAttribute;


PROCEDURE (et: Emiter) EmitReceiverInfo (m: MS.Method), NEW;
VAR
    rcvr: MS.Type;
BEGIN
    rcvr := m.GetDeclaringType();
    IF rcvr IS MS.ValueType THEN
        et.Write(in);        (* IN par mode for value type in dll's sym *) 
    ELSE
        et.Write(val);       (* value par mode for obj ref type in dll's sym *) 
    END; (* IF *)
    et.EmitTypeOrd(rcvr);
END EmitReceiverInfo;


PROCEDURE (et: Emiter)EmitAnonymousArrayPointerType(t: MS.PointerType): MS.PointerType, NEW;
VAR
    ptype: MS.PointerType;
    tgt: MS.Type;
BEGIN
    ptype := NIL;
    tgt := t.GetTarget();
    WITH tgt: MS.ArrayType DO
        ptype := tgt.GetAnonymousPointerType();
        IF ptype = NIL THEN
            ptype := MS.MakeAnonymousPointerType(tgt);
        END; (* IF *)
    ELSE
        ASSERT(FALSE);
    END; (* IF *)

    et.EmitTypeOrd(ptype);
    RETURN ptype;
END EmitAnonymousArrayPointerType;


PROCEDURE (et: Emiter) EmitFormals(m: MS.Method), NEW;
VAR
    rtype: MS.Type;
    tv: MS.FTraverser;
    formals: MS.FormalList;
    f: MS.Formal;
    ftype: MS.Type;
    dmyPType: MS.PointerType;
BEGIN
    WITH m: MS.Function DO 
        rtype := m.GetReturnType();
        et.Write(retSy);
        et.EmitTypeOrd(rtype);
    ELSE
    END; (* WITH *)
    et.Write(frmSy);
    formals := m.GetFormals();
    IF formals.Length() # 0 THEN
        NEW(tv); tv.Initialize(formals);
        f := tv.GetNextFormal();
        WHILE f # NIL DO
            et.Write(parSy);
            et.Write(f.GetParameterMode());
            ftype := f.GetType();
            WITH ftype: MS.PointerType DO
                IF ftype.IsArrayPointer() THEN
                    dmyPType := et.EmitAnonymousArrayPointerType(ftype);     (* what if the formal type is array pointer but not anonymous (created by GPCP) *)
                    f.SetType(dmyPType, FALSE);
                ELSE
                    et.EmitTypeOrd(ftype);
                END; (* IF *)
            ELSE
                et.EmitTypeOrd(ftype);
            END; (* IF *)

            f := tv.GetNextFormal();
        END; (* WHILE *)
    END; (* IF *)
    et.Write(endFm);
END EmitFormals;


PROCEDURE RequireInvokeName(mth: MS.Method): BOOLEAN;
BEGIN
    IF mth.IsConstructor() THEN
        (* constructors always require invoke name *)
        RETURN TRUE;
    ELSE
        IF MS.WithoutMethodNameMangling() THEN
            RETURN FALSE
        ELSE
            RETURN mth.IsOverload();
        END;
    END; (* IF *)
END RequireInvokeName;


PROCEDURE (et: Emiter) EmitVirtMethods(t: MS.Type), NEW;
VAR
    tv: MS.OTraverser;
    m : MS.Method;
    mname: CharOpen;
    vMod: INTEGER;
BEGIN
    NEW(tv); tv.Initialize(t.GetVirtualMethods());
    m := tv.GetNextMethod();
    WHILE m # NIL DO
        IF m.IsExported() THEN
            mname := m.GetName();
            et.Write(mthSy);
            vMod := pubMode;
            IF m.IsProtected() THEN
                vMod := protect;
            END; (* IF *)
            et.EmitName(mname, vMod);
            et.EmitMethodAttribute(m);
            et.EmitReceiverInfo(m);
            IF RequireInvokeName(m) THEN
                et.EmitString(m.GetInvokeName());
            END; (* IF *)
            et.EmitFormals(m);
        END; (* IF *)
        m := tv.GetNextMethod();
    END; (* WHILE *)
END EmitVirtMethods;


PROCEDURE (et: Emiter) EmitImplInterfaces(t: MS.Type), NEW;
(*             [iFcSy {basSy TypeOrd}]
 *)
VAR
    tv: MS.OTraverser;
    it: MS.Type;
BEGIN
    et.Write(iFcSy);
    NEW(tv); tv.Initialize(t.GetInterfaces());
    it := tv.GetNextType();
    WHILE it # NIL DO
        IF it.IsExported() THEN
            et.Write(basSy);
            et.EmitTypeOrd(it);
            IF t.IsInterface() THEN
                (* interface (t) inherits other interface (it) *)
                it.SetInHierarchy();    (* to force emiting of parent interface (it) methods *)
            END; (* IF *)
        END; (* IF *)
        it := tv.GetNextType();
    END; (* WHILE *)
END EmitImplInterfaces;


PROCEDURE (et: Emiter) EmitInterfaceType(t: MS.IntfcType), NEW;
VAR
    recAtt: INTEGER;
    base: MS.Type;
BEGIN
    recAtt := iFace;
    et.EmitTypeHeader(t);
    et.Write(recSy);
    et.Write(recAtt);
    et.Write(truSy);
    base := t.GetBaseType();
    IF base # NIL THEN
        et.Write(basSy);
        et.EmitTypeOrd(base);
    END; (* IF *)

    IF t.HasImplInterfaces() THEN
        et.EmitImplInterfaces(t);
    END; (* IF *)

    IF t.HasVirtualMethods() THEN
        et.EmitVirtMethods(t);
    END; (* IF *)
    et.Write(endRc);
END EmitInterfaceType;


PROCEDURE (et: Emiter)EmitFields(t: MS.Type), NEW;
VAR
    tv: MS.OTraverser;
    flist: MS.OrderList;
    f: MS.Field;
    vMod: INTEGER;
    ftype: MS.Type;
    dmyPType: MS.PointerType;
BEGIN
    flist := t.GetInstanceFields();
    IF flist = NIL THEN RETURN END;
    NEW(tv); tv.Initialize(flist);
    f := tv.GetNextField();
    WHILE f # NIL DO
        IF f.IsExported() THEN
            vMod := pubMode;
            IF f.IsProtected() THEN
                vMod := protect;
            END; (* IF *)
            et.EmitName(f.GetName(), vMod);
            ftype := f.GetType();
            WITH ftype: MS.PointerType DO
                IF ftype.IsArrayPointer() THEN
                    dmyPType := et.EmitAnonymousArrayPointerType(ftype);     (* what if the field type is array pointer but not anonymous (created by GPCP) *)
                    f.SetType(dmyPType);
                ELSE
                    et.EmitTypeOrd(ftype);
                END; (* IF *)
            ELSE
                et.EmitTypeOrd(ftype);
            END; (* IF *)
        END; (* IF *)
        f := tv.GetNextField();
    END; (* WHILE *)
END EmitFields;


PROCEDURE (et: Emiter) EmitEventFields(t: MS.Type), NEW;
VAR
    tv: MS.OTraverser;
    elist: MS.OrderList;
    e: MS.Event;

    ename: CharOpen;
    htype: MS.Type;
    tname: CharOpen;
BEGIN
    NEW(tv); tv.Initialize(t.GetEventList());
    e := tv.GetNextEvent();
    WHILE e # NIL DO
        et.EmitName(e.GetName(), pubMode);        (* event always be exported for an public record *)
        et.EmitTypeOrd(e.GetHandlerType());       (* we put the handler type(as .NET does) *)
        e := tv.GetNextEvent();
    END; (* WHILE *)
END EmitEventFields;


PROCEDURE (et: Emiter) EmitVariables(t: MS.Type), NEW;
VAR
    tv: MS.OTraverser;
    flist: MS.OrderList;
    f: MS.Field;
    vMod: INTEGER;
    ftype: MS.Type;
    dmyPType: MS.PointerType;
BEGIN
    flist := t.GetStaticFields();
    IF flist = NIL THEN RETURN END;
    NEW(tv); tv.Initialize(flist);
    f := tv.GetNextField();
    WHILE f # NIL DO
        IF f.IsExported() THEN
            et.Write(varSy);
            vMod := pubMode;
            IF f.IsProtected() THEN
                vMod := protect;
            END; (* IF *)
            et.EmitName(f.GetName(), vMod);
            ftype := f.GetType();
            WITH ftype: MS.PointerType DO
                IF ftype.IsArrayPointer() THEN
                    dmyPType := et.EmitAnonymousArrayPointerType(ftype);     (* what if the field type is array pointer but not anonymous (created by GPCP) *)
                    f.SetType(dmyPType);
                ELSE
                    et.EmitTypeOrd(ftype);
                END; (* IF *)
            ELSE
                et.EmitTypeOrd(ftype);
            END; (* IF *)
        END; (* IF *)
        f := tv.GetNextField();
    END; (* WHILE *)
END EmitVariables;


PROCEDURE (et: Emiter) EmitValue(lit: MS.Literal), NEW;
BEGIN
    WITH lit: MS.BoolLiteral DO
              IF lit.GetValue() THEN et.Write(truSy); ELSE et.Write(falSy); END;
    |    lit: MS.CharLiteral DO
              et.WriteChar(lit.GetValue());
    |    lit: MS.StrLiteral DO
              et.WriteString(lit.GetValue());
    |    lit: MS.NumLiteral DO
              et.WriteNum(lit.GetValue());
    |    lit: MS.FloatLiteral DO
              et.WriteReal(lit.GetValue());
    ELSE
    END; (* WITH *)
END EmitValue;


PROCEDURE (et: Emiter) EmitConstants(t: MS.Type), NEW;
VAR
    tv: MS.OTraverser;
    c: MS.Constant;
    vMod: INTEGER;
BEGIN
    NEW(tv); tv.Initialize(t.GetConstants());
    c := tv.GetNextConstant();
    WHILE c # NIL DO
        IF c.IsExported() THEN
            et.Write(conSy);
            vMod := pubMode;
            IF c.IsProtected() THEN
                vMod := protect;
            END; (* IF *)
            et.EmitName(c.GetName(), vMod);
            et.EmitValue(c.GetValue());
        END; (* IF *)
        c := tv.GetNextConstant();
    END; (* WHILE *)
END EmitConstants;


PROCEDURE (et: Emiter) EmitStaticMethods(t: MS.Type), NEW;
VAR
    tv: MS.OTraverser;
    m : MS.Method;
    mname: CharOpen;
    vMod: INTEGER;
BEGIN
    NEW(tv); tv.Initialize(t.GetStaticMethods());
    m := tv.GetNextMethod();
    WHILE m # NIL DO
        IF (m.GetDeclaringType() = et.maintyp) & (m.IsConstructor()) THEN
            (* don't emit any maintyp's constructor for a GPCP module *)
        ELSE
            IF m.IsExported() THEN
                mname := m.GetName();
                IF mname^ # "Main" THEN
                    et.Write(prcSy);
                    vMod := pubMode;
                    IF m.IsProtected() THEN vMod := protect; END;
                    et.EmitName(mname, vMod);
                    IF RequireInvokeName(m) THEN et.EmitString(m.GetInvokeName()); END;
                    IF m.IsConstructor() THEN et.Write(truSy); END;
                    et.EmitFormals(m);
                END; (* IF *)
            END; (* IF *)
        END; (* IF *)
        m := tv.GetNextMethod();
    END; (* WHILE *)
END EmitStaticMethods;


PROCEDURE (et: Emiter) EmitStrucType(t: MS.ValueType), NEW;
(*
 **  Record = TypeHeader recSy recAtt [truSy | falSy | <others>] 
 *              [basSy TypeOrd] [iFcSy {basSy TypeOrd}]
 **             {Name TypeOrd} {Method} {Statics} endRc.
 *)
VAR
    recAtt: INTEGER;
    base: MS.Type;
    basevalue: MS.Type;
BEGIN
    recAtt := noAtt;
    IF t.IsAbstract() THEN
        recAtt := abstr;
    ELSIF t.IsExtensible() THEN
        recAtt := extns;
    END; (* IF *)
    IF ~t.HasNoArgConstructor() THEN INC(recAtt, nnarg); END;
    IF t.IsValueType() THEN INC(recAtt, valTp); END;
    et.EmitTypeHeader(t);
    et.Write(recSy);
    et.Write(recAtt);
    et.Write(falSy);
    base := t.GetBaseType();
    IF (base # NIL) & (base # MS.baseTypeArray[anyRec]) THEN   (* <== *)
        et.Write(basSy);
        WITH base: MS.PointerType DO
            basevalue := base.GetTarget();
            IF t.IsInHierarchy() THEN
                base.SetInHierarchy();
                basevalue.SetInHierarchy();
                IF ~base.Dumped() THEN
                    et.AddToWorklist(base);
                ELSE
                END; (* IF *)
            ELSE
            END; (* IF *)
            (* request by Diane, base type is class, rather than record *)
            et.EmitTypeOrd(base);
        ELSE
            ASSERT(base.GetTypeOrd() = anyRec);
        END; (* WITH *)
    ELSE
        (* no base type declared, so use ANYREC as its base type *)
        et.Write(basSy);
        et.Write(anyRec);
    END; (* IF *)

    IF t.HasImplInterfaces() THEN et.EmitImplInterfaces(t); END;
    IF t.HasInstanceFields() THEN et.EmitFields(t); END;
    IF t.HasEvents() THEN et.EmitEventFields(t); END;
    IF t.HasVirtualMethods() THEN et.EmitVirtMethods(t); END;
    IF t.HasConstants() THEN et.EmitConstants(t); END;
    IF t.HasStaticFields() THEN et.EmitVariables(t); END;
    IF t.HasStaticMethods() THEN et.EmitStaticMethods(t); END;

    et.Write(endRc);
END EmitStrucType;


PROCEDURE (et: Emiter) EmitEnumType(t: MS.EnumType), NEW;
BEGIN
    et.EmitTypeHeader(t);
    et.Write(eTpSy);
    et.EmitConstants(t);
    et.Write(endRc);
END EmitEnumType;


PROCEDURE (et: Emiter) EmitDelegate(t: MS.DelegType), NEW;
VAR
    imth: MS.Method;
BEGIN
    et.EmitTypeHeader(t);
    IF t.IsMulticast() THEN
        et.Write(evtSy);
    ELSE
        et.Write(pTpSy);
    END; (* IF *)
    imth := t.GetInvokeMethod();
    et.EmitFormals(imth);
END EmitDelegate;


PROCEDURE (et: Emiter) EmitTypeList(), NEW;
VAR
    indx: INTEGER;
    type: MS.Type;
    ns: MS.Namespace;
    nt: MS.Type;
    ntv: MS.OTraverser;
    tgt: MS.Type;
BEGIN
    et.Write(start);
    indx := 0;
    WHILE indx < et.work.tide DO
        type := et.work.a[indx];
        ns := type.GetNamespace();
        IF ns # NIL THEN et.AddToImpolist(ns); END; 
        WITH type: MS.PointerType DO
            tgt := type.GetTarget();
            WITH tgt: MS.RecordType DO
                IF type.IsInHierarchy() THEN
                    et.EmitPointerType(type);
                ELSIF ~et.IsTypeForeign(type) THEN
                    (* a non-Exported type but referenced by other type *)
                    et.EmitPointerType(type);
                ELSE
                    et.EmitNamedType(type);
                END; (* IF *)
            | tgt: MS.ArrayType DO
                et.EmitPointerType(type);
            ELSE
            END; (* WITH *)
        | type: MS.ArrayType DO
            et.EmitArrayType(type);
        | type: MS.RecordType DO
            WITH type: MS.IntfcType DO
                et.EmitInterfaceType(type);
            | type: MS.ValueType DO
                WITH type: MS.EnumType DO
                    et.EmitEnumType(type);
                | type: MS.PrimType DO                (* for IntPtr and UIntPtr *)
                    IF type.IsInHierarchy() THEN
                        et.EmitStrucType(type);
                    ELSIF ~et.IsTypeForeign(type) THEN
                        (* a non-Exported type but referenced by other type *)
                        et.EmitStrucType(type);
                    ELSE
                        et.EmitNamedType(type);
                    END; (* IF *)
                ELSE
                END; (* WITH *)
            ELSE
            END; (* WITH *)
        | type: MS.NamedType DO 
            et.EmitNamedType(type);
        ELSE
        END; (* WITH *)
        INC(indx);
    END; (* WHILE *)
    et.Write(close);
END EmitTypeList;


PROCEDURE (et: Emiter) EmitModule(), NEW;
(*
 * SymFile = 
 *   Header [String (falSy | truSy | <others>)]
 *   {Import | Constant | Variable | Type | Procedure | Method} TypeList.
 * Header = magic modSy Name.
 *)
BEGIN
    (* Walk through all types to gather info about import modules *)
    PreEmit := TRUE;
    et.DirectImports();
    et.EmitTypes();
    IF et.maintyp # NIL THEN
        IF et.maintyp.HasStaticFields() THEN
            et.EmitVariables(et.maintyp);
        END; (* IF *)
        IF et.maintyp.HasStaticMethods() THEN
            et.EmitStaticMethods(et.maintyp);
        END; (* IF *)
    END; (* IF *)
    et.EmitTypeList();


    (* Now really emit type info *)
    PreEmit := FALSE;
    et.EmitHeader();
    et.EmitVersion();
    et.EmitImports();
    et.EmitTypes();
    IF et.maintyp # NIL THEN
        IF et.maintyp.HasConstants() THEN
            et.EmitConstants(et.maintyp);
        END; (* IF *)
        IF et.maintyp.HasStaticFields() THEN
            et.EmitVariables(et.maintyp);
        END; (* IF *)
        IF et.maintyp.HasStaticMethods() THEN
            et.EmitStaticMethods(et.maintyp);
        END; (* IF *)
    END; (* IF *)
    et.EmitTypeList();
    et.EmitKey(0);
END EmitModule;


PROCEDURE EmitSymbolFiles*(asb: MS.Assembly);
VAR
    et: Emiter;
    filename: CharOpen;
    tv: MS.OTraverser;
    onewordname: BOOLEAN;
    samewordname: BOOLEAN;
    inclwordname: BOOLEAN;
BEGIN
    NEW(tv); tv.Initialize(asb.GetNamespaces());
    et := NewEmiter();
    et.asbname := asb.GetName();
    et.asbfile := asb.GetFileName();
    et.version := asb.GetVersion();
    et.token := asb.GetPublicKeyToken();
    et.ns := tv.GetNextNamespace();
    IF et.ns # NIL THEN
        et.nsname := et.ns.GetName();
        onewordname := MS.IsOneWordName(et.asbname, et.nsname);
        samewordname := MS.IsSameWordName(et.asbname, et.nsname);
        IF onewordname & samewordname & (asb.NamespaceCount() = 1) THEN
            (* It is very likely to be a GPCP compiled DLL or exe *)
            et.mnameKind := SingleWord;
            et.modname := MS.MakeModuleName(et.asbname, et.nsname);
            filename := ST.StrCat(et.modname, ST.ToChrOpen(SymbolExt));
            et.file := GF.createFile(filename);
            IF et.file = NIL THEN
                Error.WriteString("Cannot create file <" + filename^ + ">"); Error.WriteLn;
                ASSERT(FALSE);
                RETURN;
            END; (* IF *)
            et.EmitModule();
            GF.CloseFile(et.file);
            et.Reset();
        ELSE
            REPEAT
                IF ~onewordname & samewordname THEN
                    (* cannot be null namespace here *)
                    et.mnameKind := MultipleWord;
                ELSE
                    et.mnameKind := DifferentWord;
                END; (* IF *)
                et.modname := MS.MakeModuleName(et.asbname, et.nsname);
                filename := ST.StrCat(et.modname, ST.ToChrOpen(SymbolExt));
                et.file := GF.createFile(filename);
                IF et.file = NIL THEN
                    Error.WriteString("Cannot create file <" + filename^ + ">"); Error.WriteLn;
                    ASSERT(FALSE);
                    RETURN;
                END; (* IF *)
                et.EmitModule();
                GF.CloseFile(et.file);
                et.Reset();
                et.ns := tv.GetNextNamespace();

                IF et.ns # NIL THEN
                    et.nsname := et.ns.GetName();
                    onewordname := (ST.StrChr(et.nsname,'.') = ST.NotExist);
                    samewordname := (et.asbname^ = ST.StrSubChr(et.nsname,'.','_')^);
                 END; (* IF *)
            UNTIL et.ns = NIL;
         END; (* IF *)
    END; (* IF *)
END EmitSymbolFiles;

END SymWriter.
