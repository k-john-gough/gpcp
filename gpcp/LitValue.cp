(* ==================================================================== *)
(*									*)
(*  Literal Valuehandler Module for the Gardens Point Component 	*)
(*  Pascal Compiler. Exports the open character array type CharOpen	*)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*									*)
(* ==================================================================== *)

MODULE LitValue;

  IMPORT
    ASCII, 
	GPCPcopyright,
	Console,
	GPText,
	CPascalS;

(* ============================================================ *)

  TYPE
    CharOpen*    = POINTER TO ARRAY OF CHAR;
    CharOpenSeq* = RECORD
		     high  : INTEGER;
		     tide- : INTEGER;
		     a-    : POINTER TO ARRAY OF CharOpen;
		   END;

    CharVector*  = VECTOR OF CHAR;

(* ============================================================ *)

  TYPE
    Value*    = POINTER TO RECORD		(* All opaque.	*)
		  ord : LONGINT;
		  flt : REAL;
		  str : CharOpen;
		END;

(* ================================================================= *)
(*                      FORWARD DECLARATIONS                         *)
(* ================================================================= *)
  PROCEDURE^ strToCharOpen*(IN str : ARRAY OF CHAR) : CharOpen;
  PROCEDURE^ arrToCharOpen*(str : CharOpen; len : INTEGER) : CharOpen;
  PROCEDURE^ subStrToCharOpen*(pos,len : INTEGER) : CharOpen;
  PROCEDURE^ chrVecToCharOpen*(vec : CharVector) : CharOpen;
(* ================================================================= *)

  PROCEDURE  newChrVal*(ch : CHAR) : Value;
    VAR val : Value;
  BEGIN
    NEW(val); val.ord := ORD(ch); RETURN val;
  END newChrVal;

  PROCEDURE  newIntVal*(nm : LONGINT) : Value;
    VAR val : Value;
  BEGIN
    NEW(val); val.ord := nm; RETURN val;
  END newIntVal;

  PROCEDURE  newFltVal*(rv : REAL) : Value;
    VAR val : Value;
  BEGIN
    NEW(val); val.flt := rv; RETURN val;
  END newFltVal;

  PROCEDURE  newSetVal*(st : SET) : Value;
    VAR val : Value;
  BEGIN
    NEW(val); val.ord := ORD(st); RETURN val;
  END newSetVal;

  PROCEDURE  newStrVal*(IN sv : ARRAY OF CHAR) : Value;
    VAR val : Value;
  BEGIN
    NEW(val); 
    val.ord := LEN(sv$);
    val.str := strToCharOpen(sv);
    RETURN val;
  END newStrVal;

  PROCEDURE  newStrLenVal*(str : CharOpen; len : INTEGER) : Value;
    VAR val : Value;
  BEGIN
    NEW(val); 
    val.ord := len;
    val.str := arrToCharOpen(str, len);
    RETURN val;
  END newStrLenVal;

  PROCEDURE  newBufVal*(p,l : INTEGER) : Value;
    VAR val : Value;
  BEGIN
    NEW(val); 
    val.ord := l;
    val.str := subStrToCharOpen(p,l);
    RETURN val;
  END newBufVal;

  PROCEDURE  escapedString*(pos,len : INTEGER) : Value;
    VAR value  : Value;
	    vector : CharVector;
		count  : INTEGER;
		theCh  : CHAR;
		cdPnt  : INTEGER;
    (* ----------------------- *)
    PROCEDURE ReportBadHex(code, offset : INTEGER);
      VAR tok : CPascalS.Token;
    BEGIN
      tok := CPascalS.prevTok;
	  CPascalS.SemError.Report(code, tok.lin, tok.col + offset);
    END ReportBadHex;
    (* ----------------------- *)
  BEGIN
    count := 0;
    NEW(value);
	NEW(vector, len * 2);
	WHILE count < len DO
	  theCh :=  CPascalS.charAt(pos+count); INC(count);
	  IF theCh = '\' THEN
	    theCh := CPascalS.charAt(pos+count); INC(count);
		CASE theCh OF
		|  '0' : APPEND(vector, 0X);
		|  '\' : APPEND(vector, '\');
		|  'a' : APPEND(vector, ASCII.BEL);
		|  'b' : APPEND(vector, ASCII.BS);
		|  'f' : APPEND(vector, ASCII.FF);
		|  'n' : APPEND(vector, ASCII.LF);
		|  'r' : APPEND(vector, ASCII.CR);
		|  't' : APPEND(vector, ASCII.HT);
		|  'v' : APPEND(vector, ASCII.VT);
		|  'u' : cdPnt := CPascalS.getHex(pos+count, 4);
		         IF cdPnt < 0 THEN ReportBadHex(-cdPnt, count); cdPnt := 0 END;
				 APPEND(vector, CHR(cdPnt)); INC(count, 4);
		|  'x' : cdPnt := CPascalS.getHex(pos+count, 2);
		         IF cdPnt < 0 THEN ReportBadHex(-cdPnt, count); cdPnt := 0 END;
		         APPEND(vector, CHR(cdPnt)); INC(count, 2);
		ELSE APPEND(vector, theCh);
		END;
      ELSE
	    APPEND(vector, theCh);
	  END;
    END;
    value.ord := LEN(vector);
	value.str := chrVecToCharOpen(vector);
    RETURN value;
  END escapedString;

