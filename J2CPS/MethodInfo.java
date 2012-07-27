/**********************************************************************/
/*                  Method Info class for J2CPS                       */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.DataInputStream;
import java.io.IOException;

public class MethodInfo extends MemberInfo {

  public TypeDesc[] parTypes;
  public TypeDesc retType;
  public String userName;
  public boolean deprecated = false;
  public int retTypeFixUp = 0;
  public int[] parFixUps;
  public boolean overridding = false;
  public boolean isInitProc = false;
  public boolean isCLInitProc = false;

  public MethodInfo(ConstantPool cp,DataInputStream stream,
                    ClassDesc thisClass) throws IOException {
    super(cp,stream,thisClass);
    parTypes = TypeDesc.GetParTypes(signature);
    retType = TypeDesc.GetType(signature,signature.indexOf(')')+1);
    if (name.equals("<init>")) { 
      userName = "Init"; 
      isInitProc = true;
      if (!ConstantPool.isStatic(accessFlags)) {
        accessFlags = (accessFlags + ConstantPool.ACC_STATIC);
      }
      if ((parTypes.length == 0) && (!ConstantPool.isPrivate(accessFlags))) { 
        thisClass.hasNoArgConstructor = true; 
      }
      retType = thisClass;
    } else if (name.equals("<clinit>")) {
      userName="CLInit"; 
      isCLInitProc = true;
    }
    if (ClassDesc.verbose) { 
      System.out.println("Method has " + parTypes.length + " parameters");
    }
    //AddImport(thisClass);
    for (int i=0; i < parTypes.length; i++)
        thisClass.TryImport(parTypes[i]);
    thisClass.TryImport(retType);
  }

  public MethodInfo(ClassDesc thisClass,String name,String jName,int acc) {
    super(thisClass,acc,jName);
    userName = name;
    if (name.equals("<init>")) { 
      if (userName == null) { userName = "Init";}
      isInitProc = true; 
    }
  }

//  public void AddImport(ClassDesc thisClass) {
//    for (int i=0; i < parTypes.length; i++)
//        thisClass.TryImport(parTypes[i]);
//    thisClass.TryImport(retType);
//  }

    @Override
  public String toString() {
    return ConstantPool.GetAccessString(accessFlags) + " " + name + " " + 
           signature;
  }

}
