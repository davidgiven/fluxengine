package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_PUTFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableList;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import javax.swing.JComponent;
import javax.swing.JTable;
import javax.swing.SwingUtilities;
import javax.swing.TransferHandler;
import javax.swing.tree.TreePath;
import java.awt.SecondaryLoop;
import java.awt.Toolkit;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.Transferable;
import java.awt.datatransfer.UnsupportedFlavorException;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionException;

class FilesystemPanelTransferHandler extends TransferHandler
{
    private static final Logger logger = LoggerFactory.getLogger(FilesystemPanelTransferHandler.class);

    private final FilesystemPanel filesystemPanel;

    public FilesystemPanelTransferHandler(FilesystemPanel filesystemPanel)
    {
        this.filesystemPanel = filesystemPanel;
    }

    private FilesystemTreeTableModel.DirNode getFileDropDirectory(int row)
    {
        /* Background drops go to root; file rows remain invalid destinations. */
        Object target;
        if (row < 0)
            target = filesystemPanel.treeTableModel.getRoot();
        else
        {
            TreePath path = filesystemPanel.treeTable.getPathForRow(row);
            target = path == null ? null : path.getLastPathComponent();
        }
        return target instanceof FilesystemTreeTableModel.DirNode directory ? directory : null;
    }

    private ImmutableList<VfsPath> getFileExportPaths()
    {
        /* Only selections consisting entirely of ordinary files can be dragged out. */
        if (!filesystemPanel.canDoMultiFileOperation.get() ||
                !filesystemPanel.treeTableModel.getCapabilities().get().contains(OP_GETFILE))
            return ImmutableList.of();
        TreePath[] selection =
                filesystemPanel.treeTable.getTreeSelectionModel().getSelectionPaths();
        if (selection == null)
            return ImmutableList.of();

        ImmutableList.Builder<VfsPath> paths = ImmutableList.builder();
        for (TreePath path : selection)
        {
            if (!(path.getLastPathComponent() instanceof FilesystemTreeTableModel.FileNode file) ||
                    file instanceof FilesystemTreeTableModel.DirNode ||
                    file.getDirent().fileType() != IS_FILE)
                return ImmutableList.of();
            paths.add(file.getDirent().path());
        }
        return paths.build();
    }

    private class ExportedFiles implements Transferable
    {
        private final ImmutableList<VfsPath> paths;
        private CompletableFuture<ImmutableList<File>> extracted;

        ExportedFiles(ImmutableList<VfsPath> paths)
        {
            this.paths = paths;
        }

        private synchronized CompletableFuture<ImmutableList<File>> extract()
        {
            if (extracted != null)
                return extracted;
            extracted = new CompletableFuture<>();
            filesystemPanel.treeTableModel.queueFilesystemOperation(fs -> {
                try
                {
                    /* Keep the files until application exit because native drop targets may
                     * copy them later. */
                    Path directory = Files.createTempDirectory("fluxengine-export-");
                    directory.toFile().deleteOnExit();
                    ImmutableList.Builder<File> files = ImmutableList.builder();
                    for (VfsPath path : paths)
                    {
                        String name = path.getName();
                        Path output = directory.resolve(name).normalize();
                        if (!directory.equals(output.getParent()))
                            throw new IOException("Invalid export filename: " + name);
                        output.toFile().deleteOnExit();
                        Bytes bytes = fs.getFile(path);
                        bytes.writeToFile(output);
                        files.add(output.toFile());
                    }
                    extracted.complete(files.build());
                } catch (Exception e)
                {
                    extracted.completeExceptionally(e);
                }
            });
            return extracted;
        }

        @Override
        public DataFlavor[] getTransferDataFlavors()
        {
            return new DataFlavor[]{DataFlavor.javaFileListFlavor};
        }

        @Override
        public boolean isDataFlavorSupported(DataFlavor flavor)
        {
            return DataFlavor.javaFileListFlavor.equals(flavor);
        }

        @Override
        public Object getTransferData(DataFlavor flavor)
                throws UnsupportedFlavorException, IOException
        {
            if (!isDataFlavorSupported(flavor))
                throw new UnsupportedFlavorException(flavor);

            /* Start reading only when the destination requests the dropped files. Cache
             * the result because native targets may request the data more than once. */
            CompletableFuture<ImmutableList<File>> result = extract();
            if (SwingUtilities.isEventDispatchThread() && !result.isDone())
            {
                /* Keep Swing responsive while a drop or clipboard consumer waits on
                 * the EDT. */
                SecondaryLoop loop =
                        Toolkit.getDefaultToolkit().getSystemEventQueue().createSecondaryLoop();
                result.whenComplete((files, error) -> SwingUtilities.invokeLater(() -> loop.exit()));
                loop.enter();
            }
            try
            {
                return result.join();
            } catch (CompletionException e)
            {
                logger.warn("Cannot extract dragged files", e.getCause());
                throw new IOException("Cannot extract files", e.getCause());
            }
        }
    }

    @Override
    public int getSourceActions(JComponent component)
    {
        return getFileExportPaths().isEmpty() ? NONE : COPY;
    }

    @Override
    protected Transferable createTransferable(JComponent component)
    {
        /* Snapshot the selection before reading from the filesystem worker. */
        ImmutableList<VfsPath> paths = getFileExportPaths();
        return paths.isEmpty() ? null : new ExportedFiles(paths);
    }

    @Override
    public boolean canImport(TransferSupport support)
    {
        /* Only copy external files into a mounted filesystem that supports writes. */
        if (!support.isDrop() || !support.isDataFlavorSupported(DataFlavor.javaFileListFlavor) ||
                (support.getSourceDropActions() & COPY) == 0 ||
                !filesystemPanel.treeTableModel.getIsMounted().get() ||
                !filesystemPanel.treeTableModel.getCapabilities().get().contains(OP_PUTFILE))
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
        FilesystemTreeTableModel.DirNode directory = getFileDropDirectory(location.getRow());
        VfsPath parentPath = directory.vfsPath();
        try
        {
            /* Reject the entire drop if it contains directories or other non-files. */
            List<?> transferred = (List<?>) support
                    .getTransferable()
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
            filesystemPanel.putFiles(parentPath, snapshot);
            return true;
        } catch (UnsupportedFlavorException | IOException e)
        {
            logger.warn("Cannot import dropped files", e);
            return false;
        }
    }
}
