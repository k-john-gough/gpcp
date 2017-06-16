/* 
 * Copyright John Gough 2016-2017
 */
package j2cps;

import java.io.File;

/**
 *
 * @author john
 */
public class ExtFilter implements java.io.FilenameFilter {
    
    private final String dotExtn;

    private ExtFilter(String dotExtn) { this.dotExtn = dotExtn; }

    public static ExtFilter NewExtFilter(String dotExtn) {
        return new ExtFilter(dotExtn);
    }

    @Override
    public boolean accept (File dir, String name) {
        return name.endsWith(this.dotExtn); 
    }
}
