# oaknut Java port

This is a standalone Java 21 port of the floppy-disc functionality of
Robert Smallshire's oaknut-adfs 13.6.1, with the shared oaknut utilities it
uses. The upstream source is https://github.com/rob-smallshire/oaknut at
revision `570d8a4f97abc31d2ce64dc5e2e7bceb1f797cb2`.

The source and translated tests retain oaknut's [MIT license](LICENSE).

## Packages

- `org.rob_smallshire.oaknut.adfs`: floppy image creation, detection,
  directories, old/new allocation maps, paths, validation and repair.
- `org.rob_smallshire.oaknut.discimage`: random-access storage, lazy write-through
  sector views, and physical/logical surface mapping.
- `org.rob_smallshire.oaknut.file`: Acorn attributes, metadata, timestamps,
  text encoding, host metadata import/export, INF files and xattrs.
- `org.rob_smallshire.oaknut.filesystem`: validation findings and explicit fixes.
  This contains no filesystem adapter or plugin registry.

Supported formats are S, M, L (interleaved or sequential), D, E/E+, F/F+,
and G/G+. Hard-disc geometries, hard-disc sidecars and partitions, the oaknut
filesystem adapter, command-line programs, and BASIC conversion are excluded.
Old-map AFS info-sector pointers can be preserved and manipulated, but this
library does not open the AFS filesystem they describe.

## Using the library

Depend on `//third_party/java/org/rob_smallshire/oaknut/adfs`:

```java
try (Adfs disc = Adfs.fromFile(imagePath))
{
    AdfsPath games = disc.path("$.Games");
    games.mkdir(true, true);
    games.resolve("ReadMe").writeText("Hello from Java\n");
    disc.flush();
}
```

`Adfs.fromFile(path, true)` opens a read-only image. `fromBuffer(ByteBuffer)`
uses the buffer's remaining bytes without moving its position, and respects
read-only buffers. It never owns the supplied buffer. `create(AdfsFormat.S)`
creates an in-memory image; `createFile` refuses to overwrite existing files.

Paths always resolve current on-disc data and can be manipulated after closing
an image, but I/O through them then fails. File-backed images use a mapped
buffer, flushed explicitly or on close. Closing releases the file channel;
the JVM manages the mapping's lifetime.

`fromBuffer(buffer, true)` prefers sequential order when a 640K image's
directory tree cannot distinguish its layouts. File opening uses `.adf` as
this tiebreak; `.adl` prefers interleaved order. Detection uses the directory
tree when it can distinguish the layouts. An explicit format overload is
also available for images with ambiguous contents.

`validate()` returns findings without changing the image. Call `fix()` on
selected repairable findings to repair stale old-directory checksums,
remove unreachable entries, truncate partially held files, and reconcile
the free-space map with a truncated capture. Buffer- and file-backed handles
restore the original image if a mutation fails.

## Lazy storage adapters

Implement `discimage.Storage` to expose a byte-addressed device with its size,
exact-length reads and writes, optional read-only status, and optional flush.
`Adfs.fromStorage(storage, false)` detects the floppy format without loading
its contents into a whole-image buffer. There is also an overload accepting
an explicit `AdfsFormat`. Creating or slicing a `SectorsView` performs no I/O;
reads and writes access only the referenced ranges. Mounting reads allocation
maps and directories needed for format/layout detection, not file contents.

The boolean controls mutation rollback. `false` disables oaknut's whole-image
snapshots when the storage owner provides transactions, as FluxEngine's
`BlockDevice` does. After a failed mutation in this mode, discard the backing
transaction and reopen the handle. `true` retains oaknut rollback, which reads
the whole image before a mutation. `fromBuffer` and `fromFile` keep rollback
enabled. Calling `toByteArray()` explicitly still reads the whole image.

Storage is borrowed: closing an ADFS handle calls `Storage.flush()` but never
closes the underlying device. Flush need not commit its owner's transaction;
the FluxEngine adapter writes through to pending block changes and leaves
commit/revert to the filesystem lifecycle. Its per-mount block cache is
populated lazily and discarded on dismount.

`Storage` methods can throw `IOException`. Sector views preserve their existing
unchecked buffer-style API by wrapping failures in `UncheckedIOException`;
`fromStorage` unwraps opening failures back to the original `IOException`.
`DiscImage.buffer()` remains available only for buffer-backed images.

## Java API choices and upstream behavior

Python dataclasses become records; operators become methods such as
`resolve`, `parent`, `list`, and `walk`. Attributes use the canonical OSFILE
bit mask in `Access`, separate from directory flags. Unsigned address words
use `long`, and image offsets and lengths use `int` because this port handles
floppies. Collections returned by the disc/path API are immutable.

`AcornMeta` exposes typed metadata directly, without an oaknut mount adapter.
Java timestamps use `Instant`, interpreting oaknut's timezone-free 1900 epoch
as UTC; centiseconds truncate finer resolution. Literal equal load/exec
pairs are not treated as timestamps.

The source's legacy rename behavior is preserved: a locked entry can be
renamed, and directory headers retain their old name and parent fields when
the directory entry moves. Renaming a shared new-map file into another
sibling group gives it independent storage to prevent later deletion from
invalidating another file. Moving a directory into itself is refused.

As upstream, boot-option mutation is unavailable on new maps. Compaction
supports old directories (S/M/L); it does not compact D or new-map discs.
Big directories have a disc name but no directory-title field.

Default host export writes a traditional `.inf` sidecar. Import tries INF,
Acorn xattrs (with PiEconetBridge fallback), then filename metadata. Explicit
format lists override this order. Extended attributes use Java NIO's user
attribute view; reads tolerate absent/unsupported attributes, while explicit
writes report unsupported hosts. BASIC programs can be read and written as
bytes; no tokenisation or detokenisation is included.

## Verification

```text
bazel build //third_party/java/org/rob_smallshire/oaknut/... //java/...
bazel test //third_party/javatests/org/rob_smallshire/oaknut/... //javatests/...
```

The Java tests exercise generated images, directory codecs, allocation maps,
metadata, host import/export and sector views. No disk images or captured image
snapshots are checked in. See
`third_party/javatests/org/rob_smallshire/oaknut/adfs/TEST_COVERAGE.md` for
coverage. Python is not required to run the Java tests.
