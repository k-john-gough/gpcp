(***********************************************************************)
(*                     Component Pascal Make Tool                      *)
(*                                                                     *)
(*           Diane Corney, 20th July 1999                              *)
(*           Modifications:                                            *)
(*                                                                     *)
(*                                                                     *)
(***********************************************************************)
MODULE CPMake ;

IMPORT GPCPcopyright,
       CPmain, 
       CPascal,
       G := CPascalG,
       S := CPascalS, 
       CPascalErrors,
       LitValue,
       ForeignName,
       GPFiles,
       GPBinFiles,
       GPTextFiles, 
       NameHash,
       CompState,
       NewSymFileRW,
       MH := ModuleHandler, 
       SF := SymbolFile,
       ProgArgs,
       FileNames,
       Error,
       RTS,
       Console;

TYPE
  ArgString = ARRAY 256 OF CHAR;
  ArgBlock = RECORD
               args : POINTER TO ARRAY OF ArgString;
               argNum : INTEGER;
             END;

CONST
  argSize = 10;

VAR
  startT, endT : LONGINT;

VAR
  toDoList, compList : MH.ModList; 
  graph : MH.ModInfo;
  token : S.Token;
  sysBkt : INTEGER;
  frnBkt : INTEGER;
  buildOK : BOOLEAN;
  compCount : INTEGER;
  args : ArgBlock;
  force : BOOLEAN;

  PROCEDURE Chuck(IN msg : ARRAY OF CHAR);
  BEGIN
    Error.WriteString('CPMake: "');
    Error.WriteString(msg); 
    Error.WriteString('" Halting...');
    Error.WriteLn; HALT(1);
  END Chuck;

  PROCEDURE Warn(IN msg : ARRAY OF CHAR);
  BEGIN
    Console.WriteString('CPMake: ');
    Console.WriteString(msg); Console.WriteLn; 
  END Warn;

  PROCEDURE Usage();
    CONST jPre = "cprun ";
	  str1 = "Usage: CPMake [";
	  str2 = "all] [gpcp-options] <ModuleName>";
	  str3 = "	For gpcp-options, type: ";
	  str4 = "gpcp ";
	  str5 = "help";
    VAR   isNt : BOOLEAN;
  BEGIN
    Console.WriteString("gardens point CPMake: " + GPCPcopyright.verStr);
    Console.WriteLn;
    isNt := RTS.defaultTarget = "net";
    IF ~isNt THEN Console.WriteString(jPre) END;
    Console.WriteString(str1);
    Console.Write(GPFiles.optChar);
    Console.WriteString(str2);
    Console.WriteLn();
    Console.WriteString(str3);
    IF ~isNt THEN Console.WriteString(jPre) END;
    Console.WriteString(str4);
    Console.Write(GPFiles.optChar);
    Console.WriteString(str5);
    Console.WriteLn();
  END Usage;

PROCEDURE ReadModuleName(VAR name : ARRAY OF CHAR);
VAR
  i, pos, parNum, numArgs : INTEGER;
  opt : ArgString;
