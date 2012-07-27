/**********************************************************************/
/*                  ConstantPool class for J2CPS                      */
/*                                                                    */   
/*                      (c) copyright QUT                             */ 
/**********************************************************************/
package J2CPS;

import java.io.DataInputStream;
import java.io.IOException;

/* The constant pool from the ClassFile */

public class ConstantPool {

  Object pool[];       /* the constant pool */

  /* Tags for constant pool entries */
  public final static int CONSTANT_Utf8               = 1;
  public static final int CONSTANT_Unicode            = 2;
  public final static int CONSTANT_Integer            = 3;
  public final static int CONSTANT_Float              = 4;
  public final static int CONSTANT_Long               = 5;
  public final static int CONSTANT_Double             = 6;
  public final static int CONSTANT_Class              = 7;
  public final static int CONSTANT_String             = 8;
  public final static int CONSTANT_Fieldref           = 9;
  public final static int CONSTANT_Methodref          = 10;
  public final static int CONSTANT_InterfaceMethodref = 11;
  public final static int CONSTANT_NameAndType        = 12;
  public final static int CONSTANT_Unknown            = 13;

  /* access flags */
  public static final int ACC_PUBLIC       = 0x0001;
  public static final int ACC_PRIVATE      = 0x0002;
  public static final int ACC_PROTECTED    = 0x0004;
  public static final int ACC_STATIC       = 0x0008;
  public static final int ACC_FINAL        = 0x0010;
  public static final int ACC_SYNCHRONIZED = 0x0020;
  public static final int ACC_VOLATILE     = 0x0040;
  public static final int ACC_TRANSIENT    = 0x0080;
  public static final int ACC_NATIVE       = 0x0100;
  public static final int ACC_INTERFACE    = 0x0200;
  public static final int ACC_ABSTRACT     = 0x0400;

  public ConstantPool(DataInputStream stream) throws IOException {
    /* read the number of entries in the constant pool */
    int count = stream.readUnsignedShort();
    /* read in the constant pool */ 
    pool = new Object[count];
    for (int i = 1; i < count; i++) {
      Object c = ReadConstant(stream);
      pool[i] = c;
      /* note that Long and Double constant occupies two entries */
      if (c instanceof Long || c instanceof Double) { i++; }
    }
    for (int i = 1; i < pool.length; i++) {
      if (pool[i] instanceof Reference) { 
        ((Reference)pool[i]).Resolve(); 
      } else if (pool[i] instanceof ClassRef) {
        ((ClassRef)pool[i]).Resolve();
      }
    }
  }

  public void EmptyConstantPool() {
    for (int i = 1; i < pool.length; i++) {
      pool[i] = null; 
    }
    pool = null;
  }

  private Object ReadConstant(DataInputStream stream) 
                                                            throws IOException {
    int tag = stream.readUnsignedByte();
    switch (tag) {
    case CONSTANT_Utf8:
      return stream.readUTF();
    case CONSTANT_Integer: 
      return new Integer(stream.readInt());
    case CONSTANT_Float: 
      return new Float(stream.readFloat());
    case CONSTANT_Long: 
      return new Long(stream.readLong());
    case CONSTANT_Double: 
      return new Double(stream.readDouble());
    case CONSTANT_Class: 
      return new ClassRef(this,stream.readUnsignedShort());
    case CONSTANT_String:
      return new StringRef(this,stream.readUnsignedShort());
    case CONSTANT_Fieldref:
      return new FieldRef(this,stream.readUnsignedShort(),
                               stream.readUnsignedShort());
    case CONSTANT_Methodref:
      return new MethodRef(this,stream.readUnsignedShort(), 
                                      stream.readUnsignedShort());
    case CONSTANT_InterfaceMethodref:
      return new InterfaceMethodRef(this,stream.readUnsignedShort(),
                                         stream.readUnsignedShort());
    case CONSTANT_NameAndType:
      return new NameAndType(this,stream.readUnsignedShort(), 
                             stream.readUnsignedShort());
    default:
      System.out.println("Unrecognized constant type: "+String.valueOf(tag));
	return null;
    }
  }
  
