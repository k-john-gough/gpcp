(* ==================================================================== *)
(*									*)
(*  VarSet Module for the Gardens Point Component Pascal Compiler.	*)
(*  Implements operations on variable length bitsets.			*)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*									*)
(* ==================================================================== *)

MODULE VarSets;

  IMPORT 
	GPCPcopyright,
	Console;

(* ============================================================ *)

  CONST bits = 32;
	iMax = bits-1;

(* ============================================================ *)

  TYPE
    VarSet* = POINTER TO RECORD
		vars : POINTER TO ARRAY OF SET;
		size : INTEGER;
	      END;

(* ============================================================ *)
(* =======  Implementation of VarSet abstract data type ======= *)
(* ============================================================ *)

  PROCEDURE newSet*(size : INTEGER) : VarSet;
    VAR tmp : VarSet;
	len : INTEGER;
  BEGIN
    NEW(tmp);
    tmp.size := size;
    IF size = 0 THEN len := 1 ELSE len := (size + iMax) DIV bits END;
    NEW(tmp.vars, len);
    RETURN tmp;
  END newSet;

(* ======================================= *)

  PROCEDURE newUniv*(size : INTEGER) : VarSet;
    VAR tmp : VarSet;
	rem : INTEGER;
	idx : INTEGER;
  BEGIN
    idx := 0;
    rem := size;
    tmp := newSet(size);
    WHILE rem > 32 DO
      tmp.vars[idx] := {0 .. iMax};
      INC(idx); DEC(rem,bits);
    END;
    tmp.vars[idx] := {0 .. (rem-1)};
    RETURN tmp;
  END newUniv;

(* ======================================= *)

  PROCEDURE newEmpty*(size : INTEGER) : VarSet;
    VAR tmp : VarSet;
	idx : INTEGER;
  BEGIN
    tmp := newSet(size);
    FOR idx := 0 TO LEN(tmp.vars^)-1 DO tmp.vars[idx] := {} END;
    RETURN tmp;
  END newEmpty;

(* ======================================= *)

  PROCEDURE (self : VarSet)newCopy*() : VarSet,NEW;
    VAR tmp : VarSet;
	idx : INTEGER;
  BEGIN
    tmp := newSet(self.size);
    FOR idx := 0 TO LEN(tmp.vars)-1 DO tmp.vars[idx] := self.vars[idx] END;
    RETURN tmp;
  END newCopy;

(* ======================================= *)

  PROCEDURE (self : VarSet)cardinality*() : INTEGER,NEW;
  BEGIN RETURN self.size END cardinality;

(* ============================================================ *)

  PROCEDURE (self : VarSet)includes*(elem : INTEGER) : BOOLEAN, NEW;
  BEGIN
    RETURN (elem < self.size) & 
		((elem MOD bits) IN self.vars[elem DIV bits]);
  END includes;

(* ============================================================ *)

  PROCEDURE (self : VarSet)Incl*(elem : INTEGER),NEW;
  BEGIN
    INCL(self.vars[elem DIV bits], elem MOD bits);
  END Incl;

(* ======================================= *)

  PROCEDURE (self : VarSet)InclSet*(add : VarSet),NEW;
    VAR i : INTEGER;
  BEGIN
    ASSERT(self.size = add.size);
    FOR i := 0 TO LEN(self.vars)-1 DO
      self.vars[i] := self.vars[i] + add.vars[i];
    END;
  END InclSet;

(* ============================================================ *)

  PROCEDURE (self : VarSet)Excl*(elem : INTEGER),NEW;
  BEGIN
    EXCL(self.vars[elem DIV bits], elem MOD bits);
  END Excl;

(* ======================================= *)

  PROCEDURE (self : VarSet)ExclSet*(sub : VarSet),NEW;
    VAR i : INTEGER;
  BEGIN
    ASSERT(self.size = sub.size);
    FOR i := 0 TO LEN(self.vars)-1 DO
      self.vars[i] := self.vars[i] - sub.vars[i];
    END;
  END ExclSet;

(* ============================================================ *)

  PROCEDURE (self : VarSet)isUniv*() : BOOLEAN, NEW;
    VAR i,r : INTEGER; s : SET;
  BEGIN
    i := 0; r := self.size;
    WHILE r > bits DO
      IF self.vars[i] # {0 .. iMax} THEN RETURN FALSE END;
      INC(i); DEC(r,bits);
    END;
    RETURN self.vars[i] = {0 .. (r-1)};
  END isUniv;

