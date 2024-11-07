#include "lib/core/globals.h"
#include "gui.h"
#include "lib/data/fluxmap.h"
#include "histogramviewer.h"

static constexpr int BORDER = 10;
static constexpr int WIDTH = 256;
static constexpr int HEIGHT = 100;

// clang-format off
wxBEGIN_EVENT_TABLE(HistogramViewer, wxWindow)
	EVT_PAINT(HistogramViewer::OnPaint)
wxEND_EVENT_TABLE();
// clang-format on

HistogramViewer::HistogramViewer(wxWindow* parent,
    wxWindowID winid,
    const wxPoint& pos,
    const wxSize& size,
    long style):
    wxWindow(parent,
        winid,
        pos,
        wxSize(WIDTH + 2 * BORDER, HEIGHT + 2 * BORDER),
        style)
{
    _font = GetFont().MakeSmaller().MakeSmaller().MakeSmaller();
}

void HistogramViewer::Redraw(const Fluxmap& fluxmap, nanoseconds_t clock)
{
    _data = FluxmapReader(fluxmap).guessClock();
    _clock = clock;
    _blank = false;
    Refresh();
}

void HistogramViewer::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    dc.SetBackground(wxSystemSettings::GetColour(wxSYS_COLOUR_FRAMEBK));
    dc.Clear();

    if (_blank)
        return;

    uint32_t max =
        *std::max_element(std::begin(_data.buckets), std::end(_data.buckets));
    dc.SetPen(*wxGREY_PEN);
    for (int x = 0; x < 256; x++)
    {
        double v = (double)_data.buckets[x] / (double)max;
        dc.DrawLine({BORDER + x, BORDER + HEIGHT},
            {BORDER + x, BORDER + HEIGHT - (int)(v * HEIGHT)});
    }

    dc.SetPen(*wxBLACK_PEN);
    dc.DrawLine({BORDER, BORDER + HEIGHT}, {BORDER + WIDTH, BORDER + HEIGHT});
    dc.DrawLine({BORDER, BORDER + HEIGHT}, {BORDER, BORDER});

    dc.SetPen(*wxRED_PEN);
    {
        int y = ((double)_data.signalLevel / (double)max) * HEIGHT;
        dc.DrawLine({0, BORDER + HEIGHT - y},
            {2 * BORDER + WIDTH, BORDER + HEIGHT - y});
    }

    if (_clock != 0.0)
    {
        int x = _clock / NS_PER_TICK;
        dc.DrawLine({BORDER + x, 2 * BORDER + HEIGHT}, {BORDER + x, 0});
    }

    {
        wxString text = "Clock interval";
        dc.SetFont(_font);
        auto size = dc.GetTextExtent(text);
        dc.DrawText(text, {BORDER + WIDTH - size.GetWidth(), BORDER + HEIGHT});
    }

    {
        wxString text = "Frequency";
        dc.SetFont(_font);
        auto size = dc.GetTextExtent(text);
        dc.DrawRotatedText(
            text, BORDER - size.GetHeight(), BORDER + size.GetWidth(), 90);
    }
}
