/**********************************************************************/
/*                Method Reference class for j2cps                    */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

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
