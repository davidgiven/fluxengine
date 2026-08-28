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
 * $Id: node.c,v 1.9 1998/11/02 22:09:05 rob Exp $
 */

package org.mars.hfsutils;

import java.util.Arrays;

import static org.mars.hfsutils.HfsConstants.*;
import static org.mars.hfsutils.HfsException.*;

public final class HfsNode
{
    private HfsNode()
    {
    }

    /*
     * Helper — set hfs_error/hfs_errno and return -1 (mirrors the C ERROR macro's
     * fail: label).
     */
    private static int fail(int errno, String msg)
    {
        Hfs.hfsError = msg;
        Hfs.hfsErrno = errno;
        return -1;
    }

    /*
     * Copy a Node (struct copy in C: *right = *left).
     * Java arrays are reference types, so we must deep-copy data and roff.
     */
    private static void copyNode(Node dst, Node src)
    {
        dst.bt = src.bt;
        dst.nnum = src.nnum;

        dst.nd.ndFLink   = src.nd.ndFLink;
        dst.nd.ndBLink   = src.nd.ndBLink;
        dst.nd.ndType    = src.nd.ndType;
        dst.nd.ndNHeight = src.nd.ndNHeight;
        dst.nd.ndNRecs   = src.nd.ndNRecs;
        dst.nd.ndResv2   = src.nd.ndResv2;

        dst.rnum = src.rnum;

        System.arraycopy(src.roff, 0, dst.roff, 0, src.roff.length);
        System.arraycopy(src.data, 0, dst.data, 0, src.data.length);
    }

    /* total bytes used by records (NOT including record offsets) */

    private static int nodeUsed(Node n)
    {
        return n.roff[n.nd.ndNRecs] - n.roff[0];
    }

    /* total bytes available for new records (INCLUDING record offsets) */

    private static int nodeFree(Node n)
    {
        return HFS_BLOCKSZ - n.roff[n.nd.ndNRecs] - 2 * (n.nd.ndNRecs + 1);
    }

    /*
     * B*-tree bitmap helpers.  bt.map is a flat byte[].
     */

    private static boolean bmtst(byte[] bm, long num)
    {
        return (bm[(int) (num >> 3)] & (0x80 >> (num & 7))) != 0;
    }

    private static void bmset(byte[] bm, long num)
    {
        bm[(int) (num >> 3)] |= (byte) (0x80 >> (num & 7));
    }

    private static void bmclr(byte[] bm, long num)
    {
        bm[(int) (num >> 3)] &= (byte) ~(0x80 >> (num & 7));
    }

    /*
     * Extract the record key from np.data at the given offset into a fresh
     * byte[].  keyunpack needs a byte[]; the C code passes a pointer into the
     * node data buffer.
     */
    private static byte[] extractRecordKey(Node np, int recOffset)
    {
        int keyLen = np.data[recOffset] & 0xff;
        int keySkip = (keyLen + 2) & ~1;
        byte[] rec = new byte[keySkip];
        System.arraycopy(np.data, recOffset, rec, 0, keySkip);
        return rec;
    }

    /*
     * NAME:	node->init()
     * DESCRIPTION:	construct an empty node
     */
    public static void n_init(Node np, BTree bt, int type, int height)
    {
        np.bt   = bt;
        np.nnum = 0xffffffffL;

        np.nd.ndFLink   = 0;
        np.nd.ndBLink   = 0;
        np.nd.ndType    = (byte) type;
        np.nd.ndNHeight = (byte) height;
        np.nd.ndNRecs   = 0;
        np.nd.ndResv2   = 0;

        np.rnum    = -1;
        np.roff[0] = 0x00e;

        Arrays.fill(np.data, (byte) 0);
    }

    /*
     * NAME:	node->new()
     * DESCRIPTION:	allocate a new b*-tree node
     */
    public static int n_new(Node np)
    {
        BTree bt = np.bt;
        long num;

        if (bt.hdr.bthFree == 0)
            return fail(EIO, "b*-tree full");

        num = 0;
        while (num < bt.hdr.bthNNodes && bmtst(bt.map, num))
            ++num;

        if (num == bt.hdr.bthNNodes)
            return fail(EIO, "free b*-tree node not found");

        np.nnum = num;

        bmset(bt.map, num);
        --bt.hdr.bthFree;

        bt.flags |= HFS_BT_UPDATE_HDR;

        return 0;
    }

