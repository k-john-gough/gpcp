//
// Body of StdIn interface.
// This file implements the code of the StdIn.cp file.
// kjg June 2004.

package CP.StdIn;

import java.io.*;

public class StdIn
{
        private static BufferedReader rdr = 
               new BufferedReader(new InputStreamReader(System.in));

	public static void ReadLn(char[] arr) throws IOException {
            String str = rdr.readLine();
            if (str == null) {
                arr[0] = '\0'; return;
            }
            int len = arr.length;
            int sLn = str.length();
            len = (sLn < len ? sLn : len-1);
            str.getChars(0, len, arr, 0);
            arr[len] = '\0';
        }

        public static char Read() throws IOException 
	{
	    return (char)rdr.read();
	}

        public static boolean More() throws IOException
	{
	    return true;         // temporary fix until we figure out
	 // return rdr.ready();  // how to get the same semantics for
	}                        // .NET and the JVM (kjg Sep. 2004)

	public static void SkipLn() throws IOException
	{
            String str = rdr.readLine(); // and discard str
	}

} // end of public class StdIn