(* ============================================================ *)

  PROCEDURE (v : Value)char*() : CHAR,NEW;	(* final method *)
  BEGIN
    RETURN CHR(v.ord);
  END char;

  PROCEDURE (v : Value)int*() : INTEGER,NEW;	(* final method *)
  BEGIN
    RETURN SHORT(v.ord);
  END int;

  PROCEDURE (v : Value)set*() : SET,NEW;	(* final method *)
  BEGIN
    RETURN BITS(SHORT(v.ord));
  END set;

  PROCEDURE (v : Value)long*() : LONGINT,NEW;	(* final method *)
  BEGIN
    RETURN v.ord;
  END long;

  PROCEDURE (v : Value)real*() : REAL,NEW;	(* final method *)
  BEGIN
    RETURN v.flt;
  END real;

  PROCEDURE (v : Value)chOpen*() : CharOpen,NEW;	(*final *)
  BEGIN
    RETURN v.str;
  END chOpen;

  PROCEDURE (v : Value)len*() : INTEGER,NEW;	(* final method *)
  BEGIN
    RETURN SHORT(v.ord);
  END len;

  PROCEDURE (v : Value)chr0*() : CHAR,NEW;	(* final method *)
  BEGIN
    RETURN v.str[0];
  END chr0;

  PROCEDURE (v : Value)GetStr*(OUT str : ARRAY OF CHAR),NEW;
  BEGIN						(* final method *)
    GPText.Assign(v.str^, str);
  END GetStr;

(* ============================================================ *)

  PROCEDURE isShortStr*(in : Value) : BOOLEAN;
    VAR idx : INTEGER;
	chr : CHAR;
  BEGIN
    FOR idx := 0 TO LEN(in.str$) - 1 DO
      chr := in.str[idx];
      IF chr > 0FFX THEN RETURN FALSE END;
    END;
    RETURN TRUE;
  END isShortStr;

(* ============================================================ *)
(* 		     Various CharOpen Utilities 		*)
(* ============================================================ *)

  PROCEDURE InitCharOpenSeq*(VAR seq : CharOpenSeq; capacity : INTEGER); 
  BEGIN
    NEW(seq.a, capacity); seq.tide := 0; seq.high := capacity-1;
  END InitCharOpenSeq;

(* -------------------------------------------- *)

  PROCEDURE ResetCharOpenSeq*(VAR seq : CharOpenSeq);
  BEGIN
    seq.tide := 0;
  END ResetCharOpenSeq;

