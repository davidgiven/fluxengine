#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/fluxmap.h"
#include "lib/sector.h"
#include "lib/flux.h"
#include "lib/core/logger.h"

namespace
{
    class LogRendererImpl : public LogRenderer
    {
    public:
        LogRenderer& add(std::string m) override
        {
            if (_atNewline && _indented)
            {
                _stream << "    ";
                _lineLen = 4;
                _indented = true;
            }

            if ((m.size() + _lineLen) > 80)
            {
                _stream << "\n    ";
                _lineLen = 4;
                _indented = true;
            }

            _stream << m;
            return *this;
        }

        LogRenderer& newsection() override
        {
            newline();
            _indented = false;
            return *this;
        }

        LogRenderer& newline() override
        {
            if (!_atNewline)
            {
                _stream << '\n';
                _atNewline = true;
            }
            _lineLen = 0;
            return *this;
        }

        void renderTo(std::ostream& stream) override {}

    private:
        bool _atNewline = false;
        bool _indented = false;
        int _lineLen = 0;
        std::stringstream _stream;
    };
}

std::unique_ptr<LogRenderer> LogRenderer::create()
{
    return std::make_unique<LogRendererImpl>();
}