
/** This is part of the body of the GPCP runtime support.
 *
 *  Written November 1998, John Gough.
 *
 *  CPJ and CPJrts contain the runtime helpers, these classes have
 *  most of the adapters for hooking into the various Java libraries.
 *  RTS.java has the user-accessible facilities of the runtime. The
 *  facilities in CPJrts are known to the compiler, but have no
 *  CP-accessible functions.  
 *
 *  There is a swindle involved here, for the bootstrap version
 *  of the compiler: any functions with OUT scalars will have
 *  a different signature in the old and new versions.  This 
 *  module implements both, by overloading the methods.
 *  There is also the method for simulating an Exec.
 */

package CP.CPJ;

import java.io.*;

/* ------------------------------------------------------------ */
/* 		        Support for CPJ.cp			*/
/* ------------------------------------------------------------ */

class CopyThread extends Thread
{  //
   //  This is a crude adapter to connect two streams together.
   //  One use of this class is to connect the output and input
   //  threads of an forked-ed process to the standard input and
   //  output streams of the parent process.
   //
    InputStream in;
    OutputStream out;

    CopyThread(InputStream i, OutputStream o) {
	in = i; out = o;
    }

    public void run() {
	try {
	    for (int ch = in.read(); ch != -1; ch = in.read()) {
		out.write(ch);
	    }
	} catch(Exception e) {
	    return;
	}
    }
}

/* ------------------------------------------------------------ */

public final class CPJ
{
	
	public static final String newLn = "\n";

	public static String MkStr(char[] arr)
	{
	    for (int i = 0; i < arr.length; i++) {
		if (arr[i] == '\0')
		    return new String(arr, 0, i);
	    }
	    return null;
	}

	public static void MkArr(String str, char[] arr)
	{
	    if (str == null) {
		arr[0] = '\0'; return;
	    }
	    int    len = str.length();
	    if (len >= arr.length)
		len = arr.length - 1;
	    str.getChars(0, len, arr, 0);
	    arr[len] = '\0';
	}

	public static String JCat(String l, String r)
	{
	    return l+r;
	}

	public static String GetProperty(String key)
	{
	    return System.getProperty(key);
	}

        // OBSOLETE 2011 ?
	/** Java compiler version */
	public static void StrToReal(String str,
					double[] o, 	// OUT param
					boolean[] r)	// OUT param
	{
	    try {
		o[0] = Double.valueOf(str.trim()).doubleValue();
		r[0] = true;
	    } catch(Exception e) {
		r[0] = false;
	    }
	}

        // OBSOLETE 2011 ?
	/** Component Pascal compiler version */
	public static double StrToReal(String str,
					boolean[] r)	// OUT param
	{
	    try {
		r[0] = true;
		return Double.valueOf(str.trim()).doubleValue();
	    } catch(Exception e) {
		r[0] = false;
		return 0.0;
	    }
	}

        // OBSOLETE 2011 ?
	/** Java compiler version */
	public static void StrToInt(String str,
					int[] o,	// OUT param
					boolean[] r)	// OUT param
	{
	    try {
		o[0] = Integer.parseInt(str.trim());
		r[0] = true;
	    } catch(Exception e) {
		r[0] = false;
	    }
	}

        // OBSOLETE 2011 ?
	/** Component Pascal compiler version */
	public static int StrToInt(String str,
					boolean[] r)	// OUT param
	{
	    try {
		r[0] = true;
		return Integer.parseInt(str.trim());
	    } catch(Exception e) {
		r[0] = false;
		return 0;
	    }
	}


    public static int ExecResult(String[] args)
    {
	try {
	    Process p = Runtime.getRuntime().exec(args);
	    CopyThread cOut = new CopyThread(p.getInputStream(), System.out);
	    cOut.start();
	    CopyThread cErr = new CopyThread(p.getErrorStream(), System.err);
	    cErr.start();
	    CopyThread cIn  = new CopyThread(System.in, p.getOutputStream());
	    cIn.start();
	    return p.waitFor();
	} catch(Exception e) {
	    System.err.println(e.toString());
	    return 1;
	}
    }

/* ------------------------------------------------------------ */

    public static void DiagProperties()
    {
	    System.getProperties().list(System.out);
    }

    public static void DiagClass(Object o)
    {
	    System.out.print(o.getClass().getName());
    }
}

/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */

