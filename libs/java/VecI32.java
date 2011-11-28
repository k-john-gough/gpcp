
/** This is the runtime support for generic vectors.
 *
 *  Written August 2004, John Gough.
 *
 *
 *
 */

package CP.CPJvec;

public class VecI32 extends VecBase
{
    public int[] elms;

    public void expand() {
        int[] tmp = new int[this.elms.length * 2];
        for (int i = 0; i < this.tide; i++) {
            tmp[i] = this.elms[i];
        }
        this.elms = tmp;
    }
}

