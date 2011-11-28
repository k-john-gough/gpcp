
/** This is part of the body of the GPCP runtime support.
 *
 *  Written November 1998, John Gough.
 *
 *  CP*rts contains the runtime helpers, this class has 
 *  adapters for hooking into the various Native libraries.
 *  These are the user accessible parts of the runtime.  The 
 *  facilities in CP*rts are known to each code-emitter, but 
 *  have no CP-accessible functions.  The interface to the 
 *  user-accessible functions are defined in RTS.cp, and the 
 *  code is defined in this file.
 *
 *  Version of 29 March 2000 (kjg) --
 *  There is a swindle involved here, for the bootstrap version
 *  of the compiler: any functions with OUT scalars will have
 *  a different signature in the old and new versions.  This 
 *  module implements both, by overloading the methods.
 *
 *  Version of October 2011 -- JVM version brought into line
 *  with the CP definition used by the current .NET version.
 *  Only the required methods are defined, the bootstrap 
 *  versions have been removed.
 */

package CP.RTS;

import java.io.*;
import CP.CPJ.*;
import CP.CPJrts.*;
import java.text.NumberFormat;

/* ------------------------------------------------------------ */
/* 		        Support for RTS.cp			*/
/* ------------------------------------------------------------ */
/*  The text of RTS.cp is interleaved here to associate the     */
/*  java with the promises of the Component Pascal source.      */
/* ------------------------------------------------------------ */
//
//  SYSTEM MODULE RTS;

public final class RTS
{
      /* Some Initializations ... */
      private static NumberFormat localFormat = NumberFormat.getInstance();
	
//  
//    VAR defaultTarget- : ARRAY 4 OF CHAR;
//        fltNegInfinity-   : SHORTREAL;
//        fltPosInfinity-   : SHORTREAL;
//        dblNegInfinity-   : REAL;
//        dblPosInfinity-   : REAL;

	public static char[] defaultTarget = {'j','v','m','\0'};
        public static float fltNegInfinity = Float.NEGATIVE_INFINITY;
        public static float fltPosInfinity = Float.POSITIVE_INFINITY;
        public static double dblNegInfinity = Double.NEGATIVE_INFINITY;
        public static double dblPosInfinity = Double.POSITIVE_INFINITY;
//  
//    TYPE  CharOpen*       = POINTER TO ARRAY OF CHAR;
//  
//    TYPE  NativeType*	= POINTER TO ABSTRACT RECORD END;
//          NativeObject*   = POINTER TO ABSTRACT RECORD END;  
//          NativeString*   = POINTER TO RECORD END;
//          NativeException*= POINTER TO EXTENSIBLE RECORD END;
//  
//    VAR   eol- : POINTER TO ARRAY OF CHAR; (* OS-specific end of line string *)
//
        public static char[] eol = { '\n', '\0' };
//  
//    (* ========================================================== *)
//    (* ============= Support for native exceptions ==============	*)
//    (* ========================================================== *)
//    PROCEDURE getStr*(x : NativeException) : CharOpen; 

	public static char[] getStr(java.lang.Exception x) {
	    String str = x.toString();
	    return CPJrts.JavaStrToChrOpen(str);
	}

//  
//  --------------------------------------------------------------
//    PROCEDURE Throw*(IN s : ARRAY OF CHAR);
//    (** Abort execution with an error *)

