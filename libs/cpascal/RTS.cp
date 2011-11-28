(** This is the user accessible static methods of the CP runtime system.
  * These are the environment-independent ones. Others are in CP*.cp
  * Note:  the bodies of these procedures are dummies, this module is
  *        compiled with -special.  The real code is in RTS.java or other.
  *
  * Version:     7 July     1999 (kjg).
  *             20 February 2000 (kjg)  Default target ...
  *              4 July     2000 (kjg)  Native types ...
  *              4 August   2001 (syc,kjg)  more methods...
  *                         2004 (kjg) vector support and globalization
  *)

(* ============================================================ *)
SYSTEM MODULE RTS;

  VAR defaultTarget- : ARRAY 4 OF CHAR;
      fltNegInfinity-   : SHORTREAL;
      fltPosInfinity-   : SHORTREAL;
      dblNegInfinity-   : REAL;
      dblPosInfinity-   : REAL;

  TYPE  CharOpen*       = POINTER TO ARRAY OF CHAR;
        CharVector*     = VECTOR OF CHAR;

  TYPE  NativeType*     = POINTER TO ABSTRACT RECORD END;
        NativeObject*   = POINTER TO EXTENSIBLE RECORD END;  
        NativeString*   = POINTER TO RECORD END;
        NativeException*= POINTER TO EXTENSIBLE RECORD END;

  VAR   eol- : POINTER TO ARRAY OF CHAR; (* OS-specific end of line string *)

  (* ========================================================== *)
  (* ============= Support for native exceptions ==============	*)
  (* ========================================================== *)
  PROCEDURE getStr*(x : NativeException) : CharOpen; 

  PROCEDURE Throw*(IN s : ARRAY OF CHAR);
  (** Abort execution with an error *)

  (* ========================================================== *)
  (* ============= Conversions FROM array of char ============= *)
  (* ========================================================== *)
  PROCEDURE StrToBool*(IN s : ARRAY OF CHAR; OUT b : BOOLEAN; OUT ok : BOOLEAN);
  (** Parse array into a BOOLEAN TRUE/FALSE *)

  PROCEDURE StrToByte*(IN s : ARRAY OF CHAR; OUT b : BYTE; OUT ok : BOOLEAN);
  (** Parse array into a BYTE integer *)

  PROCEDURE StrToUByte*(IN s : ARRAY OF CHAR; OUT b : BYTE; OUT ok : BOOLEAN);
  (** Parse array into a BYTE integer *)

  PROCEDURE StrToShort*(IN s : ARRAY OF CHAR; OUT si : SHORTINT; OUT ok : BOOLEAN);
  (** Parse an array into a CP LONGINT *)

  PROCEDURE StrToUShort*(IN s:ARRAY OF CHAR; OUT si:SHORTINT; OUT ok:BOOLEAN);
  (** Parse an array into a CP LONGINT *)

  PROCEDURE StrToInt*(IN s:ARRAY OF CHAR; OUT i:INTEGER; OUT ok:BOOLEAN);
  (** Parse an array into a CP INTEGER *)

  PROCEDURE StrToUInt*(IN s:ARRAY OF CHAR; OUT i:INTEGER; OUT ok:BOOLEAN);
  (** Parse an array into a CP INTEGER *)

  PROCEDURE StrToLong*(IN s:ARRAY OF CHAR; OUT i:LONGINT; OUT ok:BOOLEAN);
  (** Parse an array into a CP LONGINT *)

  PROCEDURE StrToULong*(IN s:ARRAY OF CHAR; OUT i:LONGINT; OUT ok:BOOLEAN);
  (** Parse an array into a CP LONGINT *)

  PROCEDURE HexStrToUByte*(IN s:ARRAY OF CHAR; OUT b:BYTE; OUT ok:BOOLEAN);
  (** Parse hexadecimal array into a BYTE integer *)

