/*
 * libhfs - library for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996-1998 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: block.c,v 1.11 1998/11/02 22:08:52 rob Exp $
 */

package org.mars.hfsutils;

import java.util.Arrays;
import java.util.Comparator;

import static org.mars.hfsutils.HfsConstants.*;
import static org.mars.hfsutils.HfsException.*;

public final class HfsBlock
{
    private HfsBlock()
    {
    }

    public static final int INUSE = HFS_BUCKET_INUSE;
    public static final int DIRTY = HFS_BUCKET_DIRTY;

    /* comparator for qsort of cache bucket pointers by block number */
    private static final Comparator<Bucket> BUCKET_CMP = new Comparator<Bucket>()
    {
        @Override
        public int compare(Bucket b1, Bucket b2)
        {
            long diff = b1.bnum - b2.bnum;

            if (diff < 0)
                return -1;
            else if (diff > 0)
                return 1;
            else
                return 0;
        }
    };

    /* functional interface for fillchain / flushchain callback */
    @FunctionalInterface
    private interface BlockIO
    {
        int operate(HfsVol vol, Bucket[] chain, int offset, int[] count);
    }

    /* Helper methods ========================================================== */

    private static int fail(int errno, String msg)
    {
        Hfs.hfsError = msg;
        Hfs.hfsErrno = errno;
        return -1;
    }

    private static boolean isInUse(Bucket b)
    {
        return (b.flags & INUSE) != 0;
    }

    private static boolean isDirty(Bucket b)
    {
        return (b.flags & DIRTY) != 0;
    }

    /*
     * BMTST(bm, num) — test a bit in the volume bitmap.
     * bm is byte[][] (array of 512-byte blocks); num is the allocation block
     * number.
     */
    private static boolean bmtst(byte[][] bm, int num)
    {
        int flatByte = num >> 3;

        return (bm[flatByte / HFS_BLOCKSZ][flatByte % HFS_BLOCKSZ]
                & (0x80 >> (num & 0x07))) != 0;
    }

    /*
     * NAME:	block->init()
     * DESCRIPTION:	initialize a volume's block cache
     */
    public static int b_init(HfsVol vol)
    {
        Bcache cache;
        int i;

        /* ASSERT(vol.cache == null); */

        cache = new Bcache();

        vol.cache = cache;

        cache.vol    = vol;
        cache.tail   = cache.chain[HFS_CACHESZ - 1];

        cache.hits   = 0;
        cache.misses = 0;

        for (i = 0; i < HFS_CACHESZ; ++i)
        {
            Bucket b = cache.chain[i];

            b.flags = 0;
            b.count = 0;

            b.bnum  = 0;
            b.data  = cache.pool[i];

            b.cnext = (i < HFS_CACHESZ - 1) ? cache.chain[i + 1] : null;
            b.cprev = (i > 0) ? cache.chain[i - 1] : null;

            b.hnext = null;
            b.hprev = null;
        }

        cache.chain[0].cprev = cache.tail;
        cache.tail.cnext     = cache.chain[0];

        for (i = 0; i < HFS_HASHSZ; ++i)
            cache.hash[i].value = null;

        return 0;

        /* fail: return -1; */
    }

    /*
     * NAME:	fillchain()
     * DESCRIPTION:	fill a chain of bucket buffers with a single read
     */
    private static int fillchain(HfsVol vol, Bucket[] chain,
                                 int offset, int[] count)
    {
        Bucket[] blist = new Bucket[HFS_BLOCKBUFSZ];
        int startOffset = offset;
        long bnum = 0;
        int len = 0;
        int i;

        for (len = 0; len < HFS_BLOCKBUFSZ &&
             (offset - startOffset) < count[0]; ++offset)
        {
            Bucket b = chain[offset];

            if (isInUse(b))
                continue;

            if (len > 0 && b.bnum != bnum)
                break;

            blist[len++] = b;
            bnum = b.bnum + 1;
        }

        count[0] = offset - startOffset;

        if (len == 0)
            return 0;
        else if (len == 1)
        {
            if (b_readpb(vol, vol.vstart + blist[0].bnum,
                         blist[0].data, 1) == -1)
                return -1;
        }
        else
        {
            byte[] buffer = new byte[HFS_BLOCKBUFSZ * HFS_BLOCKSZ];

            if (b_readpb(vol, vol.vstart + blist[0].bnum, buffer, len) == -1)
                return -1;

            for (i = 0; i < len; ++i)
                System.arraycopy(buffer, i * HFS_BLOCKSZ,
                                 blist[i].data, 0, HFS_BLOCKSZ);
        }

        for (i = 0; i < len; ++i)
        {
            blist[i].flags |=  INUSE;
            blist[i].flags &= ~DIRTY;
        }

        return 0;
    }

