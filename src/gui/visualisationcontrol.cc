#include "globals.h"
#include "gui.h"
#include "visualisationcontrol.h"
#include "fluxmap.h"
#include "flux.h"
#include "sector.h"
#include "image.h"
#include "fmt/format.h"

#define BORDER 20
#define TICK 3
#define TRACKS 82

#define SECTORSIZE 5

wxDEFINE_EVENT(TRACK_SELECTION_EVENT, TrackSelectionEvent);

DECLARE_COLOUR(AXIS, 128, 128, 128);
DECLARE_COLOUR(GOOD_SECTOR, 0, 158, 115);
DECLARE_COLOUR(BAD_SECTOR, 213, 94, 0);
DECLARE_COLOUR(MISSING_SECTOR, 86, 180, 233);
DECLARE_COLOUR(READ_ARROW, 0, 128, 0);
DECLARE_COLOUR(WRITE_ARROW, 128, 0, 0);
DECLARE_COLOUR(SELECTION_BOX, 64, 64, 255);

VisualisationControl::VisualisationControl(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    long style):
    wxWindow(parent, id, pos, size, style, "VisualisationControl")
{
	SetDoubleBuffered(true);
}

wxBEGIN_EVENT_TABLE(VisualisationControl, wxPanel)
    EVT_PAINT(VisualisationControl::OnPaint)
	EVT_MOTION(VisualisationControl::OnMotion)
	EVT_LEFT_DOWN(VisualisationControl::OnLeftDown)
	EVT_LEAVE_WINDOW(VisualisationControl::OnLeaveWindow)
wxEND_EVENT_TABLE()

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
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();

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

        int y = scaletop + _track * SECTORSIZE;
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

	if (_selectedTrack != -1)
	{
		int x = (_selectedHead ? (w2-1) : 0) + SECTORSIZE;
        int y = scaletop + _selectedTrack * SECTORSIZE - 1;
		int bw = w/2 - SECTORSIZE*2 + 2;
		int bh = SECTORSIZE + 3;
		dc.SetPen(SELECTION_BOX_PEN);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle({x, y-1, bw, bh});

		static wxFont font(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
		dc.SetFont(font);
		dc.SetBackgroundMode(wxTRANSPARENT);
		dc.SetTextForeground(*wxBLACK);

		auto centreText = [&](const std::string& text, int y)
		{
			auto size = dc.GetTextExtent(text);
			dc.DrawText(text, { w2 - size.x/2 , y });
		};

		centreText(
			fmt::format("physical: {}.{}", _selectedTrack, _selectedHead),
			scalebottom + 5);

		key_t key = {_selectedTrack, _selectedHead};
        auto it = _tracks.find(key);
		std::string logicalText = "logical: (none)";
		if (it != _tracks.end())
			logicalText = fmt::format(
					"logical: {}.{}",
					it->second->location.logicalTrack,
					it->second->location.head);

		centreText(logicalText, scalebottom + 15);
	}
}

void VisualisationControl::OnMotion(wxMouseEvent& event)
{
    auto size = GetSize();
    int w = size.GetWidth();
    int w2 = w / 2;
    int h = size.GetHeight();

    int centrey = h * 1.5;
    int scalesize = TRACKS * SECTORSIZE;
    int scaletop = h / 2 - scalesize / 2;
    int scalebottom = scaletop + scalesize - 1;

	int headno = event.GetX() > w2;

	int trackno = (event.GetY() - scaletop) / SECTORSIZE;
	if ((trackno < 0) || (trackno >= TRACKS))
		trackno = -1;
	if ((_selectedHead != headno) || (_selectedTrack != trackno))
	{
		_selectedTrack = trackno;
		_selectedHead = headno;
		Refresh();
	}
}

void VisualisationControl::OnLeftDown(wxMouseEvent& event)
{
	OnMotion(event);

	if ((_selectedHead != -1) && (_selectedTrack != -1))
	{
		key_t key = {_selectedTrack, _selectedHead};
        auto it = _tracks.find(key);
		if (it != _tracks.end())
		{
			TrackSelectionEvent event(TRACK_SELECTION_EVENT, GetId());
			event.SetEventObject(this);
			event.trackFlux = it->second;
			ProcessWindowEvent(event);
		}
	}
    else
        event.Skip();
}

void VisualisationControl::OnLeaveWindow(wxMouseEvent&)
{
	_selectedTrack = _selectedHead = -1;
	Refresh();
}

void VisualisationControl::SetMode(int track, int head, int mode)
{
    _track = track;
    _head = head;
    _mode = mode;
    Refresh();
}

void VisualisationControl::Clear()
{
    _sectors.clear();
	_tracks.clear();
    Refresh();
}

void VisualisationControl::SetTrackData(std::shared_ptr<const TrackFlux> track)
{
    key_t key = {track->location.physicalTrack, track->location.head};
	_tracks[key] = track;
    _sectors.erase(key);
    for (auto& sector : track->sectors)
        _sectors.insert({key, sector});

    Refresh();
}

void VisualisationControl::SetDiskData(std::shared_ptr<const DiskFlux> disk)
{
	_sectors.clear();
	for (const auto& track : disk->tracks)
	{
		key_t key = {track->location.physicalTrack, track->location.head};
		_tracks[key] = track;
	}

	for (const auto& sector : *(disk->image))
	{
		key_t key = {sector->physicalTrack, sector->physicalHead};
		_sectors.insert({key, sector});
	}

    Refresh();
}
