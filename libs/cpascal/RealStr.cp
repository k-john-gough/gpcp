MODULE RealStr;
(*
 * Purpose:
 *   Provides REAL/string conversions
 *
 * Log:
 *   April 96  jl  initial version
 *
 * Notes:
 *   Complies with ISO/IEC 10514-1:1996  (as RealStr)
 *
 * Modified for Component Pascal by kjg, February 2004
 *
 *)

  IMPORT RTS;

(***************************************************************)
(*                                                             *)
(*                   PRIVATE - NOT EXPORTED                    *)
(*                                                             *)
(***************************************************************)

  CONST
     err = 9999;

  TYPE
     CharPtr  = POINTER TO ARRAY OF CHAR;
     DigArray = ARRAY 28 OF CHAR;

  (*===============================================================*)

  PROCEDURE Message(OUT str : ARRAY OF CHAR; IN mss : ARRAY OF CHAR);
    VAR idx : INTEGER;
  BEGIN
    idx := 0;
    WHILE (idx < LEN(str)) & (idx < LEN(mss)) DO
      str[idx] := mss[idx]; INC(idx);
    END;
    IF idx < LEN(str) THEN str[idx] := 0X END;
  END Message;

  (*===============================================================*)

  PROCEDURE expLen(exp : INTEGER) : INTEGER;
  BEGIN
    exp := ABS(exp);
    IF    exp < 10  THEN RETURN 3;
    ELSIF exp < 100 THEN RETURN 4;
    ELSE  RETURN 5;
    END;
  END expLen;

  (*===============================================================*)

  PROCEDURE  CopyCh(ch : CHAR;
        	VAR ix : INTEGER;
        	VAR st : ARRAY OF CHAR);
  BEGIN
    IF ix < LEN(st) THEN st[ix] := ch; INC(ix) END;
  END CopyCh;

  (*===============================================================*)

  PROCEDURE CopyExp(ex : INTEGER;
        	VAR ix : INTEGER;
		VAR st : ARRAY OF CHAR);
    VAR abX, val, len, idx, dHi : INTEGER;
  BEGIN
    dHi := LEN(st) - 1;
    len := expLen(ex);
    IF ix + len > dHi THEN ix := dHi - len END;
    IF ix < 2 THEN
      FOR idx := 0 TO MIN(ix+len, dHi-1) DO st[idx] := "*"; ix := idx+1 END;
    ELSE
      CopyCh("E",ix,st);
      IF ex > 0 THEN CopyCh("+",ix,st) ELSE CopyCh("-",ix,st) END;
      abX := ABS(ex); val := abX;
      IF abX >= 100 THEN 
        CopyCh(CHR(val DIV 100 + ORD("0")),ix,st);
        val := val MOD 100;
      END;
      IF abX >= 10 THEN 
        CopyCh(CHR(val DIV 10 + ORD("0")),ix,st);
      END;
      CopyCh(CHR(val MOD 10 + ORD("0")),ix,st);
    END;
  END CopyExp;

  (*===============================================================*)

  PROCEDURE GetDigits(real   : REAL;
                  OUT digits : DigArray;
                  OUT dPoint : INTEGER;
                  OUT isNeg  : BOOLEAN);
    VAR rIdx : INTEGER;      (* the read index   *)
        wIdx : INTEGER;      (* the write index  *)
        iLen : INTEGER;      (* integer part len *)
        eVal : INTEGER;      (* exponent value   *)
        buff : DigArray;     (* temporary buffer *)
        eNeg : BOOLEAN;      (* exponent is neg. *)
        rChr : CHAR;         (* last read char   *)
  BEGIN
   (* 
    *  We want to assert that digit[0] # "0", 
    *  unless real = zero. So to avoid a sack o'woe 
    *)
    IF real = 0.0 THEN
      digits := "0";
      dPoint := 1;
      isNeg  := FALSE; RETURN;      (* PREEMPTIVE RETURN HERE *)
    END;

    RTS.RealToStrInvar(real, buff);
    rIdx := 0;
    wIdx := 0;
    eVal := 0;
   (* get optional sign *)
    isNeg := (buff[0] = "-");
    IF isNeg THEN INC(rIdx) END;
    
    rChr := buff[rIdx]; INC(rIdx);
    WHILE rChr = "0" DO 
      rChr := buff[rIdx]; INC(rIdx);
    END;

   (* get integer part *)
    WHILE (rChr <= "9") & (rChr >= "0") DO
      digits[wIdx] := rChr; INC(wIdx);
      rChr := buff[rIdx];   INC(rIdx);
    END;
    iLen := wIdx;              (* integer part ended  *)

    IF rChr = "." THEN         (* get fractional part *)
      rChr := buff[rIdx];   INC(rIdx);
      IF wIdx = 0 THEN
       (* count any leading zeros *)
        WHILE rChr = "0" DO 
          rChr := buff[rIdx]; INC(rIdx); DEC(iLen);
        END;
      END;

      WHILE (rChr <= "9") & (rChr >= "0") DO
        digits[wIdx] := rChr; INC(wIdx);
        rChr := buff[rIdx];   INC(rIdx);
      END;
    END;
    digits[wIdx] := 0X;        (* terminate char arr. *)

    IF (rChr = "E") OR (rChr = "e") THEN
                               (* get fractional part *)
      rChr := buff[rIdx];   INC(rIdx);
      IF rChr = "-" THEN
        eNeg := TRUE;
        rChr := buff[rIdx]; INC(rIdx);
      ELSE
        eNeg := FALSE;
        IF rChr = "+" THEN rChr := buff[rIdx]; INC(rIdx) END;
      END;
      WHILE (rChr <= "9") & (rChr >= "0") DO
        eVal := eVal * 10;
        INC(eVal, (ORD(rChr) - ORD("0")));
        rChr := buff[rIdx]; INC(rIdx);
      END;
      IF eNeg THEN eVal := -eVal END;
    END;

   (* At this point, if we are not ended, we have a NaN *)
    IF rChr # 0X THEN 
      digits := buff; dPoint := err;
    ELSE
     (* Index of virtual decimal point is eVal + iLen *)
      DEC(eVal);
      dPoint := iLen + eVal;
    END;
  END GetDigits;