    /*
     * NAME:	flushchain()
     * DESCRIPTION:	store a chain of bucket buffers with a single write
     */
    private static int flushchain(HfsVol vol, Bucket[] chain,
                                  int offset, int[] count)
    {
        Bucket[] blist = new Bucket[HFS_BLOCKBUFSZ];
        int startOffset = offset;
        long bnum = 0;
        int len = 0;
        int i;

        for (len = 0; len < HFS_BLOCKBUFSZ &&
             (offset - startOffset) < count[0]; ++offset)
        {
            Bucket b = chain[offset];

            if (! isInUse(b) || ! isDirty(b))
                continue;

            if (len > 0 && b.bnum != bnum)
                break;

            blist[len++] = b;
            bnum = b.bnum + 1;
        }

        count[0] = offset - startOffset;

        if (len == 0)
            return 0;
        else if (len == 1)
        {
            if (b_writepb(vol, vol.vstart + blist[0].bnum,
                          blist[0].data, 1) == -1)
                return -1;
        }
        else
        {
            byte[] buffer = new byte[HFS_BLOCKBUFSZ * HFS_BLOCKSZ];

            for (i = 0; i < len; ++i)
                System.arraycopy(blist[i].data, 0,
                                 buffer, i * HFS_BLOCKSZ, HFS_BLOCKSZ);

            if (b_writepb(vol, vol.vstart + blist[0].bnum, buffer, len) == -1)
                return -1;
        }

        for (i = 0; i < len; ++i)
            blist[i].flags &= ~DIRTY;

        return 0;
    }

    /*
     * NAME:	dobuckets()
     * DESCRIPTION:	fill or flush an array of cache buckets to a volume
     */
    private static int dobuckets(HfsVol vol, Bucket[] chain, int len,
                                 BlockIO func)
    {
        int result = 0;

        Arrays.sort(chain, 0, len, BUCKET_CMP);

        for (int i = 0; i < len; )
        {
            int[] count = {len - i};

            if (func.operate(vol, chain, i, count) == -1)
                result = -1;

            i += count[0];
        }

        return result;
    }

    private static int fillbuckets(HfsVol vol, Bucket[] chain, int len)
    {
        return dobuckets(vol, chain, len, HfsBlock::fillchain);
    }

    private static int flushbuckets(HfsVol vol, Bucket[] chain, int len)
    {
        return dobuckets(vol, chain, len, HfsBlock::flushchain);
    }

    /*
     * NAME:	block->flush()
     * DESCRIPTION:	commit dirty cache blocks to a volume
     */
    public static int b_flush(HfsVol vol)
    {
        Bcache cache = vol.cache;
        Bucket[] chain = new Bucket[HFS_CACHESZ];
        int i;

        if (cache == null || (vol.flags & HFS_VOL_READONLY) != 0)
            return 0;

        for (i = 0; i < HFS_CACHESZ; ++i)
            chain[i] = cache.chain[i];

        if (flushbuckets(vol, chain, HFS_CACHESZ) == -1)
            return -1;

        return 0;
    }

    /*
     * NAME:	block->finish()
     * DESCRIPTION:	commit and free a volume's block cache
     */
    public static int b_finish(HfsVol vol)
    {
        int result = 0;

        if (vol.cache == null)
            return result;

        result = b_flush(vol);

        vol.cache = null;

        return result;
    }

    /*
     * NAME:	findbucket()
     * DESCRIPTION:	locate a bucket in the cache, and/or its hash slot
     */
    private static Bucket findbucket(Bcache cache, long bnum,
                                     BucketSlot[] hslot)
    {
        hslot[0] = cache.hash[(int) (bnum & (HFS_HASHSZ - 1))];

        for (Bucket b = hslot[0].value; b != null;
             b = (b.hnext != null ? b.hnext.value : null))
        {
            if (isInUse(b) && b.bnum == bnum)
                return b;
        }

        return null;
    }

    /*
     * NAME:	reuse()
     * DESCRIPTION:	free a bucket for reuse, flushing if necessary
     */
    private static int reuse(Bcache cache, Bucket b, long bnum)
    {
        if (isInUse(b) && isDirty(b))
        {
            /* flush most recently unused buckets */

            Bucket[] chain = new Bucket[HFS_BLOCKBUFSZ];
            Bucket bptr = b;

            for (int i = 0; i < HFS_BLOCKBUFSZ; ++i)
            {
                chain[i] = bptr;
                bptr = bptr.cprev;
            }

            if (flushbuckets(cache.vol, chain, HFS_BLOCKBUFSZ) == -1)
                return -1;
        }

        b.flags &= ~INUSE;
        b.count  = 1;
        b.bnum   = bnum;

        return 0;
    }