    /*
     * NAME:	node->free()
     * DESCRIPTION:	deallocate and remove a b*-tree node
     */
    public static int n_free(Node np)
    {
        BTree bt = np.bt;
        Node sib = new Node();

        if (bt.hdr.bthFNode == np.nnum)
            bt.hdr.bthFNode = np.nd.ndFLink;

        if (bt.hdr.bthLNode == np.nnum)
            bt.hdr.bthLNode = np.nd.ndBLink;

        if (np.nd.ndFLink > 0)
        {
            if (HfsBTree.bt_getnode(sib, bt, np.nd.ndFLink) == -1)
                return -1;

            sib.nd.ndBLink = np.nd.ndBLink;

            if (HfsBTree.bt_putnode(sib) == -1)
                return -1;
        }

        if (np.nd.ndBLink > 0)
        {
            if (HfsBTree.bt_getnode(sib, bt, np.nd.ndBLink) == -1)
                return -1;

            sib.nd.ndFLink = np.nd.ndFLink;

            if (HfsBTree.bt_putnode(sib) == -1)
                return -1;
        }

        bmclr(bt.map, np.nnum);
        ++bt.hdr.bthFree;

        bt.flags |= HFS_BT_UPDATE_HDR;

        return 0;
    }

    /*
     * NAME:	compact()
     * DESCRIPTION:	clean up a node, removing deleted records
     */
    private static void compact(Node np)
    {
        int ptrOff, offset, nrecs, i;

        offset = 0x00e;
        ptrOff = offset;
        nrecs  = 0;

        for (i = 0; i < np.nd.ndNRecs; ++i)
        {
            int rec    = np.roff[i];
            int reclen = np.roff[i + 1] - np.roff[i];

            if ((np.data[rec] & 0xff) > 0)
            {
                np.roff[nrecs++] = offset;
                offset += reclen;

                if (ptrOff == rec)
                    ptrOff += reclen;
                else
                    System.arraycopy(np.data, rec, np.data, ptrOff, reclen);
            }
        }

        np.roff[nrecs] = offset;
        np.nd.ndNRecs  = (short) nrecs;
    }

    /*
     * NAME:	node->search()
     * DESCRIPTION:	locate a record in a node, or the record it should follow
     */
    public static int n_search(Node np, byte[] pkey)
    {
        BTree bt = np.bt;
        byte[] key1 = new byte[HFS_MAX_KEYLEN];
        byte[] key2 = new byte[HFS_MAX_KEYLEN];
        int i, comp = -1;

        bt.keyunpack.unpack(pkey, key2);

        for (i = np.nd.ndNRecs; i-- > 0; )
        {
            int rec = np.roff[i];

            if ((np.data[rec] & 0xff) == 0)
                continue;  /* deleted record */

            byte[] recKey = extractRecordKey(np, rec);

            bt.keyunpack.unpack(recKey, key1);
            comp = bt.keycompare.compare(key1, key2);

            if (comp <= 0)
                break;
        }

        np.rnum = i;

        return comp == 0 ? 1 : 0;
    }

    /*
     * NAME:	node->index()
     * DESCRIPTION:	create an index record from a key and node pointer
     */
    public static void n_index(Node np, byte[] record, int[] reclen)
    {
        int key = np.roff[0];

        if (np.bt == np.bt.f.vol.cat)
        {
            /* force the key length to be 0x25 */

            record[0] = 0x25;
            Arrays.fill(record, 1, 1 + 0x25, (byte) 0);

            int srcKeyLen = np.data[key] & 0xff;
            if (srcKeyLen > 0x25)
                srcKeyLen = 0x25;
            System.arraycopy(np.data, key + 1, record, 1, srcKeyLen);
        }
        else
        {
            int keyLen = np.data[key] & 0xff;
            int keySkip = (keyLen + 2) & ~1;
            System.arraycopy(np.data, key, record, 0, keySkip);
        }

        int recDataOffset = ((record[0] & 0xff) + 2) & ~1;
        HfsData.d_putul(record, recDataOffset, np.nnum);

        if (reclen != null)
            reclen[0] = recDataOffset + 4;
    }

