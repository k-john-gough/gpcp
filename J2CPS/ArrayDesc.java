/**********************************************************************/
/*                Array Descriptor class for j2cps                    */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.io.DataOutputStream;
import java.io.IOException;

public class ArrayDesc extends TypeDesc {
  
  static ArrayDesc[] arrayTypes = new ArrayDesc[10];
  static int numArrayTypes = 0;

  /**
   *  The type-descriptor of the element type of this array type
   */
  TypeDesc elemType;
  /**
   * The type-descriptor of the type that is a pointer to this array type
   */
  PtrDesc ptrType;
  int dim = 1;
  /**
   *  The type-descriptor of the elements of this
   *  (possibly multi-dimensional) array type.
   */
  TypeDesc ultimateElemType;
  public int elemTypeFixUp = 0;

  public ArrayDesc(int eF) {
    typeOrd = TypeDesc.arrT;
    name = "ARRAY OF " + eF;
    elemTypeFixUp = eF;
    writeDetails = true;
  }

    public ArrayDesc(int dimNum,TypeDesc eType,boolean makePtr) {
        this.name = "ARRAY OF ";
        this.writeDetails = true;
        for (int i=1; i < dimNum; i++) {
            this.name += "ARRAY OF ";
        }
        this.name += eType.name;
        this.typeOrd = TypeDesc.arrT;
        this.dim = dimNum;
        this.elemType = (dimNum == 1 ? eType : null);
        this.ultimateElemType = eType; 
        if (makePtr) {
            this.ptrType = new PtrDesc(this);
        }
    }

  public void SetPtrType(PtrDesc ptrTy) {
    ptrType = ptrTy;
  }

  public static TypeDesc GetArrayTypeFromSig(
          String sig,int start,boolean getPtr) {
    TypeDesc uEType;
    if (sig.charAt(start) != '[') {
        System.out.println(sig.substring(start) + " is not an array type!");
        System.exit(1);
    }
    int dimCount = 0, ix = start;
    while (sig.charAt(ix) == '[') { 
        ix++; dimCount++; 
    }
    uEType = TypeDesc.GetType(sig,ix);
    ArrayDesc thisArr = FindOrCreateArrayType(dimCount,uEType,getPtr);
    
    ArrayDesc arrD = thisArr;
    while (--dimCount >= 1) {
        arrD.elemType = FindOrCreateArrayType(dimCount,uEType,true);
        if (arrD.elemType instanceof ArrayDesc) {
          arrD = (ArrayDesc)arrD.elemType;
        }
    }
    arrD.elemType = uEType;
    if (getPtr)
        return thisArr.ptrType;
    else
        return thisArr;
  }

  public static ArrayDesc FindOrCreateArrayType(
          int dimNum, TypeDesc eType, boolean mkPtr) {
    //
    //  Try to find existing, identical array descriptor 
    //
    for (int i=0; i < numArrayTypes; i++) {
        if ((arrayTypes[i].dim == dimNum) && 
            (arrayTypes[i].ultimateElemType == eType))  
        {
            if (mkPtr && arrayTypes[i].ptrType == null) 
                arrayTypes[i].ptrType = new PtrDesc(arrayTypes[i]);
            return arrayTypes[i];
        }
    }
    //
    //  Otherwise allocate a new array descriptor
    //
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

  /** Write an array definition the typelist section of a symbol file
   * <p>
   * An array type declaration consists of only an array marker
   * followed by the type-ordinal of the element type.
   * 
   * @param out  the symbol file output stream
   * @param thisPack the package which this symbol file describes
   * @throws IOException 
   */
  @Override
  public void writeType(DataOutputStream out, PackageDesc thisPack) 
                                                          throws IOException {
    // Array = TypeHeader arrSy TypeOrd (Byte | Number | ) endAr.
    out.writeByte(SymbolFile.arrSy);
    SymbolFile.writeTypeOrd(out,elemType);
    out.writeByte(SymbolFile.endAr); 
  }

  public void AddImportToArray(ClassDesc thisClass) {
    if (ultimateElemType instanceof ClassDesc) {
      thisClass.AddImportToClass((ClassDesc)ultimateElemType);
    }
  }

}
