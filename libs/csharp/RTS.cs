/** This is the body of the GPCP runtime support.
 *
 *  Written November 1998, John Gough. (for jdk version)
 *  This version for Lightning, May 2000, John Gough.
 *  Support for uplevel addressing RTS.XHR, Aug-2001.
 *  Merged version for N2CPS, gpcp etc.  SYChan, KJGough. 19-Aug-2001.
 */

#if !BETA1
  #define BETA2
#endif

public class RTS
// Known in ILASM as [RTS]RTS
{
/** This is part of the body of the GPCP runtime support.
 *
 *  Written November 1998, John Gough.
 *  This version for Lightning, May 2000, John Gough.
 */

/* ------------------------------------------------------------ */
/* 		        Support for RTS.cp			*/
/* ------------------------------------------------------------ */

  public static char[] defaultTarget = {'n','e','t','\0'};
  public static char[] eol = NativeStrings.mkArr(System.Environment.NewLine);

  public static double dblPosInfinity = System.Double.PositiveInfinity;
  public static double dblNegInfinity = System.Double.NegativeInfinity;
  public static float  fltPosInfinity = System.Single.PositiveInfinity;
  public static float  fltNegInfinity = System.Single.NegativeInfinity;

  private static char[] ChrNaN    = {'N','a','N','\0'};
  private static char[] ChrPosInf = {'I','n','f','i','n','i','t','y','\0'};
  private static char[] ChrNegInf = {'-','I','n','f','i','n','i','t','y','\0'};
  private static System.String StrNaN    = new System.String(ChrNaN); 
  private static System.String StrPosInf = new System.String(ChrPosInf);
  private static System.String StrNegInf = new System.String(ChrNegInf);

  private static System.Type typDouble = System.Type.GetType("System.Double");
  private static System.Type typSingle = System.Type.GetType("System.Single");

  private static System.IFormatProvider invarCulture = 
             (System.IFormatProvider) new System.Globalization.CultureInfo("");
  private static System.IFormatProvider currentCulture = 
      (System.IFormatProvider) System.Globalization.CultureInfo.CurrentCulture;

/* -------------------------------------------------------------------- */
//  PROCEDURE getStr*(x : NativeException) : RTS.CharOpen; END getStr;
//
    	// Known in ILASM as [RTS]RTS::getStr
	public static char[] getStr(System.Exception inp)
	{
	    return CP_rts.strToChO(inp.ToString());
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToBool(IN str : ARRAY OF CHAR;
//                     OUT b  : BOOLEAN;
//                     OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToBool
	public static void StrToBool(char[]  str,
				     out bool  o, 	// OUT param
				     out bool  r)	// OUT param
	{
	    System.String bstr = new System.String(str);
	    try {
		o = System.Boolean.Parse(bstr);
		r = true;
	    } catch {
		o = false;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToByte(IN str : ARRAY OF CHAR;
//                     OUT b  : BYTE;
//                     OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToByte
	public static void StrToByte(char[]  str,
				     out sbyte  o, 	// OUT param
				     out bool  r)	// OUT param
	{
	    System.String bstr = new System.String(str);
	    try {
		o = System.SByte.Parse(bstr);
		r = true;
	    } catch {
		o = 0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToUByte(IN str : ARRAY OF CHAR;
//                      OUT b  : BYTE;
//                      OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToUByte
	public static void StrToUByte(char[]  str,
				     out sbyte  o, 	// OUT param
				     out bool  r)	// OUT param
	{
	    System.String bstr = new System.String(str);
	    try {
		o = (sbyte)System.Byte.Parse(bstr);
		r = true;
	    } catch {
		o = (sbyte)0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE HexStrToUByte(IN s : ARRAY OF CHAR; OUT b : BYTE; OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::HexStrToUByte
	public static void HexStrToUByte(char[]  str,
				     out sbyte  o, 	// OUT param
				     out bool  r)	// OUT param
	{
	    System.String bstr = new System.String(str);
	    try {
		o = (sbyte)System.Byte.Parse
                           (bstr, System.Globalization.NumberStyles.HexNumber);
		r = true;
	    } catch {
		o = (sbyte)0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToShort(IN str : ARRAY OF CHAR;
//                      OUT i  : SHORTINT;
//                      OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToShort
	public static void StrToShort(char[] str,
				     out short  o,	// OUT param
				     out bool r)	// OUT param
	{
	    System.String sstr = new System.String(str);
	    try {
		o = System.Int16.Parse(sstr);
		r = true;
	    } catch {
		o = (short) 0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToUShort(IN str : ARRAY OF CHAR;
//                       OUT i  : SHORTINT;
//                       OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToUShort
	public static void StrToUShort(char[] str,
				     out short  o,	// OUT param
				     out bool r)	// OUT param
	{
	    System.String sstr = new System.String(str);
	    try {
		o = (short)System.UInt16.Parse(sstr);
		r = true;
	    } catch {
		o = (short) 0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToInt(IN str : ARRAY OF CHAR;
//                    OUT i  : INTEGER;
//                    OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToInt
	public static void StrToInt(char[] str,
				    out int  o,		// OUT param
				    out bool r)		// OUT param
	{
	    System.String lstr = new System.String(str);
	    try {
		o = System.Int32.Parse(lstr);
		r = true;
	    } catch {
		o = 0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToUInt(IN str : ARRAY OF CHAR;
//                     OUT i  : INTEGER;
//                     OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToUInt
	public static void StrToUInt(char[] str,
				    out int o,		// OUT param
				    out bool r)		// OUT param
	{
	    System.String lstr = new System.String(str);
	    try {
		o = (int)System.UInt32.Parse(lstr);
		r = true;
	    } catch {
		o = (int)0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToLong(IN str : ARRAY OF CHAR;
//                     OUT l  : LONGINT;
//                     OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToLong
	public static void StrToLong(char[] str,
				     out long  o,	// OUT param
				     out bool r)	// OUT param
	{
	    System.String lstr = new System.String(str);
	    try {
		o = System.Int64.Parse(lstr);
		r = true;
	    } catch {
		o = (long) 0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToULong(IN str : ARRAY OF CHAR;
//                      OUT l  : LONGINT;
//                      OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToULong
	public static void StrToULong(char[] str,
				     out long o,	// OUT param
				     out bool r)	// OUT param
	{
	    System.String lstr = new System.String(str);
	    try {
		o = (long)System.UInt64.Parse(lstr);
		r = true;
	    } catch {
		o = (long) 0;
		r = false;
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToSReal(IN str : ARRAY OF CHAR;
//                      OUT r  : SHORTREAL;
//                      OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToSReal
	public static void StrToSReal(char[] str,
				     out float  o, 	// OUT param
				     out bool   r)	// OUT param
	{
	    System.String rstr = new System.String(str);
	    try {
		o = System.Single.Parse(rstr);
		r = true;
	    } catch {
                if (System.String.Compare(rstr, StrPosInf) == 0) {
                    o = System.Single.PositiveInfinity;
                    r = true;
                }
                else if (System.String.Compare(rstr, StrNegInf) == 0) {
                    o = System.Single.NegativeInfinity;
                    r = true;
                }
                else if (System.String.Compare(rstr, StrNaN) == 0) {
                    o = System.Single.NaN;
                    r = true;
                }
                else {
		    o = 0;
		    r = false;
                }
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToReal(IN str : ARRAY OF CHAR;
//                     OUT r  : REAL;
//                     OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToReal
	public static void StrToReal(char[] str,
				     out double o, 	// OUT param
				     out bool   r)	// OUT param
	{
	    System.String rstr = new System.String(str);
	    try {
		o = System.Double.Parse(rstr);
		r = true;
	    } catch {
                if (System.String.Compare(rstr, StrPosInf) == 0) {
                    o = System.Double.PositiveInfinity;
                    r = true;
                }
                else if (System.String.Compare(rstr, StrNegInf) == 0) {
                    o = System.Double.NegativeInfinity;
                    r = true;
                }
                else if (System.String.Compare(rstr, StrNaN) == 0) {
                    o = System.Double.NaN;
                    r = true;
                }
                else {
		    o = 0.0;
		    r = false;
                }
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToRealInvar(IN str : ARRAY OF CHAR;
//                          OUT r  : REAL;
//                          OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToReal
	public static void StrToRealInvar(char[] str,
				      out double o, 	// OUT param
				      out bool   r)	// OUT param
	{
	    System.String rstr = new System.String(str);
	    try {
		o = System.Double.Parse(rstr, invarCulture);
		r = true;
	    } catch {
                if (System.String.Compare(rstr, StrPosInf) == 0) {
                    o = System.Double.PositiveInfinity;
                    r = true;
                }
                else if (System.String.Compare(rstr, StrNegInf) == 0) {
                    o = System.Double.NegativeInfinity;
                    r = true;
                }
                else if (System.String.Compare(rstr, StrNaN) == 0) {
                    o = System.Double.NaN;
                    r = true;
                }
                else {
		    o = 0.0;
		    r = false;
                }
	    }
	}

/* ------------------------------------------------------------ */
// PROCEDURE StrToRealLocal(IN str : ARRAY OF CHAR;
//                          OUT r  : REAL;
//                          OUT ok : BOOLEAN);
//
	// Known in ILASM as [RTS]RTS::StrToReal
	public static void StrToRealLocal(char[] str,
				      out double o, 	// OUT param
				      out bool   r)	// OUT param
	{
	    System.String rstr = new System.String(str);
	    try {
		o = System.Double.Parse(rstr, currentCulture);
		r = true;
	    } catch {
                if (System.String.Compare(rstr, StrPosInf) == 0) {
                    o = System.Double.PositiveInfinity;
                    r = true;
                }
                else if (System.String.Compare(rstr, StrNegInf) == 0) {
                    o = System.Double.NegativeInfinity;
                    r = true;
                }
                else if (System.String.Compare(rstr, StrNaN) == 0) {
                    o = System.Double.NaN;
                    r = true;
                }
                else {
		    o = 0.0;
		    r = false;
                }
	    }
	}

/* ------------------------------------------------------------ */
//  PROCEDURE ByteToStr*(i : BYTE; OUT s : ARRAY OF CHAR);
//  (** Decode a CP BYTE into an array *)
//  BEGIN END ByteToStr;
//
	// Known in ILASM as [RTS]RTS::ByteToStr
	public static void ByteToStr(sbyte num,
				    char[] str)
	{
	    System.String lls = System.Convert.ToString(num);
            int    len = lls.Length;
            if (len >= str.Length)
                len = str.Length - 1;
	    lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
	}

/* ------------------------------------------------------------ */
//  PROCEDURE ShortToStr*(i : SHORTINT; OUT s : ARRAY OF CHAR);
//  (** Decode a CP SHORTINT into an array *)
//  BEGIN END ShortToStr;
//
	// Known in ILASM as [RTS]RTS::ShortToStr
	public static void ShortToStr(short    num,
				    char[] str)
	{
	    System.String lls = System.Convert.ToString(num);
            int    len = lls.Length;
            if (len >= str.Length)
                len = str.Length - 1;
	    lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
	}

/* ------------------------------------------------------------ */
//  PROCEDURE IntToStr*(i : INTEGER; OUT s : ARRAY OF CHAR);
//  (** Decode a CP INTEGER into an array *)
//  BEGIN END IntToStr;
//
	// Known in ILASM as [RTS]RTS::IntToStr
	public static void IntToStr(int    num,
				    char[] str)
	{
	    System.String lls = System.Convert.ToString(num);
            int    len = lls.Length;
            if (len >= str.Length)
                len = str.Length - 1;
	    lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
	}

/* ------------------------------------------------------------ */
//  PROCEDURE ObjToStr(obj: mscorlib_System.Object; OUT str: ARRAY OF CHAR);
//  (** Decode an .NET object into an array of CHAR *)
//  BEGIN END ObjToStr;
//
        // Known in ILASM as [RTS]RTS::ObjToStr
        public static void ObjToStr(System.Object obj, char[] str)
        {
            System.String lls;
            if (obj.GetType().IsEnum) {
#if BETA1
                lls = obj.ToString();
#else //BETA2
                lls = System.Convert.ToString(System.Convert.ToInt64(obj));
#endif
            }
            else {
                if (obj.GetType().Equals(typDouble)) {
#if BETA1
                    lls = System.Convert.ToDouble(obj).ToString();
#else //BETA2
                    lls = System.Convert.ToDouble(obj).ToString("R");
#endif
                }
                else if (obj.GetType().Equals(typSingle)) {
#if BETA1
                    lls = System.Convert.ToSingle(obj).ToString();
#else //BETA2
                    lls = System.Convert.ToSingle(obj).ToString("R");
#endif
                }
                else {
                    lls = System.Convert.ToString(obj);
                }
            }

            int    len = lls.Length;
            if (len >= str.Length)
                len = str.Length - 1;
            lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
        }

/* ------------------------------------------------------------ */
//  PROCEDURE LongToStr*(i : LONGINT; OUT s : ARRAY OF CHAR);
//  (** Decode a CP LONGINT into an array *)
//  BEGIN END LongToStr;
//
	// Known in ILASM as [RTS]RTS::LongToStr
	public static void LongToStr(long   num,
				     char[] str)
	{
	    System.String lls = System.Convert.ToString(num);
            int    len = lls.Length;
            if (len >= str.Length)
                len = str.Length - 1;
	    lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
	}

/* ------------------------------------------------------------ */
//  PROCEDURE SRealToStr*(r : SHORTREAL; OUT s : ARRAY OF CHAR);
//  (** Decode a CP REAL into an array *)
//  BEGIN END SRealToStr;
//
	// Known in ILASM as [RTS]RTS::SRealToStr
	public static void SRealToStr(float num,
				     char[] str)
	{
	 // System.String lls = System.Convert.ToString(num);
#if BETA1
            System.String lls = ((System.Single) num).ToString();
#else //BETA2
            System.String lls = ((System.Single) num).ToString("R");
#endif
            int    len = lls.Length;
            lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
	}

/* ------------------------------------------------------------ */
//  PROCEDURE RealToStr*(r : REAL; OUT s : ARRAY OF CHAR);
//  (** Decode a CP REAL into an array *)
//  BEGIN END RealToStr;
//
	// Known in ILASM as [RTS]RTS::RealToStr
	public static void RealToStr(double num,
				     char[] str)
	{
#if BETA1
	    System.String lls = System.Convert.ToString(num);
#else //BETA2
            System.String lls = ((System.Double) num).ToString("R");
#endif
            int    len = lls.Length;
            lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
	}

/* ------------------------------------------------------------ */
//  PROCEDURE RealToStrInvar*(r : REAL; OUT s : ARRAY OF CHAR);
//  (** Decode a CP REAL into an array *)
//  BEGIN END RealToStrInvar;
//
	// Known in ILASM as [RTS]RTS::RealToStrInvar
	public static void RealToStrInvar(double num,
				          char[] str)
	{
#if BETA1
	    System.String lls = System.Convert.ToString(num);
#else //BETA2
            System.String lls = 
                        ((System.Double) num).ToString("R", invarCulture);
#endif
            int    len = lls.Length;
            lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
	}

/* ------------------------------------------------------------ */
//  PROCEDURE RealToStrLocal*(r : REAL; OUT s : ARRAY OF CHAR);
//  (** Decode a CP REAL into an array *)
//  BEGIN END RealToStrInvar;
//
	// Known in ILASM as [RTS]RTS::RealToStrLocal
	public static void RealToStrLocal(double num,
				          char[] str)
	{
#if BETA1
	    System.String lls = System.Convert.ToString(num);
#else //BETA2
            System.String lls = 
                        ((System.Double) num).ToString("R", currentCulture);
#endif
            int    len = lls.Length;
            lls.CopyTo(0, str, 0, len);
            str[len] = '\0';
	}

/* ------------------------------------------------------------ */
//
//  PROCEDURE realToLongBits(r : REAL) : LONGINT;
//  (** Convert an ieee double into a longint with same bit pattern *)
//
  	public static long realToLongBits(double r)
  	{
	    byte[] tmp = System.BitConverter.GetBytes(r);
  	    return System.BitConverter.ToInt64(tmp,0);
  	}
//
//  PROCEDURE longBitsToReal(l : LONGINT) : REAL;
//  (** Convert a longint into an ieee double with same bit pattern *)
//
  	public static double longBitsToReal(long l)
  	{
	    byte[] tmp = System.BitConverter.GetBytes(l);
  	    return System.BitConverter.ToDouble(tmp,0);
  	}

/* ------------------------------------------------------------ */
//
//  PROCEDURE shortRealToIntBits(r : SHORTREAL) : INTEGER;
//  (** Convert an ieee double into a longint with same bit pattern *)
//
  	public static int shortRealToIntBits(float r)
  	{
	    byte[] tmp = System.BitConverter.GetBytes(r);
  	    return System.BitConverter.ToInt32(tmp,0);
  	}
//
//  PROCEDURE intBitsToShortReal(l : INTEGER) : SHORTREAL;
//  (** Convert an int into an ieee float with same bit pattern *)
//
  	public static float intBitsToShortReal(int l)
  	{
	    byte[] tmp = System.BitConverter.GetBytes(l);
  	    return System.BitConverter.ToSingle(tmp,0);
  	}

/* ------------------------------------------------------------ */
//
//  PROCEDURE hiByte(l : SHORTINT) : BYTE;
//  (** Get hi-significant byte of short *)
//
	// Known in ILASM as [RTS]RTS::hiByte
	public static sbyte hiByte(short i)
	{
	    return (sbyte) (i >> 8);
	}
//
//  PROCEDURE loByte(l : SHORTINT) : BYTE;
//  (** Get lo-significant byte of short *)
//
	// Known in ILASM as [RTS]RTS::loByte
	public static sbyte loByte(short i)
	{
	    return (sbyte) i;
	}
//
//  PROCEDURE hiShort(l : INTEGER) : SHORTINT;
//  (** Get hi-significant word of integer *)
//
	// Known in ILASM as [RTS]RTS::hiShort
	public static short hiShort(int i)
	{
	    return (short) (i >> 16);
	}
//
//  PROCEDURE loShort(l : INTEGER) : SHORTINT;
//  (** Get lo-significant word of integer *)
//
	// Known in ILASM as [RTS]RTS::loShort
	public static short loShort(int i)
	{
	    return (short) i;
	}
//
//  PROCEDURE hiInt(l : LONGINT) : INTEGER;
//  (** Get hi-significant word of long integer *)
//
	// Known in ILASM as [RTS]RTS::hiInt
	public static int hiInt(long l)
	{
	    return (int) (l >> 32);
	}
//
//  PROCEDURE loInt(l : LONGINT) : INTEGER;
//  (** Get lo-significant word of long integer *)
//
	// Known in ILASM as [RTS]RTS::loInt
	public static int loInt(long l)
	{
	    return (int) l;
	}
//
//  PROCEDURE Throw(IN s : ARRAY OF CHAR);
//  (** Abort execution with an error *)
//
	// Known in ILASM as [RTS]RTS::Throw
	public static void Throw(char[] s)
	{
		throw new System.Exception(new System.String(s));
	}
//
//  PROCEDURE GetMillis() : LONGINT;
//
	// Known in ILASM as [RTS]RTS::GetMillis
	public static long GetMillis()
	{
	    return (System.DateTime.Now.Ticks / 10000);
	}
//
//  PROCEDURE GetDateString(OUT str : ARRAY OF CHAR);
//
	// Known in ILASM as [RTS]RTS::GetDateString
	public static void GetDateString(char[] arr)
	{
	    System.String str = System.DateTime.Now.ToString();
            int len = str.Length;
            if (len >= arr.Length)
                len = arr.Length - 1;
	    str.CopyTo(0, arr, 0, len);
            arr[len] = '\0';
	}

	public static void ClassMarker(System.Object o)
	{
	    System.Console.Write(o.GetType().ToString());
	}
}
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */


public class ProgArgs
// Known in ILASM as [RTS]ProgArgs
/* 
 *  Library module for GP Component Pascal.
 *  This module allows access to the arguments in programs which
 *  import CPmain.  It is accessible from modules which do NOT
 *  import CPmain.
 *
 *  Original : kjg December 1999
 */
{
	public static System.String[] argList = null;

	// Known in ILASM as [RTS]ProgArgs::ArgNumber
	// PROCEDURE ArgNumber*() : INTEGER
	public static int ArgNumber()
	{
	    if (ProgArgs.argList == null)
		return 0;
	    else
		return argList.Length;
	}

	// Known in ILASM as [RTS]ProgArgs::GetArg
	// PROCEDURE GetArg*(num : INTEGER; OUT arg : ARRAY OF CHAR) 
	public static void GetArg(int num, char[] arr)
	{
	    int i;
	    if (argList == null) {
		arr[0] = '\0';
	    } else {
		for (i = 0; 
		     i < arr.Length && i < argList[num].Length;
		     i++) {
		    System.String str = argList[num];
		    arr[i] = str[i];
		}
		if (i == arr.Length)
		    i--;
		arr[i] = '\0';
	    }
	}

        public static void GetEnvVar(char[] name, char[] valu) {
            System.String nam = CP_rts.mkStr(name);
            System.String val = System.Environment.GetEnvironmentVariable(nam);
            CP_rts.StrToChF(valu, val);
        }

} // end of public class ProgArgs

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

public class CP_rts	
// Known in ILASM as [RTS]CP_rts
/*
 *  It is a fundamental principle that the facilities of CP_rts
 *  are known to the particular compiler backend, but are
 *  not accessible to the Programmer.  The programmer-accessible
 *  parts of the runtime are known via the module RTS.
 */
{
/* ==================================================================== *
 *		MOD and DIV helpers. With correction factors		*
 * ==================================================================== */

	// Known in ILASM as [RTS]CP_rts::CpModI
	public static int CpModI(int lVal, int rVal)
	{
           // A correction is required if the signs of
           // the two operands are different, but the
           // remainder is non-zero. Inc rem by rVal.
	    int rslt = lVal % rVal;
            if ((lVal < 0 != rVal < 0) && (rslt != 0))
                            rslt += rVal;
	    return rslt;
	}

	// Known in ILASM as [RTS]CP_rts::CpDivI
	public static int CpDivI(int lVal, int rVal)
	{
           // A correction is required if the signs of
           // the two operands are different, but the
           // remainder is non-zero. Dec quo by 1.
            int rslt = lVal / rVal;
            int remV = lVal % rVal;
            if ((lVal < 0 != rVal < 0) && (remV != 0))
              rslt--;
	    return rslt;
	}

	// Known in ILASM as [RTS]CP_rts::CpModL
	public static long CpModL(long lVal, long rVal)
	{
           // A correction is required if the signs of
           // the two operands are different, but the
           // remainder is non-zero. Inc rem by rVal.
	    long rslt = lVal % rVal;
            if ((lVal < 0 != rVal < 0) && (rslt != 0))
                            rslt += rVal;
	    return rslt;
	}

	// Known in ILASM as [RTS]CP_rts::CpDivL
	public static long CpDivL(long lVal, long rVal)
	{
           // A correction is required if the signs of
           // the two operands are different, but the
           // remainder is non-zero. Dec quo by 1.
            long rslt = lVal / rVal;
            long remV = lVal % rVal;
            if ((lVal < 0 != rVal < 0) && (remV != 0))
              rslt--;
	    return rslt;
	}

/* ==================================================================== *
 *		Various string and char-array helpers			*
 * ==================================================================== */

	// Known in ILASM as [RTS]CP_rts::mkStr
	public static System.String mkStr(char[] arr) {
	    int len = chrArrLength(arr);
	    return new System.String(arr,0,len);
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::caseMesg
	public static System.String caseMesg(int i)
	{
	    System.String s = "CASE-trap: selector = " + i;
	    return s;
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::withMesg
	public static System.String withMesg(System.Object o)
	{
	//    System.String c = o.getClass().getName();
	//    c = c.substring(c.lastIndexOf('.') + 1);
	//    c = "WITH else-trap: type = " + c;
	//  NEEDS TO USE LIGHTNING REFLECTION SERVICES HERE
	    string c = "WITH else-trap: type = " + o.ToString();
	    return c;
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::chrArrLength
	public static int chrArrLength(char[] src)
	{
	    int  ix = 0;
	    char ch;
	    do {
		ch = src[ix];
		ix++;
	    } while (ch != '\0');
// System.Console.Write(ix-1);
// System.Console.Write(' ');
// System.Console.WriteLine(src);
	    return ix-1;
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::chrArrLplus1
	public static int chrArrLplus1(char[] src)
	{
	    int  ix = 0;
	    char ch;
	    do {
		ch = src[ix];
		ix++;
	    } while (ch != '\0');
	    return ix;
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::strToChO
	public static char[] strToChO(System.String input)
	{
            if (input == null) return null;

	    int    len = input.Length;
	    char[] arr = new char[len+1];
	    input.CopyTo(0, arr, 0, len);
	    arr[len] = '\0';
	    return arr;
	//  return input.ToCharArray();
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::StrToChF
	public static void StrToChF(char[] res, System.String inp)
	{
            if (inp == null) {
                res[0] = '\0'; return;
            }
	    int    len = inp.Length;
	    inp.CopyTo(0, res, 0, len);
	    res[len] = '\0';
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::Stringify
	public static void Stringify(char[] dst, char[] src)
	{
	    int  ix = 0;
	    char ch;
	    do {
		ch = src[ix];
		dst[ix] = ch;
		ix++;
	    } while (ch != '\0');
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::ChrArrCheck
	public static void ChrArrCheck(char[] src)
	{
	    int  ix = 0;
	    char ch;
	    do {
		ch = src[ix];
		if (ch > 0xFF) throw new 
			System.Exception("error applying SHORT to array");
		ix++;
	    } while (ch != '\0');
	}

/* -------------------------------------------------------------------- */

	// Known in ILASM as [RTS]CP_rts::strCmp
        public static int strCmp(char[] l, char[] r)
	{
	    int minL;
	    int lLen = chrArrLength(l);
	    int rLen = chrArrLength(r);
	    if (lLen < rLen) minL = lLen; else minL = rLen;
// System.Console.WriteLine();
	    for (int ix = 0; ix < minL; ix++) {
		char lCh = l[ix];
		char rCh = r[ix];
		if (lCh < rCh) return -1;
		else if (lCh > rCh) return 1;
	    }
	    if (lLen < rLen) return -1;
	    else if (lLen > rLen) return 1;
	    else return 0;
	}

/* ==================================================================== *
 *		String concatenation helper methods			*
 * ==================================================================== */

	// Known in ILASM as [RTS]CP_rts::aaToStr
	public static System.String aaToStr(char[] l, char[] r)
	{
	    int llen = chrArrLength(l);
	    int rlen = chrArrLength(r);
	    System.Text.StringBuilder buff = 
				new System.Text.StringBuilder(llen + rlen);
	    return buff.Append(l,0,llen).Append(r,0,rlen).ToString();
	}

	// Known in ILASM as [RTS]CP_rts::asToStr
	public static System.String asToStr(char[] l, System.String r)
	{
	    int llen = chrArrLength(l);
	    System.Text.StringBuilder buff = 
				new System.Text.StringBuilder(3 * llen);
	    return buff.Append(l,0,llen).Append(r).ToString();
	}

	// Known in ILASM as [RTS]CP_rts::saToStr
	public static System.String saToStr(System.String l, char[] r)
	{
	    int rlen = chrArrLength(r);
	    System.Text.StringBuilder buff = 
				new System.Text.StringBuilder(3 * rlen);
	    return buff.Append(l).Append(r,0,rlen).ToString();
	}

	// Known in ILASM as [RTS]CP_rts::ssToStr
	public static System.String ssToStr(System.String l, System.String r)
	{
	    System.Text.StringBuilder buff = 
				new System.Text.StringBuilder(l);
	    return buff.Append(r).ToString();
	}
}

/* ==================================================================== */
/* ==================================================================== */
/* ==================================================================== */
/* ==================================================================== */

public class NativeStrings	
// Known in ILASM as [RTS]NativeStrings
{
/* -------------------------------------------------------------------- */
//
// PROCEDURE mkStr*(IN s : ARRAY OF CHAR) : String; BEGIN RETURN NIL END mkStr;
//
/* -------------------------------------------------------------------- */
	// Known in ILASM as [RTS]NativeStrings::mkStr
	public static System.String mkStr(char[] arr) {
	    int len = CP_rts.chrArrLength(arr);
	    return new System.String(arr,0,len);
	}

/* -------------------------------------------------------------------- */
//
//  PROCEDURE mkArr*(s : String) : RTS.CharOpen); END mkArr;
//
/* -------------------------------------------------------------------- */
    	// Known in ILASM as [RTS]NativeStrings::mkArr
	public static char[] mkArr(System.String inp)
	{
            if (inp == null) return null;

	    int    len = inp.Length;
	    char[] res = new char[len+1];
	    inp.CopyTo(0, res, 0, len);
	    res[len] = '\0';
	    return res;
	}
}
/* ==================================================================== */
/* ==================================================================== */
//
// Body of Console interface.
// This file implements the code of the Console.cp file.
// kjg May 2000.
//
public class Console
// Known in ILASM as [RTS]Console
{
	public static void WriteLn()
	{
	    System.Console.WriteLine();
	}

	public static void Write(char ch)
	{
	    System.Console.Write(ch);
	}

	private static char[] strRep(int val)
	{
	    if (val < 0) { // ==> must be minInt
		char[] min = {' ',' ','2','1','4','7','4','8','3','6','4','8'};
		return min;
	    }

	    char[] str = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
	    str[11] = (char) (val % 10 + (int) '0'); val = val / 10;
	    for (int i = 10; val != 0; i--) {
		str[i] = (char) (val % 10 + (int) '0'); val = val / 10;
	    }
	    return str;
	}

	public static void WriteInt(int val, int fwd)
	{
	    char[] str = (val >= 0 ? strRep(val) : strRep(-val));

	    int blank;
	    for (blank = 0; str[blank] == ' '; blank++)
		;
	    if (val < 0) {
		str[blank-1] = '-'; blank--;
	    }
	    // format ...
	    // 01...............901
	    // _________xxxxxxxxxxx
	    // <-blank->< 12-blank>
	    //     <-----fwd------>
	    if (fwd == 0) // magic case, put out exactly one blank
		System.Console.Write(str, blank-1, 13-blank);
	    else if (fwd < (12-blank))
		System.Console.Write(str, blank, 12-blank);
	    else if (fwd <= 12)
		System.Console.Write(str, 12-fwd, fwd);
	    else { // fwd > 12
		for (int i = fwd-12; i > 0; i--)
		    System.Console.Write(" ");
		System.Console.Write(str);
	    }
	}

	public static void WriteHex(int val, int wid)
	{
	    char[] str = new char[9];
	    int j;		// index of last blank
	    int i = 8;
	    do {
		int dig = val & 0xF;
		val = (int)((uint) val >> 4);
		if (dig >= 10)
		    str[i] = (char) (dig + ((int) 'A' - 10));
		else
		    str[i] = (char) (dig + (int) '0');
		i--;
	    } while (val != 0);
	    j = i;
	    while (i >= 0) {
		str[i] = ' '; i--;
	    }
	    if (wid == 0)	// special case, exactly one blank
	        System.Console.Write(str, j, 9-j);
	    else if (wid < (8-j))
	        System.Console.Write(str, j+1, 8-j);
	    else if (wid <= 9)
	        System.Console.Write(str, 9-wid, wid);
	    else {
		for (i = wid-9; i > 0; i--)
			System.Console.Write(" ");
	        System.Console.Write(str);
	    }
	}


	public static void WriteString(char[] str)
	{
	   int len = str.Length;
	   for (int i = 0; i < len && str[i] != '\0'; i++)
		System.Console.Write(str[i]);
	}
} // end of public class Console
/* ==================================================================== */
/* ==================================================================== */
//
// Body of StdIn module.
// This file implements the code of the StdIn.cp file.
// kjg Sep 2004.
//
// Known in ILASM as [RTS]StdIn
//
public class StdIn
{
   public static void Read(ref char c) {
       int chr = System.Console.Read();
       if (chr == -1) { 
           c = '\0';
       } else {
           c = (char) chr;
       }
   }

   public static void ReadLn(char[] arr) {
       string str = System.Console.ReadLine();
       if (str == null) {
           arr[0] = '\0'; return;
       }
       int dLen = arr.Length;
       int sLen = str.Length;
       int cLen = (sLen < dLen ? sLen : dLen-1);
       str.CopyTo(0, arr, 0, cLen);
       arr[cLen] = '\0';
   }

   public static void SkipLn() {
       string str = System.Console.ReadLine();
   }

   public static bool More() {
       return true; // temporary, until we figure out how to get the same
   }                // semantics on .NET and the JVM (kjg Sep. 2004

} // end of public class StdIn

/* ==================================================================== */
/* ==================================================================== */
//
// Body of Error interface.
// This file implements the code of the Error.cp file.
// kjg May 2000.
//
public class Error
// Known in ILASM as [RTS]Error
{
	public static void WriteLn()
	{
	    System.Console.Error.Write(System.Environment.NewLine);
	}

	public static void Write(char ch)
	{
	    System.Console.Error.Write(ch);
	}

	private static char[] strRep(int val)
	{
	    if (val < 0) { // ==> must be minInt
		char[] min = {' ',' ','2','1','4','7','4','8','3','6','4','8'};
		return min;
	    }

	    char[] str = {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
	    str[11] = (char) (val % 10 + (int) '0'); val = val / 10;
	    for (int i = 10; val != 0; i--) {
		str[i] = (char) (val % 10 + (int) '0'); val = val / 10;
	    }
	    return str;
	}

	public static void WriteInt(int val, int fwd)
	{
	    char[] str = (val >= 0 ? strRep(val) : strRep(-val));

	    int blank;
	    for (blank = 0; str[blank] == ' '; blank++)
		;
	    if (val < 0) {
		str[blank-1] = '-'; blank--;
	    }
	    // format ...
	    // 01...............901
	    // _________xxxxxxxxxxx
	    // <-blank->< 12-blank>
	    //     <-----fwd------>
	    if (fwd == 0) // magic case, put out exactly one blank
		System.Console.Error.Write(str, blank-1, 13-blank);
	    else if (fwd < (12-blank))
		System.Console.Error.Write(str, blank, 12-blank);
	    else if (fwd <= 12)
		System.Console.Error.Write(str, 12-fwd, fwd);
	    else { // fwd > 12
		for (int i = fwd-12; i > 0; i--)
		    System.Console.Error.Write(" ");
		System.Console.Error.Write(str);
	    }
	}

	public static void WriteHex(int val, int wid)
	{
	    char[] str = new char[9];
	    int j;		// index of last blank
	    int i = 8;
	    do {
		int dig = val & 0xF;
		val = (int)((uint) val >> 4);
		if (dig >= 10)
		    str[i] = (char) (dig + ((int) 'A' - 10));
		else
		    str[i] = (char) (dig + (int) '0');
		i--;
	    } while (val != 0);
	    j = i;
	    while (i >= 0) {
		str[i] = ' '; i--;
	    }
	    if (wid == 0)	// special case, exactly one blank
	        System.Console.Error.Write(str, j, 9-j);
	    else if (wid < (8-j))
	        System.Console.Error.Write(str, j+1, 8-j);
	    else if (wid <= 9)
	        System.Console.Error.Write(str, 9-wid, wid);
	    else {
		for (i = wid-9; i > 0; i--)
			System.Console.Error.Write(" ");
	        System.Console.Error.Write(str);
	    }
	}


	public static void WriteString(char[] str)
	{
	   int len = str.Length;
	   for (int i = 0; i < len && str[i] != '\0'; i++)
		System.Console.Error.Write(str[i]);
	}
  } // end of public class Error

/* ==================================================================== */

abstract public class XHR
// Known in ILASM as [RTS]XHR
{
/* ==================================================================== *
 *		eXplicit Heap-allocated activation Record		*
 * ==================================================================== */
	public XHR prev;
}
/* ==================================================================== */

namespace Vectors {
    public abstract class VecBase {
        public int tide;

        public abstract void expand();
    }

    public class VecChr : VecBase {
        public char[] elms;

        public override void expand() {
            char[] tmp = new char[this.elms.Length * 2];
            for (int i = 0; i < this.tide; i++) {
                tmp[i] = this.elms[i];
            }
            this.elms = tmp;
        }
    }

    public class VecI32 : VecBase {
        public int[] elms;

        public override void expand() {
            int[] tmp = new int[this.elms.Length * 2];
            for (int i = 0; i < this.tide; i++) {
                tmp[i] = this.elms[i];
            }
            this.elms = tmp;
        }
    }

    public class VecI64 : VecBase {
        public long[] elms;

        public override void expand() {
            long[] tmp = new long[this.elms.Length * 2];
            for (int i = 0; i < this.tide; i++) {
                tmp[i] = this.elms[i];
            }
            this.elms = tmp;
        }
    }

    public class VecR32 : VecBase {
        public float[] elms;

        public override void expand() {
            float[] tmp = new float[this.elms.Length * 2];
            for (int i = 0; i < this.tide; i++) {
                tmp[i] = this.elms[i];
            }
            this.elms = tmp;
        }
    }

    public class VecR64 : VecBase {
        public double[] elms;

        public override void expand() {
            double[] tmp = new double[this.elms.Length * 2];
            for (int i = 0; i < this.tide; i++) {
                tmp[i] = this.elms[i];
            }
            this.elms = tmp;
        }
    }

    public class VecRef : VecBase {
        public object[] elms;

        public override void expand() {
            object[] tmp = new object[this.elms.Length * 2];
            for (int i = 0; i < this.tide; i++) {
                tmp[i] = this.elms[i];
            }
            this.elms = tmp;
        }
    }

}
/* ==================================================================== */




/* ==================================================================== */
