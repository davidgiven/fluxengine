#include "lib/globals.h"
#include "lib/fluxmap.h"
#include "lib/environment.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/encoders/encoders.h"
#include "lib/decoders/decoders.h"
#include "lib/proto.h"
#include "lib/readerwriter.h"
#include "gui.h"
#include "layout.h"
#include "fluxviewerwindow.h"
#include "jobqueue.h"

class ImagerPanelImpl : public ImagerPanelGen, public ImagerPanel, JobQueue
{
private:
    enum
    {
        STATE_DEAD,
        STATE_READING_WORKING,
        STATE_READING_SUCCEEDED,
        STATE_READING_FAILED,
        STATE_WRITING_WORKING,
        STATE_WRITING_SUCCEEDED,
        STATE_WRITING_FAILED,
    };

public:
    ImagerPanelImpl(MainWindow* mainWindow, wxSimplebook* parent):
        ImagerPanelGen(parent),
        ImagerPanel(mainWindow)
    {
        visualiser->Bind(
            TRACK_SELECTION_EVENT, &ImagerPanelImpl::OnTrackSelection, this);

        parent->AddPage(this, "imager");
    }

    void OnBackButton(wxCommandEvent&) override
    {
        StartIdle();
    }

private:
    void SetState(int state)
    {
        _state = state;
        CallAfter(
            [&]()
            {
                UpdateState();
            });
    }

    void SwitchFrom() override
    {
        SetState(STATE_DEAD);
    }

    void OnQueueEmpty() override
    {
        if (_state == STATE_READING_WORKING)
            _state = STATE_READING_SUCCEEDED;
        else if (_state == STATE_WRITING_WORKING)
            _state = STATE_WRITING_SUCCEEDED;
        UpdateState();
    }

