#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/vfs/vfs.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.pb.h"
#include "lib/data/image.h"
#include "lib/config/proto.h"
#include "lib/data/sector.h"
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
            unsigned track, unsigned side, unsigned sectorId) override
        {
            auto s = _image.get(track, side, sectorId);
            if (!s)
                error("missing sector c{}.h{}.s{}", track, side, sectorId);
            return s;
        }

        std::shared_ptr<Sector> put(
            unsigned track, unsigned side, unsigned sectorId) override
        {
            return _image.put(track, side, sectorId);
        }

    private:
        Image _image;
    };
}

namespace snowhouse
{
    template <>
    struct Stringizer<std::vector<bool>>
    {
        static std::string ToString(const std::vector<bool>& vector)
        {
            std::stringstream stream;
            stream << '{';
            bool first = true;
            for (const auto& item : vector)
            {
                if (!first)
                    stream << ", ";
                stream << item;
                first = false;
            }
            stream << '}';
            return stream.str();
        }
    };

    template <>
    struct Stringizer<Bytes>
    {
        static std::string ToString(const Bytes& bytes)
        {
            std::stringstream stream;
            stream << '\n';
            hexdump(stream, bytes);
            return stream.str();
        }
    };
}

static Bytes createDirent(const std::string& filename,
    int extent,
    int records,
    const std::initializer_list<int> blocks,
    int user = 0)
{
    Bytes dirent;
    ByteWriter bw(dirent);
    bw.write_8(user);
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

static Bytes getBlock(
    const std::shared_ptr<SectorInterface>& sectors, int block, int length)
{
    Bytes bytes;
    ByteWriter bw(bytes);

    for (int i = 0; i < (length + 127) / 128; i++)
    {
        auto sector = sectors->get(block, 0, i);
        bw.append(sector->data);
    }

    return bytes;
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

static void testBitmap()
{
    auto sectors = std::make_shared<TestSectorInterface>();
    auto fs = Filesystem::createCpmFsFilesystem(
        globalConfig()->filesystem(), sectors);

    setBlock(sectors,
        0,
        createDirent("FILE", 1, 128, {1, 0, 0, 0, 0, 0, 0, 0, 2}) +
            createDirent("FILE", 2, 128, {4}) + (blank_dirent * 62));

    dynamic_cast<HasMount*>(fs.get())->mount();
    std::vector<bool> bitmap =
        dynamic_cast<HasBitmap*>(fs.get())->getBitmapForDebugging();
    AssertThat(bitmap,
        Equals(std::vector<bool>{
            1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
}
#if 0

static void testPutGet()
{
    auto sectors = std::make_shared<TestSectorInterface>();
    auto fs = Filesystem::createCpmFsFilesystem(
        globalConfig()->filesystem(), sectors);
    fs->create(true, "volume");

    fs->putFile(Path("0:FILE1"), Bytes{1, 2, 3, 4});
    fs->putFile(Path("0:FILE2"), Bytes{5, 6, 7, 8});

    dynamic_cast<HasMount*>(fs.get())->mount();
    std::vector<bool> bitmap =
        dynamic_cast<HasBitmap*>(fs.get())->getBitmapForDebugging();
    AssertThat(bitmap,
        Equals(std::vector<bool>{
            1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));

    auto directory = getBlock(sectors, 0, 256).slice(0, 64);
    AssertThat(directory,
        Equals(createDirent("FILE1", 0, 1, {1}) +
               createDirent("FILE2", 0, 1, {2})));

    auto file1 = getBlock(sectors, 1, 8).slice(0, 8);
    AssertThat(file1, Equals(Bytes{1, 2, 3, 4, 0, 0, 0, 0}));

    auto file2 = getBlock(sectors, 2, 8).slice(0, 8);
    AssertThat(file2, Equals(Bytes{5, 6, 7, 8, 0, 0, 0, 0}));
}

static void testPutBigFile()
{
    auto sectors = std::make_shared<TestSectorInterface>();
    auto fs = Filesystem::createCpmFsFilesystem(
        globalConfig()->filesystem(), sectors);
    fs->create(true, "volume");

    Bytes filedata;
    ByteWriter bw(filedata);
    while (filedata.size() < 0x9000)
        bw.write_le32(bw.pos);

    fs->putFile(Path("0:BIGFILE"), filedata);

    auto directory = getBlock(sectors, 0, 256).slice(0, 64);
    AssertThat(directory,
        Equals(createDirent("BIGFILE",
                   0,
                   0x80,
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}) +
               createDirent("BIGFILE", 2, 0x20, {17, 18})));
}

static void testDelete()
{
    auto sectors = std::make_shared<TestSectorInterface>();
    auto fs = Filesystem::createCpmFsFilesystem(
        globalConfig()->filesystem(), sectors);
    fs->create(true, "volume");

    fs->putFile(Path("0:FILE1"), Bytes{1, 2, 3, 4});
    fs->putFile(Path("0:FILE2"), Bytes{5, 6, 7, 8});
    fs->deleteFile(Path("0:FILE1"));

    auto directory = getBlock(sectors, 0, 256).slice(0, 64);
    AssertThat(directory,
        Equals((Bytes{0xe5} * 32) + createDirent("FILE2", 0, 1, {2})));
}

static void testMove()
{
    auto sectors = std::make_shared<TestSectorInterface>();
    auto fs = Filesystem::createCpmFsFilesystem(
        globalConfig()->filesystem(), sectors);
    fs->create(true, "volume");

    fs->putFile(Path("0:FILE1"), Bytes{0x55} * 0x9000);
    fs->putFile(Path("0:FILE2"), Bytes{5, 6, 7, 8});

    fs->moveFile(Path("0:FILE1"), Path("1:FILE3"));

    auto directory = getBlock(sectors, 0, 256).slice(0, 32 * 3);
    AssertThat(directory,
        Equals(createDirent("FILE3",
                   0,
                   0x80,
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                   1) +
               createDirent("FILE3", 2, 0x20, {17, 18}, 1) +
               createDirent("FILE2", 0, 1, {19})));
}

static void testPutMetadata()
{
    auto sectors = std::make_shared<TestSectorInterface>();
    auto fs = Filesystem::createCpmFsFilesystem(
        globalConfig()->filesystem(), sectors);
    fs->create(true, "volume");

    fs->putFile(Path("0:FILE1"), Bytes{0x55} * 0x9000);
    fs->putFile(Path("0:FILE2"), Bytes{5, 6, 7, 8});

    fs->putMetadata(Path("0:FILE1"),
        std::map<std::string, std::string>{
            {"mode", "SRA"}
    });

    auto directory = getBlock(sectors, 0, 256).slice(0, 32 * 3);
    AssertThat(directory,
        Equals(createDirent("FILE1   \xa0\xa0\xa0",
                   0,
                   0x80,
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}) +
               createDirent("FILE1   \xa0\xa0\xa0", 2, 0x20, {17, 18}) +
               createDirent("FILE2", 0, 1, {19})));
}

#endif
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
				tracks: 20
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
#if 0
        testBitmap();
        testPutGet();
        testPutBigFile();
        testDelete();
        testMove();
        testPutMetadata();
#endif
    }
    catch (const ErrorException& e)
    {
        std::cerr << e.message << '\n';
        exit(1);
    }

    return 0;
}