(* ------------------- Low-level String Conversions -------------------- *)
(* Three versions for different cultures.  *Invar uses invariant culture *)
(*                                         *Local uses current locale    *)
(* StrToReal & RealToStr do not behave the same on JVM and CLR.          *)
(* They is provided for compatability with versions < 1.3.1              *)
(* ------------------- Low-level String Conversions -------------------- *)

  PROCEDURE StrToReal*(IN  s  : ARRAY OF CHAR; 
                       OUT r  : REAL; 
                       OUT ok : BOOLEAN);
  (** Parse array into an ieee double REAL *)

  PROCEDURE StrToRealInvar*(IN  s  : ARRAY OF CHAR; 
                            OUT r  : REAL; 
                            OUT ok : BOOLEAN);
  (** Parse array using invariant culture, into an ieee double REAL *)

  PROCEDURE StrToRealLocal*(IN  s  : ARRAY OF CHAR; 
                            OUT r  : REAL; 
                            OUT ok : BOOLEAN);
  (** Parse array using current locale, into an ieee double REAL *)

  PROCEDURE StrToSReal*(IN  s  : ARRAY OF CHAR; 
                        OUT r  : SHORTREAL; 
                        OUT ok : BOOLEAN);
  PROCEDURE StrToSRealInvar*(IN  s  : ARRAY OF CHAR; 
                        OUT r  : SHORTREAL; 
                        OUT ok : BOOLEAN);
  PROCEDURE StrToSRealLocal*(IN  s  : ARRAY OF CHAR; 
                        OUT r  : SHORTREAL; 
                        OUT ok : BOOLEAN);
  (** Parse array into a short REAL *)

  (* ========================================================== *)
  (* ==============  Operations on Native Types  ============== *)
  (* ========================================================== *)

  PROCEDURE TypeName*(typ : NativeType) : CharOpen;

  (* ========================================================== *)
  (* ============== Operations on Native Strings ============== *)
  (* ========================================================== *)

  PROCEDURE CharAtIndex*(str : NativeString; idx : INTEGER) : CHAR;
  (* Get the character at zero-based index idx *)

  PROCEDURE Length*(str : NativeString) : INTEGER;
  (* Get the length of the native string *)

  (* ========================================================== *)
  (* ============== Conversions TO array of char ============== *)
  (* ========================================================== *)
  PROCEDURE RealToStr*(r : REAL; OUT s : ARRAY OF CHAR);
  (** Decode a CP REAL into an array *)

  PROCEDURE RealToStrInvar*(r : REAL; OUT s : ARRAY OF CHAR);
  (** Decode a CP REAL into an array in invariant culture *)

  PROCEDURE RealToStrLocal*(r : REAL; OUT s : ARRAY OF CHAR);
  (** Decode a CP REAL into an array in the current locale *)

  PROCEDURE SRealToStr*(r : SHORTREAL; OUT s : ARRAY OF CHAR);
  PROCEDURE SRealToStrInvar*(r : SHORTREAL; OUT s : ARRAY OF CHAR);
  PROCEDURE SRealToStrLocal*(r : SHORTREAL; OUT s : ARRAY OF CHAR);
  (** Decode a CP SHORTREAL into an array *)
  (* ========================================================== *)

  PROCEDURE IntToStr*(i : INTEGER; OUT s : ARRAY OF CHAR);
  (** Decode a CP INTEGER into an array *)

  PROCEDURE ObjToStr*(obj : ANYPTR; OUT s : ARRAY OF CHAR);
  (** Decode a CP INTEGER into an array *)

  PROCEDURE LongToStr*(i : LONGINT; OUT s : ARRAY OF CHAR);
  (** Decode a CP INTEGER into an array *)

  (* ========================================================== *)
  (* ========== Casts with no representation change =========== *)
  (* ========================================================== *)
  PROCEDURE realToLongBits*(r : REAL) : LONGINT;
  (** Convert an ieee double into a longint with same bit pattern *)

  PROCEDURE longBitsToReal*(l : LONGINT) : REAL;
  (** Convert an ieee double into a longint with same bit pattern *)

  PROCEDURE shortRealToIntBits*(r : SHORTREAL) : INTEGER;
  (** Convert an ieee float into an int with same bit pattern *)

  PROCEDURE intBitsToShortReal*(i : INTEGER) : SHORTREAL;
  (** Convert an int into an ieee float with same bit pattern *)

  PROCEDURE hiByte*(i : SHORTINT) : BYTE;
  (** Get hi-significant word of short *)

  PROCEDURE loByte*(i : SHORTINT) : BYTE;
  (** Get lo-significant word of short *)

  PROCEDURE hiShort*(i : INTEGER) : SHORTINT;
  (** Get hi-significant word of integer *)

  PROCEDURE loShort*(i : INTEGER) : SHORTINT;
  (** Get lo-significant word of integer *)

  PROCEDURE hiInt*(l : LONGINT) : INTEGER;
  (** Get hi-significant word of long integer *)

  PROCEDURE loInt*(l : LONGINT) : INTEGER;
  (** Get lo-significant word of long integer *)

  (* ========================================================== *)
  (* ============= Various utility procedures ================= *)
  (* ========================================================== *)
  PROCEDURE GetMillis*() : LONGINT;
  (** Get time in milliseconds *)

  PROCEDURE GetDateString*(OUT str : ARRAY OF CHAR);
  (** Get a date string in some native format *)

  PROCEDURE ClassMarker*(o : ANYPTR);
  (** Write class name to standard output *)

(* ============================================================ *)
END RTS.
