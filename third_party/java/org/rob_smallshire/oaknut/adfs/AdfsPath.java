package org.rob_smallshire.oaknut.adfs;

import com.google.common.collect.ImmutableList;
import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornMeta;
import org.rob_smallshire.oaknut.file.HostBridge;
import org.rob_smallshire.oaknut.file.MetaFormat;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.Path;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Lightweight absolute Acorn path. Navigation remains available after its disc is closed. */
public final class AdfsPath
{
    private final Adfs adfs;
    private final ImmutableList<String> parts;

    AdfsPath(Adfs adfs, List<String> parts)
    {
        this.adfs = adfs;
        this.parts = ImmutableList.copyOf(parts);
    }

    public String name()
    {
        return parts.isEmpty() ? "$" : parts.getLast();
    }

    public ImmutableList<String> parts()
    {
        return ImmutableList.<String>builder()
                .add("$").addAll(parts).build();
    }

    public AdfsPath parent()
    {
        return parts.isEmpty() ? this : new AdfsPath(adfs, parts.subList(0, parts.size() - 1));
    }

    public AdfsPath resolve(String path)
    {
        ArrayList<String> result = new ArrayList<>(parts);
        for (String part : path.split("\\.", -1))
        {
            if (part.equals("$"))
                result.clear();
            else if (part.equals("^"))
            {
                if (!result.isEmpty())
                    result.removeLast();
            } else if (part.equals("@") || part.isEmpty())
                continue;
            else
                result.add(part);
        }
        return new AdfsPath(adfs, result);
    }

    /* Append literal names without parsing dotted paths or resolving special tokens. */
    public AdfsPath resolve(List<String> components)
    {
        return new AdfsPath(
                adfs,
                ImmutableList.<String>builder().addAll(parts).addAll(components).build());
    }

    @Override
    public String toString()
    {
        return parts.isEmpty() ? "$" : "$." + String.join(".", parts);
    }

    @Override
    public boolean equals(Object other)
    {
        return other instanceof AdfsPath p && adfs == p.adfs && parts.equals(p.parts);
    }

    @Override
    public int hashCode()
    {
        return Objects.hash(System.identityHashCode(adfs), parts);
    }

    public AdfsStat stat() throws AdfsException
    {
        return AdfsStat.of(adfs.entry(parts));
    }

    public boolean exists() throws AdfsException
    {
        try
        {
            stat();
            return true;
        } catch (AdfsPathException e)
        {
            return false;
        }
    }

    public boolean isDirectory() throws AdfsException
    {
        try
        {
            return stat().directory();
        } catch (AdfsPathException e)
        {
            return false;
        }
    }

    public boolean isFile() throws AdfsException
    {
        try
        {
            return !stat().directory();
        } catch (AdfsPathException e)
        {
            return false;
        }
    }

    public ImmutableList<AdfsPath> list() throws AdfsException
    {
        ImmutableList.Builder<AdfsPath> result = ImmutableList.builder();
        for (DirectoryEntry entry : adfs.directory(parts).entries())
            result.add(resolve(List.of(entry.name())));
        return result.build();
    }

    public ImmutableList<AdfsPath> walk() throws AdfsException
    {
        ImmutableList.Builder<AdfsPath> result = ImmutableList.builder();
        walkInto(result, new java.util.HashSet<>());
        return result.build();
    }

    private void walkInto(ImmutableList.Builder<AdfsPath> result, java.util.Set<Integer> seen)
            throws AdfsException
    {
        if (!seen.add(stat().discAddress()))
            throw new AdfsDirectoryException("Directory cycle");
        for (AdfsPath child : list())
        {
            result.add(child);
            if (child.isDirectory())
                child.walkInto(result, seen);
        }
    }

    public ImmutableList<AdfsPath> glob(String pattern) throws AdfsException
    {
        int dot = pattern.indexOf('.');
        String head = dot < 0 ? pattern : pattern.substring(0, dot);
        String rest = dot < 0 ? null : pattern.substring(dot + 1);
        StringBuilder regex = new StringBuilder();
        for (char c : DirectoryFormat.nameKey(head).toCharArray())
            regex.append(c == '*' ?
                    ".*" :
                    c == '#' ? "." : java.util.regex.Pattern.quote(String.valueOf(c)));
        ImmutableList.Builder<AdfsPath> result = ImmutableList.builder();
        for (AdfsPath child : list())
            if (DirectoryFormat.nameKey(child.name()).matches(regex.toString()))
            {
                if (rest == null)
                    result.add(child);
                else if (child.isDirectory())
                    result.addAll(child.glob(rest));
            }
        return result.build();
    }

    public byte[] readBytes() throws AdfsException
    {
        DirectoryEntry entry = adfs.entry(parts);
        if (entry.directory())
            throw new AdfsPathException("Cannot read a directory as a file");
        return adfs.readObject(entry.discAddress(), entry.length());
    }