(* -------------------------------------------- *)

  PROCEDURE AppendCharOpen*(VAR seq : CharOpenSeq; elem : CharOpen);
    VAR temp : POINTER TO ARRAY OF CharOpen;
	i    : INTEGER;
  BEGIN
    IF seq.a = NIL THEN 
      InitCharOpenSeq(seq, 8);
    ELSIF seq.tide > seq.high THEN (* must expand *)
      temp := seq.a;
      seq.high := seq.high * 2 + 1;
      NEW(seq.a, seq.high+1);
      FOR i := 0 TO seq.tide-1 DO seq.a[i] := temp[i] END;
    END;
    seq.a[seq.tide] := elem; INC(seq.tide);
  END AppendCharOpen;

 (* -------------------------------------------- *
  * This function trims the string asciiz style.
  * -------------------------------------------- *)
  PROCEDURE strToCharOpen*(IN str : ARRAY OF CHAR) : CharOpen;
    VAR i : INTEGER;
        h : INTEGER;
        p : CharOpen;
  BEGIN
    h := LEN(str$); (* Length NOT including NUL *)
    NEW(p,h+1);     (* Including space for NUL *)
    FOR i := 0 TO h DO
      p[i] := str[i];
    END;
    RETURN p;
  END strToCharOpen;

 (* -------------------------------------------- *
  * This function uses ALL of the characters 
  * which may include embedded NUL characters.
  * -------------------------------------------- *)
  PROCEDURE arrToCharOpen*(str : CharOpen;
                           len : INTEGER) : CharOpen;
    VAR i : INTEGER;
        p : CharOpen;
  BEGIN
    NEW(p,len+1);
    FOR i := 0 TO len DO
      p[i] := str[i];
    END;
    RETURN p;
  END arrToCharOpen;

(* -------------------------------------------- *)

  PROCEDURE subChOToChO*(str : CharOpen;
			 off : INTEGER;
			 len : INTEGER) : CharOpen;
    VAR i : INTEGER;
        h : INTEGER;
        p : CharOpen;
  BEGIN
    NEW(p, len+1);
    FOR i := 0 TO len-1 DO
      p[i] := str[i+off];
    END;
    RETURN p;
  END subChOToChO;

(* -------------------------------------------- *)

  PROCEDURE posOf*(ch : CHAR; op : CharOpen) : INTEGER;
    VAR i : INTEGER;
  BEGIN
    FOR i := 0 TO LEN(op) - 1 DO
      IF op[i] = ch THEN RETURN i END;
    END;
    RETURN LEN(op);
  END posOf;

(* -------------------------------------------- *)

  PROCEDURE chrVecToCharOpen(vec : CharVector) : CharOpen;
    VAR i, len : INTEGER;
	    cOpen  : CharOpen;
  BEGIN
    len := LEN(vec);
    NEW(cOpen,len + 1);
	FOR i := 0 TO len -1 DO
	  cOpen[i] := vec[i];
    END;
	cOpen[len] := 0X;
	RETURN cOpen;
  END chrVecToCharOpen;

(* -------------------------------------------- *)

  PROCEDURE subStrToCharOpen*(pos,len : INTEGER) : CharOpen;
    VAR i : INTEGER;
        p : CharOpen;
  BEGIN
    NEW(p,len+1);
    FOR i := 0 TO len-1 DO
      p[i] := CPascalS.charAt(pos+i);
    END;
    p[len] := 0X;
    RETURN p;
  END subStrToCharOpen;

(* -------------------------------------------- *)

  PROCEDURE intToCharOpen*(i : INTEGER) : CharOpen;
    VAR arr : ARRAY 16 OF CHAR;
  BEGIN
    GPText.IntToStr(i, arr);
    RETURN strToCharOpen(arr);
  END intToCharOpen;   

(* -------------------------------------------- *)

  PROCEDURE ToStr*(in : CharOpen; OUT out : ARRAY OF CHAR);
  BEGIN
    IF in = NIL THEN out := "<NIL>" ELSE GPText.Assign(in^, out) END;
  END ToStr;

