/**********************************************************************/
/*                Pointer Descriptor class for J2CPS                  */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.*;
import java.util.*;

public class PtrDesc extends TypeDesc {

  TypeDesc boundType;

  public PtrDesc(TypeDesc baseType) {
    typeOrd = TypeDesc.arrPtr;
    boundType = baseType;
    if (boundType != null) { setName(); }
  }

  public PtrDesc(int inNum, int baseNum) {
    typeOrd = TypeDesc.arrPtr;
    inTypeNum = inNum;
    inBaseTypeNum = baseNum;
  }
  
  public void Init(TypeDesc baseType) {
    boundType = baseType;
    if (boundType != null) { setName(); }
  }

  public void AddImport(ClassDesc thisClass) {
    if (boundType instanceof ClassDesc) {
      thisClass.AddImport((ClassDesc)boundType);
    } else if (boundType instanceof ArrayDesc) {
      ((ArrayDesc)boundType).AddImport(thisClass);
    }
  }

  public void setName() {
    name = "POINTER TO " + boundType.name;
  }

  public void writeType(DataOutputStream out, PackageDesc thisPack) 
                                                           throws IOException {
    out.writeByte(SymbolFile.ptrSy);
    SymbolFile.writeTypeOrd(out,boundType); 
  }


}