  public final Object Get(int index) {
    return pool[index];
  }

  public int GetNumEntries() {
    return pool.length;
  }

  /** Returns a String representing the Constant Pool */
  public void PrintConstantPool() {
    System.out.println(" CONSTANT POOL ENTRIES (" + pool.length + ")");
    for (int i = 1; i < pool.length; i++) {
      System.out.print(i + " ");
      if (pool[i] instanceof String) { 
        System.out.println("<String> " + pool[i]);
      } else if (pool[i] instanceof Integer) {
        System.out.println("<Integer> " + pool[i].toString());
      } else if (pool[i] instanceof Float) {
        System.out.println("<Float  > " + pool[i].toString());
      } else if (pool[i] instanceof Long) {
        System.out.println("<Long   > " + pool[i].toString());
      } else if (pool[i] instanceof Double) {
        System.out.println("<Double>  " + pool[i].toString());
      } else {
        System.out.println(pool[i].toString());
      }
      if (pool[i] instanceof Long || pool[i] instanceof Double) i++;
    }
    System.out.println();
  }

  /** Constructs a string from a set of access flags */
  public static String GetAccessString(int flags) {
    StringBuilder result = new StringBuilder();
    if ((flags & ACC_PUBLIC) != 0) result.append("public ");
    if ((flags & ACC_PRIVATE) != 0) result.append("private ");
    if ((flags & ACC_PROTECTED) != 0) result.append("protected ");
    if ((flags & ACC_STATIC) != 0) result.append("static ");
    if ((flags & ACC_FINAL) != 0) result.append("final ");
    if ((flags & ACC_SYNCHRONIZED) != 0) result.append("synchronized ");
    if ((flags & ACC_VOLATILE) != 0) result.append("volatile ");
    if ((flags & ACC_TRANSIENT) != 0) result.append("transient ");
    if ((flags & ACC_NATIVE) != 0) result.append("native ");
    if ((flags & ACC_INTERFACE) != 0) result.append("interface ");
    if ((flags & ACC_ABSTRACT) != 0) result.append("abstract ");
    return result.toString();
  }

  /** Check if a flag has the public bit set */
  public static boolean isPublic(int flags) {
    return (flags & ACC_PUBLIC) != 0;
  }

  /** Check if a flag has the private bit set */
  public static boolean isPrivate(int flags) {
    return (flags & ACC_PRIVATE) != 0;
  }

  /** Check if a flag has the protected bit set */
  public static boolean isProtected(int flags) {
    return (flags & ACC_PROTECTED) != 0;
  }

  /** Check if a flag has the final bit set */
  public static boolean isFinal(int flags) {
    return (flags & ACC_FINAL) != 0;
  }

  /** Check if a flag has the static bit set */
  public static boolean isStatic(int flags) {
    return (flags & ACC_STATIC) != 0;
  } 

  /** Check if a flag has the native bit set */
  public static boolean isNative(int flags) {
    return (flags & ACC_NATIVE) != 0;
  }

  /** Check if a flag has the interface bit set */
  public static boolean isInterface(int flags) {
    return (flags & ACC_INTERFACE) != 0;
  }

  /** Check if a flag has the abstract bit set */
  public static boolean isAbstract(int flags) {
    return (flags & ACC_ABSTRACT) != 0;
  }

  /** Check if a flag has the synchronized bit set */
  public static boolean isSynchronized(int flags) {
    return (flags & ACC_SYNCHRONIZED) != 0;
  }

  /** Check if a flag has the volatile bit set */
  public static boolean isVolatile(int flags) {
    return (flags & ACC_VOLATILE) != 0;
  }

  /** Check if a flag has the transient bit set */
  public static boolean isTransient(int flags) {
    return (flags & ACC_TRANSIENT) != 0;
  }
}
