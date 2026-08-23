#pragma once

class FluxSource;
class FluxSinkFactory;
class ImageReader;
class ImageWriter;
class Encoder;
class Decoder;

class Context
{
public:
    virtual ~Context() {}

public:
    virtual FluxSource* GetFluxSource() = 0;
    virtual FluxSource* GetVerificationFluxSource() = 0;
    virtual FluxSinkFactory* GetFluxSink() = 0;
    virtual ImageReader* GetImageReader() = 0;
    virtual ImageWriter* GetImageWriter() = 0;
    virtual Encoder* GetEncoder() = 0;
    virtual Decoder* GetDecoder() = 0;

    static std::unique_ptr<Context> Create();
};