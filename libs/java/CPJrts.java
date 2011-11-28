
/** This is the body of the GPCP runtime support.
 *
 *  Written November 1998, John Gough.
 *
 *
 *
 */

package CP.CPJrts;
import  java.lang.reflect.*;

public class CPJrts
{

/* ==================================================================== *
 *		MOD and DIV helpers. With correction factors		*
 * ==================================================================== */

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

	public static String CaseMesg(int i)
	{
	    String s = "CASE-trap: selector = " + i;
	    return s;
	}

/* -------------------------------------------------------------------- */

	public static String WithMesg(Object o)
	{
	    String c = o.getClass().getName();
	    c = c.substring(c.lastIndexOf('.') + 1);
	    c = "WITH else-trap: type = " + c;
	    return c;
	}

/* -------------------------------------------------------------------- */

	public static int ChrArrLength(char[] src)
	{
	    int  ix = 0;
	    char ch;
	    do {
		ch = src[ix];
		ix++;
	    } while ((ch != '\0') && (ix < src.length));
	    return ix-1;
	}

/* -------------------------------------------------------------------- */

	public static int ChrArrLplus1(char[] src)
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

	public static char[] JavaStrToChrOpen(String input)
	{
	    int    len = input.length();
	    char[] str = new char[len+1];
	    input.getChars(0, len, str, 0);
	    str[len] = '\0';
	    return str;
	}

/* -------------------------------------------------------------------- */

	public static void JavaStrToFixChr(char[] out, String in)
	{
	    int    len = in.length();
	    in.getChars(0, len, out, 0);
	    out[len] = '\0';
	}

/* -------------------------------------------------------------------- */

	public static String FixChToJavaStr(char[] arr)
	{
            // This truncation makes semantics same as .NET version
            int len = ChrArrLength(arr);
	    return new String(arr, 0, len);
	}

/* -------------------------------------------------------------------- */

	public static void ChrArrStrCopy(char[] dst, char[] src)
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

	public static void ChrArrCheck(char[] src)
	{
	    int  ix = 0;
	    char ch;
	    do {
		ch = src[ix];
		if (ch > 0xFF) throw new Error("SHORT on array error");
		ix++;
	    } while (ch != '\0');
	}

/* -------------------------------------------------------------------- */

        public static int strCmp(char[] l, char[] r)
	{
	    for (int ix = 0; ix < l.length && ix < r.length; ix++) {
		if (l[ix] < r[ix]) return -1;
		else if (l[ix] > r[ix]) return 1;
		else if (l[ix] == '\0') return 0;
	    }
	    if (l.length < r.length) return -1;
	    else if (l.length < r.length) return 1;
	    else return 0;
	}

/* ==================================================================== *
 *	             Class reflection helper methods			*
 * ==================================================================== */

        static final int boolN  =  1;
        static final int sChrN  =  2; 
        static final int charN  =  3;
        static final int byteN  =  4; 
        static final int sIntN  =  5;  
        static final int intN   =  6; 
        static final int lIntN  =  7;
        static final int sReaN  =  8; 
        static final int realN  =  9;
        static final int setN   = 10;
        static final int anyRec = 11; 
        static final int anyPtr = 12;
        static final int strN   = 13; 
        static final int sStrN  = 14; 
        static final int uBytN  = 15;
        static final int metaN  = 16;

        public static Class getClassByName(String name) {
            try {
                return Class.forName(name);
            } catch(Exception e) {
		System.out.println("CPJrts.getClassByName: " + e.toString());
                return null;
            }
        }

        public static Class getClassByOrd(int ord) {
            switch (ord) {
              case boolN:   return Boolean.TYPE;
              case uBytN:
              case byteN:
              case sChrN:   return Byte.TYPE;
              case charN:   return Character.TYPE;
              case sIntN:   return Short.TYPE;
              case setN: 
              case intN:    return Integer.TYPE;
              case lIntN:   return Long.TYPE;
              case sReaN:   return Float.TYPE;
              case realN:   return Double.TYPE;
              case anyRec:
              case anyPtr:  return getClassByName("java.lang.Object");
              case strN:    return getClassByName("java.lang.String");
              case sStrN:   return getClassByName("java.lang.String");
              case metaN:   return getClassByName("java.lang.Class");
              default:      return null;
            }
        }


/* ==================================================================== *
 *		Procedure variable reflection helper method		*
 * ==================================================================== */

	public static Method getMth(String mod, String prc)
	{
	    Class    mCls = null;
	    Method[] mths = null;
	    try {
		mCls = Class.forName(mod);
		mths = mCls.getDeclaredMethods();
		for (int i = 0; i < mths.length; i++) {
		    if (mths[i].getName().equals(prc))
			return mths[i];
		}
		return null;
	    } catch(Exception e) {
		System.out.println("CPJrts.getMth: " + e.toString());
		return null;
	    }
	}

/* ==================================================================== *
 *		String concatenation helper methods			*
 * ==================================================================== */

	public static String ArrArrToString(char[] l, char[] r)
	{
	    int llen = ChrArrLength(l);
	    int rlen = ChrArrLength(r);
	    StringBuffer buff = new StringBuffer(llen + rlen);
	    return buff.append(l,0,llen).append(r,0,rlen).toString();
	}

	public static String ArrStrToString(char[] l, String r)
	{
	    int llen = ChrArrLength(l);
	    StringBuffer buff = new StringBuffer(3 * llen);
	    return buff.append(l,0,llen).append(r).toString();
	}

	public static String StrArrToString(String l, char[] r)
	{
	    int rlen = ChrArrLength(r);
	    StringBuffer buff = new StringBuffer(3 * rlen);
	    return buff.append(l).append(r,0,rlen).toString();
	}

	public static String StrStrToString(String l, String r)
	{
	    StringBuffer buff = new StringBuffer(l);
	    return buff.append(r).toString();
	}

}