(***************************************************************)

  PROCEDURE RoundRelative(VAR str : DigArray;
                          VAR exp : INTEGER;
                              num : INTEGER);
    VAR len : INTEGER;
        idx : INTEGER;
        chr : CHAR;
  BEGIN
    len := LEN(str$);   (* we want num+1 digits *)
    IF num < 0 THEN
      str[0] := 0X;
    ELSIF num = 0 THEN
      chr := str[0];
      IF chr > "4" THEN 
        str := "1"; INC(exp);
      ELSE 
        str[num] := 0X;
      END;
    ELSIF num < len THEN
      chr := str[num];
      IF chr > "4" THEN (* round up str[num-1]  *)
        idx := num-1;
        LOOP
          str[idx] := CHR(ORD(str[idx]) + 1);
          IF str[idx] <= "9" THEN EXIT;
          ELSE
            str[idx] := "0"; (* and propagate *)
            IF idx = 0 THEN  (* need a shift  *)
              FOR idx := num TO 0 BY -1 DO str[idx+1] := str[idx] END;
              str[0] := "1"; INC(exp); EXIT;
            END;
          END;
          DEC(idx);
        END;
      END;
      str[num] := 0X;
    END;
  END RoundRelative;

(***************************************************************)
(*                                                             *)
(*                     PUBLIC - EXPORTED                       *)
(*                                                             *)
(***************************************************************)

 (*===============================================================*
  *
  * Ignores any leading spaces in str. If the subsequent characters in str 
  * are in the format of a signed real number, assigns a corresponding value 
  * to real.  Assigns a value indicating the format of str to res.
  *)
  PROCEDURE StrToReal*(str  : ARRAY OF CHAR; 
                   OUT real : REAL; 
                   OUT res  : BOOLEAN);

    VAR clrStr : RTS.NativeString;
  BEGIN
    clrStr := MKSTR(str);
    RTS.StrToRealInvar(clrStr, real, res);
  END StrToReal;

 (*===============================================================*
  *
  * Converts the value of real to floating-point string form, with sigFigs
  * significant digits, and copies the possibly truncated result to str.
  *)
  PROCEDURE RealToFloat*(real    : REAL; 
                         sigFigs : INTEGER; 
                     OUT str     : ARRAY OF CHAR);

    VAR len, fWid, index, ix : INTEGER;
        dExp   : INTEGER; (* decimal exponent *)
	neg    : BOOLEAN;
	digits : DigArray;
  BEGIN
    GetDigits(real, digits, dExp, neg);
    IF dExp = err THEN Message(str, digits); RETURN END;
    RoundRelative(digits, dExp, sigFigs); 

    index := 0;
    IF neg THEN CopyCh("-", index, str) END;
    fWid := LEN(digits$);
    IF fWid = 0 THEN  (* result is 0 *)
      CopyCh("0", index, str);
      dExp := 0;
    ELSE
      CopyCh(digits[0], index, str);
    END;
    IF sigFigs > 1 THEN 
      CopyCh(".",index,str);
      IF fWid > 1 THEN
        FOR ix := 1  TO fWid - 1    DO CopyCh(digits[ix], index, str) END;
      END;
      FOR ix := fWid TO sigFigs - 1 DO CopyCh("0", index, str) END;
    END;
