(* ============================================================ *)
(* 								*)
(*	   Gardens Point Component Pascal Library Module.	*)
(* 	   Copyright (c) K John Gough 1999, 2000		*)
(*	   Created : 26 December 1999 kjg			*)
(* 								*)
(* ============================================================ *)
MODULE GPText;

  IMPORT 
	Console,
	T := GPTextFiles;

  CONST CWd = 24;
  TYPE	CVS = ARRAY CWd OF CHAR;

  PROCEDURE Write*(f : T.FILE; c : CHAR);
  (** Write a single character to file f. *)
  BEGIN
    T.WriteChar(f,c);
  END Write;

  PROCEDURE WriteLn*(f : T.FILE);
  (** Write an end of line to file f. *)
  BEGIN
    T.WriteEOL(f);
  END WriteLn;

  PROCEDURE WriteString*(f : T.FILE; IN s : ARRAY OF CHAR);
  (** Write a character string to file f. *)
    VAR l : INTEGER;
  BEGIN
    l := LEN(s$);
    T.WriteNChars(f,s,l);
  END WriteString;

  PROCEDURE WriteFiller*(f: T.FILE; IN s: ARRAY OF CHAR; c: CHAR; w: INTEGER);
  (** Write s left-justified in a field of width w, fill with char c. *)
    VAR l : INTEGER;
	i : INTEGER;
  BEGIN
    l := LEN(s$);
    IF l < w THEN
      T.WriteNChars(f,s,l);
      FOR i := l TO w-1 DO T.WriteChar(f,c) END;
    ELSE
      T.WriteNChars(f,s,w);
    END;
  END WriteFiller;

  PROCEDURE WriteRight(f : T.FILE; IN arr : CVS; sig,wid : INTEGER);
    VAR i : INTEGER;
	high : INTEGER;
  BEGIN
    IF wid = 0 THEN 
      T.WriteChar(f," ");
    ELSIF sig < wid THEN (* fill *)
      FOR i := 1 TO wid-sig DO T.WriteChar(f," ") END;
    END;
    FOR i := CWd - sig TO CWd-1 DO T.WriteChar(f,arr[i]) END;
  END WriteRight;

  PROCEDURE FormatL(n : LONGINT; OUT str : CVS; OUT sig : INTEGER);
    VAR idx : INTEGER;
        neg : BOOLEAN;
        big : BOOLEAN;
  BEGIN
    big := (n = MIN(LONGINT)); 
    IF big THEN n := n+1 END; (* boot compiler gets INC(long) wrong! *)
    neg := (n < 0);
    IF neg THEN n := -n END; (* MININT is OK! *)
    idx := CWd;
    REPEAT
      DEC(idx);
      str[idx] := CHR(n MOD 10 + ORD('0')); 
      n := n DIV 10; 
    UNTIL n = 0;
    IF neg THEN DEC(idx); str[idx] := '-' END;
    IF big THEN str[CWd-1] := CHR(ORD(str[CWd-1]) + 1) END;
    sig := CWd - idx;
  END FormatL;

  PROCEDURE FormatI(n : INTEGER; OUT str : CVS; OUT sig : INTEGER);
    VAR idx : INTEGER;
        neg : BOOLEAN;
  BEGIN
    IF n = MIN(INTEGER) THEN FormatL(n, str, sig); RETURN END;
    neg := (n < 0);
    IF neg THEN n := -n END; 
    idx := CWd;
    REPEAT
      DEC(idx);
      str[idx] := CHR(n MOD 10 + ORD('0')); 
      n := n DIV 10; 
    UNTIL n = 0;
    IF neg THEN DEC(idx); str[idx] := '-' END;
    sig := CWd - idx;
  END FormatI;

  PROCEDURE WriteInt*(f : T.FILE; n : INTEGER; w : INTEGER);
  (** Write an integer to file f to a field of width w; 
      if w = 0, then leave a space then left justify. *)
    VAR str : CVS;
	sig : INTEGER;
  BEGIN
    IF w < 0 THEN w := 0 END;
    FormatI(n, str, sig);
    WriteRight(f, str, sig, w);
  END WriteInt;

  PROCEDURE IntToStr*(n : INTEGER; OUT a : ARRAY OF CHAR);
  (** Format an integer into the character array a. *)
    VAR str : CVS;
	idx : INTEGER;
	sig : INTEGER;
  BEGIN
    FormatI(n, str, sig);
    IF sig < LEN(a) THEN
      FOR idx := 0 TO sig-1 DO a[idx] := str[CWd-sig+idx] END;
      a[sig] := 0X;
    ELSE
      FOR idx := 0 TO LEN(a) - 2 DO a[idx] := '*' END;
      a[LEN(a)-1] := 0X;
    END;
  END IntToStr;

  PROCEDURE WriteLong*(f : T.FILE; n : LONGINT; w : INTEGER);
  (** Write an longint to file f to a field of width w; 
      if w = 0, then leave a space then left justify. *)
    VAR str : CVS;
	sig : INTEGER;
  BEGIN
    IF w < 0 THEN w := 0 END;
    FormatL(n, str, sig);
    WriteRight(f, str, sig, w);
  END WriteLong;

  PROCEDURE LongToStr*(n : LONGINT; OUT a : ARRAY OF CHAR);
  (** Format a long integer into the character array a. *)
    VAR str : CVS;
	idx : INTEGER;
	sig : INTEGER;
  BEGIN
    FormatL(n, str, sig);
    IF sig < LEN(a) THEN
      FOR idx := 0 TO sig-1 DO a[idx] := str[CWd-sig+idx] END;
      a[sig] := 0X;
    ELSE
      FOR idx := 0 TO LEN(a) - 2 DO a[idx] := '*' END;
      a[LEN(a)-1] := 0X;
    END;
  END LongToStr;

  PROCEDURE Assign*(IN r : ARRAY OF CHAR; OUT l : ARRAY OF CHAR);
    VAR i : INTEGER; lh,rh : INTEGER;
  BEGIN
    rh := LEN(r$) - 1;		(* string high-val, not including nul *)
    lh := LEN(l) - 2;		(* array capacity, with space for nul *)
    lh := MIN(lh,rh);
    FOR i := 0 TO lh DO l[i] := r[i] END;
    l[lh+1] := 0X;
  END Assign;

END GPText.
