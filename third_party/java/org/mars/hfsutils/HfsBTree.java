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
 * $Id: btree.c,v 1.10 1998/11/02 22:08:54 rob Exp $
 */

package org.mars.hfsutils;

import static org.mars.hfsutils.HfsConstants.*;
import static org.mars.hfsutils.HfsException.*;

public final class HfsBTree
{
    private HfsBTree()
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
     * B*-tree bitmap helpers.  bt.map is a flat byte[].
     */

    private static boolean bmtst(byte[] bm, long num)
    {
        return (bm[(int) (num >> 3)] & (0x80 >> (num & 7))) != 0;
    }

    /*
     * HFS_NODEREC(np, rnum) — offset into np.data where record rnum starts.
     */
    public static int nodeRec(Node np, int rnum)
    {
        return np.roff[rnum];
    }

    /*
     * HFS_RECKEYLEN(ptr) — key length byte at the start of a record.
     */
    private static int recKeyLen(byte[] data, int offset)
    {
        return data[offset] & 0xff;
    }

    /*
     * HFS_RECKEYSKIP(ptr) — padded key length: (1 + keylen + 1) & ~1.
     */
    private static int recKeySkip(byte[] data, int offset)
    {
        return (1 + (data[offset] & 0xff) + 1) & ~1;
    }

    /*
     * HFS_RECDATA(ptr) — offset of data portion of a record (past the key).
     */
    private static int recData(byte[] data, int offset)
    {
        return offset + recKeySkip(data, offset);
    }

    /*
     * Copy an ExtDataRec (deep copy of the 3-element ExtDescriptor array).
     */
    private static ExtDataRec copyExtDataRec(ExtDataRec src)
    {
        ExtDataRec dst = new ExtDataRec();
        for (int i = 0; i < 3; ++i)
        {
            dst.data[i] = new ExtDescriptor();
            dst.data[i].xdrStABN   = src.data[i].xdrStABN;
            dst.data[i].xdrNumABlks = src.data[i].xdrNumABlks;
        }
        return dst;
    }

    /*
     * f_getblock — read a block from a file.
     * Java port of the C macro: f_doblock(file, num, bp, b_readab).
     * Delegates to HfsFileHandle methods which are equivalent.
     */
    private static int fGetBlock(HfsFileHandle file, long num, byte[] bp)
    {
        HfsVol vol = file.vol;
        int lpa = vol.lpa;
        int abnum = (int) (num / lpa);
        int blnum = (int) (num % lpa);
        int fabn = file.fabn;

        /* locate the appropriate extent record */

        if (abnum < fabn)
        {
            fabn = file.fabn = 0;
            file.ext = copyExtDataRec(file.cat.filRExtRec);
        }
        else
        {
            abnum -= fabn;
        }

        while (true)
        {
            for (int i = 0; i < 3; ++i)
            {
                int n = file.ext.data[i].xdrNumABlks;

                if (abnum < n)
                    return HfsBlock.b_readab(vol, file.ext.data[i].xdrStABN + abnum, blnum, bp);

                fabn += n;
                abnum -= n;
            }

            /* need to search overflow extents */
            break;
        }

        return fail(EIO, "f_getblock: extent search not yet ported");
    }

    /*
     * f_putblock — write a block to a file.
     * Java port of the C macro: f_doblock(file, num, bp, b_writeab).
     */
    private static int fPutBlock(HfsFileHandle file, long num, byte[] bp)
    {
        HfsVol vol = file.vol;
        int lpa = vol.lpa;
        int abnum = (int) (num / lpa);
        int blnum = (int) (num % lpa);
        int fabn = file.fabn;

        /* locate the appropriate extent record */

        if (abnum < fabn)
        {
            fabn = file.fabn = 0;
            file.ext = copyExtDataRec(file.cat.filRExtRec);
        }
        else
        {
            abnum -= fabn;
        }

        while (true)
        {
            for (int i = 0; i < 3; ++i)
            {
                int n = file.ext.data[i].xdrNumABlks;

                if (abnum < n)
                    return HfsBlock.b_writeab(vol, file.ext.data[i].xdrStABN + abnum, blnum, bp);

                fabn += n;
                abnum -= n;
            }

            /* need to search overflow extents */
            break;
        }

        return fail(EIO, "f_putblock: extent search not yet ported");
    }

