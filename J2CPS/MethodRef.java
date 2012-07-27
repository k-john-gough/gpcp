/**********************************************************************/
/*                Method Reference class for J2CPS                    */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

public class MethodRef extends Reference {

  public MethodRef(ConstantPool thisCp, int classIndex, int ntIndex) {
    super(thisCp,classIndex,ntIndex);
  }

  public String getMethodName() {
    return (classRef.GetName() + "." + name + type);
  }

    @Override
  public String toString() {
    this.Resolve();
    return ("<MethodReference> " + classIndex + " " + nameAndTypeIndex + " " +
            classRef.GetName() + "." + name + " " + type);
  }

}
