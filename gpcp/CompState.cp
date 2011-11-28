(* ==================================================================== *)
(*									*)
(*  State Module for the Gardens Point Component Pascal Compiler.	*)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*									*)
(*  Note that since this module is likely to be imported by most other  *)
(*  modules, it is important to ensure that it does not import others,  *)
(*  to avoid import cycles. 						*)
(*									*)
(* ==================================================================== *)

MODULE CompState;

  IMPORT 
        GPCPcopyright,
        RTS,
        Error,
        GPText,
        Symbols,
        IdDesc,
        Console, 
        CPascalS,
        NameHash,
        FileNames,
        CPascalErrors;

  CONST	prefix     = "#gpcp: ";
        millis     = "mSec";

  CONST netV1_0* = 0;
        netV1_1* = 1;
        netV2_0* = 2;

(* ==================================================================== *)
(*		     State Variables of this compilation		*)
(* ==================================================================== *)

  VAR
    ntvObj* : Symbols.Type;     (* native Object type          	*)
    ntvStr* : Symbols.Type;     (* native String type          	*)
    ntvExc* : Symbols.Type;     (* native Exceptions type       *)
    ntvTyp* : Symbols.Type;     (* native System.Type type      *)
    ntvEvt* : Symbols.Type;     (* native MulticastDelegate     *)
    rtsXHR* : Symbols.Type;     (* native XHR type descriptor   *)
    ntvVal* : Symbols.Type;     (* native ValueType type        *)

    objId*  : Symbols.Idnt;
    strId*  : Symbols.Idnt;
    excId*  : Symbols.Idnt;
    clsId*  : Symbols.Idnt;
    xhrId*  : IdDesc.FldId;     (* descriptor of RTS.XHR.prev   *)
    rtsBlk* : IdDesc.BlkId;
    prgArg* : IdDesc.BlkId;
    argLst* : IdDesc.VarId;     (* descriptor of RTS.argList    *)

    srcBkt* : INTEGER;          (* hashtable bucket of "src"    *)
    corBkt* : INTEGER;          (* bucket of "mscorlib_System"  *)

    fltInf*  : IdDesc.VarId;    (* descriptor of RTS.fltPosInf. *)
    dblInf*  : IdDesc.VarId;    (* descriptor of RTS.dblPosInf. *)
    fltNInf* : IdDesc.VarId;    (* descriptor of RTS.fltNegInf. *)
    dblNInf* : IdDesc.VarId;    (* descriptor of RTS.dblNegInf. *)

  VAR
    modNam*   : FileNames.NameString;    (* name of the _MODULE_        *)
    basNam-,                             (* base name of source _FILE_  *)
    srcNam-,                             (* name of the source file     *)
    lstNam-   : FileNames.NameString;    (* name of the listing file    *)

    target-   : ARRAY 4 OF CHAR;

    cpSymX-,                             (* User supplied CPSYM name    *)
    binDir-,                             (* PE-file directory .NET only *)
    symDir-   : FileNames.NameString;    (* Symbol file directory       *)

    strict-,
    special-,
    warning-,
    verbose-,
    extras-,
    unsafe-,
    doStats-,
    doHelp-,
    ovfCheck-,
    debug-,
    doneHelp,
    doVersion-,
    doneVersion,
    doSym-,
    doAsm-,
    doJsmn-,
    forceIlasm,
    forcePerwapi,
    doIlasm-,
    doCode-,
    quiet-,
    system-    : BOOLEAN;
    legacy*    : BOOLEAN;
    netRel-,
    listLevel-,
    hashSize-  : INTEGER;

    thisMod-   : IdDesc.BlkId;           (* Desc. of compiling module.  *)
    sysMod-    : IdDesc.BlkId;           (* Desc. of compiling module.  *)

    impSeq*    : Symbols.ScpSeq;

    totalS*    : LONGINT;
    parseS*    : LONGINT;
    parseE*    : LONGINT;
    attrib*    : LONGINT;
    symEnd*    : LONGINT;
    asmEnd*    : LONGINT;
    totalE*    : LONGINT;
    import1*   : LONGINT;
    import2*   : LONGINT;

    impMax*    : INTEGER;
    
  VAR outNam*  : POINTER TO ARRAY OF CHAR;

  VAR
    expectedNet : BOOLEAN;         (* A .NET specific option was parsed *)
    expectedJvm : BOOLEAN;         (* A JVM specific option was parsed  *)