    /*
     * f_alloc — allocate allocation blocks for a file.
     * Java port of file->alloc() from file.c lines 311-343.
     * Returns the number of allocation blocks allocated, or -1 on failure.
     */
    private static long fAlloc(HfsFileHandle file)
    {
        HfsVol vol = file.vol;
        long clumpsz;

        clumpsz = file.cat.filClpSize;
        if (clumpsz == 0)
        {
            if (file == vol.ext.f)
                clumpsz = vol.mdb.drXTClpSiz;
            else if (file == vol.cat.f)
                clumpsz = vol.mdb.drCTClpSiz;
            else
                clumpsz = vol.mdb.drClpSiz;
        }

        ExtDescriptor blocks = new ExtDescriptor();
        blocks.xdrNumABlks = (short) (clumpsz / vol.mdb.drAlBlkSiz);

        if (HfsVolume.v_allocblocks(vol, blocks) == -1)
            return -1;

        if (HfsFile.f_addextent(file, blocks) == -1)
        {
            HfsVolume.v_freeblocks(vol, blocks);
            return -1;
        }

        return blocks.xdrNumABlks;
    }

    /*
     * NAME:	btree->getnode()
     * DESCRIPTION:	retrieve a numbered node from a B*-tree file
     */
    public static int bt_getnode(Node np, BTree bt, long nnum)
    {
        int[] cursor = new int[1];
        int i;

        np.bt   = bt;
        np.nnum = nnum;

        /* verify the node exists and is marked as in-use */

        if (nnum > 0 && nnum >= bt.hdr.bthNNodes)
            return fail(EIO, "read nonexistent b*-tree node");
        else if (bt.map != null && ! bmtst(bt.map, nnum))
            return fail(EIO, "read unallocated b*-tree node");

        if (HfsFile.f_getblock(bt.f, nnum, np.data) == -1)
            return -1;

        /* unmarshal node descriptor from data[0..15] */

        cursor[0] = 0;

        np.nd.ndFLink   = HfsData.d_fetchul(np.data, cursor);
        np.nd.ndBLink   = HfsData.d_fetchul(np.data, cursor);
        np.nd.ndType    = HfsData.d_fetchsb(np.data, cursor);
        np.nd.ndNHeight = HfsData.d_fetchsb(np.data, cursor);
        np.nd.ndNRecs   = (short) HfsData.d_fetchuw(np.data, cursor);
        np.nd.ndResv2   = HfsData.d_fetchsw(np.data, cursor);

        if (np.nd.ndNRecs > HFS_MAX_NRECS)
            return fail(EIO, "too many b*-tree node records");

        /* unmarshal record offsets from end of block */

        i = np.nd.ndNRecs + 1;

        cursor[0] = HFS_BLOCKSZ - (2 * i);

        while (i-- > 0)
            np.roff[i] = HfsData.d_fetchuw(np.data, cursor);

        return 0;
    }

    /*
     * NAME:	btree->putnode()
     * DESCRIPTION:	store a numbered node into a B*-tree file
     */
    public static int bt_putnode(Node np)
    {
        BTree bt = np.bt;
        int[] cursor = new int[1];
        int i;

        /* verify the node exists and is marked as in-use */

        if (np.nnum > 0 && np.nnum >= bt.hdr.bthNNodes)
            return fail(EIO, "write nonexistent b*-tree node");
        else if (bt.map != null && ! bmtst(bt.map, np.nnum))
            return fail(EIO, "write unallocated b*-tree node");

        /* marshal node descriptor to data[0..15] */

        cursor[0] = 0;

        HfsData.d_storeul(np.data, cursor, np.nd.ndFLink);
        HfsData.d_storeul(np.data, cursor, np.nd.ndBLink);
        HfsData.d_storesb(np.data, cursor, np.nd.ndType);
        HfsData.d_storesb(np.data, cursor, np.nd.ndNHeight);
        HfsData.d_storeuw(np.data, cursor, np.nd.ndNRecs);
        HfsData.d_storesw(np.data, cursor, np.nd.ndResv2);

        if (np.nd.ndNRecs > HFS_MAX_NRECS)
            return fail(EIO, "too many b*-tree node records");

        /* marshal record offsets to end of block */

        i = np.nd.ndNRecs + 1;

        cursor[0] = HFS_BLOCKSZ - (2 * i);

        while (i-- > 0)
            HfsData.d_storeuw(np.data, cursor, np.roff[i]);

        return HfsFile.f_putblock(bt.f, np.nnum, np.data);
    }

