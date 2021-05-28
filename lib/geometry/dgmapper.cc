#include "globals.h"
#include "imagereader/imagereader.h"
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

class SimpleDisassemblingGeometryMapper : public DisassemblingGeometryMapper
{
public:
	SimpleDisassemblingGeometryMapper(const GeometryProto& config, ImageReader& reader):
			_config(config),
			_reader(reader)
	{
		int offset = 0;

		auto addBlock = [&](int cylinder, int head) {
			Trackdata& trackdata = _offsets[std::make_pair(cylinder, head)];
			trackdata.offset = offset;
			getTrackFormat(trackdata.format, cylinder, head);
			offset += trackdata.format.sectors() * trackdata.format.sector_size();
		};

		switch (_config.block_ordering())
		{
			case GeometryProto::ORDER_CHS:
				for (int cylinder = 0; cylinder < _config.cylinders(); cylinder++)
					for (int head = 0; head < _config.heads(); head++)
						addBlock(cylinder, head);
				break;

			case GeometryProto::ORDER_HCS:
				for (int head = 0; head < _config.heads(); head++)
					for (int cylinder = 0; cylinder < _config.cylinders(); cylinder++)
						addBlock(cylinder, head);
				break;

			case GeometryProto::ORDER_NSI:
				for (int cylinder = 0; cylinder < _config.cylinders(); cylinder++)
					addBlock(cylinder, 0);
				if (_config.heads() == 2)
					for (int cylinder = _config.cylinders()-1; cylinder >= 0; cylinder--)
						addBlock(cylinder, 1);
				break;
		}

		std::cout << fmt::format("GEOM: input {} image of {} cylinders, {} heads\n", 
			nameOf(_config.block_ordering()),
			_config.cylinders(), _config.heads());
	}

	const Sector* get(unsigned cylinder, unsigned head, unsigned sector) const
	{
		auto sit = _sectors.find(std::make_tuple(cylinder, head, sector));
		if (sit == _sectors.end())
		{
			Sector* s = &sit->second;
			s->status = Sector::MISSING;
			s->physicalTrack = s->logicalTrack = cylinder;
			s->physicalSide = s->logicalSide = head;
			s->logicalSector = sector;

			const auto tit = _offsets.find(std::make_pair(cylinder, head));
			if (tit == _offsets.end())
				return nullptr;

			const Trackdata& trackdata = tit->second;
			int offset = trackdata.offset + sector*trackdata.format.sector_size();
			s->data = _reader.getBlock(offset, trackdata.format.sector_size());
		}
		return &sit->second;
	}

private:
	void getTrackFormat(GeometryProto::TrackdataProto& trackdata, unsigned cylinder, unsigned head) const
	{
		trackdata.Clear();
		for (const GeometryProto::TrackdataProto& f : _config.trackdata())
		{
			if (f.has_cylinder() && (f.cylinder() != cylinder))
				continue;
			if (f.has_head() && (f.head() != head))
				continue;

			trackdata.MergeFrom(f);
		}
	}

private:
	struct Trackdata
	{
		GeometryProto::TrackdataProto format;
		int offset;
	};

	const GeometryProto& _config;
	const ImageReader& _reader;
	mutable std::map<std::pair<int, int>, Trackdata> _offsets;
	mutable std::map<std::tuple<int, int, int>, Sector> _sectors;
};

std::unique_ptr<DisassemblingGeometryMapper> createSimpleDisassemblingGeometryMapper(
	const GeometryProto& config, ImageReader& reader)
{
	return std::unique_ptr<DisassemblingGeometryMapper>(new SimpleDisassemblingGeometryMapper(config, reader));
}

