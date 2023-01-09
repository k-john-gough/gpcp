/**********************************************************************/
/*                  Member Info class for j2cps                       */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.io.DataInputStream;
import java.io.IOException;

public class MemberInfo {

  public ClassDesc owner;
  public int accessFlags;
  public String name;
  public String signature;
  
  public MemberInfo(ConstantPool cp,DataInputStream stream,ClassDesc own) 
                                                            throws IOException {
    owner = own;
    accessFlags = stream.readUnsignedShort();
    name = (String) cp.Get(stream.readUnsignedShort());
    signature = (String) cp.Get(stream.readUnsignedShort());
    /* skip the attributes */
    int attrCount = stream.readUnsignedShort();
    for (int i = 0; i < attrCount; i++) {  
      int attNameIx = stream.readUnsignedShort();
      if ("ConstantValue".equals((String)cp.Get(attNameIx)) &&
         (this instanceof FieldInfo)) {
        ((FieldInfo)this).GetConstValueAttribute(cp,stream);
      } else {
        if ("Deprecated".equals((String)cp.Get(attNameIx)) &&
         (this instanceof MethodInfo)) { ((MethodInfo)this).deprecated = true; }
        int attrLength = stream.readInt();
        for (int j = 0; j < attrLength; j++) {
          int tmp = stream.readByte();
        }
      }
    }
  }

  public MemberInfo(ClassDesc own,int acc,String nam) {
    owner = own;
    accessFlags = acc;
    name = nam;
  }

  public boolean isPublicStatic() {
    return ConstantPool.isStatic(accessFlags) && 
           ConstantPool.isPublic(accessFlags);
  }

  public final boolean isExported() {
    return (ConstantPool.isPublic(accessFlags) ||
            ConstantPool.isProtected(accessFlags)); 
  }

  public boolean isPublic() {
    return ConstantPool.isPublic(accessFlags); 
  }

  public boolean isStatic() {
    return ConstantPool.isStatic(accessFlags); 
  }

  public boolean isPrivate() {
    return ConstantPool.isPrivate(accessFlags); 
  }

  public boolean isProtected() {
    return ConstantPool.isProtected(accessFlags); 
  }

  public boolean isAbstract() {
    return ConstantPool.isAbstract(accessFlags); 
  }

  public boolean isFinal() {
    return ConstantPool.isFinal(accessFlags); 
  }

    @Override
  public String toString() { return ""; };

  
}
