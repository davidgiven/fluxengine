package org.elm_chan.ff;

/* Mutable holder for C out-parameters (DWORD* / FSIZE_t* / LBA_t*) - no C counterpart, helper for Java translation */
public final class LongRef {
    public long value;

    public LongRef() {}

    public LongRef(long value) {
        this.value = value;
    }
}
