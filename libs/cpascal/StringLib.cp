(* ============================================================= *)
(* Preliminary library module for Gardens Point Component Pascal *)
(* ============================================================= *)

MODULE StringLib; (* from GPM module StdStrings.mod kjg june 1989 *)
  IMPORT RTS;

  CONST  nul = 0X;

(* ============================================================ *)

  PROCEDURE CanAssignAll*(sLen : INTEGER;
                       IN dest : ARRAY OF CHAR) : BOOLEAN;
 (**  Check if an assignment is possible without truncation.
  *)
  BEGIN
    RETURN LEN(dest) > sLen; (* must leave room for nul *)
  END CanAssignAll;

  PROCEDURE Assign*   (IN  src : ARRAY OF CHAR;
                       OUT dst : ARRAY OF CHAR);
 (**  Assign as much as possible of src to dst,
  *   leaving room for a terminating ASCII nul.
  *)
    VAR ix, hi : INTEGER; 
        ch     : CHAR;
  BEGIN
    hi := MIN(LEN(src), LEN(dst)) - 1;
    FOR ix := 0 TO hi DO
      ch := src[ix];
      dst[ix] := ch;
      IF ch = nul THEN RETURN END;
    END;
   (*
    *  We have copied up to index "hi"
    *  without finding a nul in "src"
    *)
    dst[hi] := nul;
  END Assign;

(* ============================================================ *)

  PROCEDURE CanExtractAll*(len : INTEGER;
                           sIx : INTEGER;
                           num : INTEGER;
                       OUT dst : ARRAY OF CHAR) : BOOLEAN;
 (**  Check if an extraction of "num" charcters,
  *   starting at source index "sIx" is possible.
  *)
  BEGIN
    RETURN  (sIx + num <= len) & 
            (LEN(dst) > num);     (* leave room for nul *)
  END CanExtractAll;

  PROCEDURE Extract*  (IN  src : ARRAY OF CHAR;
                           sIx : INTEGER;
                           num : INTEGER;
                       OUT dst : ARRAY OF CHAR);
 (**  Extract "num" characters starting at index "sIx".  
  *   Result is truncated if either there are fewer characters
  *   left in the source, or the destination is too short.
  *)
    VAR ch  : CHAR; 
        sLm : INTEGER;
        dLm : INTEGER;
        dIx : INTEGER;
  BEGIN
  
    sLm := LEN(src$) - 1;  (* max index of source *)
    dLm := LEN(dst) - 1;   (* max index of dest.  *)
    IF sIx < 0 THEN RTS.Throw("StdStrings.Extract: Bad start index") END;
    IF num < 0 THEN RTS.Throw("StdStrings.Extract: Bad char. count") END;

    IF sIx > sLm THEN dst[0] := nul; RETURN END;
    IF (sIx + num - 1) < sLm THEN sLm := sIx + num - 1 END;

    dIx := 0;
    FOR sIx := sIx TO sLm DO
      IF dIx = dLm THEN dst[dIx] := nul; RETURN END;
      ch := src[sIx];
      dst[dIx] := ch; 
      INC(dIx);
    END;
    dst[dIx] := nul;
  END Extract;
  
(* ============================================================ *)

  PROCEDURE CanDeleteAll*( len : INTEGER;
                           sIx : INTEGER;
                           num : INTEGER) : BOOLEAN;
 (**  Check if "num" characters may be deleted starting
  *   from index "sIx", when len is the source length.
  *)
  BEGIN
    RETURN (sIx < len) & (sIx + num <= len);
  END CanDeleteAll;
    
  PROCEDURE Delete*(VAR str : ARRAY OF CHAR;
                        sIx : INTEGER;
                        num : INTEGER);
    VAR sLm, mIx : INTEGER;
 (**  Delete "num" characters starting from index "sIx".
  *   Less characters are deleted if there are less 
  *   than "num" characters after "sIx".
  *)
  BEGIN
    sLm := LEN(str$) - 1;
    IF sIx < 0 THEN RTS.Throw("StdStrings.Delete: Bad start index") END;
    IF num < 0 THEN RTS.Throw("StdStrings.Delete: Bad char. count") END;

    (* post : lim is length of str *)
    IF sIx < sLm THEN (* else do nothing *)
      IF sIx + num <= sLm THEN (* else sIx is unchanged *)
        mIx := sIx + num;
        WHILE mIx <= sLm DO
          str[sIx] := str[mIx]; INC(sIx); INC(mIx);
        END;
      END;
      str[sIx] := nul;
    END;
  END Delete;


