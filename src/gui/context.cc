#include "lib/core/globals.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "arch/arch.h"
#include "lib/config/config.h"
#include "context.h"
#include "gui.h"

namespace
{
    class ContextImpl : public Context
    {
    public:
        FluxSource* GetFluxSource() override
        {
            if (!_fluxSource)
            {
                _fluxSource = FluxSource::create(globalConfig());
            }
            return _fluxSource.get();
        }

        FluxSource* GetVerificationFluxSource() override
        {
            if (!_verificationFluxSource)
            {
                _verificationFluxSource = FluxSource::create(
                    globalConfig().getVerificationFluxSourceProto());
            }
            return _verificationFluxSource.get();
        }

        FluxSink* GetFluxSink() override
        {
            if (!_fluxSink)
            {
                _fluxSink = FluxSink::create(globalConfig());
            }
            return _fluxSink.get();
        }

        ImageReader* GetImageReader() override
        {
            if (!_imageReader)
            {
                _imageReader = ImageReader::create(globalConfig());
            }
            return _imageReader.get();
        }

        ImageWriter* GetImageWriter() override
        {
            if (!_imageWriter)
            {
                _imageWriter = ImageWriter::create(globalConfig());
            }
            return _imageWriter.get();
        }

        Encoder* GetEncoder() override
        {
            if (!_encoder)
            {
                _encoder = Arch::createEncoder(globalConfig());
            }
            return _encoder.get();
        }

        Decoder* GetDecoder() override
        {
            if (!_decoder)
            {
                _decoder = Arch::createDecoder(globalConfig());
            }
            return _decoder.get();
        }

    private:
        std::unique_ptr<FluxSource> _fluxSource;
        std::unique_ptr<FluxSource> _verificationFluxSource;
        std::unique_ptr<FluxSink> _fluxSink;
        std::unique_ptr<ImageReader> _imageReader;
        std::unique_ptr<ImageWriter> _imageWriter;
        std::unique_ptr<Encoder> _encoder;
        std::unique_ptr<Decoder> _decoder;
    };
}

std::unique_ptr<Context> Context::Create()
{
    return std::make_unique<ContextImpl>();
}
