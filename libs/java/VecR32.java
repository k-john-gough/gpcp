

/** This is the runtime support for generic vectors.
 *
 *  Written August 2004, John Gough.
 *
 *
 *
 */

package CP.CPJvec;

public class VecR32 extends VecBase
{
    public float[] elms;

    public void expand() {
        float[] tmp = new float[this.elms.length * 2];
        for (int i = 0; i < this.tide; i++) {
            tmp[i] = this.elms[i];
        }
        this.elms = tmp;
    }
}

