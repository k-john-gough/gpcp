/**********************************************************************/
/*      Class defining the Component Pascal reserved words            */ 
/*                                                                    */
/*                    (c) copyright QUT                               */
/**********************************************************************/
package J2CPS;

import java.util.*;

public class CPWords {

  private static final String[] reservedWords = 
    {"ARRAY","BEGIN","BY","CASE","CLOSE","CONST","DIV","DO","ELSE",
     "ELSIF","END","EXIT","FOR","IF","IMPORT","IN","IS","LOOP","MOD",
     "MODULE","NIL","OF","OR","OUT","POINTER","PROCEDURE","RECORD",
     "REPEAT","RETURN","THEN","TO","TYPE","UNTIL","VAR","WHILE","WITH"};

  public static HashMap<String,String> InitResWords() {
    HashMap<String,String> hTable = new HashMap<String,String>();
    for (int i=0; i < reservedWords.length; i++) {
      hTable.put(reservedWords[i],reservedWords[i]);
    }
    return hTable;
  }


}
