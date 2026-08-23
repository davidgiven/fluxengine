#pragma once

#include <hex/api/imhex_api/provider.hpp>
#include <hex/providers/provider.hpp>
#include <hex/api/events/events_provider.hpp>

#include <nlohmann/json.hpp>

class DiskProvider : public hex::prv::Provider
{
public:
    DiskProvider();
    ~DiskProvider() override;

    [[nodiscard]] bool isAvailable() const override;
    [[nodiscard]] bool isReadable() const override;
    [[nodiscard]] bool isWritable() const override;
    [[nodiscard]] bool isResizable() const override;
    [[nodiscard]] bool isSavable() const override;

    [[nodiscard]] bool open() override;

    void close() override;

    void readRaw(u64 offset, void* buffer, size_t size) override;
    void writeRaw(u64 offset, const void* buffer, size_t size) override;

    [[nodiscard]] u64 getActualSize() const override;

    [[nodiscard]] std::string getName() const override;
    [[nodiscard]] const char* getIcon() const override;
    [[nodiscard]] hex::UnlocalizedString getTypeName() const override;

    void loadSettings(const nlohmann::json& settings) override;

    [[nodiscard]] nlohmann::json storeSettings(
        nlohmann::json settings) const override;
};
