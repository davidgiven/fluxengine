#include "lib/globals.h"
#include "lib/usb/usbfinder.h"
#include "globals.h"
#include "fluxcomponent.h"
#include "mainwindow.h"
#include "fluxvisualiserwidget.h"
#include "fluxOverlayForm.h"

class FluxComponentImpl :
    public FluxComponent,
    public QObject,
    public Ui_fluxOverlayForm
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

        connect(_mainWindow->fluxSideComboBox,
            QOverload<int>::of(&QComboBox::activated),
            _fluxVisualiserWidget,
            &FluxVisualiserWidget::setVisibleSide);
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