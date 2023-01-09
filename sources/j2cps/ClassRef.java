/*************************************************************************/
/*                Class Reference class for j2cps                        */
/* Represents the class references in the constant pool of a class file  */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017       */ 
/*************************************************************************/
package j2cps;

public class ClassRef {

  ConstantPool cp;  /* the constant pool containing this class ref */
  String name;      /* the name of this class */
  int nameIndex;    /* the index into the constant pool */
                    /* for the name of this class       */
  ClassDesc info;   /* this class info for this class ref */
  
  public ClassRef(ConstantPool thisCp, int nameIndex) {
    this.cp = thisCp;
    this.nameIndex = nameIndex;
  }
 
  public String GetName() {
    if (name == null) { name = (String) cp.Get(nameIndex); }
    return name;
  }
  
  public ClassDesc GetClassDesc() {
    if (info == null) {
      if (name == null) { name = (String) this.cp.Get(nameIndex); }
      info = ClassDesc.GetClassDesc(name,null);
    }
    return info;
  }

  public boolean equals(ClassRef anotherClass) {
    return this.GetName().equals(anotherClass.GetName());
  }
  
  public void Resolve() {
    if (name == null) { this.name = (String) this.cp.Get(nameIndex); }
  }

    @Override
  public String toString() {
    this.Resolve();
    return ("<ClassReference> " + nameIndex + " " + name);
  }
}










