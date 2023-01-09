
MODULE MakeIndex;

  IMPORT 
        RTS,
        Console,
        Error,
        CPmain,
        ProgArgs,
        Btd := BiTypeDefs,
        Bsh := BiStateHandler;

(* ============================================================ *)

  VAR badOption : BOOLEAN;

  VAR idx : INTEGER;
      arg : ARRAY 256 OF CHAR;

  VAR appState : Bsh.State;

(* ============================================================ *)

  PROCEDURE ParseOption(opt : ARRAY OF CHAR) : BOOLEAN;
    VAR rst : Btd.CharOpen;
        arg : ARRAY 256 OF CHAR;
   (* ----------------------------------------- *)
   (*  Note: str is mutable, pat is immutable   *)
   (* ----------------------------------------- *)
    PROCEDURE StartsWith(str : ARRAY OF CHAR; IN pat : ARRAY OF CHAR) : BOOLEAN;
    BEGIN (* friendly warping of options *)
      str[LEN(pat$)] := 0X;
      RETURN str = pat;
    END StartsWith;
   (* ----------------------------------------- *)
    PROCEDURE SuffixString(IN str : ARRAY OF CHAR; ofst : INTEGER) : Btd.CharOpen;
      VAR len : INTEGER;
          idx : INTEGER;
          out : Btd.CharOpen;
    BEGIN
      len := LEN(str$) - ofst;
      IF len > 0 THEN
        NEW(out, len + 1);
        FOR idx := 0 TO len - 1 DO
          out[idx] := str[ofst + idx];
        END;
        out[len] := 0X;
        RETURN out;
      END;
      RETURN NIL;
    END SuffixString;
   (* ----------------------------------------- *)
  BEGIN
    IF opt[0] = '/' THEN opt[0] := '-' END;
    IF opt[4] = '=' THEN opt[4] := ':' END;
    IF StartsWith(opt, "-dst:") THEN
      appState.dstPath := SuffixString(opt, 5);
    ELSIF (LEN(opt$) > 4) & StartsWith("-verbose", opt) THEN
      appState.verbose := TRUE;  
    ELSE
      RETURN FALSE;
    END;
    RETURN TRUE;
  END ParseOption;

(* ============================================================ *)

BEGIN (* Static code of Module *)
  NEW(appState);
  IF ProgArgs.ArgNumber() = 0 THEN
    Console.WriteString("Usage: MakeIndex [-verb] -dst:dir"); Console.WriteLn;
    Console.WriteString("     -dst:dir    - find symfiles in directory 'dir'"); Console.WriteLn;
    Console.WriteString("     -verb[ose]  - emit progress information."); Console.WriteLn;
    Console.WriteString("Output file is 'dir\index.html'"); Console.WriteLn;
    HALT(0);
  END;
  FOR idx := 0 TO ProgArgs.ArgNumber() - 1 DO
    ProgArgs.GetArg(idx, arg);
    badOption := ~ParseOption(arg);
    IF badOption THEN
      Error.WriteString("Bad option: " + arg$ + " HALTING.");
      Error.WriteLn;
      HALT(1);
    END;
  END;
  IF appState.dstPath = NIL THEN
    Error.WriteString("-dst:dir argument is mandatory, HALTING");
    Error.WriteLn;
    HALT(1);
  END;
  appState.InitPackageList();
  appState.ListFiles();
  appState.WriteIndex();
END MakeIndex.

(* ============================================================ *)
