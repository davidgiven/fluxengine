package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_FLUXFILE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_OPTIONS;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_SERIALPORT;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DRIVE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.FORMAT;
import static java.util.stream.Collectors.toList;
import static javax.swing.JFileChooser.APPROVE_OPTION;

import com.cowlark.fluxengine.algorithms.BeginReadOperationLogMessage;
import com.cowlark.fluxengine.algorithms.BeginWriteOperationLogMessage;
import com.cowlark.fluxengine.algorithms.DiskUpdateLogMessage;
import com.cowlark.fluxengine.algorithms.EndReadOperationLogMessage;
import com.cowlark.fluxengine.algorithms.EndWriteOperationLogMessage;
import com.cowlark.fluxengine.algorithms.ReadWriteFluxOperation;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.config.UsbFinder.CandidateDevice;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogMessage.ErrorLogMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.MemoryFluxSource;
import com.cowlark.fluxengine.gui.DriveActivity.ActivityType;
import com.cowlark.fluxengine.imagewriter.ImageWriter;
import io.reactivex.rxjava3.disposables.Disposable;
import lombok.Getter;
import org.apache.commons.lang3.function.Consumers;
import org.slf4j.LoggerFactory;
import sprouts.Association;
import sprouts.From;
import sprouts.Pair;
import sprouts.Val;
import sprouts.Var;
import sprouts.Vars;
import sprouts.Viewable;
import swingtree.ComponentDelegate;
import swingtree.UI;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import java.awt.event.ActionEvent;
import java.util.function.Supplier;

public class ImagerViewModel
{
    private static final org.slf4j.Logger logger = LoggerFactory.getLogger(ImagerViewModel.class);

    private final PreferencesReaderWriter preferencesReaderWriter;

    @Getter private Var<String> statusMessage = Var.of("Ready");
    @Getter private Var<String> selectedFormat;
    @Getter private Var<String> selectedDevice;
    @Getter private Var<String> selectedFluxFile;
    @Getter private Var<String> selectedSerialPort;
    @Getter private Var<Integer> selectedDrive;

    @Getter private Var<Image> diskImage = Var.of(new Image());
    @Getter private Vars<LogMessage> logQueue = Vars.of(LogMessage.class);
    @Getter private Var<Disposable> currentOperation = Var.ofNull(Disposable.class);
    @Getter private Var<Disk> disk = Var.of(new Disk());
    @Getter private Var<DriveActivity> driveActivity =
            Var.of(new DriveActivity(ActivityType.IDLE, 0, 0));

    @Getter private Val<Boolean> busy =
            currentOperation.viewAs(Boolean.class, op -> op != null && !op.isDisposed());

    @Getter private Var<Association<String, CandidateDevice>> usbDevices =
            Var.of(Association.between(String.class, CandidateDevice.class));

    ImagerViewModel(PreferencesReaderWriter preferencesReaderWriter)
    {
        this.preferencesReaderWriter = preferencesReaderWriter;

        selectedFormat = makeStringPreference(FORMAT, "ibm");
        selectedDevice = makeStringPreference(DEVICE, DEVICE_FLUXFILE);
        selectedFluxFile = makeStringPreference(DEVICE_FLUXFILE, "");
        selectedSerialPort = makeStringPreference(DEVICE_SERIALPORT, "");
        selectedDrive = makeIntegerPreference(DRIVE, 0);

        refreshUsbDevices();
    }