    /*
     * NAME:	btree->readhdr()
     * DESCRIPTION:	read the header node of a B*-tree
     */
    public static int bt_readhdr(BTree bt)
    {
        byte[] map = null;
        int i;
        long nnum;

        if (bt_getnode(bt.hdrnd, bt, 0) == -1)
            return -1;

        if (bt.hdrnd.nd.ndType != ndHdrNode ||
            bt.hdrnd.nd.ndNRecs != 3 ||
            bt.hdrnd.roff[0] != 0x00e ||
            bt.hdrnd.roff[1] != 0x078 ||
            bt.hdrnd.roff[2] != 0x0f8 ||
            bt.hdrnd.roff[3] != 0x1f8)
            return fail(EIO, "malformed b*-tree header node");

        /* read header record */

        int rec0 = nodeRec(bt.hdrnd, 0);
        int[] cursor = new int[]{rec0};

        bt.hdr.bthDepth    = (short) HfsData.d_fetchuw(bt.hdrnd.data, cursor);
        bt.hdr.bthRoot     = HfsData.d_fetchul(bt.hdrnd.data, cursor);
        bt.hdr.bthNRecs    = HfsData.d_fetchul(bt.hdrnd.data, cursor);
        bt.hdr.bthFNode    = HfsData.d_fetchul(bt.hdrnd.data, cursor);
        bt.hdr.bthLNode    = HfsData.d_fetchul(bt.hdrnd.data, cursor);
        bt.hdr.bthNodeSize = (short) HfsData.d_fetchuw(bt.hdrnd.data, cursor);
        bt.hdr.bthKeyLen   = (short) HfsData.d_fetchuw(bt.hdrnd.data, cursor);
        bt.hdr.bthNNodes   = HfsData.d_fetchul(bt.hdrnd.data, cursor);
        bt.hdr.bthFree     = HfsData.d_fetchul(bt.hdrnd.data, cursor);

        for (i = 0; i < 76; ++i)
            bt.hdr.bthResv[i] = HfsData.d_fetchsb(bt.hdrnd.data, cursor);

        if (bt.hdr.bthNodeSize != HFS_BLOCKSZ)
            return fail(EINVAL, "unsupported b*-tree node size");

        /* read map record; construct btree bitmap */
        /* don't set bt.map until we're done, since getnode() checks it */

        map = new byte[HFS_MAP1SZ];
        System.arraycopy(bt.hdrnd.data, nodeRec(bt.hdrnd, 2), map, 0, HFS_MAP1SZ);
        bt.mapsz = HFS_MAP1SZ;

        /* read continuation map records, if any */

        nnum = bt.hdrnd.nd.ndFLink;

        while (nnum != 0)
        {
            Node n = new Node();

            if (bt_getnode(n, bt, nnum) == -1)
                return -1;

            if (n.nd.ndType != ndMapNode ||
                n.nd.ndNRecs != 1 ||
                n.roff[0] != 0x00e ||
                n.roff[1] != 0x1fa)
                return fail(EIO, "malformed b*-tree map node");

            byte[] newmap = new byte[(int) (bt.mapsz + HFS_MAPXSZ)];
            System.arraycopy(map, 0, newmap, 0, (int) bt.mapsz);
            map = newmap;

            System.arraycopy(n.data, nodeRec(n, 0), map, (int) bt.mapsz, HFS_MAPXSZ);
            bt.mapsz += HFS_MAPXSZ;

            nnum = n.nd.ndFLink;
        }

        bt.map = map;

        return 0;
    }

