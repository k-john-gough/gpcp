//
// Supertype of all procedure variable classes
//
package CP.CPlib;

public abstract class ProcType 
{
	public final java.lang.reflect.Method theMethod;

	public ProcType(java.lang.reflect.Method m) {
		theMethod = m;
	}
}
