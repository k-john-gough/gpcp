
(* ================================================================ *)
(*                                                                  *)
(*  Module of the V1.4+ gpcp tool to create symbol files from       *)
(*  the metadata of .NET assemblies, using System.Reflection API.   *)
(*                                                                  *)
(*  Copyright QUT 2004 - 2005, K John Gough 2004 - 2018.            *)
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
     IdDesc,
   (*
    *  Util := PeToCpsUtils_, only needed while bootstrapping v1.4.05
    *)
     Sys := "[mscorlib]System",
     SysRfl := "[mscorlib]System.Reflection";

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

  PROCEDURE GetVersionInfo(asm : SysRfl.Assembly; 
                       OUT inf : POINTER TO ARRAY OF INTEGER);
    VAR   asmNam : SysRfl.AssemblyName;
          sysVer : Sys.Version;
          kToken : POINTER TO ARRAY OF UBYTE;

  BEGIN [UNCHECKED_ARITHMETIC]
    asmNam := asm.GetName();
    sysVer := asmNam.get_Version();
    kToken := asmNam.GetPublicKeyToken();
    IF (sysVer.get_Major() # 0) & (kToken # NIL) & (LEN(kToken) >= 8) THEN 
      NEW(inf, 6); 
      inf[4] := (((kToken[0] * 256 + kToken[1]) * 256 + kToken[2]) *256 + kToken[3]);
      inf[5] := (((kToken[4] * 256 + kToken[5]) * 256 + kToken[6]) *256 + kToken[7]);

      inf[0] := sysVer.get_Major();
      inf[1] := sysVer.get_Minor();
      inf[2] := sysVer.get_Revision();
      inf[3] := sysVer.get_Build();
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

  PROCEDURE GetAssembly(IN nam : ARRAY OF CHAR) : SysRfl.Assembly;
    VAR basS : ArgS;
  BEGIN
    Glb.CondMsg(" Reading PE file " + nam);
    FileNames.StripExt(nam, basS);
    IF (basS = "mscorlib") THEN
      Glb.AbortMsg("Cannot load mscorlib, use the /mscorlib option instead")
    END;
    Glb.GlobInit(nam, basS);
    RETURN SysRfl.Assembly.(*ReflectionOnly*)LoadFrom(MKSTR(nam));
  END GetAssembly;

  PROCEDURE GetMscorlib() : SysRfl.Assembly;
    VAR objTp : RTS.NativeType;
  BEGIN
    Glb.CondMsg(" Reflecting loaded mscorlib assembly");
    Glb.GlobInit("mscorlib.dll", "mscorlib" );
    objTp := TYPEOF(RTS.NativeObject);
    RETURN SysRfl.Assembly.GetAssembly(objTp)
  END GetMscorlib;

(* ==================================================================== *
 * PROCEDURE Process(IN  nam : ARRAY OF CHAR;
 *                   OUT rVl : INTEGER);       (* return value *)
 * ==================================================================== *)

  PROCEDURE Process(assm : SysRfl.Assembly;
                OUT rtVl : INTEGER);       (* return value *)
    VAR indx : INTEGER;
        nSpc : VECTOR OF C2T.DefNamespace;

        vrsn : POINTER TO ARRAY OF INTEGER;
      expTps : POINTER TO ARRAY OF Sys.Type;
      vecTps : VECTOR OF Sys.Type;
      asmRfs : POINTER TO ARRAY OF SysRfl.AssemblyName;

  BEGIN
    rtVl := 0;
    expTps := assm.GetExportedTypes();
   (*
    asmRfs := Util.Utils.GetDependencies(assm);
    *
    *)
    asmRfs := assm.GetReferencedAssemblies();

    IF ~Glb.isCorLib THEN C2T.InitCorLibTypes() END;

    Glb.CondMsg(" Processing PE file");
   (*
    *  Classify allocates a new DefNamspace object for each
    *  namespace. Each object is decorated with an IdDesc.BlkId
    *  module descriptor, a vector of System.Type objects and 
    *  another of IdDesc.TypId objects.
    *  Classes on the expTps list are added to the vector of
    *  the corresponding namespace. For each such Type object
    *  a TypId object is created and inserted in the TypId
    *  vector. Each such TypId is inserted into the symbol 
	*  table of the BlkId describing that namespace.
    *)  
    C2T.Classify(expTps, nSpc);
   (*
    *  If the assembly has version/strongname info
    *  this is propagaed to each namespace of nSpc.
    *)
    GetVersionInfo(assm, vrsn);
    FOR indx := 0 TO LEN(nSpc) - 1 DO
      CopyVersionInfo(vrsn, nSpc[indx].bloc);
    END;
   (*
    *  Each namespace is traversed and an object of an appropriate 
    *  subtype of Symbols.Type is assigned to the TypId.type field.
    *  For the structured types the details are added later.
    *)
    FOR indx := 0 TO LEN(nSpc) - 1 DO
      IF ~Glb.isCorLib THEN C2T.ImportCorlib(nSpc[indx]) END;
      C2T.AddTypesToIds(nSpc[indx]); 
    END;
    IF Glb.isCorLib THEN C2T.BindSystemTypes() END;
   (*
    *  The structure of each TypId.type field is now elaborated.
    *  For record types it is only now that base-type, methods
    *  and static features are added.
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
    rtVl := 4;
  END Process;

(* ==================================================================== *)
(*                            Main Argument Loop                        *)
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
    ELSIF Glb.isCorLib THEN
      Glb.Message("Filename arguments not allowed with /mscorlib");
      Glb.AbortMsg("Rest of arguments will be skipped");
    ELSE  
      INC(filN); 
      timS := RTS.GetMillis();
      Process(GetAssembly(argS$), errs);
      timE := RTS.GetMillis();
      IF errs = 0 THEN INC(okNm) END;
      Glb.Report(argS$, resStr(errs), timE - timS);
    END;
  END;
  IF Glb.isCorLib THEN
    timS := RTS.GetMillis();    
    Process(GetMscorlib(), errs);
    timE := RTS.GetMillis();
    Glb.Report(argS$, resStr(errs), timE - timS);
  END;
  Glb.Summary(filN, okNm, timE - tim0);
 (*
  *  Return the result code of the final compilation
  *)
  IF errs # 0 THEN HALT(1) END;
END PeToCps.
