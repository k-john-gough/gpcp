(* ==================================================================== *)
(*                                                                      *)
(*  Main Module for the Gardens Point Component Pascal Compiler.        *)
(*      Copyright (c) John Gough 1999, 2000.                            *)
(*      This module was extensively modified from the driver            *)
(*      automatically produced by the M2 version of COCO/R, using       *)
(*      the CPascal.atg grammar used for the JVM version of GPCP.       *)
(*                                                                      *)
(* ==================================================================== *)

MODULE CPascal;
(* This is an example of a rudimentary main module for use with COCO/R.
   The auxiliary modules <Grammar>S (scanner) and <Grammar>P (parser)
   are assumed to have been constructed with COCO/R compiler generator. *)

  IMPORT
        GPCPcopyright,
        Symbols,
        RTS,
        FileNames,
        IdDesc,
        Error,
        Console,
        ProgArgs,
        CSt := CompState,
        CPascalP,
        Scnr := CPascalS,
        CPascalErrors,
        New := NewSymFileRW,
        Old := OldSymFileRW,
        NameHash,
        Visitor,
        Builtin,
        GPText,
        Target,
        TxtFil := GPTextFiles,
        BinFil := GPBinFiles;

(* ==================================================================== *)
(*                         Option Setting                               *)
(* ==================================================================== *)

    PROCEDURE ResetOptions*;
    BEGIN
      CSt.InitOptions;
    END ResetOptions;

    (* -------------------------- *)

    PROCEDURE Message*(IN msg : ARRAY OF CHAR);
    BEGIN
      CSt.Message(msg);
    END Message;

    (* -------------------------- *)

    PROCEDURE DoOption*(IN opt : ARRAY OF CHAR);
    BEGIN
      CSt.ParseOption(opt);
    END DoOption;

    (* -------------------------- *)

    PROCEDURE CondMsg(IN msg : ARRAY OF CHAR);
    BEGIN
      IF CSt.verbose THEN CSt.Message(msg) END;
    END CondMsg;

(* ==================================================================== *)
(*                    Calling the Compiler                              *)
(* ==================================================================== *)

    PROCEDURE Finalize*;
      VAR a : ARRAY 16 OF CHAR;
          b : ARRAY 256 OF CHAR;
    BEGIN
      IF CPascalErrors.forVisualStudio OR
         CPascalErrors.xmlErrors THEN RETURN END;
      b := "<" + CompState.modNam + ">";
      IF Scnr.errors = 0 THEN
        b := (b + " No errors");
      ELSIF Scnr.errors = 1 THEN
        b := (b + " There was one error");
      ELSE
        GPText.IntToStr(Scnr.errors, a);
        b := (b + " There were " + a + " errors");
      END;
      IF Scnr.warnings = 1 THEN
        b := (b + ", and one warning");
      ELSIF Scnr.warnings > 1 THEN
        GPText.IntToStr(Scnr.warnings, a);
        b := (b + ", and " + a + " warnings");
      END;
      IF ~CSt.quiet THEN CSt.Message(b) END;
    END Finalize;

(* ==================================================================== *)

    PROCEDURE FixListing*;
      VAR doList : BOOLEAN;
          events : INTEGER;
    BEGIN
      doList := (CSt.listLevel > Scnr.listNever);
      events := Scnr.errors;
      IF CSt.warning THEN INC(events, Scnr.warnings) END;
      IF (events > 0) OR
         (CSt.listLevel = Scnr.listAlways) THEN
        Scnr.lst := TxtFil.createFile(CSt.lstNam);
        IF Scnr.lst # NIL THEN
          CPascalErrors.PrintListing(doList);
          TxtFil.CloseFile(Scnr.lst);
          Scnr.lst := NIL;
        ELSE
          CSt.Message("cannot create file <" + CSt.lstNam + ">");
          IF events > 0 THEN CPascalErrors.PrintListing(FALSE) END;
        END;
      END;
      CPascalErrors.ResetErrorList();
    END FixListing;

(* ==================================================================== *)

    PROCEDURE Compile*(IN nam : ARRAY OF CHAR; OUT retVal : INTEGER);
    BEGIN
      CSt.CheckOptionsOK;
      retVal := 0;
      CSt.totalS := RTS.GetMillis();
      Scnr.src := BinFil.findLocal(nam);
      IF Scnr.src = NIL THEN
        CSt.Message("cannot open local file <" + nam + ">");
      ELSE
        NameHash.InitNameHash(CSt.hashSize);
        CSt.outNam := NIL;
        CSt.InitCompState(nam);
        Builtin.RebindBuiltins();
        Target.Select(CSt.thisMod, CSt.target);
        Target.Init();
        CondMsg("Starting Parse");
        CPascalP.Parse;   (* do the compilation here *)
        CSt.parseE := RTS.GetMillis();
        IF Scnr.errors = 0 THEN
          CondMsg("Doing statement attribution");
          CSt.thisMod.StatementAttribution(Visitor.newImplementedCheck());
          IF (Scnr.errors = 0) & CSt.extras THEN
            CondMsg("Doing type erasure");
            CSt.thisMod.TypeErasure(Visitor.newTypeEraser());
          END;
          IF Scnr.errors = 0 THEN
            CondMsg("Doing dataflow analysis");
            CSt.thisMod.DataflowAttribution();
            CSt.attrib := RTS.GetMillis();
            IF Scnr.errors = 0 THEN
              IF CSt.doSym THEN
                CondMsg("Emitting symbol file");
                IF CSt.legacy THEN
                  Old.EmitSymfile(CSt.thisMod);
                ELSE
                  New.EmitSymfile(CSt.thisMod);
                END;
                CSt.symEnd := RTS.GetMillis();
                IF CSt.doAsm THEN
                  IF CSt.isForeign() THEN
                    CondMsg("Foreign module: no code output");
                  ELSE
                    CondMsg("Emitting assembler");
                    Target.Emit();
                    CSt.asmEnd := RTS.GetMillis();
                    IF CSt.doCode THEN Target.Assemble() END;
                  END;
                END;
              END;
            END;
          END;
        END;
        IF Scnr.errors # 0 THEN retVal := 2 END;
        CSt.totalE := RTS.GetMillis();
        FixListing;
        Finalize;
        IF CSt.doStats THEN CSt.Report END;
      END;
    RESCUE (sysX)
      retVal := 2;
      CSt.Message("<< COMPILER PANIC >>");
      Scnr.SemError.RepSt1(299, RTS.getStr(sysX), 1, 1);
     (* 
      *  If an exception is raised during listing, FixListing will
      *  be called twice. Avoid an attempted sharing violation...
      *)
      IF Scnr.lst # NIL THEN 
        TxtFil.CloseFile(Scnr.lst);
        CSt.Message(RTS.getStr(sysX));
        Scnr.lst := NIL;
      ELSE
        FixListing;
      END;
      Finalize;
    END Compile;

(* ==================================================================== *)
(*                       Main Argument Loop                             *)
(* ==================================================================== *)

BEGIN
  CSt.InitOptions;
  CPascalErrors.Init;
  Builtin.InitBuiltins;
END CPascal.

(* ==================================================================== *)
