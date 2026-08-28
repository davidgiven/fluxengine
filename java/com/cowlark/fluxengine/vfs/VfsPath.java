package com.cowlark.fluxengine.vfs;

import com.google.common.collect.ImmutableList;

/**
 * A path within the VFS layer.
 *
 * <p>All VFS paths are absolute and use {@code /} as the directory separator,
 * regardless of the host platform's conventions.  The root directory is
 * {@code /}, represented by an empty segment list.
 */
public final class VfsPath
{
    private final ImmutableList<String> segments;

    private VfsPath(ImmutableList<String> segments)
    {
        this.segments = segments;
    }

    /**
     * Create a VFS path from one or more path components.  Each component is
     * split on {@code /}, ignoring empty components, so {@code VfsPath.of("/dir1",
     * "dir2")} and {@code VfsPath.of("/dir1/dir2")} are equivalent.  The first
     * component must be absolute (start with {@code /}).
     */
    public static VfsPath of(String first, String... more)
    {
        if (first == null || !first.startsWith("/"))
            throw new IllegalArgumentException("VFS path must be absolute: " + first);

        ImmutableList.Builder<String> builder = ImmutableList.builder();
        parse(first, builder);
        for (String part : more)
            parse(part, builder);
        return new VfsPath(builder.build());
    }

    private static void parse(String s, ImmutableList.Builder<String> builder)
    {
        for (String part : s.split("/"))
        {
            if (!part.isEmpty())
                builder.add(part);
        }
    }

    /** The root directory, {@code /}. */
    public static VfsPath root()
    {
        return new VfsPath(ImmutableList.of());
    }

    /** Whether this is the root directory. */
    public boolean isRoot()
    {
        return segments.isEmpty();
    }

    /** The path segments, e.g. {@code [dir1, dir2]} for {@code /dir1/dir2}. */
    public ImmutableList<String> segments()
    {
        return segments;
    }

    /** The parent directory, or {@code null} for the root. */
    public VfsPath getParent()
    {
        if (segments.isEmpty())
            return null;
        if (segments.size() == 1)
            return root();
        return new VfsPath(segments.subList(0, segments.size() - 1));
    }

    /** The final path component, or {@code null} for the root. */
    public String getName()
    {
        if (segments.isEmpty())
            return null;
        return segments.get(segments.size() - 1);
    }

    /** Resolve a relative path component against this path. */
    public VfsPath resolve(String child)
    {
        ImmutableList.Builder<String> builder = ImmutableList.builder();
        builder.addAll(segments);
        parse(child, builder);
        return new VfsPath(builder.build());
    }

    @Override
    public String toString()
    {
        return "/" + String.join("/", segments);
    }

    @Override
    public boolean equals(Object o)
    {
        if (this == o)
            return true;
        if (!(o instanceof VfsPath))
            return false;
        return segments.equals(((VfsPath) o).segments);
    }

    @Override
    public int hashCode()
    {
        return segments.hashCode();
    }
}