    void OnQueueFailed() override
    {
        if (_state == STATE_READING_WORKING)
            _state = STATE_READING_FAILED;
        else if (_state == STATE_WRITING_WORKING)
            _state = STATE_WRITING_FAILED;
        UpdateState();
    }

public:
    void StartReading()
    {
        try
        {
            SetPage(MainWindow::PAGE_IMAGER);
            PrepareConfig();

            visualiser->Clear();
            _currentDisk = nullptr;

            SetState(STATE_READING_WORKING);
            QueueJob(
                [this]()
                {
                    /* You need to call this if the config changes to invalidate
                     * any caches. */

                    Environment::reset();

                    auto fluxSource =
                        FluxSource::create(globalConfig().flux_source());
                    auto decoder = Decoder::create(globalConfig().decoder());
                    auto diskflux = readDiskCommand(*fluxSource, *decoder);

                    runOnUiThread(
                        [&]()
                        {
                            visualiser->SetDiskData(diskflux);
                        });
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
            if (_state == STATE_READING_WORKING)
                SetState(STATE_READING_FAILED);
        }
    }

    void StartWriting() override
    {
        try
        {
            SetPage(MainWindow::PAGE_IMAGER);
            PrepareConfig();
            if (!globalConfig().has_image_reader())
                error("This format cannot be read from images.");

            auto filename = wxFileSelector("Choose a image file to read",
                /* default_path= */ wxEmptyString,
                /* default_filename= */
                globalConfig()
                    .image_reader()
                    .filename(),
                /* default_extension= */ wxEmptyString,
                /* wildcard= */ wxEmptyString,
                /* flags= */ wxFD_OPEN | wxFD_FILE_MUST_EXIST);
            if (filename.empty())
                return;

            ImageReader::updateConfigForFilename(
                globalConfig().mutable_image_reader(), filename.ToStdString());
            ImageWriter::updateConfigForFilename(
                globalConfig().mutable_image_writer(), filename.ToStdString());
            visualiser->Clear();
            _currentDisk = nullptr;

            SetState(STATE_WRITING_WORKING);
            QueueJob(
                [this]()
                {
                    auto image =
                        ImageReader::create(globalConfig().image_reader())
                            ->readMappedImage();
                    auto encoder = Encoder::create(globalConfig().encoder());
                    auto fluxSink =
                        FluxSink::create(globalConfig().flux_sink());

                    std::unique_ptr<Decoder> decoder;
                    std::unique_ptr<FluxSource> fluxSource;
                    if (globalConfig().has_decoder())
                    {
                        decoder = Decoder::create(globalConfig().decoder());
                        fluxSource =
                            FluxSource::create(globalConfig().flux_source());
                    }

                    writeDiskCommand(*image,
                        *encoder,
                        *fluxSink,
                        decoder.get(),
                        fluxSource.get());
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
            if (_state == STATE_WRITING_WORKING)
                SetState(STATE_WRITING_FAILED);
        }
    }

    void UpdateState()
    {
        switch (_state)
        {
            case STATE_READING_WORKING:
            case STATE_READING_SUCCEEDED:
            case STATE_READING_FAILED:
                imagerSaveImageButton->Enable(
                    _state == STATE_READING_SUCCEEDED);
                imagerSaveFluxButton->Enable(_state == STATE_READING_SUCCEEDED);
                imagerGoAgainButton->Enable(_state != STATE_READING_WORKING);

                imagerToolbar->EnableTool(
                    imagerBackTool->GetId(), _state != STATE_READING_WORKING);
                break;

            case STATE_WRITING_WORKING:
            case STATE_WRITING_SUCCEEDED:
            case STATE_WRITING_FAILED:
                imagerSaveImageButton->Enable(false);
                imagerSaveFluxButton->Enable(false);
                imagerGoAgainButton->Enable(_state != STATE_WRITING_WORKING);

                imagerToolbar->EnableTool(
                    imagerBackTool->GetId(), _state != STATE_WRITING_WORKING);
                break;
        }
        imagerToolbar->Refresh();
    }

public:
    void SetVisualiserMode(int head, int track, int mode) override
    {
        visualiser->SetMode(head, track, mode);
    }

    void SetVisualiserTrackData(
        std::shared_ptr<const TrackFlux> trackdata) override
    {
        visualiser->SetTrackData(trackdata);
        if (!trackdata->trackDatas.empty())
            histogram->Redraw(*(*trackdata->trackDatas.begin())->fluxmap, 0);
    }

    void SetDisk(std::shared_ptr<const DiskFlux> diskdata) override
    {
        _currentDisk = diskdata;
    }

    void OnImagerGoAgainButton(wxCommandEvent& event) override
    {
        switch (_state)
        {
            case STATE_READING_SUCCEEDED:
            case STATE_READING_FAILED:
                StartReading();
                break;

            case STATE_WRITING_SUCCEEDED:
            case STATE_WRITING_FAILED:
                StartWriting();
                break;
        }
    }

    void OnSaveImageButton(wxCommandEvent&) override
    {
        try
        {
            if (!globalConfig().has_image_writer())
                error("This format cannot be saved.");

            auto filename =
                wxFileSelector("Choose the name of the image file to write",
                    /* default_path= */ wxEmptyString,
                    /* default_filename= */
                    globalConfig()
                        .image_writer()
                        .filename(),
                    /* default_extension= */ wxEmptyString,
                    /* wildcard= */ wxEmptyString,
                    /* flags= */ wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (filename.empty())
                return;

            ImageWriter::updateConfigForFilename(
                globalConfig().mutable_image_writer(), filename.ToStdString());

            auto image = _currentDisk->image;

            QueueJob(
                [image, this]()
                {
                    auto imageWriter =
                        ImageWriter::create(globalConfig().image_writer());
                    imageWriter->writeMappedImage(*image);
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnSaveFluxButton(wxCommandEvent&) override
    {
        try
        {
            auto filename =
                wxFileSelector("Choose the name of the flux file to write",
                    /* default_path= */ wxEmptyString,
                    /* default_filename= */ "image.flux",
                    /* default_extension= */ wxEmptyString,
                    /* wildcard= */ wxEmptyString,
                    /* flags= */ wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
            if (filename.empty())
                return;

            FluxSink::updateConfigForFilename(
                globalConfig().mutable_flux_sink(), filename.ToStdString());

            QueueJob(
                [this]()
                {
                    auto fluxSource =
                        FluxSource::createMemoryFluxSource(*_currentDisk);
                    auto fluxSink =
                        FluxSink::create(globalConfig().flux_sink());
                    writeRawDiskCommand(*fluxSource, *fluxSink);
                });
        }
        catch (const ErrorException& e)
        {
            wxMessageBox(e.message, "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnTrackSelection(TrackSelectionEvent& event)
    {
        (new FluxViewerWindow(this, event.trackFlux))->Show(true);
    }

private:
    int _state = STATE_DEAD;
    std::shared_ptr<const DiskFlux> _currentDisk;
};

ImagerPanel* ImagerPanel::Create(MainWindow* mainWindow, wxSimplebook* parent)
{
    return new ImagerPanelImpl(mainWindow, parent);
}
