#include "lib/core/globals.h"
#include "lib/usb/usbfinder.h"
#include "lib/config/config.h"
#include "globals.h"
#include "imagecomponent.h"
#include "mainwindow.h"
#include "imagevisualiserwidget.h"

class ImageComponentImpl : public ImageComponent, public QObject
{
    W_OBJECT(ImageComponentImpl)

public:
    ImageComponentImpl(MainWindow* mainWindow): _mainWindow(mainWindow)
    {
        setParent(mainWindow);

        _imageVisualiserWidget = ImageVisualiserWidget::create();
        _mainWindow->imageViewContainer->layout()->addWidget(
            _imageVisualiserWidget);
    }

public:
    void setTrackData(std::shared_ptr<const TrackFlux> track)
    {
        _imageVisualiserWidget->setTrackData(track);
    }

    void setDiskData(std::shared_ptr<const DiskFlux> disk)
    {
        //_fluxVisualiserWidget->setDiskData(disk);
    }

private:
    MainWindow* _mainWindow;
    ImageVisualiserWidget* _imageVisualiserWidget;
};
W_OBJECT_IMPL(ImageComponentImpl)

ImageComponent* ImageComponent::create(MainWindow* mainWindow)
{
    return new ImageComponentImpl(mainWindow);
}
