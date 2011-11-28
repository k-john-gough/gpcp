//
// Body of ProgArgs interface.
// This file implements the code of the ProgArgs.cp file.
// kjg December 1999.
//
// The reason that this module is implemented as a Java class is
// that the name CPmain has special meaning to the compiler, so
// it must be imported secretly in the implementation.
//

package CP.ProgArgs;
import  CP.CPmain.CPmain;

public class ProgArgs
{

	public static int ArgNumber()
	{
	    if (CP.CPmain.CPmain.args == null)
		return 0;
	    else
		return CP.CPmain.CPmain.args.length;
	}

	public static void GetArg(int num, char[] str)
	{
	    int i;
	    if (CP.CPmain.CPmain.args == null) {
		str[0] = '\0';
	    } else {
		for (i = 0; 
		     i < str.length && i < CP.CPmain.CPmain.args[num].length();
		     i++) {
		    str[i] = CP.CPmain.CPmain.args[num].charAt(i);
		}
		if (i == str.length)
		    i--;
		str[i] = '\0';
	    }
	}

        public static void GetEnvVar(char[] ss, char[] ds) 
        {
            String path = CP.CPJ.CPJ.MkStr(ss);
            String valu = System.getProperty(path);
            int i;
            for (i = 0; 
                 i < valu.length() && i < ds.length;
                 i++) {
                ds[i] = valu.charAt(i);
            }
            if (i == ds.length)
                i--;
            ds[i] = '\0';
        }

} // end of public class ProgArgs
