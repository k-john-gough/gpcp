/**********************************************************************/
/*                    FieldInfo class for j2cps                       */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.io.DataInputStream;
import java.io.IOException;


public class FieldInfo extends MemberInfo {
  
    Object constVal;
    public TypeDesc type;
    public int typeFixUp = 0;

    public FieldInfo(ConstantPool cp, DataInputStream stream,
                   ClassDesc thisClass) throws IOException {    
        super(cp,stream,thisClass);
        this.type = TypeDesc.GetType(this.signature,0);
    }

    public FieldInfo(ClassDesc cl,int acc,String nam,TypeDesc typ,Object cVal) {
        super(cl,acc,nam);
        this.type = typ;
        this.constVal = cVal;
    }

//  @Override
//  public void AddImport(ClassDesc thisClass) {
//    if (type instanceof ClassDesc) { thisClass.AddImport((ClassDesc)type); }
//  }

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

  @Override
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