    private Var<String> makeStringPreference(String preferenceName, String defaultValue)
    {
        /* Viewable.cast reinterprets the property itself as a Viewable, so the
         * listener lives exactly as long as the property (unlike view(), which
         * returns a weakly-held view that must be kept in a field). */

        Var<String> value =
                Var.of(preferencesReaderWriter.getStringPreference(preferenceName, defaultValue));
        Viewable.cast(value).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setStringPreference(
                        preferenceName,
                        it.currentValue().orElseThrowUnchecked()));
        return value;
    }


    private Var<Integer> makeIntegerPreference(String preferenceName, int defaultValue)
    {
        /* Viewable.cast reinterprets the property itself as a Viewable, so the
         * listener lives exactly as long as the property (unlike view(), which
         * returns a weakly-held view that must be kept in a field). */

        Var<Integer> value =
                Var.of(preferencesReaderWriter.getIntegerPreference(preferenceName, defaultValue));
        Viewable.cast(value).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setIntegerPreference(
                        preferenceName,
                        it.currentValue().orElseThrowUnchecked()));
        return value;
    }

    Var<Association<String, String>> getOptionsForFormat(String format)
    {
        Var<Association<String, String>> value =
                Var.of(preferencesReaderWriter.getOptionsForFormat(format));
        Viewable.cast(value).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setOptionsForFormat(
                        format,
                        it.currentValue().orElseThrowUnchecked()));
        return value;
    }

    Var<Association<String, String>> getOptionsForDevice()
    {
        Var<Association<String, String>> value =
                Var.of(preferencesReaderWriter.getOptions(DEVICE_OPTIONS));
        Viewable.cast(value).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setOptions(
                        DEVICE_OPTIONS,
                        it.currentValue().orElseThrowUnchecked()));
        return value;
    }

    void refreshUsbDevices()
    {
        usbDevices.set(Association
                .between(
                        String.class,
                        com.cowlark.fluxengine.config.UsbFinder.CandidateDevice.class)
                .put(DEVICE_FLUXFILE, new CandidateDevice())
                .put(DEVICE_SERIALPORT, new CandidateDevice())
                .putAll(UsbFinder
                        .findUsbDevices()
                        .stream()
                        .map(candidate -> Pair.of(candidate.getSerial(), candidate))
                        .collect(toList())));
    }

    void onReadDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        performOperation(
                this::makeConfigBuilder, new ReadWriteFluxOperation()
                {
                    @Override
                    public void run()
                    {
                        Disk disk = new Disk();
                        readDisk(disk);
                    }
                });
    }

    void onRereadDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        Disk disk = getDisk().get();

        performOperation(
                this::makeConfigBuilder, new ReadWriteFluxOperation()
                {
                    @Override
                    public void run()
                    {
                        readDisk(disk);
                    }
                });
    }

    void onLoadDiskImage(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setDialogTitle("Load image file");
        fileChooser.setApproveButtonText("Load");
        if (fileChooser.showOpenDialog(null) != APPROVE_OPTION)
            return;

        performOperation(
                () -> makeConfigBuilderWithFormat().withImageReader(fileChooser
                        .getSelectedFile()
                        .getPath()), new ReadWriteFluxOperation()
                {
                    @Override
                    public void run()
                    {
                        Disk disk = new Disk();
                        disk.diskLayout = getDiskLayout();
                        disk.image = getImageReader().readImage();
                        disk.image.populateSectorPhysicalLocationsFromLogicalLocations(disk.diskLayout);
                        Logger.log(new DiskUpdateLogMessage(disk));
                    }
                });
    }

    void onSaveDiskImage(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        Image image = getDisk().get().image;

        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setDialogTitle("Save image file");
        fileChooser.setApproveButtonText("Save");
        if (fileChooser.showSaveDialog(null) != APPROVE_OPTION)
            return;

        performOperation(
                () -> makeConfigBuilder().withImageWriter(fileChooser
                        .getSelectedFile()
                        .getPath()
                        .toString()), new ReadWriteFluxOperation()
                {
                    @Override
                    public void run()
                    {
                        ImageWriter.create(getConfig()).writeImage(image);
                    }
                });
    }

    void onSaveDiskFlux(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        Disk disk = getDisk().get();
        FluxSource fluxSource = new MemoryFluxSource(disk);

        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setDialogTitle("Save flux file");
        fileChooser.setApproveButtonText("Save");
        if (fileChooser.showSaveDialog(null) != APPROVE_OPTION)
            return;

        performOperation(
                () -> makeConfigBuilder().withFluxSink(fileChooser
                        .getSelectedFile()
                        .getPath()
                        .toString()), new ReadWriteFluxOperation()
                {
                    @Override
                    public FluxSource getFluxSource()
                    {
                        return fluxSource;
                    }

                    @Override
                    public void run()
                    {
                        rawWrite();
                    }
                });
    }

    void onWriteDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        Image image = getDisk().get().image;

        performOperation(
                this::makeConfigBuilder, new ReadWriteFluxOperation()
                {
                    @Override
                    public void run()
                    {
                        Disk disk = new Disk();
                        disk.diskLayout = getDiskLayout();
                        disk.image = image;
                        writeDisk(disk);
                    }
                });
    }

    void onEmergencyStop(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        Disposable operation = getCurrentOperation().get();
        if ((operation == null) || operation.isDisposed())
            return;

        ReadWriteFluxOperation.requestEmergencyStop();
        operation.dispose();

        /* Only clear the property once the dispose has actually happened;
         * dispose() blocks until the worker thread has exited. */
        getCurrentOperation().set(From.VIEW_MODEL, null);
        getDriveActivity().set(new DriveActivity(ActivityType.IDLE, 0, 0));
    }

    private void performOperation(
            Supplier<ConfigBuilder> configBuilderSupplier,
            ReadWriteFluxOperation operation)
    {
        ConfigProto config;
        try
        {
            config = configBuilderSupplier.get().build();
        } catch (FluxEngineException e)
        {
            UI.message(String.format("Configuration error: %s", e.getMessage())).showAsError();
            return;
        }

        currentOperation.set(operation.setConfig(config).create().observeOn(UiUtils.EDT).subscribe(
                this::handleLogMessage, e -> {
                    currentOperation.set(From.VIEW_MODEL, null);
                    ErrorLogMessage m = new ErrorLogMessage(e.getMessage());
                    logQueue.add(m);
                    showFatalError(m);
                }, () -> currentOperation.set(From.VIEW_MODEL, null)));
    }

    private ConfigBuilder makeConfigBuilderWithFormat()
    {
        Logger.setLogger(Consumers.nop());
        ConfigBuilder builder = new ConfigBuilder();
        builder.loadConfigFile("_global_options");
        builder.loadConfigFile(selectedFormat.get());

        for (Pair<String, String> e : getOptionsForFormat(selectedFormat.get()).get())
            builder.applyOption(e.first(), e.second());

        return builder;
    }

    private ConfigBuilder makeConfigBuilder()
    {
        ConfigBuilder builder = makeConfigBuilderWithFormat();

        for (Pair<String, String> e : getOptionsForDevice().get())
            builder.applyOption(e.first(), e.second());

        String device = getSelectedDevice().get();
        if (device.equals(DEVICE_SERIALPORT))
            builder.set("usb.port", device);
        else if (device.equals(DEVICE_FLUXFILE))
        {
            String fluxfile = getSelectedFluxFile().get();
            builder.withFluxSource(fluxfile);
            builder.withFluxSink(fluxfile);
        }

        if (!device.equals(DEVICE_FLUXFILE))
        {
            String drive = String.format("drive:%d", getSelectedDrive().get());
            builder.withFluxSource(drive);
            builder.withFluxSink(drive);
        }

        return builder;
    }

    private void handleLogMessage(LogMessage message)
    {
        switch (message)
        {
            case DiskUpdateLogMessage m -> getDisk().set(From.VIEW_MODEL, m.disk());

            case BeginReadOperationLogMessage m -> getDriveActivity().set(new DriveActivity(
                    ActivityType.READING,
                    m.track(),
                    m.head()));

            case BeginWriteOperationLogMessage m -> getDriveActivity().set(new DriveActivity(
                    ActivityType.WRITING,
                    m.track(),
                    m.head()));

            case EndReadOperationLogMessage ignored ->
                    getDriveActivity().set(new DriveActivity(ActivityType.IDLE, 0, 0));

            case EndWriteOperationLogMessage ignored ->
                    getDriveActivity().set(new DriveActivity(ActivityType.IDLE, 0, 0));

            case ErrorLogMessage m -> showFatalError(m);

            default ->
            {
            }
        }
        logQueue.add(message);
    }

    private static void showFatalError(ErrorLogMessage m)
    {
        UI.message(String.format("Error: %s", m.message())).showAsError();
    }
}
