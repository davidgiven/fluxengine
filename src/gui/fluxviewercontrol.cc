#include "lib/core/globals.h"
#include "gui.h"
#include "fluxviewercontrol.h"
#include "textviewerwindow.h"
#include "lib/data/flux.h"
#include "lib/data/fluxmap.h"
#include "lib/decoders/decoders.h"
#include "lib/decoders/decoders.pb.h"
#include "lib/data/sector.h"
#include "lib/data/layout.h"
#include "lib/data/fluxmapreader.h"
#include "lib/core/crc.h"

DECLARE_COLOUR(BACKGROUND, 192, 192, 192);
DECLARE_COLOUR(READ_SEPARATOR, 255, 0, 0);
DECLARE_COLOUR(INDEX_SEPARATOR, 255, 255, 0);
DECLARE_COLOUR(FOREGROUND, 0, 0, 0);
DECLARE_COLOUR(FLUX, 64, 64, 255);
DECLARE_COLOUR(SECTOR, 255, 255, 255);
DECLARE_COLOUR(BAD_SECTOR, 213, 94, 0);
DECLARE_COLOUR(RECORD, 200, 200, 200);

const int BORDER = 4;
const int MINIMUM_TICK_DISTANCE = 10;
const double RENDER_LIMIT = 3000.0;

FluxViewerControl::FluxViewerControl(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style):
    wxWindow(parent,
        id,
        pos,
        size,
        style | wxFULL_REPAINT_ON_RESIZE,
        "FluxViewerControl")
{
    SetDoubleBuffered(true);
}

// clang-format off
wxBEGIN_EVENT_TABLE(FluxViewerControl, wxPanel)
	EVT_PAINT(FluxViewerControl::OnPaint)
	EVT_MOUSEWHEEL(FluxViewerControl::OnMouseWheel)
    EVT_LEFT_DOWN(FluxViewerControl::OnMouseMotion)
    EVT_LEFT_UP(FluxViewerControl::OnMouseMotion)
    EVT_MOTION(FluxViewerControl::OnMouseMotion)
	EVT_CONTEXT_MENU(FluxViewerControl::OnContextMenu)
wxEND_EVENT_TABLE();
// clang-format on

void FluxViewerControl::SetScrollbar(wxScrollBar* scrollbar)
{
    _scrollbar = scrollbar;
    _scrollbar->Bind(
        wxEVT_SCROLL_THUMBTRACK, &FluxViewerControl::OnScrollbarChanged, this);
    _scrollbar->Bind(
        wxEVT_SCROLL_CHANGED, &FluxViewerControl::OnScrollbarChanged, this);
}

void FluxViewerControl::SetFlux(std::shared_ptr<const TrackFlux> flux)
{
    _flux = flux;

    _scrollPosition = 0;
    _totalDuration = 0;
    _events.clear();
    for (const auto& trackdata : _flux->trackDatas)
    {
        for (const auto& record : trackdata->records)
        {
            _events.insert(_totalDuration + record->startTime);
            _events.insert(_totalDuration + record->endTime);
        }

        _totalDuration += trackdata->fluxmap->duration();
    }

    auto size = GetSize();
    _nanosecondsPerPixel = (double)_totalDuration / (double)size.GetWidth();

    UpdateScale();
    Refresh();
}

void FluxViewerControl::UpdateScale()
{
    auto size = GetSize();
    nanoseconds_t thumbSize = size.GetWidth() * _nanosecondsPerPixel;
    _scrollbar->SetScrollbar(_scrollPosition / 1000,
        thumbSize / 1000,
        _totalDuration / 1000,
        thumbSize / 2000);

    int totalPixels = (_totalDuration / _nanosecondsPerPixel) + 1;
    if ((totalPixels != _densityMap.size()) &&
        (_nanosecondsPerPixel > RENDER_LIMIT))
    {
        _densityMap.resize(totalPixels);
        std::fill(_densityMap.begin(), _densityMap.end(), 0.0);

        int i = 0;
        for (const auto& trackdata : _flux->trackDatas)
        {
            FluxmapReader fmr(*trackdata->fluxmap);
            while (!fmr.eof())
            {
                unsigned ticks;
                if (!fmr.findEvent(F_BIT_PULSE, ticks))
                    break;

                int fx = fmr.tell().ns() / _nanosecondsPerPixel;
                _densityMap.at(i + fx)++;
            }
            i += trackdata->fluxmap->duration() / _nanosecondsPerPixel;
        }

        double max = *std::max_element(_densityMap.begin(), _densityMap.end());
        for (auto& d : _densityMap)
            d /= max;
    }
}