    /*
     * NAME:	cplace()
     * DESCRIPTION:	move a bucket to an appropriate place near head of the chain
     */
    private static void cplace(Bcache cache, Bucket b)
    {
        Bucket p;

        for (p = cache.tail.cnext; p.count > 1; p = p.cnext)
            --p.count;

        b.cnext.cprev = b.cprev;
        b.cprev.cnext = b.cnext;

        if (cache.tail == b)
            cache.tail = b.cprev;

        b.cprev = p.cprev;
        b.cnext = p;

        p.cprev.cnext = b;
        p.cprev = b;
    }

    /*
     * NAME:	hplace()
     * DESCRIPTION:	move a bucket to the head of its hash slot
     */
    private static void hplace(BucketSlot hslot, Bucket b)
    {
        if (hslot.value != b)
        {
            if (b.hprev != null)
                b.hprev.value = (b.hnext != null ? b.hnext.value : null);
            if (b.hnext != null)
                b.hnext.value.hprev = b.hprev;

            b.hprev = hslot;
            b.hnext = new BucketSlot();
            b.hnext.value = hslot.value;

            if (hslot.value != null)
                hslot.value.hprev = b.hnext;

            hslot.value = b;
        }
    }

    /*
     * NAME:	getbucket()
     * DESCRIPTION:	fetch a bucket from the cache, or an empty one to be filled
     */
    private static Bucket getbucket(Bcache cache, long bnum, boolean fill)
    {
        BucketSlot[] hslotArr = new BucketSlot[1];
        Bucket b;
        Bucket p;
        Bucket bptr;
        Bucket[] chain = new Bucket[HFS_BLOCKBUFSZ];
        BucketSlot[] slots = new BucketSlot[HFS_BLOCKBUFSZ];

        b = findbucket(cache, bnum, hslotArr);

        if (b != null)
        {
            /* cache hit; move towards head of cache chain */

            ++cache.hits;

            if (++b.count > b.cprev.count &&
                b != cache.tail.cnext)
            {
                p = b.cprev;

                p.cprev.cnext = b;
                b.cnext.cprev = p;

                p.cnext = b.cnext;
                b.cprev = p.cprev;

                p.cprev = b;
                b.cnext = p;

                if (cache.tail == b)
                    cache.tail = p;
            }
        }
        else
        {
            /* cache miss; reuse least-used cache bucket */

            int len = 0;

            ++cache.misses;

            b = cache.tail;

            if (reuse(cache, b, bnum) == -1)
                return null;

            if (fill)
            {
                chain[len]   = b;
                slots[len++] = hslotArr[0];

                for (bptr = b.cprev;
                     len < (HFS_BLOCKBUFSZ >> 1) && ++bnum < cache.vol.vlen;
                     bptr = bptr.cprev)
                {
                    if (findbucket(cache, bnum, hslotArr) != null)
                        break;

                    if (reuse(cache, bptr, bnum) == -1)
                        return null;

                    chain[len]   = bptr;
                    slots[len++] = hslotArr[0];
                }

                if (fillbuckets(cache.vol, chain, len) == -1)
                    return null;

                while (--len > 0)
                {
                    cplace(cache, chain[len]);
                    hplace(slots[len], chain[len]);
                }

                hslotArr[0] = slots[0];
            }

            /* move bucket to appropriate place in chain */

            cplace(cache, b);
        }

        /* insert at front of hash chain */

        hplace(hslotArr[0], b);

        return b;
    }

    /*
     * NAME:	block->readpb()
     * DESCRIPTION:	read blocks from the physical medium (bypassing cache)
     */
    public static int b_readpb(HfsVol vol, long bnum, byte[] buf, int blen)
    {
        long nblocks;

        nblocks = vol.priv.seek(bnum);
        if (nblocks == -1)
            return -1;

        if (nblocks != bnum)
            return fail(EIO, "block seek failed for read");

        nblocks = vol.priv.read(buf, blen);
        if (nblocks == -1)
            return -1;

        if (nblocks != blen)
            return fail(EIO, "incomplete block read");

        return 0;
    }

