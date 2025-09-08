#include <hex/api/imhex_api/hex_editor.hpp>
#include <hex/api/imhex_api/provider.hpp>
#include <hex/helpers/logger.hpp>
#include <hex/helpers/auto_reset.hpp>
#include <hex/api/content_registry/settings.hpp>
#include <hex/api/task_manager.hpp>
#include "lib/core/globals.h"
#include "lib/data/locations.h"
#include "lib/data/image.h"
#include "lib/data/sector.h"
#include "globals.h"
#include "diskprovider.h"
#include "datastore.h"

DiskProvider::DiskProvider()
{
    hex::EventProviderOpened::subscribe(
        [this](auto* newProvider)
        {
            if (newProvider == this)
                return;

            hex::ImHexApi::Provider::remove(this, true);
        });

    Events::SeekToPhysicalLocationInImage::subscribe(
        [](const CylinderHeadSector& physicalLocation)
        {
            hex::ImHexApi::HexEditor::setSelection(
                {0, 0}, hex::ImHexApi::Provider::get());
        });
}

DiskProvider::~DiskProvider()
{
    hex::EventProviderOpened::unsubscribe(this);
}

[[nodiscard]] bool DiskProvider::isAvailable() const
{
    return true;
}
[[nodiscard]] bool DiskProvider::isReadable() const
{
    return true;
}
[[nodiscard]] bool DiskProvider::isWritable() const
{
    return false;
}
[[nodiscard]] bool DiskProvider::isResizable() const
{
    return false;
}
[[nodiscard]] bool DiskProvider::isSavable() const
{
    return false;
}

[[nodiscard]] bool DiskProvider::open()
{
    return true;
}

void DiskProvider::close() {}

void DiskProvider::readRaw(u64 offset, void* buffer, size_t size)
{
    const auto& diskFlux = Datastore::getDiskFlux();
    if (diskFlux && diskFlux->image)
    {
        while (size != 0)
        {
            auto [block, blockOffset] =
                diskFlux->image->findBlockByOffset(offset);
            auto sector = diskFlux->image->getBlock(block);
            unsigned bytesRemaining =
                std::min((unsigned)size, sector->trackLayout->sectorSize - blockOffset);
            auto bytes = sector->data.slice(blockOffset, bytesRemaining);
            memcpy(buffer, bytes.cbegin(), bytes.size());

            offset += bytesRemaining;
            buffer = (char*)buffer + bytesRemaining;
            size -= bytesRemaining;
        }
    }
    else
        memset(buffer, 0, size);
}

void DiskProvider::writeRaw(u64 offset, const void* buffer, size_t size)
{
    std::ignore = offset;
    std::ignore = buffer;
    std::ignore = size;
}

[[nodiscard]] u64 DiskProvider::getActualSize() const
{
    const auto& diskFlux = Datastore::getDiskFlux();
    if (diskFlux && diskFlux->image)
        return diskFlux->image->getGeometry().totalBytes;
    return 0;
}

[[nodiscard]] std::string DiskProvider::getName() const
{
    return "FluxEngine disk image";
}

[[nodiscard]] const char* DiskProvider::getIcon() const
{
    return "";
}

void DiskProvider::loadSettings(const nlohmann::json& settings)
{
    std::ignore = settings;
}

[[nodiscard]] nlohmann::json DiskProvider::storeSettings(
    nlohmann::json settings) const
{
    return settings;
}

[[nodiscard]] hex::UnlocalizedString DiskProvider::getTypeName() const
{
    return "fluxengine.provider.disk";
}
