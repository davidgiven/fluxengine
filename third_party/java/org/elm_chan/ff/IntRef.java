package org.elm_chan.ff;

/* Mutable holder for C out-parameters (UINT* br/bw) - no C counterpart, helper for Java translation */
public final class IntRef {
    public int value;

    public IntRef() {}

    public IntRef(int value) {
        this.value = value;
    }
}
