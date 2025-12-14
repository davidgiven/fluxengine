#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "lib/core/bytes.h"
#include "gui.h"
#include "lib/data/layout.h"
#include "fileviewerwindow.h"

FileViewerWindow::FileViewerWindow(
    wxWindow* parent, const std::string& title, const Bytes& data):
    FileViewerWindowGen(parent)
{
    auto size = hexControl->GetTextExtent("M");
    SetSize(size.Scale(85, 25));
    SetTitle(title);

    {
        std::stringstream ss;
        hexdump(ss, data);
        hexControl->SetValue(ss.str());
    }

    {
        std::stringstream ss;
        bool nl = false;
        for (uint8_t c : data)
        {
            if ((c == '\r') && nl)
            {
                nl = false;
                continue;
            }

            if ((c == '\n') || (c == '\t') || ((c >= 32) && (c <= 126)))
                ss << (char)c;
            else if (c == '\r')
                ss << '\n';
            else
                ss << fmt::format("\\x{:02x}", c);

            nl = (c == '\n');
        }
        textControl->SetValue(ss.str());
    }
}

void FileViewerWindow::OnClose(wxCloseEvent& event)
{
    Destroy();
}