    public void writeBytes(byte[] bytes) throws AdfsException
    {
        writeBytes(bytes, new AcornMeta(0, 0, null));
    }

    public void writeBytes(byte[] bytes, AcornMeta metadata) throws AdfsException
    {
        adfs.writeFile(parts, bytes, metadata);
    }

    public String readText() throws AdfsException
    {
        return org.rob_smallshire.oaknut.file.TextIo.decode(readBytes(), true);
    }

    public String readText(Charset charset) throws AdfsException
    {
        return new String(readBytes(), charset);
    }

    public void writeText(String text) throws AdfsException
    {
        writeBytes(org.rob_smallshire.oaknut.file.TextIo.encode(text, "\r"));
    }

    public void touch() throws AdfsException
    {
        touch(true);
    }

    public void touch(boolean existOk) throws AdfsException
    {
        if (exists())
        {
            if (!existOk)
                throw new AdfsEntryExistsException("Entry exists");
            if (isDirectory())
                throw new AdfsPathException("Cannot touch a directory");
        } else
            writeBytes(new byte[0]);
    }

    public void mkdir() throws AdfsException
    {
        mkdir(false, false);
    }

    public void mkdir(boolean parents, boolean existOk) throws AdfsException
    {
        if (exists())
        {
            if (existOk && isDirectory())
                return;
            throw new AdfsEntryExistsException("Entry exists");
        }
        if (parents && !parent().exists())
            parent().mkdir(true, true);
        adfs.mkdir(parts);
    }

    public void unlink() throws AdfsException
    {
        adfs.remove(parts, false);
    }

    public void rmdir() throws AdfsException
    {
        adfs.remove(parts, true);
    }

    public AdfsPath rename(AdfsPath target) throws AdfsException
    {
        if (target.adfs != adfs)
            throw new AdfsPathException("Cannot rename across discs");
        adfs.rename(parts, target.parts);
        return target;
    }

    public AdfsPath rename(String target) throws AdfsException
    {
        return rename(adfs.path(target));
    }

    public AdfsPath duplicate(AdfsPath target) throws AdfsException
    {
        if (target.exists())
            throw new AdfsEntryExistsException("Target exists");
        target.writeBytes(readBytes(), stat().metadata());
        return target;
    }

    public void setMetadata(AcornMeta metadata) throws AdfsException
    {
        adfs.setMetadata(parts, metadata);
    }

    public void chmod(int access) throws AdfsException
    {
        AcornMeta m = stat().metadata();
        setMetadata(new AcornMeta(m.loadAddress(), m.executionAddress(), access));
    }

    public void lock() throws AdfsException
    {
        chmod(stat().access() | Access.L);
    }

    public void unlock() throws AdfsException
    {
        chmod(stat().access() & ~Access.L);
    }

    public void setLoadAddress(long address) throws AdfsException
    {
        AcornMeta m = stat().metadata();
        setMetadata(new AcornMeta(address, m.executionAddress(), m.access()));
    }

    public void setExecutionAddress(long address) throws AdfsException
    {
        AcornMeta m = stat().metadata();
        setMetadata(new AcornMeta(m.loadAddress(), address, m.access()));
    }

    public Integer filetype() throws AdfsException
    {
        return stat().metadata().filetype();
    }

    public void setFiletype(int type) throws AdfsException
    {
        setMetadata(stat().metadata().withFiletype(type));
    }

    public Instant datestamp() throws AdfsException
    {
        return stat().metadata().datestamp();
    }

    public void setDatestamp(Instant when) throws AdfsException
    {
        setMetadata(stat().metadata().withDatestamp(when));
    }

    public String title() throws AdfsException
    {
        if (adfs.format().big())
            throw new UnsupportedOperationException("Big directories have no title");
        return adfs.directory(parts).title();
    }

    public void setTitle(String title) throws AdfsException
    {
        adfs.setDirectoryTitle(parts, title);
    }

    public Path exportFile(Path path) throws IOException
    {
        return exportFile(path, MetaFormat.INF_TRAD);
    }

    public Path exportFile(Path path, MetaFormat format) throws IOException
    {
        return HostBridge.exportFile(
                path,
                name(),
                readBytes(),
                stat().metadata(),
                format,
                0,
                adfs.usesTypedMetadata());
    }

    public Path exportFile(Path path, MetaFormat format, int owner, boolean typedMetadata)
            throws IOException
    {
        return HostBridge.exportFile(
                path,
                name(),
                readBytes(),
                stat().metadata(),
                format,
                owner,
                typedMetadata);
    }

    public void importFile(Path path) throws IOException
    {
        importFile(path, HostBridge.DEFAULT_IMPORT_FORMATS);
    }

    public void importFile(Path path, List<MetaFormat> formats) throws IOException
    {
        HostBridge.Imported imported = HostBridge.importFile(path, formats);
        writeBytes(imported.bytes(), imported.metadata());
    }
}