(* -------------------------------------------- *)

  PROCEDURE arrayCat*(IN in : CharOpenSeq) : CharOpen;
    VAR i,j,k : INTEGER;
	len : INTEGER;
	chO : CharOpen;
	ret : CharOpen;
	chr : CHAR;
  BEGIN
    len := 1;
    FOR i := 0 TO in.tide-1 DO INC(len, LEN(in.a[i]) - 1) END;
    NEW(ret, len);
    k := 0;
    FOR i := 0 TO in.tide-1 DO 
      chO := in.a[i];
      j := 0;
      WHILE (j < LEN(chO)-1) & (chO[j] # 0X) DO 
	ret[k] := chO[j]; INC(k); INC(j);
      END;
    END;
    ret[k] := 0X;
    RETURN ret;
  END arrayCat;

(* ============================================================ *)
(* 		     Safe Operations on Values			*)
(* ============================================================ *)
(* 		     Well, will be safe later!			*)
(* ============================================================ *)

  PROCEDURE concat*(a,b : Value) : Value;
    VAR c : Value;
	i : INTEGER;
	j : INTEGER;
  BEGIN
    j := SHORT(a.ord);
    NEW(c);
    c.ord := a.ord + b.ord;
    NEW(c.str, SHORT(c.ord) + 1);
    FOR i := 0 TO j - 1 DO
      c.str[i] := a.str[i];
    END;
    FOR i := 0 TO SHORT(b.ord) DO
      c.str[i+j] := b.str[i];
    END;
    RETURN c;
  END concat;

(* -------------------------------------------- *)

  PROCEDURE entV*(a : Value) : Value;
    VAR c : Value;
  BEGIN
    IF (a.flt >= MAX(LONGINT) + 1.0) OR 
       (a.flt < MIN(LONGINT)) THEN RETURN NIL;
    ELSE NEW(c); c.ord := ENTIER(a.flt); RETURN c;
    END; 
  END entV;

(* -------------------------------------------- *)

  PROCEDURE absV*(a : Value) : Value;
    VAR c : Value;
  BEGIN
    IF a.ord = MIN(LONGINT) THEN RETURN NIL;
    ELSE NEW(c); c.ord := ABS(a.ord); RETURN c;
    END; 
  END absV;

(* -------------------------------------------- *)

  PROCEDURE negV*(a : Value) : Value;
    VAR c : Value;
  BEGIN
    IF a.ord = MIN(LONGINT) THEN RETURN NIL;
    ELSE NEW(c); c.ord := -a.ord; RETURN c;
    END; 
  END negV;

(* -------------------------------------------- *)

  PROCEDURE addV*(a,b : Value) : Value;
    VAR c : Value;
  BEGIN
    NEW(c); c.ord := a.ord + b.ord; RETURN c;
  END addV;

(* -------------------------------------------- *)

  PROCEDURE subV*(a,b : Value) : Value;
    VAR c : Value;
  BEGIN
    NEW(c); c.ord := a.ord - b.ord; RETURN c;
  END subV;

(* -------------------------------------------- *)

  PROCEDURE mulV*(a,b : Value) : Value;
    VAR c : Value;
  BEGIN
    NEW(c); c.ord := a.ord * b.ord; RETURN c;
  END mulV;

(* -------------------------------------------- *)

  PROCEDURE slashV*(a,b : Value) : Value;
    VAR c : Value;
  BEGIN
    NEW(c); c.flt := a.ord / b.ord; RETURN c;
  END slashV;

(* -------------------------------------------- *)

  PROCEDURE divV*(a,b : Value) : Value;
    VAR c : Value;
  BEGIN
    NEW(c); c.ord := a.ord DIV b.ord; RETURN c;
  END divV;

(* -------------------------------------------- *)

  PROCEDURE modV*(a,b : Value) : Value;
    VAR c : Value;
  BEGIN
    NEW(c); c.ord := a.ord MOD b.ord; RETURN c;
  END modV;

(* -------------------------------------------- *)

  PROCEDURE div0V*(a,b : Value) : Value;
    VAR c : Value;
  BEGIN
    NEW(c); c.ord := a.ord DIV0 b.ord; RETURN c;
  END div0V;

(* -------------------------------------------- *)

  PROCEDURE rem0V*(a,b : Value) : Value;
    VAR c : Value;
  BEGIN
    NEW(c); c.ord := a.ord REM0 b.ord; RETURN c;
  END rem0V;

(* -------------------------------------------- *)

  PROCEDURE strCmp*(l,r : Value) : INTEGER;
   (* warning: this routine is not unicode aware *)
    VAR index   : INTEGER;
	lch,rch : CHAR;
  BEGIN
    FOR index := 0 TO MIN(SHORT(l.ord), SHORT(r.ord)) + 1 DO
      lch := l.str[index];
      rch := r.str[index];
      IF lch < rch    THEN RETURN -1
      ELSIF lch > rch THEN RETURN 1
      ELSIF lch = 0X  THEN RETURN 0
      END;
    END;
    RETURN 0;
  END strCmp;

(* -------------------------------------------- *)

  PROCEDURE DiagCharOpen*(ptr : CharOpen);
  BEGIN
    IF ptr = NIL THEN 
      Console.WriteString("<nil>");
    ELSE 
      Console.WriteString(ptr);
    END;
  END DiagCharOpen;

(* ============================================================ *)
BEGIN (* ====================================================== *)
END LitValue. (* ============================================== *)
(* ============================================================ *)

