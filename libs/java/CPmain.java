//
// Body of CPmain interface.
// This file implements the code of the CPmain.cp file.
// kjg November 1998.

package CP.CPmain;

public class CPmain
{
/*
 *  Now empty. Methods have moved to ProgArgs.
 */
	public static String[] args;

	public static void PutArgs(String[] a)
	// This method is known to the CPascal compiler, but is
	// unknown to CPascal source programs. An initialization
	// call to this method is the first thing in the synthetic
	// main method of any module which imports CPmain.
	{
	    args = a;
	}

	public static int ArgNumber()
	{
	    return args.length;
	}

	public static void GetArg(int num, char[] str)
	{
	    int i;
	    for (i = 0; i < str.length && i < args[num].length(); i++) {
		str[i] = args[num].charAt(i);
	    }
	    if (i == str.length)
		i--;
	    str[i] = '\0';
	}
} // end of public class CPmain
