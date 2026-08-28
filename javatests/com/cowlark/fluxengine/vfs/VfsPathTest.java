package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.common.collect.ImmutableList;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class VfsPathTest
{
    @Test
    public void of_root()
    {
        VfsPath root = VfsPath.of("/");
        assertThat(root.toString()).isEqualTo("/");
        assertThat(root.segments()).isEmpty();
        assertThat(root.isRoot()).isTrue();
        assertThat(root.getParent()).isNull();
        assertThat(root.getName()).isNull();
    }

    @Test
    public void of_single()
    {
        VfsPath p = VfsPath.of("/data");
        assertThat(p.toString()).isEqualTo("/data");
        assertThat(p.segments()).isEqualTo(ImmutableList.of("data"));
        assertThat(p.isRoot()).isFalse();
        assertThat(p.getParent()).isEqualTo(VfsPath.of("/"));
        assertThat(p.getName()).isEqualTo("data");
    }

    @Test
    public void of_nested()
    {
        assertThat(VfsPath.of("/dir1/dir2").toString()).isEqualTo("/dir1/dir2");
        assertThat(VfsPath.of("/dir1", "dir2").toString()).isEqualTo("/dir1/dir2");
        assertThat(VfsPath.of("/dir1", "dir2/dir3").toString()).isEqualTo("/dir1/dir2/dir3");
        assertThat(VfsPath.of("/dir1", "dir2")).isEqualTo(VfsPath.of("/dir1/dir2"));
    }

    @Test
    public void of_ignoresEmptyAndDuplicateSlashes()
    {
        assertThat(VfsPath.of("//dir1///dir2/").toString()).isEqualTo("/dir1/dir2");
        assertThat(VfsPath.of("/", "dir1", "", "dir2").toString()).isEqualTo("/dir1/dir2");
    }

    @Test
    public void of_requiresAbsolute()
    {
        assertThrows(IllegalArgumentException.class, () -> VfsPath.of("dir1"));
    }

    @Test
    public void resolve()
    {
        assertThat(VfsPath.of("/").resolve("data")).isEqualTo(VfsPath.of("/data"));
        assertThat(VfsPath.of("/dir1").resolve("dir2")).isEqualTo(VfsPath.of("/dir1/dir2"));
        assertThat(VfsPath.of("/dir1").resolve("dir2/dir3")).isEqualTo(VfsPath.of("/dir1/dir2/dir3"));
        assertThat(VfsPath.of("/dir1").resolve("")).isEqualTo(VfsPath.of("/dir1"));
    }

    @Test
    public void getParent_chain()
    {
        VfsPath p = VfsPath.of("/a/b/c");
        assertThat(p.getParent()).isEqualTo(VfsPath.of("/a/b"));
        assertThat(p.getParent().getParent()).isEqualTo(VfsPath.of("/a"));
        assertThat(p.getParent().getParent().getParent()).isEqualTo(VfsPath.of("/"));
        assertThat(p.getParent().getParent().getParent().getParent()).isNull();
    }

    @Test
    public void getName()
    {
        assertThat(VfsPath.of("/a/b").getName()).isEqualTo("b");
        assertThat(VfsPath.of("/").getName()).isNull();
    }

    @Test
    public void equals_hashCode()
    {
        VfsPath a = VfsPath.of("/a");
        VfsPath a2 = VfsPath.of("/a");
        VfsPath b = VfsPath.of("/b");

        assertThat(a).isEqualTo(a2);
        assertThat(a).isNotEqualTo(b);
        assertThat(a.hashCode()).isEqualTo(a2.hashCode());

        assertThat(ImmutableList.of(a)).contains(a2);
    }

    @Test
    public void toString_roundTrip()
    {
        VfsPath p = VfsPath.of("/dir1/dir2/file.txt");
        assertThat(VfsPath.of(p.toString())).isEqualTo(p);
    }

    @Test
    public void windowsSeparatorNeverUsed()
    {
        /* Regression for the Windows bug where java.nio.file.Path rendered
         * "/data" as "\data".  VFS paths must always use "/". */
        VfsPath p = VfsPath.of("/dir1/dir2/data");
        assertThat(p.toString()).doesNotContain("\\");
        for (String segment : p.segments())
        {
            assertThat(segment).doesNotContain("\\");
            assertThat(segment).doesNotContain("/");
        }
    }
}