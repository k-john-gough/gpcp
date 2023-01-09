/**********************************************************************/
/*      Class defining the Component Pascal reserved words            */ 
/*                                                                    */
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.util.HashMap;

public class CPWords {

    private static final String[] reservedWords = 
      {"ARRAY","BEGIN","BY","CASE","CLOSE","CONST","DIV","DO","ELSE",
       "ELSIF","END","EXIT","FOR","IF","IMPORT","IN","IS","LOOP","MOD",
       "MODULE","NIL","OF","OR","OUT","POINTER","PROCEDURE","RECORD",
       "REPEAT","RETURN","THEN","TO","TYPE","UNTIL","VAR","WHILE","WITH"};

    public static HashMap<String,String> InitResWords() {
        HashMap<String,String> hTable = new HashMap<>();
            for (String reservedWord : reservedWords) {
                hTable.put(reservedWord, reservedWord);
            }
        return hTable;
    }


}
