# DFS translated test coverage

Source: `/packages/oaknut-dfs/tests` in oaknut revision
`570d8a4f97abc31d2ce64dc5e2e7bceb1f797cb2`. Tests use JUnit 4 and generate
images in memory or in temporary directories. No image corpus is checked in,
and neither Python nor the upstream checkout is needed to run them.

| Upstream scenarios                                                                         | Java coverage                                                                                                                                                                                            |
|--------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `test_create_dfs`, `test_dfs`, `test_dfs_advanced`, `test_catalogued_surface`              | All 12 format/layout combinations, independent sides, creation/reopening, free space, first-fit allocation, full/fragmented discs, ordered compaction, locked files and metadata                         |
| `test_acorn_dfs_catalogue`, `test_watford_dfs_catalogue`, `test_catalogue_operations`      | Hand-assembled packed entry fields, directory/lock byte, title layout, Acorn descending entries, Watford 31/62 boundaries, markers, boot options, independent sequence numbers, title/metadata mutations |
| `test_dfs_write_overwrite`, `test_dfs_rename_duplicate`                                    | Overwrite without duplicate entries, overwrite in a full catalogue, collision rejection before mutation, failure preserves original file                                                                 |
| `test_filename_case`, `test_filename_characters`, `test_dfs_directory_characters`          | Preserved spelling, insensitive lookup, wildcard/control bytes, seven-bit/length limits, unusual stored directory characters, flat directory-prefixed names                                              |
| `test_title_padding`, `test_dfs_host_address`, `test_dfs_access_convention`                | Fixed-width title padding, host/parasite address expansion, raw metadata round trips, lock-to-canonical-access conversion                                                                                |
| `test_sector_count_extent`, `test_dfs_sector_count_defect`, `test_dfs_recognition_reasons` | Malformed/extended/implausible counts, extents past declared count, corroborating soft recognition signals, explicit count repairs, foreign/mismatched catalogue rejection                               |
| `test_truncated_images`, `test_format_autodetect`                                          | Ragged captures, padding, read-only preservation, SSD/DSD size/extension detection, explicit layouts, physical expansion                                                                                 |
| `test_dfs_export_import`                                                                   | Shared INF export/import, raw addresses/access, payload preservation, text conversion                                                                                                                    |
| `test_exceptions`                                                                          | Missing files, invalid names/titles/options, catalogue/disc full, locks, read-only/closed handles, native I/O failure propagation                                                                        |

`StorageTest` additionally exercises lazy external reads, write-through updates,
borrowed storage lifetime, optional rollback after file bytes have been changed,
and strict missing-sector behavior. `CatalogueTest` independently assembles and
checks raw bytes so codec round trips are not the only format checks.

The upstream filesystem adapter, CLI, BASIC conversion, and multipart/drive/path
navigation tests are excluded by scope. Relevant byte I/O and metadata operations
from path tests are exercised through flat names instead. Fixture-only BeebEm,
game, reference-image and BASIC-corpus suites are not copied: generated regression
scenarios cover catalogue layouts and malformed metadata, but do not constitute
verification against every captured image in that corpus.
