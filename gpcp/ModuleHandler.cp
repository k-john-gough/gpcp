(***********************************************************************)
(*                     Component Pascal Make Tool                      *)
(*                                                                     *)
(*           Diane Corney, 20th July 1999                              *)
(*           Modifications:                                            *)
(*                                                                     *)
(*                                                                     *)
(***********************************************************************)
MODULE ModuleHandler;

IMPORT  GPCPcopyright,
	FileNames;

CONST
  listIncrement = 10;

TYPE

  ModName* = FileNames.NameString;

  ModInfo* = POINTER TO ModInfoRec;

  ModList*  = RECORD
                tide* : INTEGER;
                list* : POINTER TO ARRAY OF ModInfo; 
              END; 
               
  KeyList* = POINTER TO ARRAY OF INTEGER;

  ModInfoRec* = RECORD
                  name*       : ModName;
                  imports*    : ModList;
                  importedBy* : ModList;
                  importKeys* : KeyList;
                  key*        : INTEGER;
                  compile*    : BOOLEAN;
                  done*       : BOOLEAN;
                  isForeign*  : BOOLEAN;
                  importsLinked* : BOOLEAN;
                END;

  ModTree* = POINTER TO RECORD
               module : ModInfo;
               left,right : ModTree;
             END;

  
VAR
  modules : ModTree;

PROCEDURE Add* (VAR modList : ModList; mod : ModInfo);
VAR
  tmp : POINTER TO ARRAY OF ModInfo; 
  i : INTEGER;
BEGIN
  IF modList.list = NIL THEN
    NEW(modList.list,listIncrement);
    modList.tide := 0;
  ELSE
    FOR i := 0 TO modList.tide - 1 DO
      IF mod = modList.list[i] THEN RETURN; END;
    END;
    IF modList.tide >= LEN(modList.list) THEN
      tmp := modList.list;
      NEW(modList.list,(LEN(modList.list) + listIncrement)); 
      FOR i := 0 TO modList.tide - 1 DO
        modList.list[i] := tmp[i];
      END;
    END;
  END;
  modList.list[modList.tide] := mod;
  INC(modList.tide);
END Add;

PROCEDURE AddKey*(thisMod, impMod : ModInfo; impKey : INTEGER); 
VAR
  i : INTEGER;
  mods : ModList;
  tmp : POINTER TO ARRAY OF INTEGER;
BEGIN
  mods := thisMod.imports;
  IF (thisMod.importKeys = NIL) OR 
     (LEN(thisMod.importKeys) < LEN(mods.list)) THEN
    tmp := thisMod.importKeys;
    NEW(thisMod.importKeys,LEN(mods.list));
    IF (tmp # NIL) THEN
      FOR i := 0 TO LEN(tmp)-1 DO
        thisMod.importKeys[i] := tmp[i];
      END; 
    END; 
  END;
  i := 0;
  WHILE (i < LEN (mods.list)) & (mods.list[i] # impMod) DO INC(i); END;
  IF (i < LEN (mods.list)) THEN thisMod.importKeys[i] := impKey; END;
END AddKey;

PROCEDURE GetKey*(thisMod : ModInfo; impMod : ModInfo) : INTEGER;
VAR
  ix : INTEGER;
BEGIN
  (* Assert:  impMod is in imports list of thisMod *)
  ix := 0;
  WHILE (thisMod.imports.list[ix] # impMod) DO INC(ix); END;
  RETURN thisMod.importKeys[ix];
END GetKey;

PROCEDURE NewModInfo(modName : ModName) : ModInfo;
VAR
  mod : ModInfo;
BEGIN
  NEW(mod);
  mod.name := modName;
  mod.key := 0;
  mod.compile := FALSE;
  mod.done := FALSE;
  mod.isForeign := FALSE;
  mod.importsLinked := FALSE;
  mod.imports.tide := 0;
  NEW(mod.imports.list, listIncrement);
  mod.importedBy.tide := 0;
  RETURN mod;
END NewModInfo;

PROCEDURE GetModule*(modName : ModName) : ModInfo; 
VAR
  mod : ModInfo;
  node, parent : ModTree;
  found : BOOLEAN;
BEGIN
  IF (modules = NIL) THEN
    NEW (modules);
    modules.module := NewModInfo(modName);
    RETURN modules.module;
  END;
  mod := NIL;
  node := modules; 
  parent := NIL;
  found := FALSE; 
  WHILE (node # NIL) & (~found) DO
    parent := node;
    IF node.module.name = modName THEN
      found := TRUE;
      mod := node.module;
    ELSIF modName < node.module.name THEN
      node := node.left;
    ELSE
      node := node.right;
    END;
  END;
  IF ~found THEN
    ASSERT(parent # NIL);
    NEW(node);
    mod := NewModInfo(modName);
    node.module := mod; 
    IF modName < parent.module.name THEN
      parent.left := node;
    ELSE
      parent.right := node;
    END;
  END;
  RETURN mod;
END GetModule;


END ModuleHandler.
