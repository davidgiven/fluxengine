#include "globals.h"
#include "imagereader/imagereader.h"
#include "imagewriter/imagewriter.h"
#include "sectorset.h"
#include "sector.h"
#include "geometry.h"
#include "lib/geometry/geometry.pb.h"
#include "fmt/format.h"

static std::string nameOf(GeometryProto::BlockOrdering ordering)
{
	switch (ordering)
	{
		case GeometryProto::ORDER_CHS: return "CHS";
		case GeometryProto::ORDER_HCS: return "HCS";
		default: return fmt::format("order({})", ordering);
	}
}

struct Trackdata
{
	GeometryProto::TrackdataProto format;
	int offset;
};

static void parse_geometry(std::map<std::pair<int, int>, Trackdata>& offsets, const GeometryProto& config)
{
	int offset = 0;

	auto getTrackFormat = [&](GeometryProto::TrackdataProto& trackdata, unsigned cylinder, unsigned head)
	{
		trackdata.Clear();
		for (const GeometryProto::TrackdataProto& f : config.trackdata())
		{
			if (f.has_cylinder() && (f.cylinder() != cylinder))
				continue;
			if (f.has_head() && (f.head() != head))
				continue;

			trackdata.MergeFrom(f);
		}
	};

	auto addBlock = [&](int cylinder, int head) {
		Trackdata& trackdata = offsets[std::make_pair(cylinder, head)];
		trackdata.offset = offset;
		getTrackFormat(trackdata.format, cylinder, head);
		offset += trackdata.format.sectors() * trackdata.format.sector_size();
	};

	switch (config.block_ordering())
	{
		case GeometryProto::ORDER_CHS:
			for (int cylinder = 0; cylinder < config.cylinders(); cylinder++)
				for (int head = 0; head < config.heads(); head++)
					addBlock(cylinder, head);
			break;

		case GeometryProto::ORDER_HCS:
			for (int head = 0; head < config.heads(); head++)
				for (int cylinder = 0; cylinder < config.cylinders(); cylinder++)
					addBlock(cylinder, head);
			break;

		case GeometryProto::ORDER_NSI:
			for (int cylinder = 0; cylinder < config.cylinders(); cylinder++)
				addBlock(cylinder, 0);
			if (config.heads() == 2)
				for (int cylinder = config.cylinders()-1; cylinder >= 0; cylinder--)
					addBlock(cylinder, 1);
			break;
	}

	std::cout << fmt::format("GEOM: input {} image of {} cylinders, {} heads\n", 
		nameOf(config.block_ordering()), config.cylinders(), config.heads());
}

class BaseGeometryMapper
{
protected:
	BaseGeometryMapper(const GeometryProto& config):
		_config(config)
	{}

protected:
	void getOffsetOf(unsigned cylinder, unsigned head, unsigned sector, off_t& offset, unsigned& sector_size) const
	{
		const auto tit = _offsets.find(std::make_pair(cylinder, head));
		if (tit == _offsets.end())
		{
			offset = -1;
			return;
		}

		const Trackdata& trackdata = tit->second;
		sector_size = trackdata.format.sector_size();
		offset = trackdata.offset + sector*sector_size;
	}

protected:
	const GeometryProto& _config;
	mutable std::map<std::pair<int, int>, Trackdata> _offsets;
};

class SimpleDisassemblingGeometryMapper : public BaseGeometryMapper, public DisassemblingGeometryMapper
{
public:
	SimpleDisassemblingGeometryMapper(const GeometryProto& config, ImageReader& reader):
			BaseGeometryMapper(config),
			_reader(reader)
	{
		_proxy = reader.getGeometryMapper();
		if (_proxy)
		{
			std::cout << "GEOM: geometry being overridden by input image\n";
			return;
		}
		parse_geometry(_offsets, config);
	}

	const Sector* get(unsigned cylinder, unsigned head, unsigned sector) const
	{
		if (_proxy)
			return _proxy->get(cylinder, head, sector);

		std::unique_ptr<Sector>& sit = _sectors.get(cylinder, head, sector);
		if (!sit)
		{
			sit.reset(new Sector);
			sit->status = Sector::MISSING;
			sit->physicalTrack = sit->logicalTrack = cylinder;
			sit->physicalSide = sit->logicalSide = head;
			sit->logicalSector = sector;

			off_t offset;
			unsigned sector_size;
			getOffsetOf(cylinder, head, sector, offset, sector_size);
			if (offset == -1)
				return nullptr;
			sit->data = _reader.getBlock(offset, sector_size);
		}
		return sit.get();
	}

private:
	const ImageReader& _reader;
	const DisassemblingGeometryMapper* _proxy;
	mutable SectorSet _sectors;
};

class SimpleAssemblingGeometryMapper : public BaseGeometryMapper, public AssemblingGeometryMapper
{
public:
	SimpleAssemblingGeometryMapper(const GeometryProto& config, ImageWriter& writer):
			BaseGeometryMapper(config),
			_writer(writer)
	{
		_proxy = writer.getGeometryMapper();
		if (_proxy)
		{
			std::cout << "GEOM: geometry mapping not used by output image\n";
			return;
		}
		parse_geometry(_offsets, config);
	}

	void put(const Sector& data) const
	{
		if (_proxy)
		{
			_proxy->put(data);
			return;
		}

		off_t offset;
		unsigned sector_size;
		getOffsetOf(data.logicalTrack, data.logicalSide, data.logicalSector, offset, sector_size);
		if (offset == -1)
		{
			std::cout << fmt::format("GEOM: sector {}.{}.{} discarded\n",
					data.logicalTrack, data.logicalSide, data.logicalSector);
			return;
		}
		_writer.putBlock(offset, sector_size, data.data);
	}

private:
	ImageWriter& _writer;
	const AssemblingGeometryMapper* _proxy;
};

std::unique_ptr<DisassemblingGeometryMapper> createSimpleDisassemblingGeometryMapper(
	const GeometryProto& config, ImageReader& reader)
{
	return std::unique_ptr<DisassemblingGeometryMapper>(new SimpleDisassemblingGeometryMapper(config, reader));
}

std::unique_ptr<AssemblingGeometryMapper> createSimpleAssemblingGeometryMapper(
	const GeometryProto& config, ImageWriter& writer)
{
	return std::unique_ptr<AssemblingGeometryMapper>(new SimpleAssemblingGeometryMapper(config, writer));
}