(*
 *  IF dExp # 0 THEN CopyExp(dExp,index,str) END;
 *)
    CopyExp(dExp,index,str);
    IF index <= LEN(str)-1 THEN str[index] := 0X END;
  END RealToFloat;

 (*===============================================================*
  *
  * Converts the value of real to floating-point string form, with sigFigs
  * significant digits, and copies the possibly truncated result to str.
  * The number is scaled with one to three digits in the whole number part and
  * with an exponent that is a multiple of three.
  *)
  PROCEDURE RealToEng*(real    : REAL; 
                       sigFigs : INTEGER; 
                   OUT str     : ARRAY OF CHAR);
    VAR len, index, ix : INTEGER;
        dExp   : INTEGER; (* decimal exponent *)
        fact   : INTEGER;
        neg    : BOOLEAN;
	digits : DigArray;
  BEGIN
    GetDigits(real, digits, dExp, neg);
    IF dExp = err THEN Message(str, digits); RETURN END;
    RoundRelative(digits, dExp, sigFigs); 

    len := LEN(digits$); INC(dExp);
    IF len = 0 THEN dExp := 1 END;  (* result = 0 *)
    fact := ((dExp - 1) MOD 3) + 1;
    DEC(dExp,fact);	(* make exponent multiple of three *)

    index := 0;
    IF neg THEN CopyCh("-",index,str) END;
    IF fact <= len THEN
      FOR ix := 0   TO fact - 1 DO CopyCh(digits[ix],index,str) END;
    ELSE
      IF len > 0 THEN
        FOR ix := 0 TO len  - 1 DO CopyCh(digits[ix],index,str) END;
      END;
      FOR ix := len TO fact - 1 DO CopyCh("0",index,str) END;
    END;
    IF fact < sigFigs THEN 
      CopyCh(".",index,str);
      IF fact < len THEN
        FOR ix := fact TO len - 1 DO CopyCh(digits[ix],index,str) END;
      ELSE
        len := fact;
      END;
      FOR ix := len TO sigFigs - 1 DO CopyCh("0",index,str) END;
    END;
