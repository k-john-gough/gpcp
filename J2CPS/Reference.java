/**********************************************************************/
/*                   Reference class for J2CPS                        */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

public class Reference {

  ConstantPool cp;          /* The constant pool containing this ref */
  int classIndex;           /* CP index for this reference's class   */
  int nameAndTypeIndex;     /* CP index for this ref's name and type */
  ClassRef classRef;
  NameAndType nAndt;
  String name;
  String type;

  public Reference(ConstantPool thisCp, int classIndex, int ntIndex) {
    this.cp = thisCp;
    this.classIndex = classIndex;
    this.nameAndTypeIndex = ntIndex;
  }

  public String GetClassName() {
    if (this.classRef == null) { 
      this.classRef = (ClassRef) this.cp.Get(classIndex); 
    }
    return classRef.GetName();
  }

  public void Resolve() {
    this.classRef = (ClassRef) this.cp.Get(classIndex); 
    this.nAndt = (NameAndType) this.cp.Get(nameAndTypeIndex); 
    this.name = nAndt.GetName();
    this.type = nAndt.GetType();
  }

    @Override
  public String toString() {
    this.Resolve();
    return ("Class " + classIndex + "  NameAndType " + nameAndTypeIndex);
  }

}
