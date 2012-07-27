package J2CPS;

public class FieldRef extends Reference {

  public FieldRef(ConstantPool thisCp, int classIndex, int ntIndex) {
    super(thisCp,classIndex,ntIndex);
  }

  public String getFieldName() {
    return (classRef.GetName() + "." + name);
  }

    @Override
  public String toString() {
    this.Resolve();
    return ("<FieldReference> " + classIndex + " " + nameAndTypeIndex + " " +
            classRef.GetName() + "." + name + " : " + type);
  }

}
