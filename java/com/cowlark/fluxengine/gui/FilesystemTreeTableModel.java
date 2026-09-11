package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_DIR;
import static com.google.common.collect.ImmutableList.toImmutableList;

import com.cowlark.fluxengine.core.EmergencyStopException;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.vfs.Attributes;
import com.cowlark.fluxengine.vfs.Filesystem.Dirent;
import com.cowlark.fluxengine.vfs.FilesystemOperation.FilesystemCaller;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import lombok.Getter;
import lombok.Setter;
import org.jdesktop.swingx.treetable.AbstractMutableTreeTableNode;
import org.jdesktop.swingx.treetable.DefaultTreeTableModel;
import sprouts.Var;
import javax.swing.SwingUtilities;
import javax.swing.event.TreeExpansionEvent;
import javax.swing.event.TreeExpansionListener;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class FilesystemTreeTableModel extends DefaultTreeTableModel implements TreeExpansionListener
{
    private static final Dirent ROOT_DIRENT = Dirent
            .builder()
            .setFilename("/")
            .setPath(VfsPath.of("/"))
            .setFileType(IS_DIR)
            .setAttributes(ImmutableMap
                    .<String, String>builder()
                    .put(Attributes.FILENAME, "/")
                    .put(Attributes.FILE_TYPE, "dir")
                    .build())
            .build();

    private ImagerViewModel model;
    private BlockingQueue<FilesystemCaller> queue = new LinkedBlockingQueue<>();

    @Getter private final Var<Boolean> isMounted = Var.of(false);
    @Getter private final Var<Integer> bytesTotal = Var.of(0);
    @Getter private final Var<Integer> bytesUsed = Var.of(0);

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

    private class FileNode extends AbstractMutableTreeTableNode
    {
        @Getter private final Dirent dirent;

        public FileNode(Dirent dirent)
        {
            this.dirent = dirent;
        }

        @Override
        public Object getValueAt(int i)
        {
            return switch (i)
            {
                case 0 -> dirent.filename();
                case 1 -> Integer.toString(dirent.length());
                default -> "";
            };
        }

        @Override
        public int getColumnCount()
        {
            return 2;
        }
    }

    private class DirNode extends FileNode
    {
        @Getter @Setter private boolean expanded = false;

        public DirNode(Dirent de)
        {
            super(de);
        }

        @Override
        public int getColumnCount()
        {
            return 1;
        }
    }

    public FilesystemTreeTableModel(ImagerViewModel model)
    {
        this.model = model;
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

        DirNode root = new DirNode(ROOT_DIRENT);
        setRoot(root);
        insertNodeInto(new PlaceholderNode(), root, 0);

        updateFreeSpace();
        isMounted.set(true);
    }

    private void updateFreeSpace()
    {
        queue.add(fs -> {
            ImmutableMap<String, String> attrs = fs.getFilesystemMetadata();
            try
            {
                int blockSize = Integer.parseInt(attrs.get(Attributes.BLOCK_SIZE));
                int totalBlocks = Integer.parseInt(attrs.get(Attributes.TOTAL_BLOCKS));
                int usedBlocks = Integer.parseInt(attrs.get(Attributes.USED_BLOCKS));
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
    }

    public void discard()
    {
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

    @Override
    public void treeExpanded(TreeExpansionEvent event)
    {
        DirNode node = (DirNode) event.getPath().getLastPathComponent();
        if (!node.isExpanded())
            populate(node);
    }

    @Override
    public void treeCollapsed(TreeExpansionEvent event)
    {

    }

    private void populate(DirNode dir)
    {
        VfsPath path = dir.getDirent().path();
        queue.add(fs -> {
            ImmutableList<Dirent> files =
                    fs.list(path).values().stream().sorted().collect(toImmutableList());
            SwingUtilities.invokeLater(() -> {
                if (dir.isExpanded())
                    return;
                dir.setExpanded(true);

                for (Dirent file : files)
                {
                    FileNode node = new FileNode(file);
                    insertNodeInto(node, dir, dir.getChildCount());
                }
                removeNodeFromParent((PlaceholderNode) dir.getChildAt(0));
            });
        });
    }
}
