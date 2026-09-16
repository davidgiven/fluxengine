package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.when;

import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.gui.FilesystemTreeTableModel.DirNode;
import com.cowlark.fluxengine.gui.FilesystemTreeTableModel.LoadState;
import com.cowlark.fluxengine.vfs.FilesystemOperation.FilesystemCaller;
import com.cowlark.fluxengine.vfs.VfsPath;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;
import sprouts.Var;
import javax.swing.SwingUtilities;
import javax.swing.tree.TreePath;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

@RunWith(MockitoJUnitRunner.class)
public class FilesystemPanelHeadlessTest
{
    @Mock ImageViewModel mockModel;

    private FakeFilesystem fakeFs;
    private FilesystemTreeTableModel model;
    private FilesystemPanel panel;
    private Disk disk;
    private BlockingQueue<FilesystemCaller> capturedQueue;

    @BeforeClass
    public static void headless()
    {
        System.setProperty("java.awt.headless", "true");
    }

    @Before
    public void setUp() throws Exception
    {
        fakeFs = new FakeFilesystem();

        /* The root already contains "/"; add an empty directory "/empty". */
        fakeFs.addEmptyDir(VfsPath.of("/empty"));

        disk = new Disk();
        disk.image = new Image();

        /* Mockito stubs must be configured before creating the model. */
        when(mockModel.getDisk()).thenReturn(Var.of(disk));
        when(mockModel.getBusy()).thenReturn(Var.of(false));

        model = new FilesystemTreeTableModel(mockModel);
        when(mockModel.getFilesystemTreeTableModel()).thenReturn(model);

        /* Capture the queue used by mount and start a fake filesystem thread. */
        doAnswer(inv -> {
            capturedQueue = inv.getArgument(0);

            /* The image argument is ignored for the fake filesystem. */
            Thread t = new Thread(() -> {
                while (true)
                {
                    try
                    {
                        FilesystemCaller c = capturedQueue.take();
                        c.accept(fakeFs);
                    } catch (InterruptedException e)
                    {
                        Thread.currentThread().interrupt();
                        break;
                    } catch (Exception e)
                    {
                        /* Swallow the exception as ImagerViewModel does (it shows a UI message). */
                        e.printStackTrace();
                    }
                }
            });
            t.setDaemon(true);
            t.start();
            return null;
        }).when(mockModel).startFilesystemOperation(any(), any(), any());

        SwingUtilities.invokeAndWait(() -> {
            panel = new FilesystemPanel(mockModel);
        });

        /* Mount on the EDT. */
        SwingUtilities.invokeAndWait(() -> model.mount());
        awaitIdle();

        /* After mounting, the root should transition from LOADING to LOADED; wait until it is
         * loaded. */
        awaitLoaded(VfsPath.of("/"));

        /* The root is normally auto-expanded via panel.onMount, which is not called here because
         * we invoked model.mount directly; expand the root explicitly for the test. */
        SwingUtilities.invokeAndWait(() -> {
            TreePath rootPath = new TreePath(model.getRoot());
            if (!panel.getTreeTable().isExpanded(rootPath))
                panel.getTreeTable().expandPath(rootPath);
        });
        awaitIdle();
        awaitLoaded(VfsPath.of("/"));
    }

    private void awaitIdle() throws Exception
    {
        /* Pump the EDT. */
        SwingUtilities.invokeAndWait(() -> {
        });

        /* Wait until the queue is drained and no nodes are in the LOADING state. */
        long deadline = System.currentTimeMillis() + 3000;
        while (System.currentTimeMillis() < deadline)
        {
            SwingUtilities.invokeAndWait(() -> {
            });
            boolean loading = false;

            /* Check on the EDT whether any DirNode is LOADING. */
            AtomicBoolean hasLoading = new AtomicBoolean(false);
            SwingUtilities.invokeAndWait(() -> {
                hasLoading.set(hasLoadingNode((DirNode) model.getRoot()));
            });
            loading = hasLoading.get();
            boolean queueEmpty = capturedQueue == null || capturedQueue.isEmpty();

            /* The EDT queue must also be empty; invokeAndWait already drains it. */
            if (!loading && queueEmpty)
            {
                /* Give the filesystem thread a moment to enqueue the next EDT task. */
                Thread.sleep(50);

                /* Re-check. */
                SwingUtilities.invokeAndWait(() -> hasLoading.set(hasLoadingNode((DirNode) model.getRoot())));
                if (!hasLoading.get() && (capturedQueue == null || capturedQueue.isEmpty()))
                    return;
            }
            Thread.sleep(20);
        }
        throw new AssertionError("timeout waiting for idle");
    }