static int interpolate(int lo, int hi, float factor)
{
    float range = hi - lo;
    return lo + range * factor;
}

void FluxViewerControl::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    dc.SetBackground(BACKGROUND_BRUSH);
    dc.Clear();

    auto size = GetSize();
    int w = size.GetWidth();
    int h = size.GetHeight();
    constexpr int rows = 5;
    int th = h / rows;
    int th2 = th / 2;
    int ch = th * 2 / 3;
    int ch2 = ch / 2;
    int t1y = th2;
    int t2y = th + th2;
    int t3y = th * 2 + th2;
    int t4y = th * 3 + th2;
    int t5y = th * 4 + th2;

    int x = -_scrollPosition / _nanosecondsPerPixel;
    nanoseconds_t fluxStartTime = 0;
    for (auto& trackdata : _flux->trackDatas)
    {
        nanoseconds_t duration = trackdata->fluxmap->duration();
        int fw = duration / _nanosecondsPerPixel;

        if (((x + fw) > 0) && (x < w))
        {
            if (fluxStartTime != 0)
            {
                dc.SetPen(READ_SEPARATOR_PEN);
                dc.DrawLine({x, 0}, {x, h});
            }

            dc.SetPen(FOREGROUND_PEN);
            dc.DrawLine({x, t1y}, {x + fw, t1y});
            dc.DrawLine({x, t2y}, {x + fw, t2y});
            dc.DrawLine({x, t4y}, {x + fw, t4y});

            /* Index lines. */

            {
                dc.SetPen(INDEX_SEPARATOR_PEN);
                FluxmapReader fmr(*trackdata->fluxmap);
                for (;;)
                {
                    unsigned ticks;
                    if (!fmr.findEvent(F_BIT_INDEX, ticks))
                        break;

                    int fx = fmr.tell().ns() / _nanosecondsPerPixel;
                    if (((x + fx) > 0) && ((x + fx) < w))
                        dc.DrawLine({x + fx, 0}, {x + fx, h});

                    if ((x + fx) >= w)
                        break;
                }
            }

            /* Draw the horizontal scale. */

            uint64_t tickStep = 1000;
            while ((tickStep / _nanosecondsPerPixel) < MINIMUM_TICK_DISTANCE)
                tickStep *= 10;

            static wxFont font(
                6, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
            dc.SetFont(font);
            dc.SetBackgroundMode(wxTRANSPARENT);
            dc.SetTextForeground(*wxBLACK);
            dc.SetPen(FOREGROUND_PEN);

            uint64_t tick = 0;
            if (x < 0)
                tick = floor(-x * _nanosecondsPerPixel / tickStep) * tickStep;
            while (tick < duration)
            {
                int xx = tick / _nanosecondsPerPixel;
                if ((x + xx) > w)
                    break;
                int ts = ch2 / 3;
                if ((tick % (10 * tickStep)) == 0)
                    ts = ch2 / 2;
                if ((tick % (100 * tickStep)) == 0)
                    ts = ch2;
                dc.DrawLine({x + xx, t4y - ts}, {x + xx, t4y + ts});
                if ((tick % (10 * tickStep)) == 0)
                {
                    dc.DrawText(fmt::format("{:.3f}ms", tick / 1e6),
                        {x + xx, t4y - ch2});
                }

                tick += tickStep;
            }

            double relativeScrollPosition = _scrollPosition - fluxStartTime;
            if (x <= 0)
                dc.DrawText(
                    fmt::format("{:.3f}ms", relativeScrollPosition / 1e6),
                    {BORDER, t3y + ch2 / 2});

            if ((x + fw) >= w)
            {
                wxString text = fmt::format("{:.3f}ms",
                    (relativeScrollPosition + (w * _nanosecondsPerPixel)) /
                        1e6);
                auto size = dc.GetTextExtent(text);
                dc.DrawText(
                    text, {w - size.GetWidth() - BORDER, t3y + ch2 / 2});
            }

            /* Sector blocks. */

            dc.SetBackgroundMode(wxTRANSPARENT);
            dc.SetTextForeground(*wxBLACK);
            for (auto& sector : trackdata->sectors)
            {
                nanoseconds_t sr = sector->dataEndTime;
                if (!sr)
                    sr = sector->headerEndTime;

                int sp = sector->headerStartTime / _nanosecondsPerPixel;
                int sw = (sr - sector->headerStartTime) / _nanosecondsPerPixel;

                wxRect rect = {x + sp, t1y - ch2, sw, ch};
                bool hovered = rect.Contains(_mouseX, _mouseY);
                wxPen pen(FOREGROUND_COLOUR, hovered ? 2 : 1);
                dc.SetPen(pen);

                dc.SetBrush((sector->status == Sector::OK) ? SECTOR_BRUSH
                                                           : BAD_SECTOR_BRUSH);

                dc.DrawRectangle(rect);
                wxDCClipper clipper(dc, rect);

                auto text = fmt::format("c{}.h{}.s{} {}",
                    sector->logicalTrack,
                    sector->logicalSide,
                    sector->logicalSector,
                    Sector::statusToString(sector->status));
                auto size = dc.GetTextExtent(text);
                dc.DrawText(
                    text, {x + sp + BORDER, t1y - size.GetHeight() / 2});

                if (_rightClicked && hovered)
                    ShowSectorMenu(sector);
            }

            /* Record blocks. */

            for (auto& record : trackdata->records)
            {
                int rp = record->startTime / _nanosecondsPerPixel;
                int rw = (record->endTime - record->startTime) /
                         _nanosecondsPerPixel;
                int rl = x + rp;
                int rr = rl + rw;

                if ((rr >= 0) && (rl < w))
                {
                    wxRect rect = {rl, t2y - ch2, rw, ch};
                    bool hovered = rect.Contains(_mouseX, _mouseY);
                    wxPen pen(FOREGROUND_COLOUR, hovered ? 2 : 1);
                    dc.SetPen(pen);
                    dc.SetBrush(RECORD_BRUSH);

                    dc.DrawRectangle(rect);
                    wxDCClipper clipper(dc, rect);

                    if (_nanosecondsPerPixel > (RENDER_LIMIT / 4))
                    {
                        auto text =
                            fmt::format("+{:.3f}ms", record->startTime / 1e6);
                        auto size = dc.GetTextExtent(text);
                        dc.DrawText(
                            text, {rl + BORDER, t2y - size.GetHeight() / 2});
                    }
                    else
                    {
                        dc.SetPen(FOREGROUND_PEN);

                        /* This is a bit dubious. We lie to the FluxMapReader
                         * about the ticks and ns part of the seek position.
                         * This makes the maths easier later, and also avoids
                         * having to count all the way through the fluxmap
                         * to read the start of the record. */

                        FluxmapReader fmr(*trackdata->fluxmap);
                        fmr.seek({record->position, 0, 0});

                        FluxDecoder fd(&fmr, record->clock, DecoderProto());
                        while ((int)fmr.tell().ns() <=
                               (int)(record->endTime - record->startTime))
                        {
                            uint8_t b = toBytes(fd.readBits(8)).slice(0, 1)[0];

                            int xx = fmr.tell().ns() / _nanosecondsPerPixel;
                            if ((rl + xx) > (w + 50))
                                break;
                            if (((rl + xx) > 0) && (fmr.tell().ns() != 0))
                            {
                                auto text = fmt::format("{:02x}", b);
                                auto size = dc.GetTextExtent(text);
                                dc.DrawLine(
                                    {rl + xx, t2y - ch2}, {rl + xx, t2y + ch2});
                                dc.DrawText(text,
                                    {rl + xx - size.GetWidth() - BORDER,
                                        t2y - size.GetHeight() / 2});
                            }
                        }
                    }

                    if (_rightClicked && hovered)
                        ShowRecordMenu(trackdata->trackInfo, record);
                }
            }

            /* Raw flux bits. */

            if (_nanosecondsPerPixel < (RENDER_LIMIT / 8))
            {
                for (auto& record : trackdata->records)
                {
                    int rp = record->startTime / _nanosecondsPerPixel;
                    int rw = (record->endTime - record->startTime) /
                             _nanosecondsPerPixel;
                    int rl = x + rp;
                    int rr = rl + rw;

                    if ((rr >= 0) && (rl < w))
                    {
                        dc.SetPen(FOREGROUND_PEN);

                        /* This is a bit dubious. We lie to the FluxMapReader
                         * about the ticks and ns part of the seek position.
                         * This makes the maths easier later, and also avoids
                         * having to count all the way through the fluxmap
                         * to read the start of the record. */

                        FluxmapReader fmr(*trackdata->fluxmap);
                        fmr.seek({record->position, 0, 0});

                        FluxDecoder fd(&fmr, record->clock, DecoderProto());
                        std::string text;
                        while ((int)fmr.tell().ns() <=
                               (int)(record->endTime - record->startTime))
                        {
                            bool b = fd.readBit();
                            if (!b)
                            {
                                text += "0";
                                continue;
                            }
                            text += "1";

                            int xx = fmr.tell().ns() / _nanosecondsPerPixel;
                            if ((rl + xx) > (w + 50))
                                break;
                            if ((rl + xx) > 0)
                            {
                                auto size = dc.GetTextExtent(text);
                                dc.DrawText(text,
                                    {rl + xx - size.GetWidth() - BORDER, t4y});
                            }

                            text = "";
                        }
                    }
                }
            }

            /* Flux chart. */

            dc.SetPen(FLUX_PEN);
            if (_nanosecondsPerPixel > RENDER_LIMIT)
            {
                /* Draw using density map. */

                dc.SetPen(*wxTRANSPARENT_PEN);
                for (int fx = 0; fx < fw; fx++)
                {
                    if (((x + fx) > 0) && ((x + fx) < w))
                    {
                        float density =
                            _densityMap[(fluxStartTime / _nanosecondsPerPixel) +
                                        fx];
                        wxColour colour(interpolate(BACKGROUND_COLOUR.Red(),
                                            FLUX_COLOUR.Red(),
                                            density),
                            interpolate(BACKGROUND_COLOUR.Green(),
                                FLUX_COLOUR.Green(),
                                density),
                            interpolate(BACKGROUND_COLOUR.Blue(),
                                FLUX_COLOUR.Blue(),
                                density));
                        wxBrush brush(colour);
                        dc.SetBrush(brush);
                        dc.DrawRectangle({x + fx, t5y - ch2}, {1, ch});
                    }
                }
            }
            else
            {
                /* Draw discrete pulses. */

                FluxmapReader fmr(*trackdata->fluxmap);
                while (!fmr.eof())
                {
                    unsigned ticks;
                    if (!fmr.findEvent(F_BIT_PULSE, ticks))
                        break;

                    int fx = fmr.tell().ns() / _nanosecondsPerPixel;
                    if (((x + fx) > 0) && ((x + fx) < w))
                        dc.DrawLine({x + fx, t5y - ch2}, {x + fx, t5y + ch2});
                    if ((x + fx) >= w)
                        break;
                }
            }

            dc.SetPen(FLUX_PEN);
            dc.DrawLine({x, t5y}, {x + fw, t5y});
        }

        x += fw;
        fluxStartTime += duration;
    }

    /* Ruler. */

    {
        nanoseconds_t cursorTime =
            (_mouseX * _nanosecondsPerPixel) + _scrollPosition;
        auto rightEvent = _events.lower_bound(cursorTime);
        if (rightEvent != _events.end())
        {
            auto leftEvent = rightEvent;
            leftEvent--;
            if (leftEvent != _events.begin())
            {
                int lx = (*leftEvent - _scrollPosition) / _nanosecondsPerPixel;
                int rx = (*rightEvent - _scrollPosition) / _nanosecondsPerPixel;
                int dx = rx - lx;
                int y = t3y - th2;

                dc.SetBackgroundMode(wxTRANSPARENT);
                dc.SetTextForeground(*wxBLACK);
                dc.SetPen(FOREGROUND_PEN);
                dc.DrawLine({lx, y}, {rx, y});

                auto text =
                    fmt::format("{:.3f}ms", (*rightEvent - *leftEvent) / 1e6);
                auto size = dc.GetTextExtent(text);
                int mx = (lx + rx) / 2;
                dc.DrawText(text, {mx - size.GetWidth() / 2, y});
            }
        }
    }

    _rightClicked = false;
}

