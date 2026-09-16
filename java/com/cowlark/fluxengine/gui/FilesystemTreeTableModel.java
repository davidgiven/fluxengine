package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_DIR;
import static com.google.common.collect.ImmutableList.toImmutableList;

import com.cowlark.fluxengine.core.EmergencyStopException;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.vfs.FileAttributes;
import com.cowlark.fluxengine.vfs.Filesystem.Capability;
import com.cowlark.fluxengine.vfs.Filesystem.Dirent;
import com.cowlark.fluxengine.vfs.FilesystemAttributes;
import com.cowlark.fluxengine.vfs.FilesystemOperation.FilesystemCaller;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import lombok.Getter;
import org.jdesktop.swingx.treetable.AbstractMutableTreeTableNode;
import org.jdesktop.swingx.treetable.DefaultTreeTableModel;
import sprouts.ValueSet;
import sprouts.Var;
import javax.swing.SwingUtilities;
import javax.swing.tree.TreeNode;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * {@code TreeTableModel} for a filesystem image.
 *
 * <p>Ownership split:
 * <ul>
 *   <li>Expansion state is owned solely by the view ({@code JXTreeTable}). This model never
 *       stores an {@code expanded} flag and never calls {@code expandPath}.</li>
 *   <li>Structure (which files exist, sort order, dirents) is owned by this model, but the
 *       authoritative data is on the filesystem thread. All {@code Filesystem} I/O runs on the
 *       filesystem thread via {@link #queueFilesystemOperation}; all model mutations run on the
 *       EDT.</li>
 *   <li>Each {@code DirNode} has a {@link LoadState} driving the placeholder. A directory is only
 *       listed when it is expanded in the view. {@link #resyncNode} re-lists a single already-
 *       loaded directory without touching expansion state (shallow).</li>
 * </ul>
 */
public class FilesystemTreeTableModel extends DefaultTreeTableModel
{
    private static final Dirent ROOT_DIRENT = Dirent
            .builder()
            .setFilename("/")
            .setPath(VfsPath.of("/"))
            .setFileType(IS_DIR)
            .setAttributes(ImmutableMap
                    .<String, String>builder()
                    .put(FileAttributes.FILENAME.name(), "/")
                    .put(FileAttributes.FILE_TYPE.name(), "dir")
                    .build())
            .build();

    private final ImagerViewModel model;
    private final BlockingQueue<FilesystemCaller> queue = new LinkedBlockingQueue<>();

    @Getter private final Var<Boolean> isMounted = Var.of(false);
    @Getter private final Var<Integer> bytesTotal = Var.of(0);
    @Getter private final Var<Integer> bytesUsed = Var.of(0);
    @Getter private final Var<ValueSet<Capability>> capabilities =
            Var.of(ValueSet.of(Capability.class));
    @Getter private final Var<Boolean> hasPendingChanges = Var.of(false);

    enum LoadState
    {
        NOT_LOADED,
        LOADING,
        LOADED
    }

    private class PlaceholderNode extends AbstractMutableTreeTableNode
    {
        @Override
        public Object getValueAt(int i)
        {
            return "Loading...";
        }

        @Override
        public int getColumnCount()
        {
            return 1;
        }
    }

    abstract class FsNode extends AbstractMutableTreeTableNode
    {
        @Getter private Dirent dirent;

        FsNode(Dirent dirent)
        {
            this.dirent = dirent;
        }

        void setDirent(Dirent dirent)
        {
            this.dirent = dirent;
        }

        VfsPath vfsPath()
        {
            return dirent.path();
        }
    }

    class FileNode extends FsNode
    {
        public FileNode(Dirent dirent)
        {
            super(dirent);
        }

        @Override
        public Object getValueAt(int i)
        {
            return switch (i)
            {
                case 0 -> getDirent().filename();
                case 1 -> Integer.toString(getDirent().length());
                default -> "";
            };
        }

        @Override
        public int getColumnCount()
        {
            return 2;
        }
    }

    class DirNode extends FileNode
    {
        @Getter private LoadState loadState = LoadState.NOT_LOADED;

        public DirNode(Dirent de)
        {
            super(de);
        }

        void setLoadState(LoadState loadState)
        {
            this.loadState = loadState;
        }

        @Override
        public int getColumnCount()
        {
            return 1;
        }

        @Override
        public boolean isLeaf()
        {
            // Let JXTreeTable show/hide the handle based on child count, not leaf flag alone.
            // Dirs that are not yet loaded still need a handle, so they are not leaves.
            // Once LOADED, emptiness is reflected by childCount==0.
            if (loadState != LoadState.LOADED)
                return false;
            return super.isLeaf();
        }
    }

    public FilesystemTreeTableModel(ImagerViewModel model)
    {
        this.model = model;
        // Initial empty root so JXTreeTable has something before first mount.
        DirNode root = new DirNode(ROOT_DIRENT);
        setRoot(root);
        insertNodeInto(new PlaceholderNode(), root, 0);
    }

    public void mount()
    {
        Image image = model.getDisk().mapTo(Image.class, disk -> disk.image).get();
        model.startFilesystemOperation(
                queue, image, () -> {
                    bytesTotal.set(0);
                    bytesUsed.set(0);
                    isMounted.set(false);
                });

        // Reset model on EDT before the filesystem thread is usable. The view's
        // JTree will clear its expandedPaths on setRoot; population is lazy.
        // mount() is always called on EDT (button handler), so do this synchronously.
        Runnable reset = () -> {
            DirNode root = new DirNode(ROOT_DIRENT);
            // NOT_LOADED + placeholder ensures the root shows an expansion handle
            // so the user can expand it to trigger the first list.
            setRoot(root);
            insertNodeInto(new PlaceholderNode(), root, 0);
            isMounted.set(true);
            mutated();
            // The root is visible expanded by default (JXTreeTable shows the
            // placeholder row). Populate it automatically so the initial view is
            // not stuck on "Loading..." — still lazy for all other dirs.
            ensureLoaded(root);
        };
        if (SwingUtilities.isEventDispatchThread())
            reset.run();
        else
            SwingUtilities.invokeLater(reset);
    }

    /* Must be called from filesystem thread */
    public void mutated()
    {
        updateFreeSpace();
        queue.add(fs -> {
            capabilities.set(ValueSet.of(Capability.class, fs.getCapabilities()));
            hasPendingChanges.set(fs.needsFlushing());
        });
    }

    public void queueFilesystemOperation(FilesystemCaller caller)
    {
        queue.add(caller);
    }

    private void updateFreeSpace()
    {
        queue.add(fs -> {
            ImmutableMap<String, String> attrs = fs.getFilesystemMetadata();
            try
            {
                int blockSize = Integer.parseInt(attrs.get(FilesystemAttributes.BLOCK_SIZE.name()));
                int totalBlocks =
                        Integer.parseInt(attrs.get(FilesystemAttributes.TOTAL_BLOCKS.name()));
                int usedBlocks =
                        Integer.parseInt(attrs.get(FilesystemAttributes.USED_BLOCKS.name()));
                bytesTotal.set(totalBlocks * blockSize);
                bytesUsed.set(usedBlocks * blockSize);
            } catch (NumberFormatException e)
            {
                bytesTotal.set(0);
                bytesUsed.set(0);
            }
        });
    }

    public void commit()
    {
        queue.add(fs -> fs.flushChanges());
    }

    public void discard()
    {
        queue.add(fs -> fs.discardChanges());
    }

    public void unmount()
    {
        queue.add(fs -> {
            throw new EmergencyStopException();
        });
    }

    @Override
    public int getColumnCount()
    {
        return 2;
    }

    @Override
    public String getColumnName(int column)
    {
        return column == 0 ? "Name" : "Size";
    }

    /**
     * Called from the view's {@code TreeExpansionListener} when a directory is expanded.
     * Safe to call redundantly; coalesces while LOADING and no-ops when LOADED.
     */
    public void requestPopulate(VfsPath path)
    {
        assert SwingUtilities.isEventDispatchThread();
        DirNode dir = findNode(path);
        if (dir == null)
            return;
        ensureLoaded(dir);
    }

    /**
     * Overload for the expansion listener that already has the node identity.
     */
    public void requestPopulate(DirNode dir)
    {
        assert SwingUtilities.isEventDispatchThread();
        // dir must still be attached to this model; if orphaned, findNode will be null
        // and we can safely no-op.
        if (dir.getParent() == null && dir != getRoot())
            return;
        ensureLoaded(dir);
    }

    /**
     * Re-lists the directory at {@code path} and diffs its immediate children.
     * Shallow: does not recurse into expanded descendants.
     * Does not touch expansion state. No-op if the directory is not LOADED.
     * If the directory has never been loaded, delegates to {@code ensureLoaded}.
     */
    public void resyncNode(VfsPath path)
    {
        assert SwingUtilities.isEventDispatchThread();
        DirNode dir = findNode(path);
        if (dir == null)
            return;
        if (dir.getLoadState() == LoadState.LOADING)
            return;
        if (dir.getLoadState() == LoadState.NOT_LOADED)
        {
            ensureLoaded(dir);
            return;
        }
        // LOADED -> re-list
        dir.setLoadState(LoadState.LOADING);
        // Show transient loading placeholder while the FS thread works.
        // Remove any stale placeholder first then insert fresh one.
        removePlaceholderIfPresent(dir);
        insertNodeInto(new PlaceholderNode(), dir, 0);

        VfsPath captured = path;
        queue.add(fs -> {
            try
            {
                ImmutableList<Dirent> entries =
                        fs.list(captured).values().stream().sorted().collect(toImmutableList());
                SwingUtilities.invokeLater(() -> applyListResult(dir, entries, null));
            } catch (Exception ex)
            {
                SwingUtilities.invokeLater(() -> applyListResult(dir, null, ex));
            }
        });
    }

    // -------------------------------------------------------------------------
    // Internal: loading
    // -------------------------------------------------------------------------

    private void ensureLoaded(DirNode dir)
    {
        assert SwingUtilities.isEventDispatchThread();
        if (dir.getLoadState() != LoadState.NOT_LOADED)
            return;

        dir.setLoadState(LoadState.LOADING);
        // Placeholder already present for NOT_LOADED dirs (inserted when the dir was
        // created or after a failed load). Ensure one is there.
        if (!hasPlaceholder(dir))
            insertNodeInto(new PlaceholderNode(), dir, 0);

        VfsPath path = dir.vfsPath();
        queue.add(fs -> {
            try
            {
                ImmutableList<Dirent> entries =
                        fs.list(path).values().stream().sorted().collect(toImmutableList());
                SwingUtilities.invokeLater(() -> applyListResult(dir, entries, null));
            } catch (Exception ex)
            {
                SwingUtilities.invokeLater(() -> applyListResult(dir, null, ex));
            }
        });
    }

    /**
     * Applies a listing result on the EDT.
     *
     * @param dir     the directory whose children are being replaced
     * @param entries sorted dirents, or {@code null} on failure
     * @param error   non-null on failure
     */
    private void applyListResult(DirNode dir, ImmutableList<Dirent> entries, Exception error)
    {
        assert SwingUtilities.isEventDispatchThread();

        // Abandon if this node is no longer attached (e.g. mount replaced the root
        // while the FS thread was listing).
        if (dir.getParent() == null && dir != getRoot())
            return;

        removePlaceholderIfPresent(dir);

        if (error != null)
        {
            // Per spec: reset to NOT_LOADED so next expand retries. No persistent
            // FAILED state or placeholder.
            dir.setLoadState(LoadState.NOT_LOADED);
            insertNodeInto(new PlaceholderNode(), dir, 0);
            // Still refresh free-space/capabilities in case the error was transient.
            return;
        }

        assert entries != null;

        // Diff current children against new entries.
        // Map existing non-placeholder children by path.
        Map<VfsPath, FsNode> existingByPath = new HashMap<>();
        List<FsNode> existingOrder = new ArrayList<>();
        for (int i = 0; i < dir.getChildCount(); i++)
        {
            TreeNode child = dir.getChildAt(i);
            if (child instanceof FsNode fsChild)
            {
                existingByPath.put(fsChild.vfsPath(), fsChild);
                existingOrder.add(fsChild);
            }
        }

        // Build new sorted list of nodes, reusing identities where possible.
        // We do a simple diff: remove stale, insert missing, reorder to sorted order,
        // and update dirents for survivors (so rename/size changes are reflected).
        // To keep expanded grandchildren alive, we must reuse the DirNode object
        // for surviving directories.
        Map<VfsPath, Dirent> newByPath = new HashMap<>();
        for (Dirent d : entries)
            newByPath.put(d.path(), d);

        // Remove stale children
        for (FsNode existing : List.copyOf(existingOrder))
        {
            if (!newByPath.containsKey(existing.vfsPath()))
                removeNodeFromParent(existing);
        }

        // Now insert/update in sorted order.
        // After removals, dir.getChildCount() reflects only survivors; we reinsert
        // in order by walking entries and ensuring each is at the expected index.
        for (int targetIndex = 0; targetIndex < entries.size(); targetIndex++)
        {
            Dirent wanted = entries.get(targetIndex);
            FsNode existing = existingByPath.get(wanted.path());

            if (existing != null && existing.getParent() == dir)
            {
                // Survivor: update dirent and move to correct index if needed.
                existing.setDirent(wanted);
                int currentIndex = dir.getIndex(existing);
                if (currentIndex != targetIndex)
                {
                    // DefaultTreeTableModel has no move; remove and reinsert.
                    removeNodeFromParent(existing);
                    insertNodeInto(existing, dir, targetIndex);
                }
            } else
            {
                // New child
                FsNode newNode = switch (wanted.fileType())
                {
                    case IS_DIR -> {
                        DirNode dn = new DirNode(wanted);
                        // New dirs are NOT_LOADED with a placeholder so they show a handle.
                        dn.setLoadState(LoadState.NOT_LOADED);
                        yield dn;
                    }
                    case IS_FILE -> new FileNode(wanted);
                };
                insertNodeInto(newNode, dir, targetIndex);
                if (newNode instanceof DirNode dn)
                    insertNodeInto(new PlaceholderNode(), dn, 0);
            }
        }

        dir.setLoadState(LoadState.LOADED);

        // If the dir is now empty, it has 0 children and the view will hide the
        // expansion handle automatically (DirNode.isLeaf() reflects LOADED+empty).
        // No empty-directory placeholder per spec.
    }

    private boolean hasPlaceholder(DirNode dir)
    {
        if (dir.getChildCount() == 0)
            return false;
        for (int i = 0; i < dir.getChildCount(); i++)
            if (dir.getChildAt(i) instanceof PlaceholderNode)
                return true;
        return false;
    }

    private void removePlaceholderIfPresent(DirNode dir)
    {
        for (int i = dir.getChildCount() - 1; i >= 0; i--)
        {
            TreeNode child = dir.getChildAt(i);
            if (child instanceof PlaceholderNode ph)
                removeNodeFromParent(ph);
        }
    }

    // -------------------------------------------------------------------------
    // Lookup helpers (no nodeMap)
    // -------------------------------------------------------------------------

    /**
     * Finds the {@code DirNode} for {@code path} by walking from the root via
     * {@code VfsPath.segments()}. Returns {@code null} if any ancestor is not
     * yet LOADED or the path does not exist.
     * Must be called on the EDT.
     */
    DirNode findNode(VfsPath path)
    {
        assert SwingUtilities.isEventDispatchThread();
        TreeNode root = (TreeNode) getRoot();
        if (root == null)
            return null;
        if (path.isRoot())
            return (DirNode) root;

        DirNode cur = (DirNode) root;
        for (String seg : path.segments())
        {
            if (cur.getLoadState() != LoadState.LOADED)
                return null;
            DirNode next = null;
            for (int i = 0; i < cur.getChildCount(); i++)
            {
                TreeNode child = cur.getChildAt(i);
                if (child instanceof DirNode dn && seg.equals(dn.getDirent().filename()))
                {
                    next = dn;
                    break;
                }
                // Also consider that a dir may have been listed but the child
                // lookup by filename is correct even if there are FileNodes interleaved.
                // So if not found among DirNodes, scan FileNodes too — but only DirNodes
                // can be returned as the walk continues.
            }
            // Fallback scan includes FileNodes for error detection, but for the walk
            // we only follow DirNodes; if seg matches a FileNode it's not a directory.
            if (next == null)
            {
                // Check if a file with that name exists (then path is not a dir)
                for (int i = 0; i < cur.getChildCount(); i++)
                {
                    TreeNode child = cur.getChildAt(i);
                    if (child instanceof FsNode fn && seg.equals(fn.getDirent().filename()))
                        return null;
                }
                return null;
            }
            cur = next;
        }
        return cur;
    }

    /**
     * Builds a {@code TreePath} for {@code node} by walking to the root.
     * Must be called on the EDT.
     */
    javax.swing.tree.TreePath buildTreePath(FsNode node)
    {
        List<TreeNode> chain = new ArrayList<>();
        TreeNode cur = node;
        while (cur != null)
        {
            chain.add(0, cur);
            cur = cur.getParent();
        }
        return new javax.swing.tree.TreePath(chain.toArray());
    }

    // Legacy API kept for incremental migration; delegates to resync.
    // Not used by new panel code but left non-public to avoid external callers.
    void legacyAddNode(DirNode dir, VfsPath child)
    {
        resyncNode(dir.vfsPath());
    }
}