    /*
     * NAME:	btree->writehdr()
     * DESCRIPTION:	write the header node of a B*-tree
     */
    public static int bt_writehdr(BTree bt)
    {
        long mapsz, nnum;
        int i;

        /* ASSERT: bt.hdrnd.bt == bt && bt.hdrnd.nnum == 0 && ... */

        /* marshal BTHdrRec into node record 0 */

        int rec0 = nodeRec(bt.hdrnd, 0);
        int[] cursor = new int[]{rec0};

        HfsData.d_storeuw(bt.hdrnd.data, cursor, bt.hdr.bthDepth);
        HfsData.d_storeul(bt.hdrnd.data, cursor, bt.hdr.bthRoot);
        HfsData.d_storeul(bt.hdrnd.data, cursor, bt.hdr.bthNRecs);
        HfsData.d_storeul(bt.hdrnd.data, cursor, bt.hdr.bthFNode);
        HfsData.d_storeul(bt.hdrnd.data, cursor, bt.hdr.bthLNode);
        HfsData.d_storeuw(bt.hdrnd.data, cursor, bt.hdr.bthNodeSize);
        HfsData.d_storeuw(bt.hdrnd.data, cursor, bt.hdr.bthKeyLen);
        HfsData.d_storeul(bt.hdrnd.data, cursor, bt.hdr.bthNNodes);
        HfsData.d_storeul(bt.hdrnd.data, cursor, bt.hdr.bthFree);

        for (i = 0; i < 76; ++i)
            HfsData.d_storesb(bt.hdrnd.data, cursor, bt.hdr.bthResv[i]);

        /* write map (256 bytes) to record 2 */

        System.arraycopy(bt.map, 0, bt.hdrnd.data, nodeRec(bt.hdrnd, 2), HFS_MAP1SZ);

        if (bt_putnode(bt.hdrnd) == -1)
            return -1;

        mapsz = bt.mapsz - HFS_MAP1SZ;

        nnum = bt.hdrnd.nd.ndFLink;

        while (mapsz > 0)
        {
            Node n = new Node();

            if (nnum == 0)
                return fail(EIO, "truncated b*-tree map");

            if (bt_getnode(n, bt, nnum) == -1)
                return -1;

            if (n.nd.ndType != ndMapNode ||
                n.nd.ndNRecs != 1 ||
                n.roff[0] != 0x00e ||
                n.roff[1] != 0x1fa)
                return fail(EIO, "malformed b*-tree map node");

            System.arraycopy(bt.map, (int) (bt.mapsz - mapsz),
                             n.data, nodeRec(n, 0), HFS_MAPXSZ);

            if (bt_putnode(n) == -1)
                return -1;

            nnum = n.nd.ndFLink;
            mapsz -= HFS_MAPXSZ;
        }

        bt.flags &= ~HFS_BT_UPDATE_HDR;

        return 0;
    }

    /*
     * High-Level B*-Tree Routines ============================================= */

    /*
     * NAME:	btree->space()
     * DESCRIPTION:	assert space for new records, or extend the file
     */
    public static int bt_space(BTree bt, int nrecs)
    {
        long nnodes;

        nnodes = (long) nrecs * (bt.hdr.bthDepth + 1);

        if (nnodes <= bt.hdr.bthFree)
            return 0;

        /* make sure the extents tree has room too */

        if (bt != bt.f.vol.ext)
        {
            if (bt_space(bt.f.vol.ext, 1) == -1)
                return -1;
        }

        long space = fAlloc(bt.f);
        if (space == -1)
            return -1;

        nnodes = space * (bt.f.vol.mdb.drAlBlkSiz / bt.hdr.bthNodeSize);

        bt.hdr.bthNNodes += nnodes;
        bt.hdr.bthFree   += nnodes;

        bt.flags |= HFS_BT_UPDATE_HDR;

        bt.f.vol.flags |= HFS_VOL_UPDATE_ALTMDB;

        while (bt.hdr.bthNNodes > bt.mapsz * 8)
        {
            byte[] newmap;
            Node mapnd = new Node();

            /* extend tree map */

            newmap = new byte[(int) (bt.mapsz + HFS_MAPXSZ)];
            if (bt.map != null)
                System.arraycopy(bt.map, 0, newmap, 0, (int) bt.mapsz);

            for (int j = (int) bt.mapsz; j < (int) (bt.mapsz + HFS_MAPXSZ); ++j)
                newmap[j] = 0;

            bt.map    = newmap;
            bt.mapsz += HFS_MAPXSZ;

            HfsNode.n_init(mapnd, bt, ndMapNode, 0);
            if (HfsNode.n_new(mapnd) == -1)
                return -1;

            mapnd.nd.ndNRecs = 1;
            mapnd.roff[1]    = 0x1fa;

            /* link the new map node */

            if (bt.hdrnd.nd.ndFLink == 0)
            {
                bt.hdrnd.nd.ndFLink = mapnd.nnum;
                mapnd.nd.ndBLink    = 0;
            }
            else
            {
                Node n = new Node();
                long linknum;

                linknum = bt.hdrnd.nd.ndFLink;

                while (true)
                {
                    if (bt_getnode(n, bt, linknum) == -1)
                        return -1;

                    if (n.nd.ndFLink == 0)
                        break;

                    linknum = n.nd.ndFLink;
                }

                n.nd.ndFLink      = mapnd.nnum;
                mapnd.nd.ndBLink  = n.nnum;

                if (bt_putnode(n) == -1)
                    return -1;
            }

            if (bt_putnode(mapnd) == -1)
                return -1;
        }

        return 0;
    }

