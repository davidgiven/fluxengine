#ifndef VISUALISATIONCONTROL_H
#define VISUALISATIONCONTROL_H

#include <memory>
#include <map>
#include <wx/control.h>

class Sector;
class DiskFlux;
class TrackFlux;

enum
{
    VISMODE_NOTHING,
    VISMODE_READING,
    VISMODE_WRITING
};

class TrackSelectionEvent : public wxEvent
{
public:
    TrackSelectionEvent(wxEventType eventType, int winId):
        wxEvent(winId, eventType)
    {
    }

    wxEvent* Clone() const override
    {
        return new TrackSelectionEvent(*this);
    }

    std::shared_ptr<const TrackFlux> trackFlux;
};

wxDECLARE_EVENT(TRACK_SELECTION_EVENT, TrackSelectionEvent);

class VisualisationControl : public wxWindow
{
public:
    VisualisationControl(wxWindow* parent,
        wxWindowID winid,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0);

public:
    void Clear();
    void SetMode(int head, int track, int mode);
    void SetTrackData(std::shared_ptr<const TrackFlux> track);
    void SetDiskData(std::shared_ptr<const DiskFlux> disk);

private:
    void OnPaint(wxPaintEvent& evt);
    void OnMotion(wxMouseEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeaveWindow(wxMouseEvent& evt);

private:
    typedef std::pair<unsigned, unsigned> key_t;

    int _head;
    int _track;
    int _mode = VISMODE_NOTHING;
    int _selectedHead = -1;
    int _selectedTrack = -1;
    std::multimap<key_t, std::shared_ptr<const Sector>> _sectors;
    std::map<key_t, std::shared_ptr<const TrackFlux>> _tracks;
    wxDECLARE_EVENT_TABLE();
};

#endif
