/**********************************************************************/
/*                  Method Info class for j2cps                       */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

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
        this.parTypes = TypeDesc.GetParTypes(signature);
        this.retType = TypeDesc.GetType(signature,signature.indexOf(')')+1);
        if (this.name.equals("<init>")) { 
            this.userName = "Init"; 
            this.isInitProc = true;
            if (!ConstantPool.isStatic(accessFlags)) {
              this.accessFlags = (this.accessFlags + ConstantPool.ACC_STATIC);
            }
            if ((this.parTypes.length == 0) && 
                    (!ConstantPool.isPrivate(this.accessFlags))) { 
              thisClass.hasNoArgConstructor = true; 
            }
            this.retType = thisClass;
        } else if (this.name.equals("<clinit>")) {
            this.userName="CLInit"; 
            this.isCLInitProc = true;
        } else {
            this.userName=this.name;
        }
        if (ClassDesc.VERBOSE) {
            int parNm = this.parTypes.length;
            System.out.printf("Method %s has %d %s",
                    this.name, parNm, (parNm != 1 ?
                        "parameters" : "parameter"));
        }
        if (this.isExported()) {
            for (TypeDesc parType : this.parTypes) {
                // 
                //  The package of this type must be placed on 
                //  this package's import list iff:
                //   *  the par type is public or protected AND
                //   *  the method's package is not CURRENT.
                //
                if (parType.parentPkg != thisClass.parentPkg) {
                    parType.blame = this;
                    thisClass.TryImport(parType);
                }
            }
            if (this.retType.parentPkg != thisClass.parentPkg) {
                this.retType.blame = this;
                thisClass.TryImport(this.retType);
            }
        }
    }

  public MethodInfo(ClassDesc thisClass,String name,String jName,int acc) {
    super(thisClass,acc,jName);
    this.userName = name;
    if (name.equals("<init>")) { 
      if (userName == null) { 
          this.userName = "Init";
      }
      this.isInitProc = true; 
    }
  }

    @Override
  public String toString() {
    return ConstantPool.GetAccessString(this.accessFlags) + " " + 
            this.name + " " + this.signature;
  }

}
