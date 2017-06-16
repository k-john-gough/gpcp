/**********************************************************************/
/*                Pointer Descriptor class for j2cps                  */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.io.DataOutputStream;
import java.io.IOException;

public class PtrDesc extends TypeDesc {

  TypeDesc boundType;

  public PtrDesc(TypeDesc baseType) {
    this.typeOrd = TypeDesc.arrPtr;
    this.boundType = baseType;
    if (this.boundType != null) { 
        this.name = "POINTER TO " + this.boundType.name; 
    }
  }

  public PtrDesc(int inNum, int baseNum) {
    this.typeOrd = TypeDesc.arrPtr;
    this.inTypeNum = inNum;
    this.inBaseTypeNum = baseNum;
  }
  
  public void Init(TypeDesc baseType) {
    this.boundType = baseType;
    if (this.boundType != null) { setName(); }
  }

  public void AddImportToPtr(ClassDesc thisClass) {
    if (this.boundType instanceof ClassDesc) {
        ((ClassDesc)this.boundType).blame = this.blame;
        thisClass.AddImportToClass((ClassDesc)this.boundType);
    } else if (this.boundType instanceof ArrayDesc) {
        ((ArrayDesc)this.boundType).blame = this.blame;
        ((ArrayDesc)this.boundType).AddImportToArray(thisClass);
    }
  }

  public void setName() {
    this.name = "POINTER TO " + this.boundType.name;
  }
  
  /** Write a pointer type definition to the typelist section of a symbol file.
   * <p>
   * A pointer type declaration consists of only an array marker
   * followed by the type-ordinal of the bound type.
   * 
   * @param out  the symbol file output stream
   * @param thisPack the package which this symbol file describes
   * @throws IOException 
   */
  

    @Override
  public void writeType(DataOutputStream out, PackageDesc thisPack) 
                                                           throws IOException {
    out.writeByte(SymbolFile.ptrSy);
    SymbolFile.writeTypeOrd(out,this.boundType); 
  }


}


