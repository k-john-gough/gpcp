// (* ========================================================= *)
// (**	Interface to the ILASM Byte-code assembler.		*)
// (*	K John Gough, 10th June 1999				*)
// (*	Modifications:						*)
// (*		Version for GPCP V0.3 April 2000 (kjg)		*)
// (* ========================================================= *)
// (*	The real code is in MsilAsm.cool			*)	
// (* ========================================================= *)
//
//MODULE MsilAsm;
//
//  PROCEDURE Init*(); BEGIN END Init;
//
//  PROCEDURE Assemble*(IN fil,opt : ARRAY OF CHAR; main : BOOLEAN); 
//  BEGIN END Assemble;
//
//  PROCEDURE DoAsm*(IN fil,opt : ARRAY OF CHAR; 
//                    main,vbse : BOOLEAN;
//                     OUT rslt : INTEGER); 
//  BEGIN END Assemble;
//
//END MsilAsm.
// 
package CP.MsilAsm;

public class MsilAsm {

    public static void Init() {
//	if (main == null) 
//	    main = new jasmin.Main();
    }

    public static void Assemble(char[] fil, char[] opt, boolean main) {
    }

    public static int DoAsm(char[] fil, char[] opt, 
				boolean main, boolean vrbs) {
//	String fName = CP.CPJ.CPJ.MkStr(fil);
//	main.assemble(null, fName, false);
        return 0;
    }

}
