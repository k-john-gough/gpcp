/**********************************************************************/
/*                 String Reference class for J2CPS                   */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.*;

public class StringRef {

  ConstantPool cp;  /* the constant pool containing this string ref */
  String str;       /* the string this ref refers to                */
  int strIndex;     /* the CP index for this string                 */
  
  public StringRef(ConstantPool thisCp, int strIx) {
    this.cp = thisCp;
    this.strIndex = strIx;
  }
 
  public String GetString() {
    if (this.str == null) { this.str = (String) cp.Get(strIndex); }
    return str;
  }

  public void Resolve() {
    this.str = (String) this.cp.Get(strIndex);
  }

  public String toString() {
    this.Resolve();
    return ("<StringRef>  " + this.strIndex + " " + str);
  }

}