void FluxViewerControl::OnMouseWheel(wxMouseEvent& event)
{
    int x = event.GetX();

    _scrollPosition += x * _nanosecondsPerPixel;
    if (event.GetWheelRotation() > 0)
        _nanosecondsPerPixel /= 1.2;
    else
        _nanosecondsPerPixel *= 1.2;
    _nanosecondsPerPixel = std::max(30.0, _nanosecondsPerPixel);
    _scrollPosition -= x * _nanosecondsPerPixel;

    UpdateScale();
    Refresh();
}

void FluxViewerControl::OnScrollbarChanged(wxScrollEvent& event)
{
    _scrollPosition = event.GetPosition() * 1000LL;
    Refresh();
}

void FluxViewerControl::OnMouseMotion(wxMouseEvent& event)
{
    event.Skip();

    if (event.Leaving())
        _mouseX = _mouseY = -1;
    else
    {
        _mouseX = event.GetX();
        _mouseY = event.GetY();
    }

    if (event.ButtonDown(wxMOUSE_BTN_LEFT))
    {
        _dragStartX = event.GetX();
        _dragStartPosition = _scrollPosition;
    }
    else if (event.ButtonUp(wxMOUSE_BTN_LEFT))
    {
        _dragStartX = -1;
        _dragStartPosition = -1;
    }
    else if (event.Dragging() && event.LeftIsDown() && (_dragStartX != -1))
    {
        int dx = _dragStartX - event.GetX();
        nanoseconds_t dt = dx * _nanosecondsPerPixel;
        _scrollPosition = _dragStartPosition + dt;
        UpdateScale();
    }

    Refresh();
}