    /*
     * NAME:	insertx()
     * DESCRIPTION:	recursively locate a node and insert a record
     */
    private static int insertx(Node np, byte[] record, int[] reclen)
    {
        Node child = new Node();
        int result = 0;

        if (HfsNode.n_search(np, record) != 0)
            return fail(EIO, "b*-tree record already exists");

        switch (np.nd.ndType)
        {
        case ndIndxNode:
        {
            int rec;

            if (np.rnum == -1)
                rec = nodeRec(np, 0);
            else
                rec = nodeRec(np, np.rnum);

            long childNum = HfsData.d_getul(np.data, recData(np.data, rec));

            if (bt_getnode(child, np.bt, childNum) == -1 ||
                insertx(child, record, reclen) == -1)
                return -1;

            if (np.rnum == -1)
            {
                HfsNode.n_index(child, record, null);
                if (reclen[0] == 0)
                {
                    result = bt_putnode(np);
                    return result;
                }
            }

            if (reclen[0] != 0)
                result = HfsNode.n_insert(np, record, reclen);

            break;
        }

        case ndLeafNode:
            result = HfsNode.n_insert(np, record, reclen);
            break;

        default:
            return fail(EIO, "unexpected b*-tree node");
        }

        return result;
    }

    /*
     * NAME:	btree->insert()
     * DESCRIPTION:	insert a new node record into a tree
     */
    public static int bt_insert(BTree bt, byte[] record, int reclen)
    {
        Node root = new Node();

        if (bt.hdr.bthRoot == 0)
        {
            /* create root node */

            HfsNode.n_init(root, bt, ndLeafNode, 1);
            if (HfsNode.n_new(root) == -1 ||
                bt_putnode(root) == -1)
                return -1;

            bt.hdr.bthDepth = 1;
            bt.hdr.bthRoot  = root.nnum;
            bt.hdr.bthFNode = root.nnum;
            bt.hdr.bthLNode = root.nnum;

            bt.flags |= HFS_BT_UPDATE_HDR;
        }
        else if (bt_getnode(root, bt, bt.hdr.bthRoot) == -1)
            return -1;

        byte[] newrec = new byte[HFS_MAX_RECLEN];
        System.arraycopy(record, 0, newrec, 0, reclen);

        int[] reclenArr = new int[]{reclen};

        if (insertx(root, newrec, reclenArr) == -1)
            return -1;

        if (reclenArr[0] != 0)
        {
            byte[] oroot = new byte[HFS_MAX_RECLEN];
            int orootlen;

            /* root node was split; create a new root */

            HfsNode.n_index(root, oroot, null);
            orootlen = reclenArr[0];

            HfsNode.n_init(root, bt, ndIndxNode, root.nd.ndNHeight + 1);
            if (HfsNode.n_new(root) == -1)
                return -1;

            ++bt.hdr.bthDepth;
            bt.hdr.bthRoot = root.nnum;

            bt.flags |= HFS_BT_UPDATE_HDR;

            /* insert index records for new root */

            HfsNode.n_search(root, oroot);
            HfsNode.n_insertx(root, oroot, orootlen);

            HfsNode.n_search(root, newrec);
            HfsNode.n_insertx(root, newrec, reclenArr[0]);

            if (bt_putnode(root) == -1)
                return -1;
        }

        ++bt.hdr.bthNRecs;
        bt.flags |= HFS_BT_UPDATE_HDR;

        return 0;
    }

