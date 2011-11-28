(* ==================================================================== *)
(*									*)
(*  NameHash Module for the Gardens Point Component Pascal Compiler.	*)
(*  Implements the main symbol hash table. Uses closed hashing algrthm  *)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*									*)
(* ==================================================================== *)

MODULE NameHash;

  IMPORT 
	GPCPcopyright,
	Console,
	GPText,
	V := LitValue,
	CPascalS,
	RTS;

(* ============================================================ *)

  VAR
    name      : POINTER TO ARRAY OF V.CharOpen;
    size-     : INTEGER;
    entries-  : INTEGER;
    mainBkt*  : INTEGER;
    winMain*  : INTEGER;
    staBkt*   : INTEGER;

(* ============================================================ *)
  PROCEDURE^ enterStr*(IN str : ARRAY OF CHAR) : INTEGER;
(* ============================================================ *)

  PROCEDURE Reset;
    VAR i : INTEGER;
  BEGIN
    FOR i := 0 TO size-1 DO name[i] := NIL END;
  END Reset;

(* -------------------------------------------- *)

  PROCEDURE InitNameHash*(nElem : INTEGER);
  BEGIN
    IF    nElem <=  4099 THEN nElem :=  4099;
    ELSIF nElem <=  8209 THEN nElem :=  8209;
    ELSIF nElem <= 12289 THEN nElem := 12289;
    ELSIF nElem <= 18433 THEN nElem := 18433;
    ELSIF nElem <= 32833 THEN nElem := 32833;
    ELSIF nElem <= 46691 THEN nElem := 46691;
    ELSE  nElem := 65521;
    END;
    IF (name # NIL) & (size >= nElem) THEN
      Reset();
    ELSE
      size := nElem;
      NEW(name, nElem); 
    END;
    entries := 0;
    mainBkt := enterStr("CPmain");
    winMain := enterStr("WinMain");
    staBkt  := enterStr("STA");
  END InitNameHash;

(* ============================================================ *)

  PROCEDURE HashtableOverflow();
    CONST str = "Overflow: Use -hsize > current ";
  BEGIN
    RTS.Throw(str + V.intToCharOpen(size)^);
  END HashtableOverflow;

(* ============================================================ *)

  PROCEDURE hashStr(IN str : ARRAY OF CHAR) : INTEGER;
    VAR tot : INTEGER;
	idx : INTEGER;
	len : INTEGER;
  BEGIN [UNCHECKED_ARITHMETIC]
    (* need to turn off overflow checking *)
    len := LEN(str$);
    tot := 0;
    FOR idx := 0 TO len-1 DO
      INC(tot, tot);
      IF tot < 0 THEN INC(tot) END;
      INC(tot, ORD(str[idx]));
    END;
    RETURN tot MOD size; 
  END hashStr;

(* -------------------------------------------- *)

  PROCEDURE hashSubStr(pos,len : INTEGER) : INTEGER;
    VAR tot : INTEGER;
	idx : INTEGER;
  BEGIN [UNCHECKED_ARITHMETIC]
    (* need to turn off overflow checking *)
    tot := 0;
    FOR idx := 0 TO len-1 DO
      INC(tot, tot);
      IF tot < 0 THEN INC(tot) END;
      INC(tot, ORD(CPascalS.charAt(pos+idx)));
    END;
    RETURN tot MOD size; 
  END hashSubStr;

(* -------------------------------------------- *)

  PROCEDURE equalSubStr(val : V.CharOpen; pos,len : INTEGER) : BOOLEAN;
    VAR i : INTEGER;
  BEGIN
   (* 
    * LEN(val) includes the terminating nul character.
    *)
    IF LEN(val) # len+1 THEN RETURN FALSE END;
    FOR i := 0 TO len-1 DO
      IF CPascalS.charAt(pos+i) # val[i] THEN RETURN FALSE END;
    END;
    RETURN TRUE;
  END equalSubStr;

(* -------------------------------------------- *)

  PROCEDURE equalStr(val : V.CharOpen; IN str : ARRAY OF CHAR) : BOOLEAN;
    VAR i : INTEGER;
  BEGIN
   (*
    * LEN(val) includes the terminating nul character.
    * LEN(str$) does not include the nul character.
    *)
    IF LEN(val) # LEN(str$)+1 THEN RETURN FALSE END;
    FOR i := 0 TO LEN(val)-1 DO
      IF str[i] # val[i] THEN RETURN FALSE END;
    END;
    RETURN TRUE;
  END equalStr;

(* -------------------------------------------- *)

  PROCEDURE enterStr*(IN str : ARRAY OF CHAR) : INTEGER;
    VAR step : INTEGER;
	key  : INTEGER;
	val  : V.CharOpen;
  BEGIN
    step := 1;
    key  := hashStr(str);
    val  := name[key];
    WHILE (val # NIL) & ~equalStr(val,str) DO
      INC(key, step);
      INC(step,2); 
      IF step >= size THEN HashtableOverflow() END;
      IF key >= size THEN DEC(key,size) END; (* wrap-around *)
      val := name[key];
    END;
   (* Loop has been exitted. But for which reason? *)
    IF val = NIL THEN
      INC(entries);
      name[key] := V.strToCharOpen(str);
    END;				(* ELSE val already in table ... *)
    RETURN key;
  END enterStr;

(* -------------------------------------------- *)

  PROCEDURE enterSubStr*(pos,len : INTEGER) : INTEGER;
    VAR step : INTEGER;
	key  : INTEGER;
	val  : V.CharOpen;
  BEGIN
    step := 1;
    key  := hashSubStr(pos,len);
    val  := name[key];
    WHILE (val # NIL) & ~equalSubStr(val,pos,len) DO
      INC(key, step);
      INC(step,2); 
      IF step >= size THEN HashtableOverflow() END;
      IF key >= size THEN DEC(key,size) END; (* wrap-around *)
      val := name[key];
    END;
   (* Loop has been exitted. But for which reason? *)
    IF val = NIL THEN
      INC(entries);
      name[key] := V.subStrToCharOpen(pos,len);
    END;				(* ELSE val already in table ... *)
    RETURN key;
  END enterSubStr;

(* -------------------------------------------- *)

  PROCEDURE charOpenOfHash*(hsh : INTEGER) : V.CharOpen;
  BEGIN
    RETURN name[hsh];
  END charOpenOfHash;

(* ============================================================ *)
BEGIN (* ====================================================== *)
END NameHash. (* =========================================== *)
(* ============================================================ *)

