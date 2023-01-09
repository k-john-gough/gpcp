/*
 * (c) copyright John Gough, 2016-2017
 */
package j2cps;

// import java.util.function.Predicate;

/**
 *
 * @author john
 */
public class Util {
    //
    // All these constants are package private
    //
    static final int PRISTINE = 0;
    static final int ONLIST   = 0x1;
    static final int ONTODO   = 0x2;
    static final int ONSYMS   = 0x4;
    static final int FROMJAR  = 0x8;
    static final int FROMCLS  = 0x10;
    static final int APINEEDS = 0x20;
    static final int FOUNDCPS = 0x40;
    static final int CURRENT  = 0x80;

    static final char FWDSLSH = '/';
    static final char FILESEP = 
                    System.getProperty("file.separator").charAt(0);
    static final char JAVADOT = '.';
    static final char LOWLINE = '_';

    public void Assert(boolean p) {
        if (!p)
            throw new IllegalArgumentException( "Assertion Failed"); 
    }
    
    public static String Tag(int tag) {
        switch (tag) {
            case ONLIST: return "ONLIST,";
            case ONTODO: return "ONTODO,";
            case ONSYMS: return "ONSYMS,";
            case FROMJAR: return "FROMJAR,";
            case FROMCLS: return "FROMCLS,";
            case APINEEDS: return "APINEEDS,";
            case FOUNDCPS: return "FOUNDCPS,";
            case CURRENT: return "CURRENT,";
            default: return "unknown,";    
        }
    }
}
