
(* ================================================================ *)
(*                                                                  *)
(*  Module of the V1.4+ gpcp tool to create symbol files from       *)
(*  the metadata of .NET assemblies, using the PERWAPI interface.   *)
(*                                                                  *)
(*  Copyright QUT 2004 - 2005.                                      *)
(*                                                                  *)
(*  This code released under the terms of the GPCP licence.         *)
(*                                                                  *)
(*  This Module:   <PeToCps>                                        *)
(*       Base module. Command line processing etcetera.             *)
(*       Original module, kjg December 2004                         *)
(*                                                                  *)
(* ================================================================ *)

MODULE PeToCps;
  IMPORT CPmain, RTS, GPCPcopyright, ProgArgs, 
     GPFiles,
     FileNames,
     Glb := N2State,
     C2T := ClsToType,
     Per := "[QUT.PERWAPI]QUT.PERWAPI",
     Sys := "[mscorlib]System",
     IdDesc;

  TYPE
    ArgS = ARRAY 256 OF CHAR;

  VAR
    chr0 : CHAR;
    argN : INTEGER;
    filN : INTEGER;
    okNm : INTEGER;
    errs : INTEGER;
    tim0 : LONGINT;
    timS : LONGINT;
    timE : LONGINT;
    argS : ArgS;
    resS : Glb.CharOpen;

(* ==================================================================== *)

  PROCEDURE resStr(res : INTEGER) : Glb.CharOpen;
    VAR tmp : Glb.CharOpen;
  BEGIN
    CASE res OF
    | 0 : tmp := BOX("succeeded");
    | 1 : tmp := BOX("input not found");
    | 2 : tmp := BOX("output not created");
    | 3 : tmp := BOX("failed");
    | 4 : tmp := BOX("error <" + resS^ + ">");
    END;
    RETURN tmp;
  END resStr;

(* ------------------------------------------------------- *)

  PROCEDURE ExceptionName(x : RTS.NativeException) : Glb.CharOpen;
    VAR ptr : Glb.CharOpen;
        idx : INTEGER;
  BEGIN
    ptr := RTS.getStr(x);
    FOR idx := 0 TO LEN(ptr^) - 1 DO
      IF ptr[idx] <= " " THEN ptr[idx] := 0X; RETURN ptr END;
    END;
    RETURN ptr;
  END ExceptionName;

(* ------------------------------------------------------- *)

  PROCEDURE GetVersionInfo(pef : Per.PEFile; 
                       OUT inf : POINTER TO ARRAY OF INTEGER);
    CONST tag = "PublicKeyToken=";
    VAR   asm : Per.Assembly;
          str : Sys.String;
          arr : Glb.CharOpen;
          idx : INTEGER;
          tok : LONGINT;
  BEGIN
    asm := pef.GetThisAssembly();
    IF (asm.MajorVersion() # 0) & (LEN(asm.Key()) > 0) THEN
      NEW(inf, 6);
      tok := asm.KeyTokenAsLong(); 
      inf[4] := RTS.hiInt(tok);
      inf[5] := RTS.loInt(tok);

      inf[0] := asm.MajorVersion();
      inf[1] := asm.MinorVersion();
      inf[2] := asm.BuildNumber();
      inf[3] := asm.RevisionNumber();

    ELSE
      inf := NIL;
    END;
  END GetVersionInfo;

(* ------------------------------------------------------- *)

  PROCEDURE CopyVersionInfo(inf : POINTER TO ARRAY OF INTEGER;
                            blk : IdDesc.BlkId);
    VAR ix : INTEGER;
  BEGIN
    IF inf # NIL THEN
      NEW(blk.verNm);
      FOR ix := 0 TO 5 DO
        blk.verNm[ix] := inf[ix];
      END;
    END;
  END CopyVersionInfo;

(* ==================================================================== *)

  PROCEDURE Process(IN  nam : ARRAY OF CHAR;
                    OUT rVl : INTEGER);       (* return value *)
    VAR peFl : Per.PEFile;
        clss : POINTER TO ARRAY OF Per.ClassDef;
        indx : INTEGER;
        nSpc : VECTOR OF C2T.DefNamespace;
        basS : ArgS;
        vrsn : POINTER TO ARRAY OF INTEGER;
  BEGIN
    rVl := 0;
    FileNames.StripExt(nam, basS);

    Glb.CondMsg(" Reading PE file");
    peFl := Per.PEFile.ReadPublicClasses(MKSTR(nam));

    Glb.GlobInit(nam, basS);

    IF ~Glb.isCorLib THEN C2T.InitCorLibTypes() END;

    Glb.CondMsg(" Processing PE file");
    clss := peFl.GetClasses();
    C2T.Classify(clss, nSpc);
   (*
    *  Define BlkId for every namespace
    *)
    GetVersionInfo(peFl, vrsn);
    FOR indx := 0 TO LEN(nSpc) - 1 DO
      C2T.MakeBlkId(nSpc[indx], Glb.basNam);
      CopyVersionInfo(vrsn, nSpc[indx].bloc);
    END;

   (*
    *  Define TypIds in every namespace
    *)
    FOR indx := 0 TO LEN(nSpc) - 1 DO
      IF ~Glb.isCorLib THEN C2T.ImportCorlib(nSpc[indx]) END;
      C2T.MakeTypIds(nSpc[indx]); 
    END;
    IF Glb.isCorLib THEN C2T.BindSystemTypes() END;
   (*
    *  Define structure of every class
    *)
    FOR indx := 0 TO LEN(nSpc) - 1 DO
      C2T.DefineClss(nSpc[indx]); 
    END;
   (*
    *  Write out symbol file(s)
    *)
    FOR indx := 0 TO LEN(nSpc) - 1 DO
      Glb.ResetBlkIdFlags(nSpc[indx].bloc);
      Glb.EmitSymbolfile(nSpc[indx].bloc);
    END;
    Glb.CondMsg(" Completing normally");
  RESCUE (sysX)
    resS := ExceptionName(sysX);
    Glb.Message(" " + resS^);
    Glb.Message(" " + RTS.getStr(sysX)^);
    rVl := 4;
  END Process;

(* ==================================================================== *)
(*			      Main Argument Loop			*)
(* ==================================================================== *)

BEGIN
  filN := 0;
  tim0 := RTS.GetMillis();
  Glb.Message(GPCPcopyright.verStr);
  FOR argN := 0 TO ProgArgs.ArgNumber()-1 DO
    ProgArgs.GetArg(argN, argS);
    chr0 := argS[0];
    IF (chr0 = '-') OR (chr0 = GPFiles.optChar) THEN (* option string *)
      argS[0] := "-";
      Glb.ParseOption(argS$);
    ELSE
      timS := RTS.GetMillis();
      Process(argS$, errs); 
      INC(filN); 
      IF errs = 0 THEN INC(okNm) END;
      timE := RTS.GetMillis();

      Glb.Report(argS$, resStr(errs), timE - timS);
    END;
  END;
  Glb.Summary(filN, okNm, timE - tim0);
 (*
  *  Return the result code of the final compilation
  *)
  IF errs # 0 THEN HALT(1) END;
END PeToCps.