    /*
     * NAME:	split()
     * DESCRIPTION:	divide a node into two and insert a record
     */
    private static int split(Node left, byte[] record, int[] reclen)
    {
        BTree bt = left.bt;
        Node right = new Node();
        Node side = null;
        int mark, i;

        /* create a second node by cloning the first */

        copyNode(right, left);

        if (n_new(right) == -1)
            return -1;

        left.nd.ndFLink  = right.nnum;
        right.nd.ndBLink = left.nnum;

        /* divide all records evenly between the two nodes */

        mark = (nodeUsed(left) + 2 * left.nd.ndNRecs + reclen[0] + 2) >> 1;

        if (left.rnum == -1)
        {
            side  = left;
            mark -= reclen[0] + 2;
        }

        for (i = 0; i < left.nd.ndNRecs; ++i)
        {
            Node np;
            int rec;

            np  = (mark > 0) ? right : left;
            rec = np.roff[i];

            mark -= (np.roff[i + 1] - np.roff[i]) + 2;

            np.data[rec] = 0;  /* HFS_SETKEYLEN(rec, 0) */

            if (left.rnum == i)
            {
                side  = (mark > 0) ? left : right;
                mark -= reclen[0] + 2;
            }
        }

        compact(left);
        compact(right);

        /* insert the new record and store the modified nodes */

        assert side != null;

        n_search(side, record);
        n_insertx(side, record, reclen[0]);

        if (HfsBTree.bt_putnode(left) == -1 ||
            HfsBTree.bt_putnode(right) == -1)
            return -1;

        /* create an index record in the parent for the new node */

        n_index(right, record, reclen);

        /* update link pointers */

        if (bt.hdr.bthLNode == left.nnum)
        {
            bt.hdr.bthLNode = right.nnum;
            bt.flags |= HFS_BT_UPDATE_HDR;
        }

        if (right.nd.ndFLink > 0)
        {
            Node sib = new Node();

            if (HfsBTree.bt_getnode(sib, right.bt, right.nd.ndFLink) == -1)
                return -1;

            sib.nd.ndBLink = right.nnum;

            if (HfsBTree.bt_putnode(sib) == -1)
                return -1;
        }

        return 0;
    }

    /*
     * NAME:	node->insertx()
     * DESCRIPTION:	insert a record into a node (which must already have room)
     */
    public static void n_insertx(Node np, byte[] record, int reclen)
    {
        int rnum, i;

        rnum = np.rnum + 1;

        /* push other records down to make room */

        int dstEnd   = np.roff[np.nd.ndNRecs] + reclen;
        int srcStart = np.roff[rnum] + reclen;

        for (int p = dstEnd - 1; p >= srcStart; --p)
            np.data[p] = np.data[p - reclen];

        ++np.nd.ndNRecs;

        for (i = np.nd.ndNRecs; i > rnum; --i)
            np.roff[i] = np.roff[i - 1] + reclen;

        /* write the new record */

        System.arraycopy(record, 0, np.data, np.roff[rnum], reclen);
    }

    /*
     * NAME:	node->insert()
     * DESCRIPTION:	insert a new record into a node; return a record for parent
     */
    public static int n_insert(Node np, byte[] record, int[] reclen)
    {
        /* check for free space */

        if (np.nd.ndNRecs >= HFS_MAX_NRECS ||
            reclen[0] + 2 > nodeFree(np))
            return split(np, record, reclen);

        n_insertx(np, record, reclen[0]);
        reclen[0] = 0;

        return HfsBTree.bt_putnode(np);
    }

    /*
     * NAME:	join()
     * DESCRIPTION:	combine two nodes into a single node
     */
    private static int join(Node left, Node right, byte[] record, int[] flag)
    {
        int i, offset;

        /* copy records and offsets */

        System.arraycopy(right.data, right.roff[0],
                         left.data, left.roff[left.nd.ndNRecs],
                         nodeUsed(right));

        offset = left.roff[left.nd.ndNRecs] - right.roff[0];

        for (i = 1; i <= right.nd.ndNRecs; ++i)
            left.roff[++left.nd.ndNRecs] = offset + right.roff[i];

        if (HfsBTree.bt_putnode(left) == -1)
            return -1;

        /* eliminate node and update link pointers */

        if (n_free(right) == -1)
            return -1;

        record[0] = 0;  /* HFS_SETKEYLEN(record, 0) */
        flag[0] = 1;

        return 0;
    }

    /*
     * NAME:	node->delete()
     * DESCRIPTION:	remove a record from a node
     */
    public static int n_delete(Node np, byte[] record, int[] flag)
    {
        int rec;

        rec = np.roff[np.rnum];

        np.data[rec] = 0;  /* HFS_SETKEYLEN(rec, 0) */
        compact(np);

        if (np.nd.ndNRecs == 0)
        {
            if (n_free(np) == -1)
                return -1;

            record[0] = 0;  /* HFS_SETKEYLEN(record, 0) */
            flag[0] = 1;

            return 0;
        }

        /* see if we can join with our left sibling */

        if (np.nd.ndBLink > 0)
        {
            Node left = new Node();

            if (HfsBTree.bt_getnode(left, np.bt, np.nd.ndBLink) == -1)
                return -1;

            if (np.nd.ndNRecs + left.nd.ndNRecs <= HFS_MAX_NRECS &&
                nodeUsed(np) + 2 * np.nd.ndNRecs <= nodeFree(left))
                return join(left, np, record, flag);
        }

        if (np.rnum == 0)
        {
            /* special case: first record changed; update parent record key */

            n_index(np, record, null);
            flag[0] = 1;
        }

        return HfsBTree.bt_putnode(np);
    }
}