BEGIN
  numArgs := ProgArgs.ArgNumber();
  args.argNum := 0;
  CompState.InitOptions();
  IF numArgs < 1 THEN
    Usage();
    HALT(1);
  END;
  IF numArgs > 1 THEN
    NEW(args.args, numArgs-1);
    FOR parNum := 0 TO numArgs-2 DO
      ProgArgs.GetArg(parNum,opt);
      IF (opt[0] = '-') OR (opt[0] = GPFiles.optChar) THEN
	opt[0] := '-';
	IF opt = "-all" THEN 
	  force := TRUE;
	ELSE
          CPascal.DoOption(opt);
          args.args[args.argNum] := opt;
          INC(args.argNum);
	END;
      ELSE
        Console.WriteString("Unknown option:  " + opt); 
	Console.WriteLn;
      END;
    END; 
  END;
  ProgArgs.GetArg(numArgs-1,name);
  IF (name[0] = '-') OR (name[0] = GPFiles.optChar) THEN
    Usage();
    HALT(1);
  END;
  i := 0;
  WHILE (name[i] # '.') & (name[i] # 0X) & (i < LEN(name)) DO INC(i); END;
  IF (i < LEN(name)) & (name[i] = '.') THEN
    WHILE (name[i] # 0X) & (i < LEN(name)) DO
      name[i] := 0X; INC(i);
    END;
  END;
END ReadModuleName;

PROCEDURE Check (sym : INTEGER; mod : MH.ModInfo);
BEGIN
  IF token.sym # sym THEN 
    S.ParseErr.Report(sym,token.lin,token.col);
    GPBinFiles.CloseFile(S.src);
    CPascal.FixListing();
    CPascal.Finalize();
    Chuck("Parse error(s) in module <" + mod.name^ + ">");
  END;
END Check;

PROCEDURE DoImport(mod : MH.ModInfo; VAR mainImported : BOOLEAN);
VAR
  mName : MH.ModName;
  aMod  : MH.ModInfo;
  last  : S.Token;
  strng, impNm : MH.ModName;
BEGIN
  Check(G.identSym,mod);
  last := token;
  token := S.get();			(* read past ident *)
  IF (token.sym = G.colonequalSym) THEN
    last  := S.get();			(* read past ":="  *)
    token := S.get();			(* read past ident *)
  END;
  IF last.sym = G.identSym THEN
    mName := LitValue.subStrToCharOpen(last.pos, last.len);
  ELSIF last.sym = G.stringSym THEN
    strng := LitValue.subStrToCharOpen(last.pos+1, last.len-2);
    ForeignName.ParseModuleString(strng, impNm);
    mName := impNm;
  ELSE
    mName := NIL;
    Chuck("Bad module name for alias import");
  END;
  IF (NameHash.enterSubStr(last.pos, last.len) = NameHash.mainBkt) OR 
     (NameHash.enterSubStr(last.pos, last.len) = NameHash.winMain) THEN
    mainImported := TRUE;
  ELSE
    aMod := MH.GetModule(mName);
    MH.Add(mod.imports,aMod);
    MH.Add(aMod.importedBy,mod);
    IF ~aMod.importsLinked THEN MH.Add(toDoList,aMod); END;
  END;
END DoImport;

PROCEDURE LinkImports(mod : MH.ModInfo);
VAR
  mName : FileNames.NameString;
  cpmainImported : BOOLEAN;
  hsh : INTEGER;
BEGIN
  CompState.InitCompState(mod.name^ + ".cp");
  mod.importsLinked := TRUE;
  cpmainImported := FALSE;
  S.Reset;
  token := S.get(); 
  IF (token.sym = G.identSym) THEN
    hsh := NameHash.enterSubStr(token.pos,token.len);
    IF (hsh = sysBkt) OR (hsh = frnBkt) THEN
      mod.isForeign := TRUE;
      token := S.get();
    END;
  END; 
  Check(G.MODULESym,mod); token := S.get();
  Check(G.identSym,mod);
  S.GetString(token.pos,token.len,mName);
  IF (mName # mod.name^) THEN 
    Chuck("File " + mod.name^ + ".cp does not contain MODULE " + mName);
  END;
  token := S.get();
  IF token.sym = G.lbrackSym THEN
    (* mod.isForeign := TRUE; *)
    token := S.get();  (* skip string and rbracket *)
    token := S.get(); 
    token := S.get();
  END; 
  Check(G.semicolonSym,mod); token := S.get();
  IF (token.sym = G.IMPORTSym) THEN
    token := S.get();
    DoImport(mod,cpmainImported);
    WHILE (token.sym = G.commaSym) DO
      token := S.get();
      DoImport(mod,cpmainImported);
    END;
  END;
  IF (mod = graph) & ~cpmainImported THEN
    Warn("WARNING: " + mod.name^ + " is not a base module.");
    Warn("Modules that " + mod.name^ + " depends on will be checked for consistency");
    Warn("Modules that depend on " + mod.name^ + " will not be checked or recompiled");
  END;
END LinkImports;

PROCEDURE BuildGraph() : BOOLEAN;
VAR
  name : FileNames.NameString;
  nextIx : INTEGER;
  nextModule : MH.ModInfo;
  srcFound : BOOLEAN;
BEGIN
  NEW(graph); 
  ReadModuleName(name);
  graph := MH.GetModule(BOX(name$));
  S.src := GPBinFiles.findLocal(graph.name^ + ".cp");
  IF S.src = NIL THEN
    Chuck("Could not find base file <" + graph.name^ + ".cp>");
  ELSE
    GPBinFiles.CloseFile(S.src);
  END;
  MH.Add(toDoList,graph);
  nextIx := 0; 
  WHILE (nextIx < toDoList.tide) DO
    nextModule := toDoList.list[nextIx]; INC(nextIx);
    S.src := GPBinFiles.findLocal(nextModule.name^ + ".cp");
    SF.OpenSymbolFile(nextModule.name, S.src = NIL);
    IF S.src = NIL THEN
      IF SF.file = NIL THEN 
        Chuck("Cannot find source file <" + nextModule.name^ + 
                  ".cp> or symbol file <" + nextModule.name^ + 
                  ".cps> on CPSYM path.");
      ELSE 
        SF.ReadSymbolFile(nextModule,FALSE); 
      END ;
    ELSE
      LinkImports(nextModule); 
      IF force OR (SF.file = NIL) OR ~GPFiles.isOlder(S.src,SF.file) THEN
        nextModule.compile := TRUE; 
(*
 *      IF force THEN
 *	  Console.WriteString("force: Setting compile flag on ");
 *	  Console.WriteString(nextModule.name);
 *	  Console.WriteLn;
 *      ELSIF (SF.file = NIL) THEN
 *	  Console.WriteString("file=NIL: Setting compile flag on ");
 *	  Console.WriteString(nextModule.name);
 *	  Console.WriteLn;
 *      ELSIF ~GPFiles.isOlder(S.src,SF.file) THEN
 *	  Console.WriteString("isOlder: Setting compile flag on ");
 *	  Console.WriteString(nextModule.name);
 *	  Console.WriteLn;
 *      END;
 *)
      ELSE 
        SF.ReadSymbolFile(nextModule,TRUE);
      END;
      SF.CloseSymFile();		(* or .NET barfs! *)
    END;
  END;
  RETURN TRUE;
RESCUE (buildX)
  Console.WriteString("#cpmake:  ");
  Console.WriteString(RTS.getStr(buildX));
  Console.WriteLn;
  RETURN FALSE;
END BuildGraph;

PROCEDURE CompileModule(mod : MH.ModInfo; VAR retVal : INTEGER);
VAR
  i : INTEGER;
BEGIN
  CompState.InitOptions();
  FOR i := 0 TO args.argNum-1 DO
    CPascal.DoOption(args.args[i]);
  END; 
  IF mod.isForeign THEN
    IF ~CompState.quiet THEN
      Console.WriteString(
	  "#cpmake:  " + mod.name^ + " is foreign, compiling with -special.");
      Console.WriteLn;
      Console.WriteString(
	  "#cpmake:  Foreign implementation may need recompilation.");
      Console.WriteLn;
    END;
    CPascal.DoOption("-special");
  ELSIF ~CompState.quiet THEN
    Console.WriteString("#cpmake:  compiling " + mod.name^);
    Console.WriteLn;
  END;
  CPascal.Compile(mod.name^ + ".cp",retVal); 
  mod.key := NewSymFileRW.GetLastKeyVal(); 
  INC(compCount);
END CompileModule;

PROCEDURE DFS(VAR node : MH.ModInfo);
VAR
  ix,retVal : INTEGER;
  imp : MH.ModInfo;
BEGIN
  IF ~node.done THEN
    node.done := TRUE;
    FOR ix := 0 TO node.imports.tide-1 DO
      DFS(node.imports.list[ix]);
    END;
    IF node.compile THEN
      retVal := 0;
      CompileModule(node,retVal);
      IF retVal # 0 THEN
        Chuck("Compile errors in module <" + node.name^ + ">");
      END;
    END;
    FOR ix := 0 TO node.importedBy.tide-1 DO
      imp := node.importedBy.list[ix];
      IF (~imp.compile) & (node.key # MH.GetKey(imp,node)) THEN
        node.importedBy.list[ix].compile := TRUE;
      END;
    END; 
  END;
END DFS;

PROCEDURE WalkGraph(VAR node : MH.ModInfo);
BEGIN
  DFS(node);
RESCUE (compX)
  Console.WriteString("#cpmake:  ");
  Console.WriteString(RTS.getStr(compX));
  Console.WriteLn;
END WalkGraph;

BEGIN
  force := FALSE;
  compCount := 0;
  NameHash.InitNameHash(0);
  sysBkt := NameHash.enterStr("SYSTEM");
  frnBkt := NameHash.enterStr("FOREIGN");
  CPascalErrors.Init();
  buildOK := BuildGraph();
  IF buildOK THEN
    startT := RTS.GetMillis();
    WalkGraph(graph);
    endT := RTS.GetMillis();
    Console.WriteString("#cpmake:  ");
    IF compCount = 0 THEN
      Console.WriteString("no re-compilation required.");
    ELSIF compCount = 1 THEN
      Console.WriteString("one module compiled.");
    ELSE
      Console.WriteInt(compCount,1);
      Console.WriteString(" modules compiled.");
    END;
    Console.WriteLn;
    CompState.TimeMsg("Total Compilation Time ", endT - startT);
  END;
END CPMake.
