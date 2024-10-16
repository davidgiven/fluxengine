#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/core/logger.h"
#include "lib/config/proto.h"
#include "lib/usb/usb.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsource/fluxsource.pb.h"

class HardwareFluxSource : public FluxSource
{
private:
    class HardwareFluxSourceIterator : public FluxSourceIterator
    {
    public:
        HardwareFluxSourceIterator(int track, int head):
            _track(track),
            _head(head)
        {
        }

        bool hasNext() const override
        {
            return true;
        }

        std::unique_ptr<const Fluxmap> next() override
        {
            const auto& drive = globalConfig()->drive();

            usbSetDrive(
                drive.drive(), drive.high_density(), drive.index_mode());
            usbSeek(_track);

            Bytes data = usbRead(_head,
                drive.sync_with_index(),
                drive.revolutions() * drive.rotational_period_ms() * 1e6,
                drive.hard_sector_threshold_ns());
            auto fluxmap = std::make_unique<Fluxmap>();
            fluxmap->appendBytes(data);
            return fluxmap;
        }

    private:
        int _track;
        int _head;
    };

public:
    HardwareFluxSource(const HardwareFluxSourceProto& conf): _config(conf) {}

    ~HardwareFluxSource() {}

public:
    std::unique_ptr<FluxSourceIterator> readFlux(int track, int head) override
    {
        return std::make_unique<HardwareFluxSourceIterator>(track, head);
    }

    void recalibrate() override
    {
        usbRecalibrate();
    }

    void seek(int track) override
    {
        usbSeek(track);
    }

    bool isHardware() override
    {
        return true;
    }

private:
    const HardwareFluxSourceProto& _config;
    bool _measured;
};

std::unique_ptr<FluxSource> FluxSource::createHardwareFluxSource(
    const HardwareFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new HardwareFluxSource(config));
}