void FluxViewerControl::OnContextMenu(wxContextMenuEvent& event)
{
    _rightClicked = true;
    Refresh();
}

void FluxViewerControl::ShowSectorMenu(std::shared_ptr<const Sector> sector)
{
    wxMenu menu;

    menu.Bind(
        wxEVT_MENU,
        [&](wxCommandEvent&)
        {
            DisplaySectorSummary(sector);
        },
        menu.Append(wxID_ANY, "Show sector summary")->GetId());

    menu.Bind(
        wxEVT_MENU,
        [&](wxCommandEvent&)
        {
            DisplayDecodedData(sector);
        },
        menu.Append(wxID_ANY, "Show decoded data")->GetId());

    menu.Bind(
        wxEVT_MENU,
        [&](wxCommandEvent&)
        {
            DisplayRawData(sector);
        },
        menu.Append(wxID_ANY, "Show raw data")->GetId());

    _rightClicked = false;
    PopupMenu(&menu, _mouseX, _mouseY);
}

void FluxViewerControl::ShowRecordMenu(std::shared_ptr<const TrackInfo>& layout,
    std::shared_ptr<const Record> record)
{
    wxMenu menu;

    menu.Bind(
        wxEVT_COMMAND_MENU_SELECTED,
        [&](wxCommandEvent&)
        {
            DisplayRawData(layout, record);
        },
        menu.Append(wxID_ANY, "Show record data")->GetId());

    _rightClicked = false;
    PopupMenu(&menu, _mouseX, _mouseY);
}

