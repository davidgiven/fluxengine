#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/core/logger.h"

namespace
{
    class LogRendererImpl : public LogRenderer
    {
    public:
        LogRendererImpl(std::ostream& stream): _stream(stream) {}

    private:
        void indent()
        {
            _stream << "       ";
            _lineLen = 7;
            _space = true;
        }

    public:
        LogRenderer& add(const std::string& m) override
        {
            if (_newline && !_header)
                indent();

            if (!_space)
            {
                _stream << ' ';
                _lineLen++;
            }

            _newline = false;
            _header = false;

            _lineLen += m.size();
            if (_lineLen >= 80)
            {
                _stream << '\n';
                indent();
            }
            _stream << m;
            _space = !m.empty() && isspace(m[m.size() - 1]);
            return *this;
        }

        LogRenderer& header(const std::string& m) override
        {
            if (!_newline)
                _stream << '\n';
            _stream << m;
            _lineLen = m.size();
            _header = true;
            _newline = true;
            _space = !m.empty() && isspace(m[m.size() - 1]);
            return *this;
        }

        LogRenderer& comma() override
        {
            if (!_newline || _header)
            {
                _stream << ';';
                _space = false;
            }
            return *this;
        }

        LogRenderer& newline() override
        {
            if (!_header)
            {
                if (!_newline)
                    _stream << '\n';

                _lineLen = 0;
                _header = false;
                _newline = true;
                _space = true;
            }
            return *this;
        }

    private:
        bool _header = false;
        bool _newline = false;
        bool _space = false;
        int _lineLen = 0;
        std::ostream& _stream;
    };
}

std::unique_ptr<LogRenderer> LogRenderer::create(std::ostream& stream)
{
    return std::make_unique<LogRendererImpl>(stream);
}