(* ============================================================ *)

  PROCEDURE CanInsertAll*(sLen : INTEGER;
                          sIdx : INTEGER;
                      VAR dest : ARRAY OF CHAR) : BOOLEAN;
 (**  Check if "sLen" characters may be inserted into "dest"
  *   starting from index "sIdx".
  *)
    VAR dLen : INTEGER;
        dCap : INTEGER;
  BEGIN
    dCap := LEN(dest)-1; (* max chars in destination string  *)
    dLen := LEN(dest$);  (* current chars in destination str *)
    RETURN (sIdx < dLen) & 
           (dLen + sLen < dCap);
  END CanInsertAll;
  
  PROCEDURE Insert*   (IN  src : ARRAY OF CHAR;
                           sIx : INTEGER;
                       VAR dst : ARRAY OF CHAR);
 (**  Insert "src" string into "dst" starting from index 
  *   "sIx".  Less characters are inserted if there is not
  *   sufficient space in the destination.  The destination is
  *   unchanged if "sIx" is beyond the end of the initial string.
  *)
    VAR dLen, sLen, dCap, iEnd, cEnd : INTEGER; 
        idx : INTEGER;
  BEGIN
    dCap := LEN(dst)-1;
    sLen := LEN(src$);
    dLen := LEN(dst$);  (* dst[dLen] is index of the nul *)
    IF sIx < 0 THEN RTS.Throw("StdStrings.Insert: Bad start index") END;
  
    (* skip trivial case *)
    IF (sIx > dLen) OR (sLen = 0) THEN RETURN END;
  
    iEnd := MIN(sIx + sLen,  dCap); (* next index after last insert position *)
    cEnd := MIN(dLen + sLen, dCap); (* next index after last string position *)
  
    FOR idx := cEnd-1 TO iEnd BY -1 DO
       dst[idx]  := dst[idx-sLen];
    END;
  
    FOR idx := 0 TO sLen - 1 DO
      dst[idx+sIx] := src[idx];
    END;
    dst[cEnd] := nul;
  END Insert;

(* ============================================================ *)

  PROCEDURE CanReplaceAll*(len : INTEGER;
                           sIx : INTEGER;
                       VAR dst : ARRAY OF CHAR) : BOOLEAN;
 (**  Check if "len" characters may be replaced in "dst"
  *   starting from index "sIx".
  *)
  BEGIN
    RETURN len + sIx <= LEN(dst$);
  END CanReplaceAll;

  PROCEDURE Replace*  (IN  src : ARRAY OF CHAR;
                           sIx : INTEGER;
                       VAR dst : ARRAY OF CHAR);
 (**  Insert the characters of "src" string into "dst" starting 
  *   from index "sIx".  Less characters are replaced if the
  *   initial length of the destination string is insufficient.
  *   The string length of "dst" is unchanged.
  *)
    VAR dLen, sLen, ix : INTEGER;
  BEGIN
    dLen := LEN(dst$);
    sLen := LEN(src$);
    IF sIx >= dLen THEN RETURN END;
    IF sIx < 0 THEN RTS.Throw("StdStrings.Replace: Bad start index") END;

    FOR ix := sIx TO MIN(sIx+sLen-1, dLen-1) DO
      dst[ix] := src[ix-sIx]; 
    END;
  END Replace;

(* ============================================================ *)

  PROCEDURE CanAppendAll*(len : INTEGER;
                      VAR dst : ARRAY OF CHAR) : BOOLEAN;
 (**  Check if "len" characters may be appended to "dst"
  *)
    VAR dLen : INTEGER;
        dCap : INTEGER;
  BEGIN
    dCap := LEN(dst)-1; (* max chars in destination string  *)
    dLen := LEN(dst$);  (* current chars in destination str *)
    RETURN dLen + len <= dCap;
  END CanAppendAll;

  PROCEDURE Append*(src : ARRAY OF CHAR;
                VAR dst : ARRAY OF CHAR);
 (**  Append the characters of "src" string onto "dst".
  *   Less characters are appended if the length of the 
  *   destination string is insufficient.
  *)
    VAR dLen, dCap, sLen : INTEGER;
        idx : INTEGER;
  BEGIN
    dCap := LEN(dst)-1; (* max chars in destination string  *)
    dLen := LEN(dst$);  (* current chars in destination str *)
    sLen := LEN(src$);
    FOR idx := 0 TO sLen-1 DO
      IF dLen = dCap THEN dst[dCap] := nul; RETURN END;
      dst[dLen] := src[idx]; INC(dLen);
    END;
    dst[dLen] := nul;
  END Append;