    private boolean hasLoadingNode(DirNode dir)
    {
        if (dir.getLoadState() == LoadState.LOADING)
            return true;
        for (int i = 0; i < dir.getChildCount(); i++)
        {
            if (dir.getChildAt(i) instanceof DirNode child)
                if (hasLoadingNode(child))
                    return true;
        }
        return false;
    }

    private void awaitLoaded(VfsPath path) throws Exception
    {
        long deadline = System.currentTimeMillis() + 3000;
        while (System.currentTimeMillis() < deadline)
        {
            awaitIdle();
            AtomicReference<DirNode> ref = new AtomicReference<>();
            SwingUtilities.invokeAndWait(() -> ref.set(model.findNode(path)));
            DirNode n = ref.get();
            if (n != null && n.getLoadState() == LoadState.LOADED)
                return;
            Thread.sleep(20);
        }
        throw new AssertionError("timeout waiting for LOADED " + path);
    }

    @Test
    public void rootAutoPopulatedAfterMount_headless() throws Exception
    {
        awaitLoaded(VfsPath.of("/"));
        SwingUtilities.invokeAndWait(() -> {
            DirNode root = model.findNode(VfsPath.of("/"));
            assertThat(root).isNotNull();
            assertThat(root.getLoadState()).isEqualTo(LoadState.LOADED);

            /* The root should have one child, "empty". */
            assertThat(root.getChildCount()).isEqualTo(1);
            assertThat(((FilesystemTreeTableModel.FsNode) root.getChildAt(0)).vfsPath()).isEqualTo(
                    VfsPath.of("/empty"));

            /* The placeholder should be gone. */
            assertThat(model.hasPlaceholder(root)).isFalse();
        });
    }

    @Test
    public void expandedEmptyGainsChildIsVisible_placeholderNotCollapsedBeforePopulation()
            throws Exception
    {
        /* Expand "/empty" (which is empty). */
        SwingUtilities.invokeAndWait(() -> {
            DirNode empty = model.findNode(VfsPath.of("/empty"));
            assertThat(empty).isNotNull();
            TreePath tp = model.buildTreePath(empty);
            panel.getTreeTable().expandPath(tp);

            /* The TreeExpansionListener will call requestPopulate, which delegates to
             * ensureLoaded. */
        });
        awaitLoaded(VfsPath.of("/empty"));
        SwingUtilities.invokeAndWait(() -> {
            DirNode empty = model.findNode(VfsPath.of("/empty"));
            assertThat(empty.getChildCount()).isEqualTo(0);
            assertThat(empty.getLoadState()).isEqualTo(LoadState.LOADED);
            assertThat(model.isLeaf(empty)).isTrue();

            /* "empty" is a leaf per the specification; JTree auto-collapses the leaf but with
             * everExpanded tracking it should still be considered expanded, so isExpanded should
             * be false after auto-collapse while everExpanded retains it. */
            assertThat(panel.everExpanded.contains(VfsPath.of("/empty"))).isTrue();
        });

        /* Create a directory inside the expanded "empty" directory via the fake filesystem and
         * resync. */
        fakeFs.createDirectory(VfsPath.of("/empty/newDir"));
        SwingUtilities.invokeAndWait(() -> panel.resyncAllExpandedAfterMutation());
        awaitIdle();
        awaitLoaded(VfsPath.of("/empty"));

        SwingUtilities.invokeAndWait(() -> {
            DirNode empty = model.findNode(VfsPath.of("/empty"));
            assertThat(empty).isNotNull();

            /* The placeholder should have been kept until after the diff so the child count
             * never transiently drops to zero; the node should not collapse before population
             * and the new child must be present. */
            assertThat(empty.getChildCount()).isEqualTo(1);
            FilesystemTreeTableModel.FsNode child =
                    (FilesystemTreeTableModel.FsNode) empty.getChildAt(0);
            assertThat(child.vfsPath()).isEqualTo(VfsPath.of("/empty/newDir"));

            /* After gaining a child, "empty" is no longer a leaf; the handle should reappear and
             * the node should be re-expanded. */
            assertThat(empty.isLeaf()).isFalse();
            TreePath tp = model.buildTreePath(empty);
            assertThat(panel.getTreeTable().isExpanded(tp)).isTrue();

            /* The placeholder for the new directory should exist. */
            DirNode newDir = model.findNode(VfsPath.of("/empty/newDir"));
            assertThat(newDir).isNotNull();
            assertThat(model.hasPlaceholder(newDir)).isTrue();
        });
    }

