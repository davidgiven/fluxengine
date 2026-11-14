package org.rob_smallshire.oaknut.file;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.Path;
import java.nio.file.Files;
import java.nio.file.attribute.UserDefinedFileAttributeView;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Host metadata formats used by ADFS import/export. The canonical sidecar appends .inf. */
public final class HostBridge
{
    public record Imported(byte[] bytes, AcornMeta metadata)
    {
    }
    public static final List<MetaFormat> DEFAULT_IMPORT_FORMATS =
        List.of(MetaFormat.INF_TRAD, MetaFormat.XATTR_ACORN, MetaFormat.FILENAME_RISCOS);
    private HostBridge() {}

    public static int toPieb(int access)
    {
        return (access & 0x33) | ((access & Access.L) >>> 1) | (Access.isRunOnly(access) ? 8 : 0) |
            ((access & Access.HIDDEN) != 0 ? 128 : 0);
    }
    public static int fromPieb(int perm)
    {
        return (perm & 0x33) | ((perm & 4) << 1) | ((perm & 8) >>> 1) |
            ((perm & 128) != 0 ? Access.HIDDEN : 0);
    }
    private static String quoteName(String name)
    {
        boolean quote = name.isEmpty() || name.equals("TAPE") || name.startsWith("\"") ||
                        name.chars().anyMatch(c -> c <= 32 || c > 126);
        if (!quote)
            return name;
        StringBuilder encoded = new StringBuilder("\"");
        for (byte b : AcornEncoding.encode(name, false))
        {
            int c = b & 255;
            if (c < 32 || c > 126 || c == '"' || c == '%')
                encoded.append(String.format("%%%02X", c));
            else
                encoded.append((char)c);
        }
        return encoded.append('"').toString();
    }
    public static String formatInf(String name, AcornMeta meta, int length)
    {
        return String.format("%-11s %08X %08X %08X",
                             quoteName(name),
                             meta.loadAddress(),
                             meta.executionAddress(),
                             length) +
            (meta.access() == null ? "" : String.format(" %02X", meta.access() & 255));
    }
    public static String formatPiebInf(AcornMeta meta, int owner)
    {
        return String.format("%x %x %x %x",
                             owner,
                             meta.loadAddress(),
                             meta.executionAddress(),
                             toPieb(meta.access() == null ? Access.WR | Access.PR : meta.access()));
    }
    private static Integer infAccess(String field)
    {
        if (List.of("Locked", "LOCKED", "L").contains(field))
            return 0x19;
        if (field.equals("D") || field.equals("d"))
            return 0;
        if (field.equals("E"))
            return Access.E;
        if (field.equals("e"))
            return Access.PE;
        if (field.contains("/"))
            return Access.parse(field);
        if (field.matches("[0-9A-Fa-f]+"))
            return null;
        int access = 0;
        for (char c : field.toCharArray())
            access |= switch (c)
            {
                case 'R' -> Access.R;
                case 'W' -> Access.W;
                case 'r' -> Access.PR;
                case 'w' -> Access.PW;
                case 'l' -> Access.PL;
                case 'E' -> Access.E;
                case 'e' -> Access.PE;
                case 'L' -> Access.L;
                default -> throw new IllegalArgumentException("Invalid INF access");
            };
        return access;
    }
    private static long infAddress(String field)
    {
        long value = Long.parseLong(field, 16);
        if (field.length() == 6 && (value & 0xff0000) == 0xff0000)
            value |= 0xffff0000L;
        return value;
    }
    public static AcornMeta parseInf(String text)
    {
        try
        {
            String line = text.strip().split("[\\r\\n]", 2)[0];
            String[] all = line.split("\\s+");
            if ((all.length == 4 || all.length == 5) && all[3].length() <= 2 &&
                java.util.Arrays.stream(all).allMatch(f -> f.matches("[0-9A-Fa-f]+")))
                return new AcornMeta(Long.parseLong(all[1], 16),
                                     Long.parseLong(all[2], 16),
                                     fromPieb(Integer.parseInt(all[3], 16)));
            if (line.startsWith("TAPE "))
                line = line.substring(5).stripLeading();
            int nameEnd = line.startsWith("\"") ? line.indexOf('"', 1) + 1 : line.indexOf(' ');
            if (nameEnd <= 0)
                return null;
            String[] fields = line.substring(nameEnd).strip().split("\\s+");
            if (fields.length == 0 || fields[0].isEmpty())
                return null;
            Integer symbolic = infAccess(fields[0]);
            if (symbolic != null)
                return new AcornMeta(0, 0, symbolic);
            long load = infAddress(fields[0]), exec = load;
            Integer access = 0x33;
            if (fields.length > 1 && !fields[1].contains("=") && !fields[1].equals("NEXT"))
                try
                {
                    exec = infAddress(fields[1]);
                }
                catch (IllegalArgumentException ignored)
                {
                }
            if (fields.length > 2 && List.of("Locked", "LOCKED", "L").contains(fields[2]))
                access = 0x19;
            else if (fields.length > 3 && !fields[3].contains("=") && !fields[3].equals("NEXT"))
            {
                try
                {
                    access = infAccess(fields[3]);
                    if (access == null)
                        access = Integer.parseInt(fields[3], 16);
                }
                catch (IllegalArgumentException e)
                {
                    access = null;
                }
            }
            return new AcornMeta(load, exec, access);
        }
        catch (IllegalArgumentException e)
        {
            return null;
        }
    }
    public static String filenameSuffix(AcornMeta meta, boolean typed)
    {
        if (typed && meta.filetype() != null)
            return String.format(",%03x", meta.filetype());
        return String.format(",%x-%x", meta.loadAddress(), meta.executionAddress());
    }
    public static AcornMeta parseFilename(String name)
    {
        Matcher m = Pattern
                        .compile(
                            ".*,(?:([0-9a-fA-F]{8})([0-9a-fA-F]{8})|([0-9a-fA-F]{1,8})-([0-9a-fA-" +
                            "F]{1,8})|([0-9a-fA-F]{3}))$")
                        .matcher(name);
        if (!m.matches())
            return null;
        if (m.group(1) != null)
            return new AcornMeta(
                Long.parseLong(m.group(1), 16), Long.parseLong(m.group(2), 16), null);
        if (m.group(3) != null)
            return new AcornMeta(
                Long.parseLong(m.group(3), 16), Long.parseLong(m.group(4), 16), null);
        return new AcornMeta(0xfff00000L | (Long.parseLong(m.group(5), 16) << 8), 0, null);
    }
    public static Path
    exportFile(Path path, String name, byte[] bytes, AcornMeta meta, MetaFormat format)
        throws IOException
    { return exportFile(path, name, bytes, meta, format, 0, true); }
    public static Path exportFile(Path path,
                                  String name,
                                  byte[] bytes,
                                  AcornMeta meta,
                                  MetaFormat format,
                                  int owner,
                                  boolean typedMetadata) throws IOException
    {
        Path target =
            format == MetaFormat.FILENAME_RISCOS || format == MetaFormat.FILENAME_LOADEXEC
                ? path.resolveSibling(
                      path.getFileName() +
                      filenameSuffix(meta, format == MetaFormat.FILENAME_RISCOS && typedMetadata))
                : path;
        Path parent = target.toAbsolutePath().getParent();
        Files.createDirectories(parent);
        Files.write(target, bytes);
        if (format == MetaFormat.INF_TRAD || format == MetaFormat.INF_PIEB)
        {
            String line = format == MetaFormat.INF_TRAD ? formatInf(name, meta, bytes.length)
                                                        : formatPiebInf(meta, owner);
            Files.writeString(target.resolveSibling(target.getFileName() + ".inf"),
                              line + "\n",
                              StandardCharsets.ISO_8859_1);
        }
        else if (format == MetaFormat.XATTR_ACORN || format == MetaFormat.XATTR_PIEB)
            writeXattrs(target, meta, format == MetaFormat.XATTR_PIEB, owner);
        return target;
    }
    public static Imported importFile(Path path) throws IOException
    { return importFile(path, DEFAULT_IMPORT_FORMATS); }
    public static Imported importFile(Path path, List<MetaFormat> formats) throws IOException
    {
        AcornMeta meta = null;
        for (MetaFormat format : formats)
        {
            if (format == MetaFormat.INF_TRAD || format == MetaFormat.INF_PIEB)
            {
                Path sidecar = path.resolveSibling(path.getFileName() + ".inf");
                if (!Files.exists(sidecar))
                {
                    Path parent = path.toAbsolutePath().getParent();
                    try (java.util.stream.Stream<Path> entries = Files.list(parent))
                    {
                        sidecar = entries
                                      .filter(p
                                              -> p.getFileName().toString().equalsIgnoreCase(
                                                  path.getFileName() + ".inf"))
                                      .findFirst()
                                      .orElse(sidecar);
                    }
                }
                if (Files.exists(sidecar))
                    meta = parseInf(Files.readString(sidecar, StandardCharsets.ISO_8859_1));
            }
            else if (format == MetaFormat.XATTR_ACORN || format == MetaFormat.XATTR_PIEB)
                meta = readXattrs(path, format == MetaFormat.XATTR_PIEB);
            else
                meta = parseFilename(path.getFileName().toString());
            if (meta != null)
                break;
        }
        return new Imported(Files.readAllBytes(path),
                            meta == null ? new AcornMeta(0, 0, null) : meta);
    }