static void dumpSectorMetadata(
    std::ostream& s, std::shared_ptr<const Sector> sector)
{
    s << fmt::format(
             "Sector status:     {}\n", Sector::statusToString(sector->status))
      << fmt::format("Physical location: c{}.h{}\n",
             sector->physicalTrack,
             sector->physicalSide)
      << fmt::format("Clock:             {:.2f}us / {:.0f}kHz\n",
             sector->clock / 1000.0,
             1000000.0 / sector->clock)
      << fmt::format("Bytecode position: {}\n", sector->position)
      << fmt::format(
             "Data CRC16:        {:4x}\n", crc16(CCITT_POLY, sector->data));
}

static void dumpRecordMetadata(
    std::ostream& s, std::shared_ptr<const Record> record)
{
    s << fmt::format("Bytecode position: {}\n", record->position)
      << fmt::format(
             "Start:             {:.2f}ms\n", record->startTime / 1000000.0)
      << fmt::format(
             "End:               {:.2f}ms\n", record->endTime / 1000000.0)
      << fmt::format(
             "Data CRC16:        {:4x}\n", crc16(CCITT_POLY, record->rawData));
}

void FluxViewerControl::DisplayDecodedData(std::shared_ptr<const Sector> sector)
{
    std::stringstream s;

    auto title = fmt::format("User data for c{}.h{}.s{}",
        sector->logicalTrack,
        sector->logicalSide,
        sector->logicalSector);
    s << title << '\n';
    dumpSectorMetadata(s, sector);
    s << '\n';

    hexdump(s, sector->data);

    TextViewerWindow::Create(this, title, s.str())->Show();
}

