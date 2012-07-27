/**********************************************************************/
/*          Interface Method Reference class for J2CPS                */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

public class InterfaceMethodRef extends Reference {

  public InterfaceMethodRef(ConstantPool thisCp, int classIndex, int ntIndex) {
    super(thisCp,classIndex,ntIndex);
  }

  public String getIntMethName() {
    return (classRef.GetName() + "." + name + type);
  }

    @Override
  public String toString() {
    this.Resolve();
    return ("<InterfaceMethReference>  Class " + classIndex + 
            "  NameAndType " + nameAndTypeIndex);
  }

}
