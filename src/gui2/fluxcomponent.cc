#include "lib/globals.h"
#include "lib/usb/usbfinder.h"
#include "globals.h"
#include "fluxcomponent.h"
#include "mainwindow.h"
#include "fluxvisualiserwidget.h"

class FluxComponentImpl : public FluxComponent, public QObject
{
    W_OBJECT(FluxComponentImpl)

public:
    FluxComponentImpl(MainWindow* mainWindow): _mainWindow(mainWindow)
    {
        setParent(mainWindow);

        _fluxVisualiserWidget = FluxVisualiserWidget::create();
        _mainWindow->fluxViewContainer->layout()->addWidget(
            _fluxVisualiserWidget);
        _fluxVisualiserWidget->refresh();
    }

private:
    MainWindow* _mainWindow;
    FluxVisualiserWidget* _fluxVisualiserWidget;
};
W_OBJECT_IMPL(FluxComponentImpl)

FluxComponent* FluxComponent::create(MainWindow* mainWindow)
{
    return new FluxComponentImpl(mainWindow);
}