    /* Java's user-attribute view adds the user. namespace on Unix. */
    private static String attributeName(String name)
    { return System.getProperty("os.name").equals("Linux") ? name.substring(5) : name; }
    public static void writeXattrs(Path path, AcornMeta meta, boolean pieb, int owner)
        throws IOException
    {
        UserDefinedFileAttributeView view =
            Files.getFileAttributeView(path, UserDefinedFileAttributeView.class);
        if (view == null)
            throw new IOException("Extended attributes unsupported");
        Map<String, String> attrs = new LinkedHashMap<>();
        if (pieb)
        {
            attrs.put("user.econet_owner", String.format("%X", owner));
            attrs.put("user.econet_load", String.format("%08X", meta.loadAddress()));
            attrs.put("user.econet_exec", String.format("%08X", meta.executionAddress()));
            attrs.put("user.econet_perm",
                      String.format("%02X", toPieb(meta.access() == null ? 19 : meta.access())));
        }
        else
        {
            attrs.put("user.acorn.load", String.format("%08X", meta.loadAddress()));
            attrs.put("user.acorn.exec", String.format("%08X", meta.executionAddress()));
            if (meta.access() != null)
                attrs.put("user.acorn.attr", String.format("%02X", meta.access() & 255));
        }
        for (Map.Entry<String, String> entry : attrs.entrySet())
            view.write(attributeName(entry.getKey()),
                       StandardCharsets.US_ASCII.encode(entry.getValue()));
    }
    private static String readAttribute(UserDefinedFileAttributeView view, String name)
        throws IOException
    {
        name = attributeName(name);
        if (!view.list().contains(name))
            return null;
        ByteBuffer bytes = ByteBuffer.allocate(view.size(name));
        view.read(name, bytes);
        bytes.flip();
        return StandardCharsets.US_ASCII.decode(bytes).toString();
    }
    public static AcornMeta readXattrs(Path path, boolean pieb)
    {
        try
        {
            UserDefinedFileAttributeView view =
                Files.getFileAttributeView(path, UserDefinedFileAttributeView.class);
            if (view == null)
                return null;
            String load = readAttribute(view, pieb ? "user.econet_load" : "user.acorn.load");
            if (load == null)
                return pieb ? null : readXattrs(path, true);
            String exec = readAttribute(view, pieb ? "user.econet_exec" : "user.acorn.exec");
            String attr = readAttribute(view, pieb ? "user.econet_perm" : "user.acorn.attr");
            Integer access = attr == null ? null : Integer.parseInt(attr, 16);
            if (pieb && access != null)
                access = fromPieb(access);
            return new AcornMeta(
                Long.parseLong(load, 16), exec == null ? 0 : Long.parseLong(exec, 16), access);
        }
        catch (IOException | IllegalArgumentException | UnsupportedOperationException e)
        {
            return null;
        }
    }
}
