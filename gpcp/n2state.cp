
(* ================================================================ *)
(*                                                                  *)
(*  Module of the V1.4+ gpcp tool to create symbol files from       *)
(*  the metadata of .NET assemblies, using the PERWAPI interface.   *)
(*                                                                  *)
(*  Copyright QUT 2004 - 2005.                                      *)
(*                                                                  *)
(*  This code released under the terms of the GPCP licence.         *)
(*                                                                  *)
(*  This Module:   <N2State>                                        *)
(*       Holds global state for the process, plus utilities.        *)
(*       Original module, kjg December 2004                         *)
(*                                                                  *)
(* ================================================================ *)

MODULE N2State;
  IMPORT GPCPcopyright, 
         CompState,
         LitValue,
         ProgArgs,
         Console, 
         Error,
         GPText,
         ForeignName,
         RW := NewSymFileRW,
         Id := IdDesc,
         Ty := TypeDesc,
         Sy := Symbols,
         Nh := NameHash,
         Sys := "[mscorlib]System",
         RTS; 

 (* ---------------------------------------------------------- *)

  CONST prefix = "PeToCps: ";
        abtMsg = " ... Aborting";
        usgMsg = 'Usage: "PeToCps [options] filenames"';

 (* ---------------------------------------------------------- *)

  TYPE  CharOpen* = POINTER TO ARRAY OF CHAR;

 (* ---------------------------------------------------------- *)

  TYPE  CleanDump = POINTER TO RECORD (Sy.SymForAll) END;

 (* ---------------------------------------------------------- *)

  VAR   netDflt-  : BOOLEAN;
        verbose-  : BOOLEAN;
        Verbose-  : BOOLEAN;
        superVb-  : BOOLEAN;
        generics- : BOOLEAN;
        legacy-   : BOOLEAN;
        cpCmpld-  : BOOLEAN;

 (* ---------------------------------------------------------- *)

  VAR   thisMod-  : Id.BlkId;
        isCorLib- : BOOLEAN;
        hashSize- : INTEGER;
        ctorBkt-  : INTEGER;
        initBkt-  : INTEGER;
        srcNam-   : CharOpen;
        basNam-   : CharOpen;
        impSeq*   : Sy.ScpSeq;
        typSeq-   : Sy.TypeSeq;

 (* ---------------------------------------------------------- *)

  PROCEDURE^ AbortMsg*(IN str : ARRAY OF CHAR);
  PROCEDURE^ Message*(IN str : ARRAY OF CHAR);

 (* ---------------------------------------------------------- *)

  PROCEDURE ListTy*(ty : Sy.Type);
  BEGIN
    Sy.AppendType(typSeq, ty);
  END ListTy;

 (* ---------------------------------------------------------- *)

  PROCEDURE AddMangledNames(mod : Id.BlkId; asm, nms : CharOpen);
  BEGIN
    mod.hash    := Nh.enterStr(ForeignName.MangledName(asm, nms));
    mod.scopeNm := ForeignName.QuotedName(asm, nms);
  END AddMangledNames;

 (* ---------------------------------------------------------- *)

  PROCEDURE GlobInit*(IN src, bas : ARRAY OF CHAR);
  BEGIN
    Nh.InitNameHash(hashSize);
    srcNam := BOX(src$);
    basNam := BOX(bas$);
    isCorLib := (bas = "mscorlib");

    CompState.CreateThisMod;
    thisMod := CompState.thisMod;

    Sy.ResetScpSeq(impSeq);
    ctorBkt := Nh.enterStr(".ctor");
    initBkt := Nh.enterStr("init");
  END GlobInit;

 (* ------------------------------------- *)

  PROCEDURE BlkIdInit*(blk : Id.BlkId; asm, nms : CharOpen);
  BEGIN
    blk.SetKind(Id.impId);
    AddMangledNames(blk, asm, nms);
    IF Sy.refused(blk, thisMod) THEN 
      AbortMsg("BlkId insert failure -- " + Nh.charOpenOfHash(blk.hash)^);
    END;
    Sy.AppendScope(impSeq, blk)
  END BlkIdInit;

 (* ------------------------------------- *)

  PROCEDURE InsertImport*(blk : Id.BlkId);
  BEGIN
    IF Sy.refused(blk, thisMod) THEN 
      AbortMsg("BlkId insert failure in <thisMod>");
    END;
    Sy.AppendScope(impSeq, blk)
  END InsertImport;

 (* ------------------------------------- *)

  PROCEDURE (dmpr : CleanDump)Op*(id : Sy.Idnt);
  BEGIN
    WITH id : Id.TypId DO
      IF id.type.dump >= Sy.tOffset THEN id.type.dump := 0 END;
    ELSE
    END;
  END Op;

 (* ------------------------------------- *)

  PROCEDURE ResetBlkIdFlags*(mod : Id.BlkId);
    VAR indx : INTEGER;
        impB : Sy.Scope;
        dmpr : CleanDump;
        typI : Sy.Type;
  BEGIN
   (*
    *   Clear the "dump" marker from non built-in types
    *)
    FOR indx := 0 TO typSeq.tide - 1 DO
      typI := typSeq.a[indx];
      IF typI.dump >= Sy.tOffset THEN 
        typI.dump := 0;
        typI.force := Sy.noEmit;
      END;
    END;
    mod.SetKind(Id.modId);

    IF superVb THEN
      Message("Preparing symfile <" + Nh.charOpenOfHash(mod.hash)^ + ">");
    END;
    FOR indx := 0 TO impSeq.tide - 1 DO
      impB := impSeq.a[indx];
      IF impB # mod THEN
        impB.SetKind(Id.impId);
      END;
    END;
  END ResetBlkIdFlags;

 (* ---------------------------------------------------------- *)

  PROCEDURE WLn(IN str : ARRAY OF CHAR);
  BEGIN
    Console.WriteString(str); Console.WriteLn;
  END WLn;

  PROCEDURE Message*(IN str : ARRAY OF CHAR);
  BEGIN
    Console.WriteString(prefix);
    Console.WriteString(str);
    Console.WriteLn;
  END Message;

  PROCEDURE CondMsg*(IN str : ARRAY OF CHAR);
  BEGIN
    IF verbose THEN Message(str) END;
  END CondMsg;

  PROCEDURE AbortMsg*(IN str : ARRAY OF CHAR);
  BEGIN
    Error.WriteString(prefix);
    Error.WriteString(str);
    Error.WriteLn;
    HALT(1); 
  END AbortMsg;
    
  PROCEDURE Usage();
  BEGIN
    Message(usgMsg); 
    Message("filenames should have explicit .EXE or .DLL extension"); 
    IF netDflt THEN
      WLn("Options: /big       ==> allocate huge hash table");
      WLn("         /copyright ==> display copyright notice");
      WLn("         /generics  ==> enable CLI v2.0 generics");
      WLn("         /help      ==> display this message");
      WLn("         /legacy    ==> produce compatible symbol file");
      WLn("         /verbose   ==> chatter on about progress"); 
      WLn("         /Verbose   ==> go on and on and on about progress"); 
    ELSE
      WLn("Options: -big       ==> allocate huge hash table");
      WLn("         -copyright ==> display copyright notice");
      WLn("         -generics  ==> enable CLI v2.0 generics");
      WLn("         -help      ==> display this message");
      WLn("         -legacy    ==> produce compatible symbol file");
      WLn("         -verbose   ==> chatter on about progress"); 
      WLn("         -Verbose   ==> go on and on and on about progress"); 
    END;
  END Usage;

 (* ---------------------------------------------------------- *)

  PROCEDURE ReportTim(tim : LONGINT);
    CONST millis = " mSec";
  BEGIN
    Console.WriteInt(SHORT(tim), 0);
    Console.WriteString(millis);
  END ReportTim;

  PROCEDURE Report*(IN nam, res : ARRAY OF CHAR; tim : LONGINT);
  BEGIN
    Console.WriteString(prefix);
    Console.WriteString(" Input file <");
    Console.WriteString(nam);
    Console.WriteString("> ");
    Console.WriteString(res);
    IF verbose THEN
      Console.WriteString(", time: ");
      ReportTim(tim);
    END;
    Console.WriteLn;
  END Report;

  PROCEDURE Summary*(flNm, okNm : INTEGER; tim : LONGINT);
    CONST sumPre = " Summary: ";
  BEGIN
    Console.WriteString(prefix);
    Console.WriteString(sumPre);
    IF flNm = 0 THEN
      Console.WriteString(" No input files specified");
    ELSE
      Console.WriteInt(flNm,1); 
      Console.WriteString(" input files");
      IF okNm < flNm THEN
        Console.WriteInt(flNm - okNm, 0); 
        Console.WriteString(" failed");
      END;
      IF verbose THEN
        Console.WriteLn;
        Console.WriteString(prefix);
        Console.WriteString(sumPre);
        Console.WriteString("Total elapsed time: ");
        ReportTim(tim);
      END;
    END;
    Console.WriteLn;
  END Summary;

 (* ---------------------------------------------------------- *)

  PROCEDURE ParseOption*(IN arg : ARRAY OF CHAR);
  BEGIN
    IF    arg = "-big" THEN
        hashSize := 40000;
    ELSIF arg = "-verbose" THEN
        verbose := TRUE;
        Verbose := FALSE;
        superVb := FALSE;
    ELSIF arg = "-Verbose" THEN
        verbose := TRUE;
        Verbose := TRUE;
        superVb := FALSE;
    ELSIF arg = "-generics" THEN
        generics := TRUE;
    ELSIF arg = "-legacy" THEN
        legacy := TRUE;
        CompState.legacy := TRUE;
    ELSIF arg = "-VERBOSE" THEN
        verbose := TRUE;
        Verbose := TRUE;
        superVb := TRUE;
    ELSIF arg = "-help" THEN
        Usage();
    ELSIF arg = "-copyright" THEN
        GPCPcopyright.Write();
    ELSE  
        Message("Bad Option " + arg); Usage;
    END;
  END ParseOption;

 (* ---------------------------------------------------------- *)

  PROCEDURE EmitSymbolfile*(blk : Id.BlkId);
  BEGIN
    RW.EmitSymfile(blk);
    Message(" Output file <" +
            Nh.charOpenOfHash(blk.hash)^ + 
            ".cps> created");
  END EmitSymbolfile;

 (* ---------------------------------------------------------- *)

BEGIN
  netDflt  := (RTS.defaultTarget = "net");
  generics := FALSE;
  verbose  := FALSE;
  Verbose  := FALSE;
  superVb  := FALSE;
  legacy   := FALSE;
  cpCmpld  := FALSE; (* pending the custom attribute *)
  hashSize := 5000;
  Sy.InitScpSeq(impSeq, 10);
  Sy.InitTypeSeq(typSeq, 10);
  CompState.ParseOption("/special");
END N2State.
