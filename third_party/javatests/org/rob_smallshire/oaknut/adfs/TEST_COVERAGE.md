# Test coverage

The JUnit 4 tests adapt scenarios from oaknut-adfs 13.6.1 at revision
`570d8a4f97abc31d2ce64dc5e2e7bceb1f797cb2` and its shared utilities.

All images are generated in memory during tests. Persistence and host-transfer
tests use temporary files that JUnit removes. No disk images, compressed image
fixtures, recorded snapshots, or external image downloads are required.

- `AdfsTest`: all floppy formats, geometry, extension defaults, creation and
  reopening, paths, directory operations, sorting, locks, atomic failure,
  titles, boot options, sharing, big-directory growth, metadata and compaction.
- `LayoutTest`: old/new/big directory serialization, attributes, unsigned
  addresses, checksums, name heaps, staged writes, fragmented allocation across
  holes and zones, and full free-space maps.
- `DiscImageTest`: noncontiguous write-through views, surface interleaving,
  logical sector order, read-only buffers and bounds checks.
- `FileUtilitiesTest`: access conventions, timestamps, Acorn text, INF and
  filename metadata, import/export precedence, and extended attributes.

Tests tied to the removed real-disc corpus, recorded Python API snapshots,
and byte-for-byte comparisons with stored blank images have been removed.
The retained tests do not provide independent real-disc compatibility coverage.

Hard-disc support, the oaknut filesystem adapter, nested AFS filesystems,
command-line tools and BASIC conversion remain outside this port's scope.