(* ============================================================ *)

  PROCEDURE Capitalize*(VAR str : ARRAY OF CHAR);
    VAR ix : INTEGER;
  BEGIN
    FOR ix := 0 TO LEN(str$)-1 DO str[ix] := CAP(str[ix]) END;
  END Capitalize;

(* ============================================================ *)

  PROCEDURE FindNext*    (IN  pat : ARRAY OF CHAR;
                          IN  str : ARRAY OF CHAR;
                              bIx : INTEGER;  (* Begin index *)
                          OUT fnd : BOOLEAN;
                          OUT pos : INTEGER);
 (**  Find the first occurrence of the pattern string "pat"
  *   in "str" starting the search from index "bIx".
  *   If no match is found "fnd" is set FALSE and "pos"
  *   is set to "bIx".  Empty patterns match everywhere.
  *)
    VAR pIx, sIx : INTEGER;
        pLn, sLn : INTEGER;
        sCh      : CHAR;
  BEGIN
    pos := bIx;
    pLn := LEN(pat$);
    sLn := LEN(str$);

    (* first check that string extends to bIx *)
    IF bIx >= sLn - pLn THEN fnd := FALSE; RETURN END;
    IF pLn = 0 THEN fnd := TRUE; RETURN END;
    IF bIx < 0 THEN RTS.Throw("StdStrings.FindNext: Bad start index") END;

    sCh := pat[0];
    FOR sIx := bIx TO sLn - pLn - 1 DO
      IF str[sIx] = sCh THEN (* possible starting point! *)
        pIx := 0;
        REPEAT
          INC(pIx);   
          IF pIx = pLn THEN fnd := TRUE; pos := sIx; RETURN END;
        UNTIL str[sIx + pIx] # pat[pIx];
      END;
    END;
    fnd := FALSE;
  END FindNext;
  
(* ============================================================ *)

  PROCEDURE FindPrev*(IN  pat : ARRAY OF CHAR;
                      IN  str : ARRAY OF CHAR;
                          bIx : INTEGER;  (* begin index *)
                      OUT fnd : BOOLEAN;
                      OUT pos : INTEGER);

 (**  Find the previous occurrence of the pattern string "pat"
  *   in "str" starting the search from index "bIx".
  *   If no match is found "fnd" is set FALSE and "pos"
  *   is set to "bIx".  A pattern starting from "bIx" is found.
  *   Empty patterns match everywhere.
  *)
    VAR pIx, sIx : INTEGER;
        pLn, sLn : INTEGER;
        sCh      : CHAR;
  BEGIN
    pos := bIx;
    pLn := LEN(pat$);
    sLn := LEN(str$);

    IF pLn = 0 THEN fnd := TRUE; RETURN END;
    IF pLn > sLn THEN fnd := FALSE; RETURN END;
    IF bIx < 0 THEN RTS.Throw("StdStrings.FindPrev: Bad start index") END;

    (* start searching from bIx OR sLn - pLn *)
    sCh := pat[0];
    FOR sIx := MIN(bIx, sLn - pLn - 1) TO 0 BY - 1 DO
      IF str[sIx] = sCh THEN (* possible starting point! *)
        pIx := 0;
        REPEAT 
          INC(pIx);
          IF pIx = pLn THEN fnd := TRUE; pos := sIx; RETURN END;
        UNTIL str[sIx + pIx] # pat[pIx];
      END;
    END;
    fnd := FALSE;
  END FindPrev;
  
(* ============================================================ *)

  PROCEDURE FindDiff*   (IN  str1 : ARRAY OF CHAR;
                         IN  str2 : ARRAY OF CHAR;
                         OUT diff : BOOLEAN;
                         OUT dPos : INTEGER);
 (**  Find the index of the first charater of difference 
  *   between the two input strings.  If the strings are
  *   identical "diff" is set FALSE, and "dPos" is zero.
  *)
    VAR ln1, ln2, idx : INTEGER;
  BEGIN
    ln1 := LEN(str1$);
    ln2 := LEN(str2$);

    FOR idx := 0 TO MIN(ln1, ln2) DO
      IF str1[idx] # str2[idx] THEN
        diff := TRUE; dPos := idx; RETURN;   (* PRE-EMPTIVE RETURN *)
      END;
    END;
    dPos := 0;
    diff := (ln1 # ln2);  (* default result *)
  END FindDiff;
  
(* ============================================================ *)
END StringLib.