    /*
     * NAME:	deletex()
     * DESCRIPTION:	recursively locate a node and delete a record
     */
    private static int deletex(Node np, byte[] key, byte[] record, int[] flag)
    {
        Node child = new Node();
        int found;
        int result = 0;

        found = HfsNode.n_search(np, key);

        switch (np.nd.ndType)
        {
        case ndIndxNode:
        {
            int rec;

            if (np.rnum == -1)
                return fail(EIO, "b*-tree record not found");

            rec = nodeRec(np, np.rnum);

            long childNum = HfsData.d_getul(np.data, recData(np.data, rec));

            if (bt_getnode(child, np.bt, childNum) == -1)
                return -1;

            /* pass a copy of the index record data for the child to write into */
            byte[] recCopy = new byte[HFS_MAX_RECLEN];
            int recLen = np.roff[np.rnum + 1] - np.roff[np.rnum];
            System.arraycopy(np.data, rec, recCopy, 0, recLen);

            if (deletex(child, key, recCopy, flag) == -1)
                return -1;

            if (flag[0] != 0)
            {
                flag[0] = 0;

                if (recKeyLen(np.data, rec) == 0)
                {
                    result = HfsNode.n_delete(np, record, flag);
                    break;
                }

                if (np.rnum == 0)
                {
                    /* propagate index record change into parent */

                    HfsNode.n_index(np, record, null);
                    flag[0] = 1;
                }

                result = bt_putnode(np);
            }

            break;
        }

        case ndLeafNode:
            if (found == 0)
                return fail(EIO, "b*-tree record not found");

            result = HfsNode.n_delete(np, record, flag);
            break;

        default:
            return fail(EIO, "unexpected b*-tree node");
        }

        return result;
    }

    /*
     * NAME:	btree->delete()
     * DESCRIPTION:	remove a node record from a tree
     */
    public static int bt_delete(BTree bt, byte[] key)
    {
        Node root = new Node();
        byte[] record = new byte[HFS_MAX_RECLEN];
        int[] flag = new int[]{0};

        if (bt.hdr.bthRoot == 0)
            return fail(EIO, "empty b*-tree");

        if (bt_getnode(root, bt, bt.hdr.bthRoot) == -1 ||
            deletex(root, key, record, flag) == -1)
            return -1;

        if (bt.hdr.bthDepth > 1 && root.nd.ndNRecs == 1)
        {
            /* root only has one record; eliminate it and decrease the tree depth */

            int rec0 = nodeRec(root, 0);

            --bt.hdr.bthDepth;
            bt.hdr.bthRoot = HfsData.d_getul(root.data, recData(root.data, rec0));

            if (HfsNode.n_free(root) == -1)
                return -1;
        }
        else if (bt.hdr.bthDepth == 1 && root.nd.ndNRecs == 0)
        {
            /* root node was deleted */

            bt.hdr.bthDepth = 0;
            bt.hdr.bthRoot  = 0;
        }

        --bt.hdr.bthNRecs;
        bt.flags |= HFS_BT_UPDATE_HDR;

        return 0;
    }

    /*
     * NAME:	btree->search()
     * DESCRIPTION:	locate a data record given a search key
     */
    public static int bt_search(BTree bt, byte[] key, Node np)
    {
        int found = 0;
        long nnum;

        nnum = bt.hdr.bthRoot;

        if (nnum == 0)
        {
            Hfs.hfsErrno = ENOENT;
            Hfs.hfsError = null;
            return 0;
        }

        while (true)
        {
            if (bt_getnode(np, bt, nnum) == -1)
                return -1;

            found = HfsNode.n_search(np, key);

            switch (np.nd.ndType)
            {
            case ndIndxNode:
            {
                int rec;

                if (np.rnum == -1)
                {
                    Hfs.hfsErrno = ENOENT;
                    Hfs.hfsError = null;
                    return 0;
                }

                rec  = nodeRec(np, np.rnum);
                nnum = HfsData.d_getul(np.data, recData(np.data, rec));

                break;
            }

            case ndLeafNode:
                if (found == 0)
                {
                    Hfs.hfsErrno = ENOENT;
                    Hfs.hfsError = null;
                    return 0;
                }

                return found;

            default:
                return fail(EIO, "unexpected b*-tree node");
            }
        }
    }

    public static int nodeRecLen(Node np, int rnum)
    {
        return np.roff[rnum + 1] - np.roff[rnum];
    }
}
