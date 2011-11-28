


/** This is the runtime support for generic vectors.
 *
 *  Written August 2004, John Gough.
 *
 *
 *
 */

package CP.CPJvec;

public class VecR64 extends VecBase
{
    public double[] elms;

    public void expand() {
        double[] tmp = new double[this.elms.length * 2];
        for (int i = 0; i < this.tide; i++) {
            tmp[i] = this.elms[i];
        }
        this.elms = tmp;
    }
}