    /*
     * NAME:	block->writepb()
     * DESCRIPTION:	write blocks to the physical medium (bypassing cache)
     */
    public static int b_writepb(HfsVol vol, long bnum, byte[] buf, int blen)
    {
        long nblocks;

        nblocks = vol.priv.seek(bnum);
        if (nblocks == -1)
            return -1;

        if (nblocks != bnum)
            return fail(EIO, "block seek failed for write");

        nblocks = vol.priv.write(buf, blen);
        if (nblocks == -1)
            return -1;

        if (nblocks != blen)
            return fail(EIO, "incomplete block write");

        return 0;
    }

    /*
     * NAME:	block->readlb()
     * DESCRIPTION:	read a logical block from a volume (or from the cache)
     */
    public static int b_readlb(HfsVol vol, long bnum, byte[] bp)
    {
        if (vol.vlen > 0 && bnum >= vol.vlen)
            return fail(EIO, "read nonexistent logical block");

        if (vol.cache != null)
        {
            Bucket b;

            b = getbucket(vol.cache, bnum, true);
            if (b == null)
                return -1;

            System.arraycopy(b.data, 0, bp, 0, HFS_BLOCKSZ);
        }
        else
        {
            if (b_readpb(vol, vol.vstart + bnum, bp, 1) == -1)
                return -1;
        }

        return 0;
    }

    /*
     * NAME:	block->writelb()
     * DESCRIPTION:	write a logical block to a volume (or to the cache)
     */
    public static int b_writelb(HfsVol vol, long bnum, byte[] bp)
    {
        if (vol.vlen > 0 && bnum >= vol.vlen)
            return fail(EIO, "write nonexistent logical block");

        if (vol.cache != null)
        {
            Bucket b;

            b = getbucket(vol.cache, bnum, false);
            if (b == null)
                return -1;

            if (! isInUse(b) ||
                !Arrays.equals(b.data, bp))
            {
                System.arraycopy(bp, 0, b.data, 0, HFS_BLOCKSZ);
                b.flags |= INUSE | DIRTY;
            }
        }
        else
        {
            if (b_writepb(vol, vol.vstart + bnum, bp, 1) == -1)
                return -1;
        }

        return 0;
    }

    /*
     * NAME:	block->readab()
     * DESCRIPTION:	read a block from an allocation block from a volume
     */
    public static int b_readab(HfsVol vol, int anum, int index, byte[] bp)
    {
        /* verify the allocation block exists and is marked as in-use */

        if (anum >= vol.mdb.drNmAlBlks)
            return fail(EIO, "read nonexistent allocation block");
        else if (vol.vbm != null && ! bmtst(vol.vbm, anum))
            return fail(EIO, "read unallocated block");

        return b_readlb(vol, vol.mdb.drAlBlSt + anum * vol.lpa + index, bp);
    }

    /*
     * NAME:	block->writeab()
     * DESCRIPTION:	write a block to an allocation block to a volume
     */
    public static int b_writeab(HfsVol vol, int anum, int index, byte[] bp)
    {
        /* verify the allocation block exists and is marked as in-use */

        if (anum >= vol.mdb.drNmAlBlks)
            return fail(EIO, "write nonexistent allocation block");
        else if (vol.vbm != null && ! bmtst(vol.vbm, anum))
            return fail(EIO, "write unallocated block");

        if (HfsVolume.v_dirty(vol) == -1)
            return -1;

        return b_writelb(vol, vol.mdb.drAlBlSt + anum * vol.lpa + index, bp);
    }

    /*
     * NAME:	block->size()
     * DESCRIPTION:	return the number of physical blocks on a volume's medium
     */
    public static long b_size(HfsVol vol)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        long low, high, mid;

        high = vol.priv.seek(-1);

        if (high != -1 && high > 0)
            return high;

        /* manual size detection: first check there is at least 1 block in medium */

        if (b_readpb(vol, 0, b, 1) == -1)
        {
            fail(EIO, "size of medium indeterminable or empty");
            return 0;
        }

        for (low = 0, high = 2880;
             high > 0 && b_readpb(vol, high - 1, b, 1) != -1;
             high <<= 1)
            low = high - 1;

        if (high == 0)
        {
            fail(EIO, "size of medium indeterminable or too large");
            return 0;
        }

        /* common case: 1440K floppy */

        if (low == 2879 && b_readpb(vol, 2880, b, 1) == -1)
            return 2880;

        /* binary search for other sizes */

        while (low < high - 1)
        {
            mid = (low + high) >> 1;

            if (b_readpb(vol, mid, b, 1) == -1)
                high = mid;
            else
                low = mid;
        }

        return low + 1;
    }
}
