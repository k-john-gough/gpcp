/**********************************************************************/
/*                Array Descriptor class for J2CPS                    */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.DataOutputStream;
import java.io.IOException;

public class ArrayDesc extends TypeDesc {
  
  static ArrayDesc[] arrayTypes = new ArrayDesc[10];
  static int numArrayTypes = 0;

  TypeDesc elemType;
  PtrDesc ptrType;
  int dim = 1;
  TypeDesc ultimateElemType;
  public int elemTypeFixUp = 0;

  public ArrayDesc(int eF) {
    typeOrd = TypeDesc.arrT;
    name = "ARRAY OF " + eF;
    elemTypeFixUp = eF;
    writeDetails = true;
  }

  public ArrayDesc (int dimNum,TypeDesc eType,boolean makePtr) {
    name = "ARRAY OF ";
    writeDetails = true;
    for (int i=1; i < dimNum; i++) {
      name = name + "ARRAY OF ";
    }
    name = name + eType.name;
    typeOrd = TypeDesc.arrT;
    dim = dimNum;
    elemType = eType;
    ultimateElemType = eType; 
    if (makePtr) {
      ptrType = new PtrDesc(this);
    }
  }

  public void SetPtrType(PtrDesc ptrTy) {
    ptrType = ptrTy;
  }

  public static TypeDesc GetArrayType(String sig,int start,boolean getPtr) {
    TypeDesc uEType;
    if (sig.charAt(start) != '[') {
      System.out.println(sig.substring(start) + " is not an array type!");
      System.exit(1);
    }
    int dimCount = 0, ix = start;
    while (sig.charAt(ix) == '[') { ix++; dimCount++; }
    uEType = TypeDesc.GetType(sig,ix);
    ArrayDesc thisArr = FindArrayType(dimCount,uEType,getPtr);
    dimCount--;
    ArrayDesc arrD = thisArr;
    while (dimCount > 1) {
      arrD.elemType = FindArrayType(dimCount,uEType,true);
      if (arrD.elemType instanceof ArrayDesc) {
        arrD = (ArrayDesc)arrD.elemType;
      }
      dimCount--; 
    }
    arrD.elemType = uEType;
    if (getPtr) { return thisArr.ptrType; } else { return thisArr; }
  }

  public static ArrayDesc FindArrayType(int dimNum, TypeDesc eType,
                                                               boolean mkPtr) {
    for (int i=0; i < numArrayTypes; i++) {
      if ((arrayTypes[i].dim == dimNum) && 
          (arrayTypes[i].ultimateElemType == eType))  {
        if (mkPtr && arrayTypes[i].ptrType == null) { 
          arrayTypes[i].ptrType = new PtrDesc(arrayTypes[i]);
        }
        return arrayTypes[i];
      }
    }
    arrayTypes[numArrayTypes++] = new ArrayDesc(dimNum,eType,mkPtr);
    if (numArrayTypes == arrayTypes.length) {
      ArrayDesc[] temp = arrayTypes;
      arrayTypes = new ArrayDesc[numArrayTypes * 2];
      System.arraycopy(temp, 0, arrayTypes, 0, numArrayTypes);
    }
    return arrayTypes[numArrayTypes-1];
  }

  @Override
  public String getTypeMnemonic() {
    return 'a' + elemType.getTypeMnemonic();
  }

  @Override
  public void writeType(DataOutputStream out, PackageDesc thisPack) 
                                                          throws IOException {
    // Array = TypeHeader arrSy TypeOrd (Byte | Number | ) endAr.
    out.writeByte(SymbolFile.arrSy);
    SymbolFile.writeTypeOrd(out,elemType);
    out.writeByte(SymbolFile.endAr); 
  }

  public void AddImport(ClassDesc thisClass) {
    if (ultimateElemType instanceof ClassDesc) {
      thisClass.AddImport((ClassDesc)ultimateElemType);
    }
  }

}