(*
 *  IF dExp # 0 THEN CopyExp(dExp,index,str) END;
 *)
    CopyExp(dExp,index,str);
    IF index <= LEN(str)-1 THEN str[index] := 0X END;
  END RealToEng;

 (*===============================================================*
  *
  * Converts the value of real to fixed-point string form, rounded to the 
  * given place relative to the decimal point, and copies the result to str.
  *)
  PROCEDURE RealToFixed*(real  : REAL; 
                         place : INTEGER; (* requested no of frac. places *)
                     OUT str   : ARRAY OF CHAR);
    VAR lWid   : INTEGER;     (* Leading digit-str width  *)
        fWid   : INTEGER;     (* Width of fractional part *)
        tWid   : INTEGER;     (* Total width of str-rep.  *)
        zWid   : INTEGER;     (* Leading zeros in frac.   *)
        len    : INTEGER;     (* Significant digit length *)
        dExp   : INTEGER;     (* Pos. of rad. in dig-arr. *)
        dLen   : INTEGER;     (* Length of dest. array    *)

        index  : INTEGER;
        ix     : INTEGER;
        neg    : BOOLEAN;
        radix  : BOOLEAN;
	digits : DigArray;
  BEGIN
   (* the decimal point and fraction part *)
   (* ["-"] "0" "." d^(fWid)       -- if dExp < 0 *)
   (* ["-"] d^(lWid) "." d^(fWid)  -- if fWid > 0 *)
   (* ["-"] d^(lWid)               -- if fWid = 0 *)
  
    tWid := 0;
    dLen := LEN(str);
    IF place >= 0 THEN fWid := place ELSE fWid := 0 END;
    radix := (fWid > 0);

    GetDigits(real, digits, dExp, neg);
    IF dExp = err THEN Message(str, digits); RETURN END;

    RoundRelative(digits, dExp, place+dExp+1); (* this can change dExp! *)

    (* Semantics of dExp value    *)
    (*  012345 ...  digit index   *)
    (*  dddddd ...  digit content *)
    (*    ^-------- dExp value    *)
    (* "ddd.ddd..." result str.   *)

    len := LEN(digits$); 
    IF len = 0 THEN neg := FALSE END; (* don't print "-0" *)
    IF dExp >= 0 THEN lWid := dExp+1 ELSE lWid := 1 END;

    IF neg   THEN INC(tWid) END;
    IF radix THEN INC(tWid) END;
    INC(tWid, lWid);
    INC(tWid, fWid);

    IF tWid > dLen THEN tWid := dLen END;

    index := 0;
   (*
    *  Now copy the optional signe
    *)
    IF neg THEN CopyCh("-",index,str) END;
   (*
    *  Now copy the integer part
    *)
    IF dExp < 0 THEN 
      CopyCh("0",index,str);
    ELSE
      IF lWid <= len THEN
        FOR ix := 0   TO lWid - 1 DO CopyCh(digits[ix],index,str) END;
      ELSE
        IF len > 0 THEN
          FOR ix := 0 TO len - 1  DO CopyCh(digits[ix],index,str) END;
        END;
        FOR ix := len TO lWid - 1 DO CopyCh("0",index,str) END;
      END;
    END;
   (*
    *  Now copy the fractional part
    *)
    IF radix THEN 
      CopyCh(".",index,str);
      IF dExp < 0 THEN
       (*   012345 ...  digit idx  *)
       (*   dddddd ...  digit str. *)
       (*  ^-------- dExp = -1     *)
        zWid := MIN(-dExp-1, fWid); (* leading zero width *)
        FOR ix := 0 TO zWid - 1 DO CopyCh("0",index,str) END;
        FOR ix := 0 TO len - 1  DO CopyCh(digits[ix],index,str) END;
      ELSIF lWid < len THEN
        FOR ix := lWid TO len - 1 DO CopyCh(digits[ix],index,str) END;
      END;
      WHILE index < tWid DO CopyCh("0",index,str) END;
    END;
    IF index <= dLen-1 THEN str[index] := 0X END;
  END RealToFixed;

 (*===============================================================*
  *
  * Converts the value of real as RealToFixed if the sign and magnitude can be
  * shown within the capacity of str, or otherwise as RealToFloat, and copies
  * the possibly truncated result to str.
  * The number of places or significant digits are implementation-defined.
  *)
  PROCEDURE RealToStr*(real: REAL; OUT str: ARRAY OF CHAR);
  BEGIN
    RTS.RealToStrInvar(real, str);
  RESCUE (x);
    RealToFloat(real, 16, str);
  END RealToStr;

(* ---------------------------------------- *)

END RealStr.
