/**********************************************************************/
/*                 Type Descriptor class for j2cps                    */
/*                                                                    */   
/*  (c) copyright QUT, John Gough 2000-2012, John Gough, 2012-2017    */ 
/**********************************************************************/
package j2cps;

import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;

public class TypeDesc {

  public static final int noTyp  = 0;
  public static final int boolT  = 1;
  public static final int sCharT = 2;
  public static final int charT  = 3;
  public static final int byteT  = 4;
  public static final int shortT = 5;
  public static final int intT   = 6;
  public static final int longT  = 7;
  public static final int floatT = 8;
  public static final int dbleT  = 9;
  public static final int setT   = 10;
  public static final int anyRT  = 11;
  public static final int anyPT  = 12;
  public static final int strT   = 13;
  public static final int sStrT  = 14;
  public static final int specT  = 15;
  public static final int ordT   = 16;
  public static final int arrT   = 17;
  public static final int classT = 18;
  public static final int arrPtr = 19;
  
  public int typeFixUp = 0;

  private static final String[] typeStrArr = 
                              { "?","B","c","C","b","i","I","L","r","R",
                                "?","?","?","?","?","?","?","a","O","?"};
  
  /**
   *  Component Pascal version of java language name
   */
  public String name;
  
  /**
   *  If this boolean is true then the type is
   *  dumped to the symbol file with full 
   *  member information. If false just the 
   *  name and type ordinal number is emitted.
   */
  public boolean writeDetails = false;
  
  /**
   *  This field holds the package descriptor
   *  to which this type belongs.
   */
  public PackageDesc parentPkg = null;
  
    // ###########################
    // Temporary code for finding rogue elements in imports lists.
    // MethodInfo or FieldInfo that caused addition to import list.
    Object blame = null;
    // ###########################


  private static TypeDesc[] basicTypes = new TypeDesc[specT];

  int   inTypeNum=0; 
  int   outTypeNum=0; 
  int   inBaseTypeNum = 0;
  int   typeOrd = 0;
  
  static ArrayList<TypeDesc> types = new ArrayList<>();

  public TypeDesc() {
    inTypeNum = 0;
    outTypeNum = 0;
    typeOrd = 0;
  }

  private TypeDesc(int ix) {
  /* ONLY used for basic types */
    inTypeNum = ix;
    outTypeNum = ix;
    typeOrd = ix;
  }

  public String getTypeMnemonic() {
    return typeStrArr[typeOrd];
  }

  
  public static TypeDesc GetBasicType(int index) {
    return basicTypes[index];
  }

  public static TypeDesc GetType(String sig,int start) {
    int tOrd = GetTypeOrd(sig,start);
      switch (tOrd) {
          case classT:
              return ClassDesc.GetClassDesc(GetClassName(sig,start),null);
          case arrT:
              return ArrayDesc.GetArrayTypeFromSig(sig,start,true);
          default:
              return basicTypes[tOrd];
      }
  }
  
  private static String GetClassName(String sig,int start) {
    if (sig.charAt(start) != 'L') { 
      System.out.println(sig.substring(0) + " is not a class name string!");
      System.exit(1);
    }
    int endCName = sig.indexOf(';',start);
    if (endCName == -1) {
      return sig.substring(start+1);
    } else {
      return sig.substring(start+1,endCName);
    }
  }

  private static int GetTypeOrd(String sig,int start) {
    switch (sig.charAt(start)) {
      case 'B' : return byteT; 
      case 'C' : return charT; 
      case 'D' : return dbleT;
      case 'F' : return floatT; 
      case 'I' : return intT; 
      case 'J' : return longT; 
      case 'S' : return shortT; 
      case 'Z' : return boolT; 
      case 'V' : return noTyp;
      case 'L' : return classT;
      case '[' : return arrT;
    }
    return 0;
  }

  public static TypeDesc[] GetParTypes(String sig) {
    types.clear();
    TypeDesc[] typeArr;
    if (sig.charAt(0) != '(') {
      System.out.println(sig + " is not a parameter list!");
      System.exit(1);
    }
    int index = 1;
    while (sig.charAt(index) != ')') {
      if (sig.charAt(index) == '[') { 
        types.add(ArrayDesc.GetArrayTypeFromSig(sig,index,false));  
      } else {
        types.add(GetType(sig,index));
      }
        switch (sig.charAt(index)) {
            case 'L':
                index = sig.indexOf(';',index) + 1;
                break;
            case '[':
                while (sig.charAt(index) == '[') { 
                    index++; 
                }
                if (sig.charAt(index) == 'L') { 
                    index = sig.indexOf(';',index) + 1;
                } else { 
                    index++; 
                }
                break;
            default:
                index++;
                break;
        }
    } 
    typeArr = new TypeDesc[types.size()]; 
    for (int i=0; i < types.size(); i++) {
      typeArr[i] = types.get(i);
    }
    return typeArr; 
  }

  public static void InitTypes() {
    for (int i=0; i < specT; i++) {
      basicTypes[i] = new TypeDesc(i);
      basicTypes[i].name = "BasicType" + i;
      SymbolFile.typeList[i] = basicTypes[i];
    }
  }

  public void writeType (DataOutputStream out, PackageDesc thisPack) 
                                                           throws IOException {
    System.err.println("TRYING TO WRITE A TYPEDESC! with ord " + typeOrd);
    System.exit(1);
  }
}

