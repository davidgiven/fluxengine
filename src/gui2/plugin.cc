#include <hex/plugin.hpp>
#include <hex/api/content_registry/provider.hpp>
#include <hex/api/imhex_api/provider.hpp>
#include <hex/api/localization_manager.hpp>
#include <hex/api/events/events_provider.hpp>
#include <nlohmann/json.hpp>
#include <romfs/romfs.hpp>

class FluxEngineProvider : public hex::prv::Provider
{

public:
    FluxEngineProvider()
    {
        hex::EventProviderOpened::subscribe(
            [this](auto* newProvider)
            {
                if (newProvider == this)
                    return;

                hex::ImHexApi::Provider::remove(this, true);
            });
    }

    ~FluxEngineProvider() override
    {
        hex::EventProviderOpened::unsubscribe(this);
    }

    [[nodiscard]] bool isAvailable() const override
    {
        return true;
    }
    [[nodiscard]] bool isReadable() const override
    {
        return true;
    }
    [[nodiscard]] bool isWritable() const override
    {
        return false;
    }
    [[nodiscard]] bool isResizable() const override
    {
        return false;
    }
    [[nodiscard]] bool isSavable() const override
    {
        return false;
    }

    [[nodiscard]] bool open() override
    {
        return true;
    }
    void close() override {}

    void readRaw(u64 offset, void* buffer, size_t size) override
    {
        std::ignore = offset;
        std::ignore = buffer;
        std::ignore = size;
    }
    void writeRaw(u64 offset, const void* buffer, size_t size) override
    {
        std::ignore = offset;
        std::ignore = buffer;
        std::ignore = size;
    }
    [[nodiscard]] u64 getActualSize() const override
    {
        return 0x00;
    }

    [[nodiscard]] std::string getName() const override
    {
        return "ImHex";
    }

    [[nodiscard]] const char* getIcon() const override
    {
        return "";
    }

    void loadSettings(const nlohmann::json& settings) override
    {
        std::ignore = settings;
    }

    [[nodiscard]] nlohmann::json storeSettings(
        nlohmann::json settings) const override
    {
        return settings;
    }

    [[nodiscard]] hex::UnlocalizedString getTypeName() const override
    {
        return "fluxengine.provider.fluxengine";
    }
};

#define IMHEX_PLUGIN_NAME fluxengine
#define IMHEX_VERSION ""
#define IMHEX_PLUGIN_FEATURES_CONTENT \
    {                                 \
    }
IMHEX_PLUGIN_SETUP("FluxEngine", "David Given", "FluxEngine integration")
{
    // hex::log::debug("Using romfs: '{}'", romfs::name());
    hex::LocalizationManager::addLanguages(romfs::get("lang/languages.json").string(), [](const std::filesystem::path &path) {
        return romfs::get(path).string();
    });

    hex::ContentRegistry::Provider::add<FluxEngineProvider>(true);
}