(* ============================================================ *)

  PROCEDURE (self : VarSet)isEmpty*() : BOOLEAN, NEW;
    VAR i : INTEGER;
  BEGIN
    IF self.size <= 32 THEN RETURN self.vars[0] = {} END;
    FOR i := 0 TO LEN(self.vars)-1 DO
      IF self.vars[i] # {} THEN RETURN FALSE END;
    END;
    RETURN TRUE;
  END isEmpty;

(* ============================================================ *)

  PROCEDURE (self : VarSet)not*() : VarSet, NEW;
    VAR tmp : VarSet;
	rem : INTEGER;
	idx : INTEGER;
  BEGIN
    idx := 0; 
    rem := self.size;
    tmp := newSet(rem);
    WHILE rem > 32 DO
      tmp.vars[idx] := {0 .. iMax} - self.vars[idx];
      INC(idx); DEC(rem,bits);
    END;
    tmp.vars[idx] := {0 .. (rem-1)} - self.vars[idx];
    RETURN tmp;
  END not;

(* ======================================= *)

  PROCEDURE (self : VarSet)Neg*(),NEW;
    VAR rem : INTEGER;
	idx : INTEGER;
  BEGIN
    idx := 0; 
    rem := self.size;
    WHILE rem > 32 DO
      self.vars[idx] := {0 .. iMax} - self.vars[idx];
      INC(idx); DEC(rem,bits);
    END;
    self.vars[idx] := {0 .. (rem-1)} - self.vars[idx];
  END Neg;

(* ============================================================ *)

  PROCEDURE (self : VarSet)cup*(rhs : VarSet) : VarSet,NEW;
    VAR tmp : VarSet;
    VAR i   : INTEGER;
  BEGIN
    ASSERT(self.size = rhs.size);
    tmp := newSet(self.size);
    FOR i := 0 TO LEN(self.vars)-1 DO
      tmp.vars[i] := self.vars[i] + rhs.vars[i];
    END;
    RETURN tmp;
  END cup;

(* ======================================= *)

  PROCEDURE (self : VarSet)Union*(rhs : VarSet),NEW;
  BEGIN
    self.InclSet(rhs);
  END Union;

(* ============================================================ *)

  PROCEDURE (self : VarSet)cap*(rhs : VarSet) : VarSet,NEW;
    VAR tmp : VarSet;
    VAR i   : INTEGER;
  BEGIN
    ASSERT(self.size = rhs.size);
    tmp := newSet(self.size);
    FOR i := 0 TO LEN(self.vars)-1 DO
      tmp.vars[i] := self.vars[i] * rhs.vars[i];
    END;
    RETURN tmp;
  END cap;

(* ======================================= *)

  PROCEDURE (self : VarSet)Intersect*(rhs : VarSet),NEW;
    VAR i   : INTEGER;
  BEGIN
    ASSERT(self.size = rhs.size);
    FOR i := 0 TO LEN(self.vars)-1 DO
      self.vars[i] := self.vars[i] * rhs.vars[i];
    END;
  END Intersect;

(* ============================================================ *)

  PROCEDURE (self : VarSet)xor*(rhs : VarSet) : VarSet,NEW;
    VAR tmp : VarSet;
	i   : INTEGER;
  BEGIN
    ASSERT(self.size = rhs.size);
    tmp := newSet(self.size);
    FOR i := 0 TO LEN(self.vars)-1 DO
      tmp.vars[i] := self.vars[i] / rhs.vars[i];
    END;
    RETURN tmp;
  END xor;

(* ======================================= *)

  PROCEDURE (self : VarSet)SymDiff*(rhs : VarSet),NEW;
    VAR i   : INTEGER;
  BEGIN
    ASSERT(self.size = rhs.size);
    FOR i := 0 TO LEN(self.vars)-1 DO
      self.vars[i] := self.vars[i] / rhs.vars[i];
    END;
  END SymDiff;

(* ============================================================ *)

  PROCEDURE (self : VarSet)Diagnose*(),NEW;
    VAR i,j : INTEGER;
	lim : INTEGER;
	chr : CHAR;
  BEGIN
    j := 0;
    lim := self.size-1;
    Console.Write('{'); 
    FOR i := 0 TO self.size-1 DO
      chr := CHR(i MOD 10 + ORD('0'));
      IF self.includes(i) THEN 
	Console.Write(chr);
      ELSE
	Console.Write('.');
      END;
      IF (chr = '9') & (i < lim) THEN 
	IF j < 6 THEN INC(j) ELSE Console.WriteLn; j := 0 END;
	Console.Write('|');
      END;
    END;
    Console.Write('}');
  END Diagnose;

(* ============================================================ *)
END VarSets.  (* ============================================== *)
(* ============================================================ *)