    @Test
    public void discardPreservesExpansion_headless() throws Exception
    {
        /* Expand "/empty". */
        SwingUtilities.invokeAndWait(() -> {
            DirNode empty = model.findNode(VfsPath.of("/empty"));
            panel.getTreeTable().expandPath(model.buildTreePath(empty));
        });
        awaitLoaded(VfsPath.of("/empty"));

        /* Create a new directory and then discard it. */
        fakeFs.createDirectory(VfsPath.of("/empty/a"));
        SwingUtilities.invokeAndWait(() -> panel.resyncAllExpandedAfterMutation());
        awaitLoaded(VfsPath.of("/empty"));

        /* Also expand "/empty/a". */
        SwingUtilities.invokeAndWait(() -> {
            DirNode a = model.findNode(VfsPath.of("/empty/a"));
            assertThat(a).isNotNull();
            panel.getTreeTable().expandPath(model.buildTreePath(a));
        });
        awaitLoaded(VfsPath.of("/empty/a"));

        /* Take a snapshot before discarding. */
        Set<VfsPath> before = new HashSet<>();
        SwingUtilities.invokeAndWait(() -> before.addAll(panel.snapshotExpandedVfsPaths()));
        assertThat(before).contains(VfsPath.of("/empty"));
        assertThat(before).contains(VfsPath.of("/empty/a"));

        /* Simulate a dirty change by creating another directory to be discarded;
         * FakeFilesystem's discard is a no-op, so adding a directory and then discarding should
         * still resync the same snapshot; call discard via the model (which uses the
         * expandedSupplier snapshot). */
        SwingUtilities.invokeAndWait(() -> model.discard());
        awaitIdle();

        /* After discarding, expanded paths should still be resynced and remain expanded where
         * possible. */
        SwingUtilities.invokeAndWait(() -> {

            /* The root, "/empty", and "/empty/a" should still be LOADED. */
            assertThat(model
                    .findNode(VfsPath.of("/empty"))
                    .getLoadState()).isEqualTo(LoadState.LOADED);
            assertThat(panel
                    .getTreeTable()
                    .isExpanded(model.buildTreePath(model.findNode(VfsPath.of("/empty"))))).isTrue();
        });
    }

    @Test
    public void placeholderKeptUntilAfterDiff_noTransientZeroCollapse() throws Exception
    {
        /* Expand "/empty". */
        SwingUtilities.invokeAndWait(() -> {
            panel
                    .getTreeTable()
                    .expandPath(model.buildTreePath(model.findNode(VfsPath.of("/empty"))));
        });
        awaitLoaded(VfsPath.of("/empty"));

        /* Resync "empty" (not the LOADING placeholder case; this tests the ensureLoaded path):
         * for a NOT_LOADED directory, ensureLoaded inserts a placeholder, fs.list returns empty,
         * and applyListResult should keep the placeholder until after the diff; create a new
         * empty subdirectory "/empty/subEmpty", expand it, then resync it while empty. */
        fakeFs.createDirectory(VfsPath.of("/empty/subEmpty"));
        SwingUtilities.invokeAndWait(() -> panel.resyncAllExpandedAfterMutation());
        awaitLoaded(VfsPath.of("/empty"));
        SwingUtilities.invokeAndWait(() -> {
            DirNode sub = model.findNode(VfsPath.of("/empty/subEmpty"));
            assertThat(sub).isNotNull();
            panel.getTreeTable().expandPath(model.buildTreePath(sub));
        });

        awaitLoaded(VfsPath.of("/empty/subEmpty"));
        SwingUtilities.invokeAndWait(() -> {
            DirNode sub = model.findNode(VfsPath.of("/empty/subEmpty"));
            assertThat(sub.getChildCount()).isEqualTo(0);
        });

        /* Resync "subEmpty" (empty); it should not transiently collapse before population (which
         * remains empty). */
        SwingUtilities.invokeAndWait(() -> model.resyncNode(VfsPath.of("/empty/subEmpty")));
        awaitIdle();
        SwingUtilities.invokeAndWait(() -> {
            DirNode sub = model.findNode(VfsPath.of("/empty/subEmpty"));
            assertThat(sub.getLoadState()).isEqualTo(LoadState.LOADED);

            /* It should remain in everExpanded even though the leaf was auto-collapsed, so the
             * snapshot still contains it. */
            assertThat(panel.everExpanded.contains(VfsPath.of("/empty/subEmpty"))).isTrue();
        });
    }
}
