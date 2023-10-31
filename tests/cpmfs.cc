#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.pb.h"
#include "lib/image.h"
#include "lib/proto.h"
#include "lib/sector.h"
#include "snowhouse/snowhouse.h"
#include <google/protobuf/text_format.h>

using namespace snowhouse;

static Bytes blank_dirent = Bytes{0xe5} * 32;

namespace
{
    class TestSectorInterface : public SectorInterface
    {
    public:
        std::shared_ptr<const Sector> get(
            unsigned track, unsigned side, unsigned sectorId)
        {
            auto s = _image.get(track, side, sectorId);
            if (!s)
                error("missing sector c{}.h{}.s{}", track, side, sectorId);
            return s;
        }

        std::shared_ptr<Sector> put(
            unsigned track, unsigned side, unsigned sectorId)
        {
            return _image.put(track, side, sectorId);
        }

    private:
        Image _image;
    };
}

static Bytes createDirent(const std::string& filename,
    int extent,
    int records,
    const std::initializer_list<int> blocks)
{
    Bytes dirent;
    ByteWriter bw(dirent);
    bw.write_8(0);
    bw.append(filename);
    while (bw.pos != 12)
        bw.write_8(' ');

    bw.write_8(extent & 0x1f);
    bw.write_8(0);
    bw.write_8(extent >> 5);
    bw.write_8(records);

    for (int block : blocks)
        bw.write_8(block);
    while (bw.pos != 32)
        bw.write_8(0);

    return dirent;
}

static void setBlock(
    const std::shared_ptr<SectorInterface>& sectors, int block, Bytes data)
{
    for (int i = 0; i < 8; i++)
        sectors->put(block, 0, i)->data = data.slice(i * 256, 256);
}

static void testPartialExtent()
{
    auto sectors = std::make_shared<TestSectorInterface>();
    auto fs = Filesystem::createCpmFsFilesystem(
        globalConfig()->filesystem(), sectors);

    setBlock(sectors,
        0,
        createDirent("FILE", 0, 1, {1, 0, 0, 0, 0, 0, 0, 0, 0}) +
            (blank_dirent * 63));
    setBlock(sectors, 1, {1});

    auto files = fs->list(Path());
    AssertThat(files.size(), Equals(1));

    auto data = fs->getFile(Path("0:FILE"));
    AssertThat(data.size(), Equals(128));
    AssertThat(data[0x4000 * 0], Equals(1));
}

static void testLogicalExtents()
{
    auto sectors = std::make_shared<TestSectorInterface>();
    auto fs = Filesystem::createCpmFsFilesystem(
        globalConfig()->filesystem(), sectors);

    setBlock(sectors,
        0,
        createDirent("FILE", 1, 128, {1, 0, 0, 0, 0, 0, 0, 0, 2}) +
            createDirent("FILE", 2, 128, {3}) + (blank_dirent * 62));
    setBlock(sectors, 1, {1});
    setBlock(sectors, 2, {2});
    setBlock(sectors, 3, {3});

    auto files = fs->list(Path());
    AssertThat(files.size(), Equals(1));

    auto data = fs->getFile(Path("0:FILE"));
    AssertThat(data.size(), Equals(0x4000 * 3));
    AssertThat(data[0x4000 * 0], Equals(1));
    AssertThat(data[0x4000 * 1], Equals(2));
    AssertThat(data[0x4000 * 2], Equals(3));
}

int main(void)
{
    try
    {
        const std::string text = R"M(
			drive {
				drive_type: DRIVETYPE_80TRACK
			}

			layout {
				format_type: FORMATTYPE_80TRACK
				tracks: 10
				sides: 1
				layoutdata {
					sector_size: 256
					physical {
						start_sector: 0
						count: 8
					}
				}
			}

			filesystem {
				type: CPMFS
				cpmfs {
					block_size: 2048
					dir_entries: 64
				}
			}
		)M";
        google::protobuf::TextFormat::MergeFromString(
            text, globalConfig().overrides());

        testPartialExtent();
        testLogicalExtents();
    }
    catch (const ErrorException& e)
    {
        std::cerr << e.message << '\n';
        exit(1);
    }

    return 0;
}
