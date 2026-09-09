package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_DIR;

import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.vfs.Attributes;
import com.cowlark.fluxengine.vfs.Filesystem.Dirent;
import com.cowlark.fluxengine.vfs.FilesystemOperation.FilesystemCaller;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.FluentIterable;
import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.Iterators;
import lombok.Getter;
import org.jdesktop.swingx.treetable.AbstractMutableTreeTableNode;
import org.jdesktop.swingx.treetable.DefaultTreeTableModel;
import sprouts.Var;
import javax.swing.SwingUtilities;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

public class FilesystemTreeTableModel extends DefaultTreeTableModel
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
        model.startFilesystemOperation(queue, image);

        DirNode root = new DirNode(ROOT_DIRENT);
        setRoot(root);
        populate(root);
    }

    public void commit()
    {
    }

    public void discard()
    {
    }

    public void unmount()
    {
        queue.add(null);
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

    private void populate(DirNode dir)
    {
        VfsPath path = dir.getDirent().path();
        queue.add(fs -> {
            ImmutableCollection<Dirent> files = fs.list(path).values();
            SwingUtilities.invokeLater(() -> {
                for (Dirent file : files)
                {
                    FileNode node = new FileNode(file);
                    insertNodeInto(node, dir, dir.getChildCount());
                }
            });
        });
    }
}