(* ==================================================================== *)
(*				Utilities				*)
(* ==================================================================== *)

    PROCEDURE SetQuiet*(); 
    BEGIN
      CPascalErrors.nowarn := TRUE;
    END SetQuiet;
    
    PROCEDURE RestoreQuiet*();
    BEGIN
      CPascalErrors.nowarn := ~warning;
    END RestoreQuiet;

    PROCEDURE targetIsNET*() : BOOLEAN;
    BEGIN
      RETURN target = "net";
    END targetIsNET;

    PROCEDURE targetIsJVM*() : BOOLEAN;
    BEGIN
      RETURN target = "jvm";
    END targetIsJVM;

    PROCEDURE Message*(IN mss : ARRAY OF CHAR);
    BEGIN
      Console.WriteString(prefix);
      Console.WriteString(mss);
      Console.WriteLn;
    END Message;

    PROCEDURE PrintLn*(IN mss : ARRAY OF CHAR);
    BEGIN
      Console.WriteString(mss);
      Console.WriteLn;
    END PrintLn;

    PROCEDURE ErrMesg*(IN mss : ARRAY OF CHAR);
    BEGIN
      Console.WriteString(prefix);
      Error.WriteString(mss);
      Error.WriteLn;
    END ErrMesg;

    PROCEDURE Abort*(IN mss : ARRAY OF CHAR);
    BEGIN
      ErrMesg(mss); ASSERT(FALSE);
    END Abort;

    PROCEDURE isForeign*() : BOOLEAN;
    BEGIN
      RETURN 
        (Symbols.rtsMd IN thisMod.xAttr) OR
        (Symbols.frnMd IN thisMod.xAttr);
    END isForeign;

    PROCEDURE TimeMsg*(IN mss : ARRAY OF CHAR; tim : LONGINT);
    BEGIN
      IF (tim < 0) OR (tim >= totalS) THEN tim := 0 END;
      Console.WriteString(prefix);
      Console.WriteString(mss);
      Console.WriteInt(SHORT(tim), 5);
      Console.WriteString(millis);
      Console.WriteLn;
    END TimeMsg;

(* ==================================================================== *)

    PROCEDURE Usage;
    BEGIN
      PrintLn("gardens point component pascal: " + GPCPcopyright.verStr);
      Message("Usage from the command line ...");
      IF RTS.defaultTarget = "net" THEN
PrintLn("       $ gpcp [cp-options] file {file}");
PrintLn("# CP Options ...");
PrintLn("       /bindir=XXX  ==> Place binary files in directory XXX");
PrintLn("       /copyright   ==> Display copyright notice");
PrintLn("       /cpsym=XXX   ==> Use environ. variable XXX instead of CPSYM");
PrintLn("       /debug       ==> Generate debugging information (default)");
PrintLn("       /nodebug     ==> Give up debugging for maximum speed");
PrintLn("       /dostats     ==> Give a statistical summary");
PrintLn("       /extras      ==> Enable experimental compiler features");
PrintLn("       /help        ==> Write out this usage message");
PrintLn("       /hsize=NNN   ==> Set hashtable size >= NNN (0 .. 65000)");
PrintLn("       /ilasm       ==> Force compilation via ILASM");
PrintLn("       /list        ==> (default) Create *.lst file if errors");
PrintLn("       /list+       ==> Unconditionally create *.lst file");
PrintLn("       /list-       ==> Don't create error *.lst file");
PrintLn("       /noasm       ==> Don't create asm (or object) files");
PrintLn("       /nocode      ==> Don't create any object files");
PrintLn("       /nocheck     ==> Don't perform arithmetic overflow checks");
PrintLn("       /nosym       ==> Don't create *.sym (or asm or object) files");
PrintLn("       /perwapi     ==> Force compilation via PERWAPI");
PrintLn("       /quiet       ==> Compile silently if possible");
PrintLn("       /strict      ==> Disallow non-standard constructs");
PrintLn("       /special     ==> Compile dummy symbol file");
PrintLn("       /symdir=XXX  ==> Place symbol files in directory XXX");
PrintLn("       /target=XXX  ==> Emit (jvm|net|dcf) assembly");
PrintLn("       /unsafe      ==> Allow unsafe code generation");
PrintLn("       /vX.X        ==> (v1.0 | v1.1 | v2.0) CLR target version");
PrintLn("       /verbose     ==> Emit verbose diagnostics");
PrintLn("       /version     ==> Write out version number");
PrintLn("       /vserror     ==> Print error messages in Visual Studio format");
PrintLn("       /warn-       ==> Don't emit warnings");
PrintLn("       /nowarn      ==> Don't emit warnings");
PrintLn("       /whidbey     ==> Target code for Whidbey Beta release");
PrintLn("       /xmlerror    ==> Print error messages in XML format");
PrintLn(' Unix-style options: "-option" are recognized also');
      ELSE
        IF RTS.defaultTarget = "jvm" THEN
