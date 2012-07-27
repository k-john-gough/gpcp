/**********************************************************************/
/*               NameAndType Reference class for J2CPS                */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

public class NameAndType {

  ConstantPool cp;          /* The constant pool containing this N & T */
  int nameIndex;            /* CP index for this N & T's name          */
  int typeIndex;            /* CP index for this N & T'x type          */
  String name;
  String type;

  public NameAndType(ConstantPool thisCp, int nameIx, int typeIx) {
    this.cp = thisCp;
    this.nameIndex = nameIx;
    this.typeIndex = typeIx;
  }

  public String GetName() {
    if (this.name == null) { this.name = (String) this.cp.Get(nameIndex); }
    return this.name;
  }

  public String GetType() {
    if (this.type == null) { this.type = (String) this.cp.Get(typeIndex); }
    return this.type;
  }

  public void Resolve() {
    if (this.name == null) { this.name = (String) this.cp.Get(nameIndex); }
    if (this.type == null) { this.type = (String) this.cp.Get(typeIndex); }
  }

    @Override
  public String toString() {
    this.Resolve();
    return "<NameAndType> " + nameIndex + " " + this.name + 
           "              " + typeIndex + " " + this.type;
  }

}
