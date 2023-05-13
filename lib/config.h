#pragma once

#ifdef __cplusplus

#include "lib/fluxsource/fluxsource.pb.h"

class ConfigProto;
class OptionProto;
class FluxSource;
class FluxSink;
class ImageReader;
class ImageWriter;
class Encoder;
class Decoder;

class OptionException : public ErrorException
{
public:
    OptionException(const std::string& message): ErrorException(message) {}
};

class OptionNotFoundException : public OptionException
{
public:
    OptionNotFoundException(const std::string& message):
        OptionException(message)
    {
    }
};

class InvalidOptionException : public OptionException
{
public:
    InvalidOptionException(const std::string& message): OptionException(message)
    {
    }
};

class InapplicableOptionException : public OptionException
{
public:
    InapplicableOptionException(const std::string& message):
        OptionException(message)
    {
    }
};

class Config
{
public:
    /* Direct access to the proto configuration. */

    ConfigProto* operator->() const;
    operator ConfigProto*() const;
    operator ConfigProto&() const;

    /* Set and get individual config keys. */

    void set(std::string key, std::string value);
    std::string get(std::string key);

    /* Reset the entire configuration. */

    void clear();

    /* Set up the entire configuration in one go. */

    void initialise(std::set<std::string> options,
        std::vector<std::pair<std::string, std::string>> overrides);

    /* Merge in one config file. */

    void readConfigFile(std::string filename);

    /* Option management: look up an option by name, determine whether an option
     * is valid, and apply an option. */

    const OptionProto& findOption(const std::string& option);
    void checkOptionValid(const OptionProto& option);
    bool isOptionValid(const OptionProto& option);
    void applyOption(const OptionProto& option);

    /* Adjust overall inputs and outputs. */

    void setFluxSource(std::string value);
    void setFluxSink(std::string value);
    void setVerificationFluxSource(std::string value);
    void setCopyFluxTo(std::string value);
    void setImageReader(std::string value);
    void setImageWriter(std::string value);

    /* Fetch the sources, opening them if necessary. */

    bool hasFluxSource() const;
    std::shared_ptr<FluxSource>& getFluxSource();
    bool hasImageReader() const;
    std::shared_ptr<ImageReader>& getImageReader();
    bool hasVerificationFluxSource() const;
    std::shared_ptr<FluxSource>& getVerificationFluxSource();

    /* Fetch the encoder and decoder, creating them if necessary. */

    bool hasEncoder() const;
    std::shared_ptr<Encoder>& getEncoder();
    bool hasDecoder() const;
    std::shared_ptr<Decoder>& getDecoder();

    /* Create the sinks: these are not cached. */

    bool hasFluxSink() const;
    std::unique_ptr<FluxSink> getFluxSink();
    bool hasImageWriter() const;
    std::unique_ptr<ImageWriter> getImageWriter();

private:
    std::shared_ptr<FluxSource> _fluxSource;
    std::shared_ptr<ImageReader> _imageReader;
    std::shared_ptr<FluxSource> _verificationFluxSource;
    std::shared_ptr<Encoder> _encoder;
    std::shared_ptr<Decoder> _decoder;
    FluxSourceProto _verificationFluxSourceProto;
};

extern Config& globalConfig();

#endif