PrintLn("       $ cprun gpcp [cp-options] file {file}, OR");
PrintLn("       $ java [java-options] CP.gpcp.gpcp [cp-options] file {file}");
        ELSIF RTS.defaultTarget = "dcf" THEN
          PrintLn("       $ gpcp [cp-options] file {file}");
	END;
PrintLn("# CP Options ...");
PrintLn("       -clsdir=XXX  ==> Set class tree root in directory XXX");
PrintLn("       -copyright   ==> Display copyright notice");
PrintLn("       -cpsym=XXX   ==> Use environ. variable XXX instead of CPSYM");
PrintLn("       -dostats     ==> Give a statistical summary");
PrintLn("       -extras      ==> Enable experimental compiler features");
PrintLn("       -help        ==> Write out this usage message");
PrintLn("       -hsize=NNN   ==> Set hashtable size >= NNN (0 .. 65000)");
PrintLn("       -jasmin      ==> Ceate asm files and run Jasmin");
PrintLn("       -list        ==> (default) Create *.lst file if errors");
PrintLn("       -list+       ==> Unconditionally create *.lst file");
PrintLn("       -list-       ==> Don't create error *.lst file");
PrintLn("       -nocode      ==> Don't create any object files");
PrintLn("       -noasm       ==> Don't create asm (or object) files");
PrintLn("       -nosym       ==> Don't create *.sym (or asm or object) files");
PrintLn("       -quiet       ==> Compile silently if possible");
PrintLn("       -special     ==> Compile dummy symbol file");
PrintLn("       -strict      ==> Disallow non-standard constructs");
PrintLn("       -symdir=XXX  ==> Place symbol files in directory XXX");
PrintLn("       -target=XXX  ==> Emit (jvm|net|dcf) assembly");
PrintLn("       -verbose     ==> Emit verbose diagnostics");
PrintLn("       -version     ==> Write out version number");
PrintLn("       -warn-       ==> Don't emit warnings");
PrintLn("       -nowarn      ==> Don't emit warnings");
PrintLn("       -xmlerror    ==> Print error messages in XML format");
        IF RTS.defaultTarget = "jvm" THEN
PrintLn("# Java Options ...");
PrintLn("       -D<name>=<value>  pass <value> to JRE as system property <name>");
PrintLn("       -DCPSYM=$CPSYM    pass value of CPSYM environment variable to JRE");
        END;
      END;
      Message("This program comes with NO WARRANTY");
      Message("Read source/GPCPcopyright for license details");
    END Usage;

