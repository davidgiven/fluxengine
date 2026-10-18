package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_CREATEDIR;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_DELETE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_MOVE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_PUTFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;
import static com.google.common.collect.ImmutableList.toImmutableList;
import static com.google.common.collect.ImmutableMap.toImmutableMap;
import static com.google.common.collect.Streams.stream;
import static swingtree.UIFactoryMethods.button;
import static swingtree.UIFactoryMethods.html;
import static swingtree.UIFactoryMethods.label;
import static swingtree.UIFactoryMethods.panel;
import static swingtree.UIFactoryMethods.scrollPane;
import static swingtree.UIFactoryMethods.separator;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.gui.FilesystemTreeTableModel.DirNode;
import com.cowlark.fluxengine.gui.FilesystemTreeTableModel.FileNode;
import com.cowlark.fluxengine.gui.FilesystemTreeTableModel.FsNode;
import com.cowlark.fluxengine.vfs.FileAttributes;
import com.cowlark.fluxengine.vfs.Filesystem.Capability;
import com.cowlark.fluxengine.vfs.Filesystem.Dirent;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;
import org.apache.commons.io.FileUtils;
import org.jdesktop.swingx.JXTreeTable;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import sprouts.Tuple;
import sprouts.Val;
import sprouts.Var;
import sprouts.Viewable;
import swingtree.ComponentDelegate;
import swingtree.UI;
import javax.swing.DropMode;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTable;
import javax.swing.TransferHandler;
import javax.swing.SwingUtilities;
import javax.swing.event.TreeExpansionEvent;
import javax.swing.event.TreeExpansionListener;
import javax.swing.tree.TreePath;
import javax.swing.tree.TreeSelectionModel;
import java.awt.event.ActionEvent;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.UnsupportedFlavorException;
import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class FilesystemPanel extends JPanel
{
    private static final Logger logger = LoggerFactory.getLogger(FilesystemPanel.class);

    final ImageViewModel model;
    final JXTreeTable treeTable;
    final FilesystemTreeTableModel treeTableModel;
    final TreeSelectionModel treeSelectionModel;
    final Var<Tuple<TreePath>> filesSelected;
    final Set<VfsPath> everExpanded = new HashSet<>();

    JXTreeTable getTreeTable()
    {
        return treeTable;
    }

    public FilesystemPanel(ImageViewModel model)
    {
        this.model = model;
        this.treeTableModel = model.getFilesystemTreeTableModel();

        Val<Boolean> allowedWhenMounted = treeTableModel.getIsMounted().view();
        Val<Boolean> notMounted = allowedWhenMounted.viewAs(Boolean.class, m -> !m);
        Val<Boolean> allowedWhenNotMounted = Viewable.of(
                model.getBusy(),
                treeTableModel.getIsMounted(),
                (busy, mounted) -> !mounted && !busy);
        Val<Boolean> hasPendingChanges = treeTableModel.getHasPendingChanges();

        treeTable = new JXTreeTable(model.getFilesystemTreeTableModel());
        treeTableModel.setExpandedSupplier(this::snapshotExpandedVfsPaths);
        treeTable.addTreeExpansionListener(new TreeExpansionListener()
        {
            @Override
            public void treeExpanded(TreeExpansionEvent event)
            {
                Object last = event.getPath().getLastPathComponent();
                if (last instanceof DirNode dir)
                {
                    everExpanded.add(dir.vfsPath());
                    treeTableModel.requestPopulate(dir);
                } else if (last instanceof FsNode fsNode)
                {
                    /* Non-dir nodes are not expandable; no-op. */
                }
            }

            @Override
            public void treeCollapsed(TreeExpansionEvent event)
            {
                Object last = event.getPath().getLastPathComponent();
                if (last instanceof DirNode dir)
                {
                    /* Keep empty dirs that auto-collapsed when isLeaf became true (handle
                     * disappears). They are still considered expanded for resync purposes so a
                     * new child becomes visible. */
                    if (dir.getChildCount() != 0 || !treeTableModel.isLeaf(dir))
                        everExpanded.remove(dir.vfsPath());
                }
            }
        });
        treeTable.setRootVisible(true);
        treeTable.setShowGrid(true, true);
        treeTable.setLeafIcon(null);
        treeTable.setOpenIcon(null);
        treeTable.setClosedIcon(null);
        treeTable.putClientProperty("FlatLaf.style", "showHorizontalLines: true");

        /* Accept external files on directory rows without changing the current selection. */
        treeTable.setFillsViewportHeight(true);
        treeTable.setDropMode(DropMode.ON);
        treeTable.setTransferHandler(createFileDropHandler());

        filesSelected = Var.of(Tuple.of(TreePath.class));
        treeSelectionModel = treeTable.getTreeSelectionModel();
        treeSelectionModel.setSelectionMode(TreeSelectionModel.DISCONTIGUOUS_TREE_SELECTION);
        treeSelectionModel.addTreeSelectionListener(e -> {
            TreePath[] paths = treeSelectionModel.getSelectionPaths();
            filesSelected.set(Tuple.of(TreePath.class, paths));
        });

        Var<Integer> bytesTotal = model.getFilesystemTreeTableModel().getBytesTotal();
        Var<Integer> bytesUsed = model.getFilesystemTreeTableModel().getBytesUsed();

        Val<Boolean> canDoSingleFileOperation = Viewable.of(
                allowedWhenMounted,
                filesSelected,
                (mounted, files) -> isValidSelection(files) && (files.size() == 1) && mounted);
        Val<Boolean> canDoSingleFileNotDirOperation = Viewable.of(
                allowedWhenMounted,
                filesSelected,
                (mounted, files) -> isValidSelection(files) && (files.size() == 1) && mounted &&
                        !(Iterables
                                .getOnlyElement(files)
                                .getLastPathComponent() instanceof DirNode));
        Val<Boolean> canDoMultiFileOperation = Viewable.of(
                allowedWhenMounted,
                filesSelected,
                (mounted, files) -> isValidSelection(files) && (files.size() >= 1) && mounted);

        UI
                .of(this)
                .withLayout("fill, wrap 1, insets 5, hidemode 3")
                .add(
                        "growx", panel("insets 2")
                                .add(
                                        "align left",
                                        button("Get")
                                                .isEnabledIf(ifCapability(
                                                        canDoMultiFileOperation,
                                                        OP_GETFILE))
                                                .onClick(this::getFile))
                                .add(
                                        "align left",
                                        button("Put")
                                                .isEnabledIf(ifCapability(
                                                        canDoSingleFileOperation,
                                                        OP_PUTFILE))
                                                .onClick(this::putFiles))
                                .add(
                                        "align left",
                                        button("Rename")
                                                .isEnabledIf(ifCapability(
                                                        canDoSingleFileOperation,
                                                        OP_MOVE))
                                                .onClick(this::renameFile))
                                .add(
                                        "align left",
                                        button("Delete")
                                                .isEnabledIf(ifCapability(
                                                        canDoMultiFileOperation,
                                                        OP_DELETE))
                                                .onClick(this::deleteFiles))
                                .add(
                                        "align left", button("Create dir")
                                                .isEnabledIf(ifCapability(
                                                        canDoMultiFileOperation,
                                                        OP_CREATEDIR))
                                                .onClick(this::createDirectory))
                                .add(
                                        "align left", button("Info")
                                                .isEnabledIf(ifCapability(
                                                        canDoSingleFileOperation,
                                                        OP_GETDIRENT))
                                                .onClick(this::infoFile))
                                .add(
                                        "align left", button("View")
                                                .isEnabledIf(ifCapability(
                                                        canDoSingleFileNotDirOperation,
                                                        OP_GETFILE))
                                                .onClick(this::viewFile)))
                .add(
                        "grow, push",
                        scrollPane().add(UI.of(treeTable)).isVisibleIf(allowedWhenMounted))
                .add(
                        "grow, push, align " + "center, w 100%!", html("""
                                <html><center><b>Filesystem not mounted</b>
                                <br>Load some data and press 'Mount' to see files!</center></html>""")
                                .withHorizontalAlignment(UI.HorizontalAlignment.CENTER)
                                .isVisibleIf(notMounted))
                .add(
                        "growx", panel("insets 2")
                                .add(
                                        "align left",
                                        button("Mount")
                                                .isEnabledIf(allowedWhenNotMounted)
                                                .onClick(this::onMount))
                                .add(
                                        "align left", button("Discard")
                                                .isEnabledIf(and(
                                                        allowedWhenMounted,
                                                        hasPendingChanges))
                                                .onClick(delegate -> {
                                                    Set<VfsPath> snap = snapshotExpandedVfsPaths();
                                                    treeTableModel.discard();
                                                    treeTableModel.queueFilesystemOperation(fs -> SwingUtilities.invokeLater(
                                                            () -> {
                                                                for (VfsPath p : snap)
                                                                {
                                                                    DirNode n =
                                                                            treeTableModel.findNode(
                                                                                    p);
                                                                    if (n == null ||
                                                                            n.getChildCount() == 0)
                                                                        continue;
                                                                    TreePath tp =
                                                                            treeTableModel.buildTreePath(
                                                                                    n);
                                                                    if (!treeTable.isExpanded(tp))
                                                                        treeTable.expandPath(tp);
                                                                }
                                                            }));
                                                }))
                                .add(
                                        "align left", button("Commit")
                                                .isEnabledIf(and(
                                                        allowedWhenMounted,
                                                        hasPendingChanges))
                                                .onClick(delegate -> treeTableModel.commit()))
                                .add(
                                        "align left", button("Unmount")
                                                .isEnabledIf(and(
                                                        allowedWhenMounted,
                                                        not(hasPendingChanges)))
                                                .onClick(delegate -> treeTableModel.unmount()))
                                .add("push", separator())
                                .add(
                                        "align right", label(bytesUsed.viewAs(
                                                String.class,
                                                FileUtils::byteCountToDisplaySize)).isVisibleIf(
                                                allowedWhenMounted))
                                .add("align right", label("/").isVisibleIf(allowedWhenMounted))
                                .add(
                                        "align right", label(bytesTotal.viewAs(
                                                String.class,
                                                FileUtils::byteCountToDisplaySize)).isVisibleIf(
                                                allowedWhenMounted))
                                .add(
                                        "align right", UI
                                                .progressBar(
                                                        UI.Axis.HORIZONTAL, 0, 100, Viewable.of(
                                                                bytesUsed,
                                                                bytesTotal,
                                                                (used, total) -> (total == 0) ?
                                                                        0 :
                                                                        (100 * used / total)))
                                                .isVisibleIf(allowedWhenMounted)

                                ));
    }

    private Val<Boolean> ifCapability(Val<Boolean> base, Capability cap)
    {
        return Viewable.of(
                base,
                treeTableModel.getCapabilities(),
                (b, caps) -> b && caps.contains(cap));
    }

    private void getFile(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        Tuple<TreePath> paths = filesSelected.get();
        if (paths.size() == 1)
        {
            FileNode node = (FileNode) Iterables.getOnlyElement(paths).getLastPathComponent();
            if (node.getDirent().fileType() == IS_FILE)
            {
                VfsPath vfsPath = node.getDirent().path();
                String filename = node.getDirent().filename();
                UiUtils.promptAndSave(
                        this, "Save file", filename, saver -> {
                            treeTableModel.queueFilesystemOperation(fs -> {
                                Bytes bytes = fs.getFile(vfsPath);
                                saver.accept(bytes);
                            });
                        });
                return;
            }
        }

        /* Snapshot paths on EDT before crossing to FS thread. */
        ImmutableList<VfsPath> snapshot = stream(paths)
                .map(p -> ((FileNode) p.getLastPathComponent()).getDirent().path())
                .collect(toImmutableList());
        UiUtils.promptAndSave(
                this, "Save multiple files", "files.zip", saver -> {
                    treeTableModel.queueFilesystemOperation(fs -> {
                        Bytes bytes = fs.getFiles(fs, snapshot);
                        saver.accept(bytes);
                    });
                });
    }

    private void viewFile(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        TreePath path = Iterables.getOnlyElement(filesSelected.get());
        FileNode file = (FileNode) path.getLastPathComponent();
        VfsPath vfsPath = file.getDirent().path();
        treeTableModel.queueFilesystemOperation(fs -> {
            Bytes data = fs.getFile(vfsPath);
            SwingUtilities.invokeLater(() -> FileViewerDialogue.show(
                    this,
                    vfsPath.toString(),
                    data));
        });
    }

    DirNode getFileDropDirectory(int row)
    {
        /* Background drops go to root; file rows remain invalid destinations. */
        Object target;
        if (row < 0)
            target = treeTableModel.getRoot();
        else
        {
            TreePath path = treeTable.getPathForRow(row);
            target = path == null ? null : path.getLastPathComponent();
        }
        return target instanceof DirNode directory ? directory : null;
    }

    private TransferHandler createFileDropHandler()
    {
        return new TransferHandler()
        {
            @Override
            public boolean canImport(TransferSupport support)
            {
                /* Only copy external files into a mounted filesystem that supports writes. */
                if (!support.isDrop()
                        || !support.isDataFlavorSupported(DataFlavor.javaFileListFlavor)
                        || (support.getSourceDropActions() & COPY) == 0
                        || !treeTableModel.getIsMounted().get()
                        || !treeTableModel.getCapabilities().get().contains(OP_PUTFILE))
                    return false;

                /* Resolve the hovered directory, using root for empty table space. */
                JTable.DropLocation location = (JTable.DropLocation) support.getDropLocation();
                if (getFileDropDirectory(location.getRow()) == null)
                    return false;
                support.setDropAction(COPY);
                return true;
            }

            @Override
            public boolean importData(TransferSupport support)
            {
                if (!canImport(support))
                    return false;

                /* Snapshot the destination on the EDT before handing work to the filesystem. */
                JTable.DropLocation location = (JTable.DropLocation) support.getDropLocation();
                DirNode directory = getFileDropDirectory(location.getRow());
                VfsPath parentPath = directory.vfsPath();
                try
                {
                    /* Reject the entire drop if it contains directories or other non-files. */
                    List<?> transferred = (List<?>) support.getTransferable()
                            .getTransferData(DataFlavor.javaFileListFlavor);
                    ImmutableList.Builder<File> files = ImmutableList.builder();
                    for (Object item : transferred)
                    {
                        if (!(item instanceof File file) || !file.isFile())
                            return false;
                        files.add(file);
                    }
                    ImmutableList<File> snapshot = files.build();
                    if (snapshot.isEmpty())
                        return false;

                    /* Queue the same import operation used by the file chooser. */
                    putFiles(parentPath, snapshot);
                    return true;
                } catch (UnsupportedFlavorException | IOException e)
                {
                    logger.warn("Cannot import dropped files", e);
                    return false;
                }
            }
        };
    }

    private void putFiles(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        DirNode parent = getParentNodeOfSelection();
        VfsPath parentPath = parent.getDirent().path();

        JFileChooser chooser = new JFileChooser();
        chooser.setDialogTitle("Open files");
        chooser.setMultiSelectionEnabled(true);

        int result = chooser.showOpenDialog(this);
        if (result != JFileChooser.APPROVE_OPTION)
            return;

        putFiles(parentPath, ImmutableList.copyOf(chooser.getSelectedFiles()));
    }

    private void putFiles(VfsPath parentPath, ImmutableList<File> files)
    {
        /* Read host files and write their contents on the filesystem worker thread. */
        treeTableModel.queueFilesystemOperation(fs -> {
            for (File file : files)
            {
                Bytes data = Bytes.readFromFile(file.toPath());
                String leafName = file.getName();
                VfsPath vfsName = parentPath.resolve(leafName);
                fs.putFile(vfsName, data);
            }
            resyncAllExpandedAsync();
        });
    }

    private void deleteFiles(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        Tuple<TreePath> paths = filesSelected.get();
        ImmutableList<VfsPath> vfsPaths = stream(paths)
                .map(p -> ((FileNode) p.getLastPathComponent()).getDirent().path())
                .collect(toImmutableList());

        treeTableModel.queueFilesystemOperation(fs -> {
            for (VfsPath vfsPath : vfsPaths)
                fs.deleteFileRecursively(vfsPath);
            resyncAllExpandedAsync();
        });
    }

    private void infoFile(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        TreePath path = Iterables.getOnlyElement(filesSelected.get());
        VfsPath vfsPath = ((FileNode) path.getLastPathComponent()).getDirent().path();

        treeTableModel.queueFilesystemOperation(fs -> {
            Dirent de = fs.getDirent(vfsPath);
            SwingUtilities.invokeLater(() -> FileInfoDialogue.show(
                    this,
                    "File info: " + de.path().toString(),
                    de
                            .attributes()
                            .entrySet()
                            .stream()
                            .collect(toImmutableMap(
                                    e -> FileAttributes.getHumanName(e.getKey()),
                                    e -> e.getValue()))));
        });

    }

    private void createDirectory(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        DirNode parent = getParentNodeOfSelection();
        VfsPath parentPath = parent.getDirent().path();

        String childName = JOptionPane.showInputDialog(this, "Enter new directory name:");
        if (childName == null || childName.isBlank())
            return;
        VfsPath childVfsPath = parentPath.resolve(childName);
        treeTableModel.queueFilesystemOperation(fs -> {
            fs.createDirectory(childVfsPath);
            resyncAllExpandedAsync();
        });
    }

    private DirNode getParentNodeOfSelection()
    {
        TreePath parentPath = Iterables.getOnlyElement(filesSelected.get());
        DirNode parent = switch (parentPath.getLastPathComponent())
        {
            case DirNode dir -> dir;
            case FileNode file -> (DirNode) parentPath.getParentPath().getLastPathComponent();
            default -> throw new IllegalStateException(
                    "Unexpected value: " + parentPath.getLastPathComponent());
        };
        return parent;
    }

    Set<VfsPath> snapshotExpandedVfsPaths()
    {
        assert SwingUtilities.isEventDispatchThread();
        Object rootObj = treeTableModel.getRoot();
        if (!(rootObj instanceof DirNode rootDir))
            return Set.of();
        /* Walk the model tree. JXTreeTable.getExpandedDescendants skips leaf nodes, and empty
         * LOADED dirs become leaf (handle disappears) and are auto-collapsed by JTree
         * (expandedState cleared). We keep everExpanded for empty dirs that were expanded so a
         * new child inside an expanded empty dir is still resynced and re-expanded. */
        Set<VfsPath> result = new HashSet<>();
        java.util.ArrayDeque<DirNode> stack = new java.util.ArrayDeque<>();
        stack.push(rootDir);
        while (!stack.isEmpty())
        {
            DirNode cur = stack.pop();
            TreePath tp = treeTableModel.buildTreePath(cur);
            boolean expanded = treeTable.isExpanded(tp);
            if (!expanded && everExpanded.contains(cur.vfsPath()) && cur.isLeaf())
                expanded = true;

            /* Root is considered expanded even if JTree reports it collapsed after setRoot;
             * always include it so discard keeps free-space current. */
            if (expanded || cur == rootDir)
                result.add(cur.vfsPath());

            /* Only descend into expanded dirs — descendants of a collapsed dir cannot be
             * visible/expanded. Include children regardless of LOADED state so that NOT_LOADED
             * expanded placeholders are also captured. */
            if (expanded || cur == rootDir)
            {
                if (cur.getLoadState() == FilesystemTreeTableModel.LoadState.LOADED)
                {
                    for (int i = 0; i < cur.getChildCount(); i++)
                    {
                        if (cur.getChildAt(i) instanceof DirNode childDir)
                            stack.push(childDir);
                    }
                }
            }
        }
        return result;
    }

    void resyncAllExpandedAfterMutation()
    {
        assert SwingUtilities.isEventDispatchThread();
        Set<VfsPath> snap = snapshotExpandedVfsPaths();
        treeTableModel.resyncPaths(snap);
        treeTableModel.mutated();
        /* Re-expand snapshot paths that were auto-collapsed when empty (isLeaf became true) but
         * now have children after the resync. */
        treeTableModel.queueFilesystemOperation(fs -> SwingUtilities.invokeLater(() -> {
            for (VfsPath p : snap)
            {
                DirNode n = treeTableModel.findNode(p);
                if (n == null || n.getChildCount() == 0)
                    continue;
                TreePath tp = treeTableModel.buildTreePath(n);
                if (!treeTable.isExpanded(tp))
                    treeTable.expandPath(tp);
            }
        }));
    }

    void resyncAllExpandedAsync()
    {
        /* Called from FS thread after a mutation; schedule EDT resync that preserves expansion
         * and re-expands empty dirs that gained children. */
        SwingUtilities.invokeLater(this::resyncAllExpandedAfterMutation);
    }

    private void renameFile(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        TreePath path = Iterables.getOnlyElement(filesSelected.get());
        DirNode parent = (DirNode) path.getParentPath().getLastPathComponent();
        FileNode child = (FileNode) path.getLastPathComponent();

        String newChild = JOptionPane.showInputDialog(this, "Enter new leaf filename:");
        if (newChild == null || newChild.isBlank())
            return;
        VfsPath parentPath = parent.getDirent().path();
        VfsPath oldPath = child.getDirent().path();
        VfsPath newVfsPath = parentPath.resolve(newChild);
        treeTableModel.queueFilesystemOperation(fs -> {
            fs.moveFile(oldPath, newVfsPath);
            resyncAllExpandedAsync();
        });
    }

    /**
     * Returns true if the selection is valid — i.e. no selected path is an
     * ancestor (or duplicate) of another selected path. Selections spanning
     * multiple unrelated folders are fine.
     */
    private static boolean isValidSelection(Tuple<TreePath> paths)
    {
        for (int i = 0; i < paths.size(); i++)
        {
            for (int j = i + 1; j < paths.size(); j++)
            {
                if (paths.get(i).isDescendant(paths.get(j)) ||
                        paths.get(j).isDescendant(paths.get(i)))
                {
                    return false;
                }
            }
        }
        return true;
    }

    private void onMount(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        treeTableModel.mount();
        /* Root is lazily populated in the model, but the JXTreeTable clears expansion on
         * setRoot. Expand it after the EDT has processed the structure change so the initial
         * file list becomes visible without requiring a manual click. */
        SwingUtilities.invokeLater(() -> {
            Object root = treeTableModel.getRoot();
            if (root != null)
            {
                TreePath rootPath = new TreePath(root);
                if (!treeTable.isExpanded(rootPath))
                    treeTable.expandPath(rootPath);
            }
        });
    }

    private static Val<Boolean> and(Val<Boolean> v1, Val<Boolean> v2)
    {
        return Viewable.of(v1, v2, (b1, b2) -> b1 && b2);
    }

    private static Val<Boolean> or(Val<Boolean> v1, Val<Boolean> v2)
    {
        return Viewable.of(v1, v2, (b1, b2) -> b1 || b2);
    }

    private static Val<Boolean> not(Val<Boolean> v1)
    {
        return v1.viewAs(Boolean.class, b -> !b);
    }
}