void FluxViewerControl::DisplaySectorSummary(
    std::shared_ptr<const Sector> sector)
{
    std::stringstream s;

    auto title = fmt::format("Sector summary c{}.h{}.s{}",
        sector->logicalTrack,
        sector->logicalSide,
        sector->logicalSector);
    s << title << '\n';

    std::vector<std::shared_ptr<const Sector>> sectors;
    for (auto& trackdata : _flux->trackDatas)
    {
        if ((trackdata->trackInfo->logicalTrack == sector->logicalTrack) &&
            (trackdata->trackInfo->logicalSide == sector->logicalSide))
        {
            for (auto& sec : trackdata->sectors)
            {
                if (*sec == *sector)
                    sectors.push_back(sec);
            }
        }
    }

    s << fmt::format("Number of times seen: {}\n", sectors.size());

    for (int i = 0; i < sectors.size(); i++)
    {
        auto& sec = sectors[i];
        s << fmt::format("\nInstance {}:\n\n", i);
        dumpSectorMetadata(s, sec);
        s << '\n';
    }

    TextViewerWindow::Create(this, title, s.str())->Show();
}

void FluxViewerControl::DisplayRawData(std::shared_ptr<const Sector> sector)
{
    std::stringstream s;

    auto title = fmt::format("Raw data for c{}.h{}.s{}",
        sector->logicalTrack,
        sector->logicalSide,
        sector->logicalSector);
    s << title << '\n';
    dumpSectorMetadata(s, sector);
    s << fmt::format("Number of records: {}\n", sector->records.size());

    for (int i = 0; i < sector->records.size(); i++)
    {
        auto& record = sector->records[i];
        s << fmt::format("\nRecord {}:\n\n", i);
        dumpRecordMetadata(s, record);
        s << '\n';
        hexdump(s, record->rawData);
    }

    TextViewerWindow::Create(this, title, s.str())->Show();
}

void FluxViewerControl::DisplayRawData(std::shared_ptr<const TrackInfo>& layout,
    std::shared_ptr<const Record> record)
{
    std::stringstream s;

    auto title = fmt::format("Raw data for record c{}.h{} + {:.3f}ms",
        layout->physicalTrack,
        layout->physicalSide,
        record->startTime / 1e6);
    s << title << "\n";
    dumpRecordMetadata(s, record);
    s << '\n';
    hexdump(s, record->rawData);

    TextViewerWindow::Create(this, title, s.str())->Show();
}
