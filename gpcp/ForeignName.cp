(* ================================================================ *)
(*                                                                  *)
(*  Module of the V1.4+ gpcp tool to create symbol files from       *)
(*  the metadata of .NET assemblies, using the PERWAPI interface.   *)
(*  Also used in GPCP itself.                                       *)
(*                                                                  *)
(*  Copyright K John Gough, QUT 2004 - 2007.                        *)
(*                                                                  *)
(*  This code released under the terms of the GPCP licence.         *)
(*                                                                  *)
(* ================================================================ *)

MODULE ForeignName;
  IMPORT GPCPcopyright;

 (* ---------------------------------------------------------- *)

  TYPE  CharOpen* = POINTER TO ARRAY OF CHAR;

 (* ---------------------------------------------------------- *)

  PROCEDURE QuotedName*(asmN, nmsN : CharOpen) : CharOpen;
  BEGIN
    IF nmsN = NIL THEN RETURN BOX("[" + asmN^ + "]");
    ELSE RETURN BOX("[" + asmN^ + "]" + nmsN^);
    END;
  END QuotedName;

  PROCEDURE MangledName*(asmN, nmsN : CharOpen) : CharOpen;
    CONST prefix = 2; equal = 1; unequal = 0;
    VAR   sNm, aNm : CharOpen;
   (* ------------------------------------------------ *)
    PROCEDURE canonEq(l,r : CharOpen) : INTEGER;
      VAR cl, cr : CHAR; ix : INTEGER;
    BEGIN
      ix := 0; cl := l[ix];
      WHILE cl # 0X DO
        cr := r[ix];
        IF CAP(cl) # CAP(cr) THEN RETURN unequal END;
        INC(ix); cl := l[ix];
      END;
      cr := r[ix];
      IF    cr = 0X  THEN RETURN equal;
      ELSIF cr = "_" THEN RETURN prefix;
      ELSE (* -------- *) RETURN unequal;
      END;
    END canonEq;
   (* ------------------------------------------------ *)
    PROCEDURE canonicalizeId(str : CharOpen) : CharOpen;
      VAR ix : INTEGER; co : CharOpen; ch : CHAR;
    BEGIN
      NEW(co, LEN(str));
      FOR ix := 0 TO LEN(str)-1 DO 
        ch := str[ix];
        IF (ch >= 'a') & (ch <= 'z') OR
           (ch >= 'A') & (ch <= 'Z') OR
           (ch >= '0') & (ch <= '9') OR
           (ch >= 0C0X) & (ch <= 0D6X) OR
           (ch >= 0D8X) & (ch <= 0F6X) OR
           (ch >= 0F8X) & (ch <= 0FFX) OR
           (ch = 0X) THEN (* skip *) ELSE ch := '_' END;
        co[ix] := ch;
      END;
      RETURN co;
    END canonicalizeId;
   (* ------------------------------------------------ *)
    PROCEDURE merge(str : CharOpen; pos : INTEGER) : CharOpen;
      VAR res : CharOpen;
          len : INTEGER;
          idx : INTEGER;
    BEGIN
      len := LEN(str);
      NEW(res, len+1);
      FOR idx := 0 TO pos-1 DO res[idx] := str[idx] END;
      res[pos] := "_";
      FOR idx := pos TO len-1 DO res[idx+1] := str[idx] END;
      RETURN res;
    END merge;
   (* ------------------------------------------------ *)
  BEGIN
    aNm := canonicalizeId(asmN);
    IF (nmsN = NIL) OR (nmsN[0] = 0X) THEN
     (*
      *   There is no namespace name, so the CP
      *   name is "aNm" and the scopeNm is "[asmN]"
      *) 
      RETURN aNm;
    ELSE
      sNm := canonicalizeId(nmsN);
      CASE canonEq(aNm, sNm) OF
      | unequal :
         (* 
          *  The CP name is "aNm_sNm"
          *  and scopeNm is "[asmN]nmsN"
          *) 
          RETURN BOX(aNm^ + "_" + sNm^);
      | equal   :
         (* 
          *  The CP name is "sNm_"
          *  and scopeNm is "[asmN]nmsN"
          *) 
          RETURN BOX(sNm^ + "_");
      | prefix  :
         (* 
          *  The CP name is prefix(sNm) + "_" + suffix(sNm)
          *  and scopeNm is "[asmN]nmsN"
          *) 
          RETURN merge(sNm, LEN(aNm$));
      END;
    END;
  END MangledName;

 (* ---------------------------------------------------------- *)

  PROCEDURE ParseModuleString*(str : CharOpen; OUT nam : CharOpen);
    VAR idx : INTEGER;
        max : INTEGER;
        lBr : INTEGER;
        rBr : INTEGER;
        chr : CHAR;
        fNm : CharOpen;
        cNm : CharOpen;
  BEGIN
    lBr := 0; 
    rBr := 0;
    max := LEN(str^) - 1;
    FOR idx := 0 TO max DO
      chr := str[idx];
      IF chr = '[' THEN lBr := idx;
      ELSIF chr = ']' THEN rBr := idx;
      END;
    END;
    IF (lBr = 0) & (rBr > 1) & (rBr < max) THEN
      NEW(fNm, rBr - lBr);
      NEW(cNm, max - rBr + 1);
      FOR idx := 0 TO rBr - lBr - 2 DO fNm[idx] := str[idx + lBr + 1] END;
      FOR idx := 0 TO max - rBr - 1 DO cNm[idx] := str[idx + rBr + 1] END;
      nam := MangledName(fNm, cNm);
    ELSE
      nam := NIL;
    END;
  END ParseModuleString;

 (* ---------------------------------------------------------- *)

END ForeignName.

