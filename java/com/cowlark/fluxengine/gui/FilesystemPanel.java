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
import com.cowlark.fluxengine.vfs.FileAttributes;
import com.cowlark.fluxengine.vfs.Filesystem;
import com.cowlark.fluxengine.vfs.Filesystem.Capability;
import com.cowlark.fluxengine.vfs.Filesystem.Dirent;
import com.cowlark.fluxengine.vfs.VfsPath;
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
import javax.swing.JButton;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.tree.TreePath;
import javax.swing.tree.TreeSelectionModel;
import java.awt.event.ActionEvent;
import java.io.IOException;

public class FilesystemPanel extends JPanel
{
    private static final Logger logger = LoggerFactory.getLogger(FilesystemPanel.class);

    private final ImagerViewModel model;
    private final JXTreeTable treeTable;
    private final FilesystemTreeTableModel treeTableModel;
    private final TreeSelectionModel treeSelectionModel;
    private final Var<Tuple<TreePath>> filesSelected;

    public FilesystemPanel(ImagerViewModel model)
    {
        this.model = model;
        this.treeTableModel = model.getFilesystemTreeTableModel();

        Val<Boolean> allowedWhenMounted = treeTableModel.getIsMounted().view();
        Val<Boolean> notMounted = allowedWhenMounted.viewAs(Boolean.class, m -> !m);
        Val<Boolean> allowedWhenNotMounted = Viewable.of(
                model.getBusy(),
                treeTableModel.getIsMounted(),
                (busy, mounted) -> !mounted && !busy);

        treeTable = new JXTreeTable(model.getFilesystemTreeTableModel());
        treeTable.addTreeExpansionListener(model.getFilesystemTreeTableModel());
        treeTable.setRootVisible(true);
        treeTable.setShowGrid(true, true);
        treeTable.setLeafIcon(null);
        treeTable.setOpenIcon(null);
        treeTable.setClosedIcon(null);
        treeTable.putClientProperty("FlatLaf.style", "showHorizontalLines: true");

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
                                        button("Put").isEnabledIf(ifCapability(
                                                canDoSingleFileOperation,
                                                OP_PUTFILE)))
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
                                        "align left",
                                        button("View").isEnabledIf(ifCapability(
                                                canDoSingleFileOperation,
                                                OP_GETFILE))))
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
                                                .onClick(delegate -> treeTableModel.mount()))
                                .add(
                                        "align left",
                                        button("Discard")
                                                .isEnabledIf(allowedWhenMounted)
                                                .onClick(delegate -> treeTableModel.discard()))
                                .add(
                                        "align left",
                                        button("Commit")
                                                .isEnabledIf(allowedWhenMounted)
                                                .onClick(delegate -> treeTableModel.commit()))
                                .add(
                                        "align left",
                                        button("Unmount")
                                                .isEnabledIf(allowedWhenMounted)
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
                UiUtils.promptAndSave(
                        this, "Save file", node.getDirent().filename(), saver -> {
                            treeTableModel.queueFilesystemOperation(fs -> {
                                Bytes bytes = fs.getFile(node.getDirent().path());
                                saver.accept(bytes);
                            });
                        });
                return;
            }
        }

        UiUtils.promptAndSave(
                this, "Save multiple files", "files.zip", saver -> {
                    treeTableModel.queueFilesystemOperation(fs -> {
                        saver.accept(recursivelyAddPathsToZipfile(fs, paths));
                    });
                });
    }

    private void deleteFiles(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        Tuple<TreePath> paths = filesSelected.get();

        treeTableModel.queueFilesystemOperation(fs -> {
            try
            {
                for (TreePath path : paths)
                {
                    FileNode file = (FileNode) path.getLastPathComponent();
                    fs.deleteFileRecursively(file.getDirent().path());
                    treeTableModel.removeNodeFromParent(file);
                }
            } finally
            {
                treeTableModel.mutated();
            }
        });
    }

    private void infoFile(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        TreePath path = Iterables.getOnlyElement(filesSelected.get());

        treeTableModel.queueFilesystemOperation(fs -> {
            FileNode file = (FileNode) path.getLastPathComponent();
            Dirent de = file.getDirent();
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
        TreePath parentPath = Iterables.getOnlyElement(filesSelected.get());
        DirNode parent = switch (parentPath.getLastPathComponent())
        {
            case DirNode dir -> dir;
            case FileNode file -> (DirNode) parentPath.getParentPath().getLastPathComponent();
            default -> throw new IllegalStateException(
                    "Unexpected value: " + parentPath.getLastPathComponent());
        };

        String childName = JOptionPane.showInputDialog(this, "Enter new directory name:");
        VfsPath childVfsPath = parent.getDirent().path().resolve(childName);
        treeTableModel.queueFilesystemOperation(fs -> {
            fs.createDirectory(childVfsPath);
            treeTableModel.addNode(parent, childVfsPath);
        });
    }

    private void renameFile(
            ComponentDelegate<JButton, ActionEvent> delegate)
    {
        TreePath path = Iterables.getOnlyElement(filesSelected.get());
        DirNode parent = (DirNode) path.getParentPath().getLastPathComponent();
        FileNode child = (FileNode) path.getLastPathComponent();

        String newChild = JOptionPane.showInputDialog(this, "Enter new leaf filename:");
        VfsPath newVfsPath = parent.getDirent().path().resolve(newChild);
        treeTableModel.queueFilesystemOperation(fs -> {
            fs.moveFile(child.getDirent().path(), newVfsPath);
            SwingUtilities.invokeLater(() -> {
                treeTableModel.addNode(parent, newVfsPath);
                treeTableModel.removeNodeFromParent(child);
            });
        });
    }

    private static Bytes recursivelyAddPathsToZipfile(Filesystem fs, Iterable<TreePath> paths)
            throws IOException
    {
        return fs.getFiles(
                fs,
                stream(paths)
                        .map(path -> ((FileNode) path.getLastPathComponent()).getDirent().path())
                        .collect(toImmutableList()));
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

}
