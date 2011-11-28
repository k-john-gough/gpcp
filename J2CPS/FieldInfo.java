/**********************************************************************/
/*                    FieldInfo class for J2CPS                       */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.*;
import java.util.*;

public class FieldInfo extends MemberInfo {
  
  Object constVal;
  public TypeDesc type;
  public int typeFixUp = 0;

  public FieldInfo(ConstantPool cp, DataInputStream stream,
                   ClassDesc thisClass) throws IOException {
    
    super(cp,stream,thisClass);
    type = TypeDesc.GetType(signature,0);
    if (type instanceof ClassDesc) { thisClass.AddImport((ClassDesc)type); }
  }

  public FieldInfo(ClassDesc cl,int acc,String nam,TypeDesc typ,Object cVal) {
    super(cl,acc,nam);
    type = typ;
    constVal = cVal;
  }

  public void AddImport(ClassDesc thisClass) {
    if (type instanceof ClassDesc) { thisClass.AddImport((ClassDesc)type); }
  }

  public void GetConstValueAttribute (ConstantPool cp, DataInputStream stream) 
                                                            throws IOException {
    int attLen = stream.readInt();
    constVal = cp.Get(stream.readUnsignedShort()); 
    if (constVal instanceof StringRef) {
      constVal = ((StringRef)constVal).GetString();
    }
  }

  public Object GetConstVal() {
    return constVal;
  }

  public boolean isConstant() {
    return ((constVal != null) && ConstantPool.isFinal(accessFlags) &&
            ConstantPool.isStatic(accessFlags) && 
            (ConstantPool.isPublic(accessFlags) ||
             ConstantPool.isProtected(accessFlags)));
  }

  public String toString() {
    if (constVal == null) {
      return ConstantPool.GetAccessString(accessFlags) + " " + 
             signature + " " + name;
    } else {
      return ConstantPool.GetAccessString(accessFlags) + " " + 
             signature + " " + name + " = " + constVal.toString();
    }
  }

}