	public static void Throw(char[] s) throws Exception {
		throw new Exception(new String(s));
	}

/* ------------------------------------------------------------ */
//   PROCEDURE TypeName*(str : NativeType) : CharOpen;
//  (* Get the character at zero-based index idx *)
//
    public static char[] TypeName(java.lang.Class t) {
      return CPJrts.JavaStrToChrOpen(t.getSimpleName());
    }

/* ------------------------------------------------------------ */
//   PROCEDURE CharAtIndex*(str : NativeString; idx : INTEGER) : CHAR;
//  (* Get the character at zero-based index idx *)
//
    public static char CharAtIndex( String s, int i ) { return s.charAt(i); }

/* ------------------------------------------------------------ */
//  PROCEDURE Length*(str : NativeString) : INTEGER;
//  (* Get the length of the native string *)
//
    public static int Length( String s ) { return s.length(); }



//  
//    (* ========================================================== *)
//    (* ============= Conversions FROM array of char ============= *)
//    (* ========================================================== *)
//    PROCEDURE StrToBool*(IN s : ARRAY OF CHAR; OUT b : BOOLEAN; OUT ok : BOOLEAN);
//    (** Parse array into a BOOLEAN TRUE/FALSE *)
//
	public static boolean StrToBool(char[] str,
				        boolean[] r)	// OUT param
	{
	    try {
		r[0] = true;
		return Boolean.parseBoolean(CPJ.MkStr(str));
	    } catch(Exception e) {
		r[0] = false;
	        return false;
	    }
        }
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToByte*(IN s : ARRAY OF CHAR; OUT b : BYTE; OUT ok : BOOLEAN);
//    (** Parse array into a BYTE integer (unsigned byte in CP *)
//
	public static byte StrToByte(char[] str,
				     boolean[] r)	// OUT param
	{
	    try {
		r[0] = true;
                int value = Integer.parseInt(CPJ.MkStr(str));
                if (value >= -128 && value < 128)
                    return (byte)value;
	    } catch(Exception e) {
	    }
            r[0] = false;
	    return 0;
        }
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToUByte*(IN s : ARRAY OF CHAR; OUT b : BYTE; OUT ok : BOOLEAN);
//    (** Parse array into a BYTE integer *)
//
	public static byte StrToUByte(char[] str,
				      boolean[] r)	// OUT param
	{
	    try {
		r[0] = true;
                int value = Integer.parseInt(CPJ.MkStr(str));
                if (value >= 0 && value < 256)
                    return (byte)value;
	    } catch(Exception e) {
	    }
            r[0] = false;
	    return 0;
        }
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToShort*(IN s : ARRAY OF CHAR; OUT si : SHORTINT; OUT ok : BOOLEAN);
//    (** Parse an array into a CP SHORTINT *)
//
	public static short StrToShort(char[] str,
				       boolean[] r)	// OUT param
	{
	    try {
		r[0] = true;
                int value = Integer.parseInt(CPJ.MkStr(str));
                if (value >= -0x8000 && value < 0x7fff)
                    return (short)value;
	    } catch(Exception e) {
	    }
            r[0] = false;
	    return 0;
        }
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToUShort*(IN s:ARRAY OF CHAR; OUT si:SHORTINT; OUT ok:BOOLEAN);
//    (** Parse an array into a CP Unsigned SHORTINT *)
//
	public static short StrToUShort(char[] str,
				        boolean[] r)	// OUT param
	{
	    try {
		r[0] = true;
                int value = Integer.parseInt(CPJ.MkStr(str));
                if (value > 0 && value < 0xffff)
                    return (short)value;
	    } catch(Exception e) {
	    }
            r[0] = false;
	    return 0;
        }
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToInt*(IN s:ARRAY OF CHAR; OUT i:INTEGER; OUT ok:BOOLEAN);
//    (** Parse an array into a CP INTEGER *)
//    (*  Note that first OUT or VAR scalar becomes return value if a pure procedure *)
//  
	public static int StrToInt(char[] str,
				   boolean[] r)	// OUT param
	{
	    try {
		r[0] = true;
		return Integer.parseInt(CPJ.MkStr(str));
	    } catch(Exception e) {
		r[0] = false;
	        return 0;
	    }
	}
//
//  --------------------------------------------------------------
//    PROCEDURE StrToUInt*(IN s:ARRAY OF CHAR; OUT i:INTEGER; OUT ok:BOOLEAN);
//    (** Parse an array into a CP INTEGER *)
//
	public static int StrToUInt(char[] str,
				    boolean[] r) // OUT param
	{
	    try {
		r[0] = true;
                long value = Long.parseLong(CPJ.MkStr(str));
                if (value > 0 && value < 0xffffffff)
                    return (int)value;
	    } catch(Exception e) {
            }
	    r[0] = false;
	    return 0;
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToLong*(IN s:ARRAY OF CHAR; OUT i:LONGINT; OUT ok:BOOLEAN);
//    (** Parse an array into a CP LONGINT *)
//
	public static long StrToLong(char[] str,
				     boolean[] r) // OUT param
	{
	    try {
		r[0] = true;
		return Long.parseLong(CPJ.MkStr(str));
	    } catch(Exception e) {
		r[0] = false;
	        return 0;
	    }
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToULong*(IN s:ARRAY OF CHAR; OUT i:LONGINT; OUT ok:BOOLEAN);
//    (** Parse an array into a CP LONGINT *)
//
      // Throw method not found exception.
//  
//  --------------------------------------------------------------
//    PROCEDURE HexStrToUByte*(IN s:ARRAY OF CHAR; OUT b:BYTE; OUT ok:BOOLEAN);
//    (** Parse hexadecimal array into a BYTE integer *)
//
	public static byte HexStrToUByte(char[] str,
				 	 boolean[] r)	// OUT param
        {
	    try {
		r[0] = true;
		return Byte.decode(CPJ.MkStr(str)).byteValue();
	    } catch(Exception e) {
		r[0] = false;
	        return 0;
	    }
        }
//  
//  (* ------------------- Low-level String Conversions -------------------- *)
//  (* Three versions for different cultures.  *Invar uses invariant culture *)
//  (*                                         *Local uses current locale    *)
//  (* StrToReal & RealToStr do not behave the same on JVM and CLR.          *)
//  (* They is provided for compatability with versions < 1.3.1              *)
//  (* ------------------- Low-level String Conversions -------------------- *)
//  
//    PROCEDURE StrToReal*(IN  s  : ARRAY OF CHAR; 
//                         OUT r  : REAL; 
//                         OUT ok : BOOLEAN);
//    (** Parse array into an ieee double REAL *)
//
	public static double StrToReal(char[] str,
				       boolean[] r) // OUT param
	{
	    try {
		r[0] = true;
		return Double.valueOf(CPJ.MkStr(str)).doubleValue();
	    } catch(Exception e) {
		r[0] = false;
		return 0.0;
	    }
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToRealInvar*(IN  s  : ARRAY OF CHAR; 
//                              OUT r  : REAL; 
//                              OUT ok : BOOLEAN);
//    (** Parse array using invariant culture, into an ieee double REAL *)
//
	public static double StrToRealInvar(char[] str,
				            boolean[] r) // OUT param
	{
	    try {
		r[0] = true;
		return Double.valueOf(CPJ.MkStr(str)).doubleValue();
	    } catch(Exception e) {
		r[0] = false;
		return 0.0;
	    }
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToRealLocal*(IN  s  : ARRAY OF CHAR; 
//                              OUT r  : REAL; 
//                              OUT ok : BOOLEAN);
//    (** Parse array using current locale, into an ieee double REAL *)
//
	public static double StrToRealLocal(char[] str,
					    boolean[] r) // OUT param
	{
	    try {
		r[0] = true;
		return localFormat.parse(CPJ.MkStr(str)).doubleValue();
	    } catch(Exception e) {
		r[0] = false;
		return 0.0;
	    }
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE StrToSReal*(IN  s  : ARRAY OF CHAR; 
//                          OUT r  : SHORTREAL; 
//                          OUT ok : BOOLEAN);
//
	public static float StrToSReal(char[] str,
				       boolean[] r) // OUT param
	{
	    try {
		r[0] = true;
		return Float.valueOf(CPJ.MkStr(str)).floatValue();
	    } catch(Exception e) {
		r[0] = false;
		return 0.0F;
	    }
	}
//
//  --------------------------------------------------------------
//    PROCEDURE StrToSRealInvar*(IN  s  : ARRAY OF CHAR; 
//                               OUT r  : SHORTREAL; 
//                               OUT ok : BOOLEAN);
//
	public static float StrToSRealInvar(char[] str,
				            boolean[] r) // OUT param
	{
	    try {
		r[0] = true;
		return Float.valueOf(CPJ.MkStr(str)).floatValue();
	    } catch(Exception e) {
		r[0] = false;
		return 0.0F;
	    }
	}
//
//  --------------------------------------------------------------
//    PROCEDURE StrToSRealLocal*(IN  s  : ARRAY OF CHAR; 
//                          OUT r  : SHORTREAL; 
//                          OUT ok : BOOLEAN);
//    (** Parse array into a short REAL *)
//  
	public static float StrToSRealLocal(char[] str,
					     boolean[] r) // OUT param
	{
	    try {
		r[0] = true;
		return localFormat.parse(CPJ.MkStr(str)).floatValue();
	    } catch(Exception e) {
		r[0] = false;
		return 0.0F;
	    }
	}
//
//    (* ========================================================== *)
//    (* ============== Conversions TO array of char ============== *)
//    (* ========================================================== *)
//    PROCEDURE RealToStr*(r : REAL; OUT s : ARRAY OF CHAR);
//    (** Decode a CP REAL into an array *)
//  
	public static void RealToStr(double num,
				     char[] str)
	{
	    String jls = String.valueOf(num);
            int    len = jls.length();
            if (len >= str.length)
                len = str.length - 1;
            jls.getChars(0, len, str, 0);
            str[len] = '\0';
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE RealToStrInvar*(r : REAL; OUT s : ARRAY OF CHAR);
//    (** Decode a CP REAL into an array in invariant culture *)
//  
	public static void RealToStrInvar(double num,
				          char[] str)
	{
	    String jls = String.valueOf(num);
            int    len = jls.length();
            if (len >= str.length)
                len = str.length - 1;
            jls.getChars(0, len, str, 0);
            str[len] = '\0';
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE RealToStrLocal*(r : REAL; OUT s : ARRAY OF CHAR);
//    (** Decode a CP REAL into an array in the current locale *)
//  
	public static void RealToStrLocal(double num,
				          char[] str)
	{
	    String jls = localFormat.format(num);
            int    len = jls.length();
            if (len >= str.length)
                len = str.length - 1;
            jls.getChars(0, len, str, 0);
            str[len] = '\0';
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE SRealToStr*(r : SHORTREAL; OUT s : ARRAY OF CHAR);
//  
	public static void SRealToStr(float num,
				      char[] str)
	{
	    String jls = Float.toString(num);
            int    len = jls.length();
            if (len >= str.length)
                len = str.length - 1;
            jls.getChars(0, len, str, 0);
            str[len] = '\0';
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE SRealToStrInvar*(r : SHORTREAL; OUT s : ARRAY OF CHAR);
//  
	public static void SRealToStrInvar(float num,
				           char[] str)
	{
	    String jls = Float.toString(num);
            int    len = jls.length();
            if (len >= str.length)
                len = str.length - 1;
            jls.getChars(0, len, str, 0);
            str[len] = '\0';
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE SRealToStrLocal*(r : SHORTREAL; OUT s : ARRAY OF CHAR);
//    (** Decode a CP SHORTREAL into an array *)
//  
	public static void SRealToStrLocal(float num,
				           char[] str)
	{
	    String jls = localFormat.format(num);
            int    len = jls.length();
            if (len >= str.length)
                len = str.length - 1;
            jls.getChars(0, len, str, 0);
            str[len] = '\0';
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE IntToStr*(i : INTEGER; OUT s : ARRAY OF CHAR);
//    (** Decode a CP INTEGER into an array *)
//  
	public static void IntToStr(int num,
				    char[] str)
	{
	    String jls = String.valueOf(num);
            int    len = jls.length();
            if (len >= str.length)
                len = str.length - 1;
            jls.getChars(0, len, str, 0);
            str[len] = '\0';
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE ObjToStr*(obj : ANYPTR; OUT s : ARRAY OF CHAR);
//    (** Decode a CP INTEGER into an array *)
//  
        public static void ObjToStr(Object obj, char[] str) {
            CPJ.MkArr(obj.getClass().getName(), str);
        }
//  
//  --------------------------------------------------------------
//    PROCEDURE LongToStr*(i : LONGINT; OUT s : ARRAY OF CHAR);
//    (** Decode a CP INTEGER into an array *)
//  
	public static void LongToStr(long num,
				    char[] str)
	{
	    String jls = String.valueOf(num);
            int    len = jls.length();
            if (len >= str.length)
                len = str.length - 1;
            jls.getChars(0, len, str, 0);
            str[len] = '\0';
	}
//  
//    (* ========================================================== *)
//    (* ========== Casts with no representation change =========== *)
//    (* ========================================================== *)
//    PROCEDURE realToLongBits*(r : REAL) : LONGINT;
//    (** Convert an ieee double into a longint with same bit pattern *)
//
	public static long realToLongBits(double r) {
	    return java.lang.Double.doubleToLongBits(r);
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE longBitsToReal*(l : LONGINT) : REAL;
//    (** Convert an ieee double into a longint with same bit pattern *)
//  
	public static double longBitsToReal(long l) {
	    return java.lang.Double.longBitsToDouble(l);
	}
//
//  --------------------------------------------------------------
//    PROCEDURE shortRealToIntBits*(r : SHORTREAL) : INTEGER;
//    (** Convert an ieee float into an int with same bit pattern *)
//  
	public static int shortRealToIntBits(float f) {
	    return Float.floatToIntBits(f);
	}
//
//  --------------------------------------------------------------
//    PROCEDURE intBitsToShortReal*(i : INTEGER) : SHORTREAL;
//    (** Convert an int into an ieee float with same bit pattern *)
//  
	public static float intBitsToShortReal(int i) {
	    return Float.intBitsToFloat(i);
	}
//
//  --------------------------------------------------------------
//    PROCEDURE hiByte*(i : SHORTINT) : BYTE;
//    (** Get hi-significant word of short *)
//  
	public static byte hiByte(short s) {
	    return (byte) (s >> 8);
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE loByte*(i : SHORTINT) : BYTE;
//    (** Get lo-significant word of short *)
//  
	public static byte loByte(short s) {
	    return (byte) s;
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE hiShort*(i : INTEGER) : SHORTINT;
//    (** Get hi-significant word of integer *)
//  
	public static short hiShort(int i) {
	    return (short) (i >> 16);
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE loShort*(i : INTEGER) : SHORTINT;
//    (** Get lo-significant word of integer *)
//  
	public static short loShort(int i) {
	    return (short) i;
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE hiInt*(l : LONGINT) : INTEGER;
//    (** Get hi-significant word of long integer *)
//  
	public static int hiInt(long l) {
	    return (int) (l >> 32);
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE loInt*(l : LONGINT) : INTEGER;
//    (** Get lo-significant word of long integer *)
//  
	public static int loInt(long l) {
	    return (int) l;
	}
//  
//    (* ========================================================== *)
//    (* ============= Various utility procedures ================= *)
//    (* ========================================================== *)
//
//    PROCEDURE GetMillis*() : LONGINT;
//    (** Get time in milliseconds *)

	public static long GetMillis() {
	    return System.currentTimeMillis();
	}
//
//  --------------------------------------------------------------
//    PROCEDURE GetDateString*(OUT str : ARRAY OF CHAR);
//    (** Get a date string in some native format *)
//
	public static void GetDateString(char[] str) {
	    String date = new java.util.Date().toString();
	    int len = date.length();
	    date.getChars(0, len, str, 0);
	    str[len] = '\0';
	}
//  
//  --------------------------------------------------------------
//    PROCEDURE ClassMarker*(o : ANYPTR);
//    (** Write class name to standard output *)
//
	public static void ClassMarker(Object o) {
	    System.out.print(o.getClass().getName());
	}
//  
//  END RTS.
  /* ------------------------------------------------------------ */
  /* ------------------------------------------------------------ */
  /* ------------------------------------------------------------ */
}

