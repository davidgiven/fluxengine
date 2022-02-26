#include "globals.h"
#include "gui.h"
#include "visualisation.h"
#include "fluxmap.h"
#include "flux.h"
#include "sector.h"
#include "fmt/format.h"

#define BORDER 20
#define TICK 3
#define TRACKS 82

#define SECTORSIZE 5

#define DECLARE_COLOUR(name, red, green, blue)             \
    static const wxColour name##_COLOUR(red, green, blue); \
    static const wxBrush name##_BRUSH(name##_COLOUR);      \
    static const wxPen name##_PEN(name##_COLOUR)

DECLARE_COLOUR(AXIS, 128, 128, 128);
DECLARE_COLOUR(GOOD_SECTOR, 0, 158, 115);
DECLARE_COLOUR(BAD_SECTOR, 213, 94, 0);
DECLARE_COLOUR(MISSING_SECTOR, 86, 180, 233);
DECLARE_COLOUR(READ_ARROW, 0, 128, 0);
DECLARE_COLOUR(WRITE_ARROW, 128, 0, 0);

VisualisationControl::VisualisationControl(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style):
    wxWindow(parent, id, pos, size, style, "VisualisationControl")
{
}

wxBEGIN_EVENT_TABLE(VisualisationControl, wxPanel)
    EVT_PAINT(VisualisationControl::OnPaint) wxEND_EVENT_TABLE()

        void VisualisationControl::OnPaint(wxPaintEvent&)
{
    auto size = GetSize();
    int w = size.GetWidth();
    int w2 = w / 2;
    int h = size.GetHeight();

    int centrey = h * 1.5;
    int outerradius = centrey - BORDER;
    int innerradius = centrey - h + BORDER;
    int scalesize = TRACKS * SECTORSIZE;
    int scaletop = h / 2 - scalesize / 2;
    int scalebottom = scaletop + scalesize - 1;

    wxPaintDC dc(this);
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxLIGHT_GREY_BRUSH);
    dc.DrawCircle({w2, centrey}, outerradius);
    dc.SetBrush(dc.GetBackground());
    dc.DrawCircle({w2, centrey}, innerradius);

    dc.SetPen(AXIS_PEN);
    dc.DrawLine({w2, scaletop}, {w2, scalebottom});

    if (_mode != VISMODE_NOTHING)
    {
        if (_mode == VISMODE_READING)
		{
			dc.SetPen(READ_ARROW_PEN);
			dc.SetBrush(READ_ARROW_BRUSH);
		}
        else if (_mode == VISMODE_WRITING)
		{
			dc.SetPen(WRITE_ARROW_PEN);
			dc.SetBrush(WRITE_ARROW_BRUSH);
		}

        int factor = (_head == 0) ? -1 : 1;

        int y = scaletop + _cylinder * SECTORSIZE;
		wxPoint points[] = {
			{ w2 + factor*TICK, y-1 },
			{ w2 + factor*TICK, y+SECTORSIZE-1 },
			{ w2 + factor*TICK*2, y+SECTORSIZE/2 }
		};
		dc.DrawPolygon(3, points);
    }

    for (int track = 0; track <= TRACKS; track++)
    {
        int y = scaletop + track * SECTORSIZE;
        dc.SetBrush(AXIS_BRUSH);
        dc.SetPen(AXIS_PEN);
        dc.DrawLine({w2 - TICK, y-1}, {w2 + TICK, y-1});

        auto drawSectors = [&](int head)
        {
            key_t key = {track, head};
            std::vector<std::shared_ptr<const Sector>> sectors;
            for (auto it = _sectors.lower_bound(key);
                 it != _sectors.upper_bound(key);
                 it++)
                sectors.push_back(it->second);
            std::sort(
                sectors.begin(), sectors.end(), sectorPointerSortPredicate);

            int x = 1;
            for (const auto& sector : sectors)
            {
                if (sector->status == Sector::OK)
                {
                    dc.SetBrush(GOOD_SECTOR_BRUSH);
                    dc.SetPen(GOOD_SECTOR_PEN);
                }
                else if (sector->status == Sector::MISSING)
                {
                    dc.SetBrush(MISSING_SECTOR_BRUSH);
                    dc.SetPen(MISSING_SECTOR_PEN);
                }
                else
                {
                    dc.SetBrush(BAD_SECTOR_BRUSH);
                    dc.SetPen(BAD_SECTOR_PEN);
                }

                if (head == 0)
                    dc.DrawRectangle(
                        {w2 - x * SECTORSIZE - (SECTORSIZE - 1), y},
                        {SECTORSIZE - 1, SECTORSIZE - 1});
                else
                    dc.DrawRectangle({w2 + x * SECTORSIZE + 1, y},
                        {SECTORSIZE - 1, SECTORSIZE - 1});
                x++;
            }
        };

        drawSectors(0);
        drawSectors(1);
    }
}

void VisualisationControl::SetMode(int cylinder, int head, int mode)
{
    _cylinder = cylinder;
    _head = head;
    _mode = mode;
    Refresh();
}

void VisualisationControl::Clear()
{
    _sectors.clear();
    Refresh();
}

void VisualisationControl::SetTrackData(std::shared_ptr<const TrackFlux> track)
{
    key_t key = {track->physicalCylinder, track->physicalHead};
    _sectors.erase(key);
    for (auto& sector : track->sectors)
        _sectors.insert({key, sector});

    Refresh();
}

void VisualisationControl::SetDiskData(std::shared_ptr<const DiskFlux> disk)
{
    Refresh();
}
