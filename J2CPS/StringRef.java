/**********************************************************************/
/*                 String Reference class for j2cps                   */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

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

    @Override
  public String toString() {
    this.Resolve();
    return ("<StringRef>  " + this.strIndex + " " + str);
  }

}