(* ==================================================================== *)
(*			     Option Setting				*)
(* ==================================================================== *)

    PROCEDURE ParseOption*(IN opt : ARRAY OF CHAR);
      VAR copy : ARRAY 16 OF CHAR;
          indx : INTEGER;
     (* ----------------------------------------- *)
      PROCEDURE Unknown(IN str : ARRAY OF CHAR);
      BEGIN
        Message('Unknown option "' + str + '"');
        doHelp := TRUE;
      END Unknown;
     (* ----------------------------------------- *)
      PROCEDURE BadSize();
      BEGIN Message('hsize must be integer in range 0 .. 65000') END BadSize;
     (* ----------------------------------------- *)
      PROCEDURE ParseSize(IN opt : ARRAY OF CHAR);
        VAR ix : INTEGER;
            nm : INTEGER;
            ch : CHAR;
      BEGIN
        nm := 0;
        ix := 7;
        WHILE opt[ix] # 0X DO
          ch := opt[ix];
          IF (ch >= '0') & (ch <= '9') THEN
            nm := nm * 10 + ORD(ch) - ORD('0');
            IF nm > 65521 THEN BadSize; hashSize := nm; RETURN END;
          ELSE
            BadSize; doHelp := TRUE; hashSize := nm; RETURN;
          END;
          INC(ix);
        END;
        hashSize := nm;
      END ParseSize;
     (* ----------------------------------------- *)
      PROCEDURE GetSuffix(preLen : INTEGER;
                         IN  opt : ARRAY OF CHAR;
                         OUT dir : ARRAY OF CHAR);
        VAR idx : INTEGER;
            chr : CHAR;
      BEGIN
        idx := preLen;
        chr := opt[idx];
        WHILE (chr # 0X) & (idx < LEN(opt)) DO
          dir[idx - preLen] := chr;
          INC(idx); chr := opt[idx];
        END;
      END GetSuffix;
     (* ----------------------------------------- *)
      PROCEDURE StartsWith(str : ARRAY OF CHAR; IN pat : ARRAY OF CHAR) : BOOLEAN;
      BEGIN
        str[LEN(pat$)] := 0X;
        RETURN str = pat;
      END StartsWith;
     (* ----------------------------------------- *)
    BEGIN
      indx := 1;
      WHILE (indx < 16) & (indx < LEN(opt)) DO
        copy[indx-1] := opt[indx]; INC(indx);
      END;
      copy[15] := 0X;

      CASE copy[0] OF
      | "b" : 
          IF StartsWith(copy, "bindir=") THEN
            GetSuffix(LEN("/bindir="), opt, binDir);
            expectedNet := TRUE;
            IF ~quiet THEN 
              Message("bin directory set to <" + binDir +">");
            END;
          ELSE 
            Unknown(opt);
          END;
      | "c" : 
          IF copy = "copyright" THEN 
            GPCPcopyright.Write;
          ELSIF StartsWith(copy, "clsdir=") THEN
            GetSuffix(LEN("/clsdir="), opt, binDir);
            expectedJvm := TRUE;
            IF ~quiet THEN 
              Message("output class tree rooted at <" + binDir +">");
            END;
          ELSIF StartsWith(copy, "cpsym=") THEN
            GetSuffix(LEN("/cpsym="), opt, cpSymX);
            IF ~quiet THEN 
              Message("using %" + cpSymX +"% as symbol file path");
            END;
          ELSE
            Unknown(opt);
          END;
      | "d" : 
          IF copy = "dostats" THEN 
            doStats := TRUE;
          ELSIF copy = "debug" THEN
            debug := TRUE;
            expectedNet := TRUE;
          ELSE 
            Unknown(opt);
          END;
      | "e" : IF copy = "extras" THEN extras := TRUE ELSE Unknown(opt) END;
      | "h" : 
          copy[6] := 0X;
          IF copy = "help" THEN
            doHelp := TRUE;
          ELSIF copy = "hsize=" THEN
            ParseSize(opt);
          ELSE
            Unknown(opt);
          END;
      | "i" : 
          IF copy = "ilasm" THEN 
            forceIlasm := TRUE;
            expectedNet := TRUE;
          ELSE 
            Unknown(opt);
          END;
      | "j" :
          IF copy = "jasmin" THEN
            doCode     := TRUE;
            doJsmn     := TRUE;
            expectedJvm := TRUE;
          ELSE 
            Unknown(opt);
          END;
      | "l" :
          IF copy = "list-" THEN
            listLevel  := CPascalS.listNever;
          ELSIF copy = "list+" THEN
            listLevel  := CPascalS.listAlways;
          ELSIF copy = "list" THEN
            listLevel  := CPascalS.listErrOnly;
          ELSIF copy = "legacy" THEN
            legacy := TRUE;
          ELSE 
            Unknown(opt);
          END;
      | "n" :
          IF copy = "nosym" THEN
            doSym      := FALSE;
            doAsm      := FALSE;
            doCode     := FALSE;
          ELSIF copy = "noasm" THEN
            doAsm      := FALSE;
            doCode     := FALSE;
          ELSIF copy = "nocode" THEN
            doCode     := FALSE;
          ELSIF copy = "nowarn" THEN
            warning    := FALSE;
            CPascalErrors.nowarn := TRUE;
          ELSIF copy = "nocheck" THEN
            ovfCheck   := FALSE;
            expectedNet := TRUE;
          ELSIF copy = "nodebug" THEN
            debug    := FALSE;
            expectedNet := TRUE;
          ELSE 
            Unknown(opt);
          END;
          | "p" :
          IF copy = "perwapi" THEN
            forcePerwapi := TRUE;
            expectedNet := TRUE;
          ELSE
            Unknown(opt);
          END;
      | "q" :
          IF copy = "quiet" THEN
            quiet := TRUE;
            warning := FALSE;
          ELSE
            Unknown(opt);
          END;
      | "s" :
          IF copy = "special" THEN
            doAsm      := FALSE;
            special    := TRUE;
            strict     := FALSE;
          ELSIF copy = "strict" THEN
            strict     := TRUE;
          ELSIF StartsWith(copy, "symdir=") THEN
            GetSuffix(LEN("/symdir="), opt, symDir);
            IF ~quiet THEN 
              Message("sym directory set to <" + symDir +">");
            END;
          ELSE
            Unknown(opt);
          END;
      | "t" :
          IF (copy = "target=jvm") OR
             (copy = "target=JVM") THEN
            IF RTS.defaultTarget = "jvm" THEN
              Message("JVM is default target for this build");
            END;
            target := "jvm";
          ELSIF (copy = "target=vos") OR
                (copy = "target=net") OR
                (copy = "target=NET") THEN
            IF RTS.defaultTarget = "net" THEN
              Message("NET is default target for this build");
            END;
            target := "net";
          ELSIF copy = "target=dcf" THEN
            Message('DCode emitter not yet available, using "target=' +
                                                    RTS.defaultTarget + '"');
          ELSE 
            Unknown(opt);
          END;
      | "u" :
          IF copy = "unsafe" THEN
            unsafe := TRUE;
            expectedNet := TRUE;
          ELSE 
            Unknown(opt);
          END;
      | "v" :
          IF copy = "version" THEN
            doVersion  := TRUE;
          ELSIF copy = "verbose" THEN
            quiet      := FALSE;
            warning    := TRUE;
            verbose    := TRUE;
            doStats    := TRUE;
            CPascalErrors.prompt := TRUE;
          ELSIF copy = "vserror" THEN
            CPascalErrors.forVisualStudio := TRUE;
            expectedNet := TRUE;
          ELSIF copy = "v1.0" THEN
            netRel := netV1_0;
            expectedNet := TRUE;
          ELSIF copy = "v1.1" THEN
            netRel := netV1_1;
            expectedNet := TRUE;
          ELSIF copy = "v2.0" THEN
            netRel := netV2_0;
            expectedNet := TRUE;
          ELSE 
            Unknown(opt);
          END;
      | "w" :
          IF copy = "warn-" THEN
            warning    := FALSE;
            CPascalErrors.nowarn := TRUE;
          ELSIF copy = "whidbey" THEN
            netRel := netV2_0;
            expectedNet := TRUE;
          ELSE 
            Unknown(opt);
          END;
      | "x" :
          IF copy = "xmlerror" THEN
            CPascalErrors.xmlErrors := TRUE;
          ELSE 
            Unknown(opt);
          END;
      ELSE
        Unknown(opt);
      END;
      IF doVersion & ~doneVersion THEN 
        Message(target + GPCPcopyright.verStr); 
        doneVersion := TRUE;
      END;
      IF doHelp & ~doneHelp THEN Usage; doneHelp := TRUE END;
    END ParseOption;

(* ==================================================================== *)

    PROCEDURE CheckOptionsOK*;
    BEGIN
      IF target = "net" THEN
        IF expectedJvm THEN Message
          ("WARNING - a JVM-specific option was specified for .NET target");
          expectedJvm := FALSE;
        END;
      ELSIF target = "jvm" THEN
        IF expectedNet THEN Message
          ("WARNING - a .NET-specific option was specified for JVM target");
          expectedNet := FALSE;
        END;
      END;
     (* 
      *  If debug is set, for this version, ILASM is used unless /perwapi is explicit
      *  If debug is clar, for this versin, PERWAPI is used unless /ilasm is explicit
      *)
      IF forceIlasm THEN      doIlasm := TRUE;
      ELSIF forcePerwapi THEN doIlasm := FALSE;
      ELSE                    doIlasm := debug;
      END;
    END CheckOptionsOK;

(* ==================================================================== *)

    PROCEDURE CreateThisMod*();
    BEGIN
      NEW(thisMod); 
      thisMod.SetKind(IdDesc.modId);
      thisMod.ovfChk := ovfCheck;
    END CreateThisMod;

    PROCEDURE InitCompState*(IN nam : ARRAY OF CHAR);
    BEGIN
      IF verbose THEN Message("opened local file <" + nam + ">") END;
      GPText.Assign(nam, srcNam);
      CPascalErrors.SetSrcNam(nam);
      FileNames.StripExt(nam, basNam);
      FileNames.AppendExt(basNam, "lst", lstNam);

      CreateThisMod;
      xhrId := IdDesc.newFldId();
      xhrId.hash := NameHash.enterStr("prev");
      srcBkt     := NameHash.enterStr("src");
      corBkt     := NameHash.enterStr("mscorlib_System");

      NEW(sysMod); 
      sysMod.SetKind(IdDesc.impId);
    END InitCompState;

(* ==================================================================== *)

  PROCEDURE Report*;
    VAR str1 : ARRAY 8 OF CHAR;
        str2 : ARRAY 8 OF CHAR;
  BEGIN
    Message(target + GPCPcopyright.verStr); 
    GPText.IntToStr(CPascalS.line, str1);
    Message(str1 + " source lines");
    GPText.IntToStr(impMax, str1);
    Message("import recursion depth " + str1);
    GPText.IntToStr(NameHash.size, str2);
    GPText.IntToStr(NameHash.entries, str1);
    Message(str1 + " entries in hashtable of size " + str2);
    TimeMsg("import time   ", import2 - import1);
    TimeMsg("source time   ", parseS  - totalS);
    TimeMsg("parse time    ", parseE  - parseS - import2 + import1);
    TimeMsg("analysis time ", attrib  - parseE);
    TimeMsg("symWrite time ", symEnd  - attrib);
    TimeMsg("asmWrite time ", asmEnd  - symEnd);
    TimeMsg("assemble time ", totalE  - asmEnd);
    TimeMsg("total time    ", totalE  - totalS);
  END Report;

(* ==================================================================== *)

  PROCEDURE InitOptions*;
  BEGIN
    legacy      := FALSE;
    warning     := TRUE;
    verbose     := FALSE;
    doHelp      := FALSE; doneHelp    := FALSE;
    doVersion   := FALSE; doneVersion := FALSE;
    ovfCheck    := TRUE;
    debug       := TRUE;
    netRel      := netV2_0; (* probably should be from RTS? *)
    doSym       := TRUE;
    extras      := FALSE;
    unsafe      := FALSE;
    doStats     := FALSE;
    doJsmn      := FALSE;
    doIlasm     := TRUE;
    forceIlasm  := FALSE;
    forcePerwapi := FALSE;
    doCode      := TRUE;
    doAsm       := TRUE;
    special     := FALSE;
    strict      := FALSE;
    quiet       := FALSE;
    system      := FALSE;
    listLevel   := CPascalS.listErrOnly;
    hashSize    := 5000;        (* gets default hash size *)
    expectedNet := FALSE;
    expectedJvm := FALSE;
    cpSymX      := "CPSYM";
  END InitOptions;

(* ==================================================================== *)
BEGIN
  GPText.Assign(RTS.defaultTarget, target);
END CompState.
(* ==================================================================== *)

