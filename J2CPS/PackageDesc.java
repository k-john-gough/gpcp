/**********************************************************************/
/*                Package Desscriptor class for J2CPS                 */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

public class PackageDesc {

  private static final char qSepCh = '/';
  private static final char fSepCh = 
                             System.getProperty("file.separator").charAt(0);
  private static final char jSepCh = '.';
  private static final char nSepCh = '_';
  private static ArrayList<PackageDesc> toDo = new ArrayList<PackageDesc>(2);
  private static ArrayList<PackageDesc> syms = new ArrayList<PackageDesc>(2);
  private static HashMap<String,PackageDesc> packageList = new HashMap<String,PackageDesc>();
  private File packageFile;

  public ClassDesc[] classes;
  public String name, cpName, javaName, dirName;
  public ArrayList<PackageDesc> imports = new ArrayList<PackageDesc>();
  public int impNum = -1;
  public boolean anonPackage = false;

  public static PackageDesc MakeNewPackageDesc(String pName, boolean anon) {
      PackageDesc desc = new PackageDesc(pName, anon);
      if (!anon)
          packageList.put(desc.name, desc);
      toDo.add(desc);
      return desc;
  }
  
  private PackageDesc(String pName, boolean anon) {
    if (anon) {
      name = pName;
      cpName = pName;
      javaName = pName;
      anonPackage = true;
    } else {
      MakeName(pName); 
    }
  }

  private void MakeName(String pName) {
    name = pName.replace(jSepCh,qSepCh);
    name = name.replace(fSepCh,qSepCh);  /* name is now .../... */
    cpName = name.replace(qSepCh,nSepCh);
    javaName = name.replace(qSepCh,jSepCh);
    if (qSepCh != fSepCh) {
      dirName = name.replace(qSepCh,fSepCh);
    } else {
      dirName = name;
    }
  }
 
  public static PackageDesc getPackage(String packName) {
    packName = packName.replace(jSepCh,qSepCh); 
    PackageDesc pack = (PackageDesc)packageList.get(packName);
    if (pack == null) { pack = PackageDesc.MakeNewPackageDesc(packName,false); }
    return pack;
  }

  public static PackageDesc getClassPackage(String className) {
    className = className.replace(jSepCh,qSepCh); 
    String pName = className.substring(0,className.lastIndexOf(qSepCh));
    PackageDesc pack = (PackageDesc)packageList.get(pName);
    if (pack == null) { pack = PackageDesc.MakeNewPackageDesc(pName,false); }
    return pack;
  }

  public void AddImport(TypeDesc ty) {
    if (ty instanceof ClassDesc) {
      ClassDesc aClass = (ClassDesc)ty;
      if (aClass.packageDesc == null) {
        System.err.println("ERROR: Class "+aClass.qualName+" has no package");
        System.exit(0);
      } 
      if ((this!=aClass.packageDesc)&&(!imports.contains(aClass.packageDesc))){ 
        imports.add(aClass.packageDesc); 
      }
    } 
  }
 
  public void AddImport(PackageDesc pack) {
    if ((this != pack) && (!imports.contains(pack))){ 
      boolean ok = imports.add(pack); 
    }
  }

  public void ResetImports() {
    for (int i=0; i < imports.size(); i++) {
      imports.get(i).impNum = -1;
    }
  }

  private void AddImportList(ArrayList impList) {
    for (int i=0; i < impList.size(); i++) {
      AddImport((PackageDesc)impList.get(i));
    }
  }

  public void ReadPackage() throws IOException, FileNotFoundException {
    boolean ok = syms.add(this);
    if (anonPackage) {
      classes = new ClassDesc[1];
      classes[0] = ClassDesc.GetClassDesc(name,this);
      boolean ok2 = classes[0].ReadClassFile(J2CPSFiles.OpenClassFile(name));
      return;
    } 
    packageFile = J2CPSFiles.getPackageFile(dirName);
    String[] classFiles = packageFile.list(new J2CPSFiles());
    classes = new ClassDesc[classFiles.length];
    for (int i = 0; i < classFiles.length; i++) {
      String cName = name + qSepCh + 
                     classFiles[i].substring(0,classFiles[i].lastIndexOf('.'));
      ClassDesc nextClass = ClassDesc.GetClassDesc(cName,this);
      if (nextClass.ReadClassFile(J2CPSFiles.OpenClassFile(packageFile, 
                                                        classFiles[i]))) {
        classes[i] = nextClass;
      }
    } 
  }

  public static void ReadPackages() throws IOException, FileNotFoundException {
    int j = 0;
    toDo.get(0).ReadPackage();

    if (!ClassDesc.verbose)      // Lightweight progress indicator ...
      System.out.println("INFO: reading dependents ");

    for (int i=1; i < toDo.size(); i++) {
      PackageDesc pack = toDo.get(i);
      /* look for symbol file first */
      pack.packageFile = J2CPSFiles.FindSymbolFile(pack.cpName);
      if (pack.packageFile == null) {
        pack.ReadPackage();
        if (!ClassDesc.verbose) { System.out.print('+'); j++; }
      } else {
        if (ClassDesc.verbose)  {
          System.out.println("Reading Symbol File <" + 
                                             pack.packageFile.getPath() + ">");
        }
        SymbolFile.ReadSymbolFile(pack.packageFile,pack);
        if (!ClassDesc.verbose) { System.out.print('-'); j++; }
      }
      if (j >= 79) { System.out.println(); j = 0; }
    }
    if (!ClassDesc.verbose && j > 0) System.out.println();
  }

  public static void WriteSymbolFiles() throws IOException {
    for (int i=0; i < syms.size(); i++) {
      HashMap<String,MethodInfo> pScope = new HashMap<String,MethodInfo>();
      PackageDesc nextPack = syms.get(i);
      for (int j=0; j < nextPack.classes.length; j++) {
        if (nextPack.classes[j] != null) {
          if (ClassDesc.overloadedNames) {
            nextPack.classes[j].GetSuperImports(); 
          } else {
            nextPack.classes[j].GetSuperFields(pScope); 
          }
          nextPack.AddImportList(nextPack.classes[j].imports);
          ClassDesc superCl = nextPack.classes[j].superClass;
          while (superCl != null) {
            nextPack.AddImport(superCl);
            nextPack.AddImportList(superCl.imports);
            superCl = superCl.superClass;
          }
        }
      }
    }
    for (int i=0; i < syms.size(); i++) {
      PackageDesc nextPack = syms.get(i);
      SymbolFile.WriteSymbolFile(nextPack);
    }
  }
}
