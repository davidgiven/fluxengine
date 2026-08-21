package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_FLUXFILE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DEVICE_MANUAL;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.DRIVE;
import static com.cowlark.fluxengine.gui.PreferencesReaderWriter.FORMAT;
import static java.util.stream.Collectors.toList;

import com.cowlark.fluxengine.algorithms.BeginReadOperationLogMessage;
import com.cowlark.fluxengine.algorithms.BeginWriteOperationLogMessage;
import com.cowlark.fluxengine.algorithms.DiskUpdateLogMessage;
import com.cowlark.fluxengine.algorithms.EndReadOperationLogMessage;
import com.cowlark.fluxengine.algorithms.EndWriteOperationLogMessage;
import com.cowlark.fluxengine.algorithms.ReadWriteFluxOperation;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.config.UsbFinder.CandidateDevice;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogMessage.ErrorLogMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.gui.DriveActivity.ActivityType;
import io.reactivex.rxjava3.disposables.Disposable;
import lombok.Getter;
import org.apache.commons.lang3.function.Consumers;
import sprouts.Association;
import sprouts.From;
import sprouts.Pair;
import sprouts.Val;
import sprouts.Var;
import sprouts.Vars;
import sprouts.Viewable;
import swingtree.ComponentDelegate;
import javax.swing.JButton;
import java.awt.event.ActionEvent;

public class ImagerViewModel
{
    private final PreferencesReaderWriter preferencesReaderWriter;

    @Getter private Var<String> statusMessage = Var.of("Ready");
    @Getter private Var<String> selectedFormat;
    @Getter private Var<String> selectedDevice;
    @Getter private Var<Integer> selectedDrive;
    @Getter private Var<Association<String, String>> options =
            Var.of(Association.between(String.class, String.class));
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

        selectedFormat = Var.of(preferencesReaderWriter.getStringPreference(FORMAT, "ibm"));
        selectedDevice =
                Var.of(preferencesReaderWriter.getStringPreference(DEVICE, DEVICE_FLUXFILE));
        selectedDrive = Var.of(preferencesReaderWriter.getIntegerPreference(DRIVE, 0));
        options.set(preferencesReaderWriter.getOptionsForFormat(selectedFormat.get()));

        refreshUsbDevices();

        /* Viewable.cast reinterprets the property itself as a Viewable, so the
         * listener lives exactly as long as the property (unlike view(), which
         * returns a weakly-held view that must be kept in a field). */
        Viewable.cast(selectedFormat).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setStringPreference(
                        FORMAT,
                        it.currentValue().orElseThrowUnchecked()));
        Viewable.cast(selectedDevice).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setStringPreference(
                        DEVICE,
                        it.currentValue().orElseThrowUnchecked()));
        Viewable.cast(selectedDrive).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setIntegerPreference(
                        DRIVE,
                        it.currentValue().orElseThrowUnchecked()));
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
                Var.of(preferencesReaderWriter.getOptions(PreferencesReaderWriter.DEVICE_OPTIONS));
        Viewable.cast(value).onChange(
                From.VIEW,
                it -> preferencesReaderWriter.setOptions(
                        PreferencesReaderWriter.DEVICE_OPTIONS,
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
                .put(DEVICE_MANUAL, new CandidateDevice())
                .putAll(UsbFinder
                        .findUsbDevices()
                        .stream()
                        .map(candidate -> Pair.of(candidate.getSerial(), candidate))
                        .collect(toList())));
    }

    void onReadDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
        class ReadOperation extends ReadWriteFluxOperation
        {
            @Override
            public void run()
            {
                Disk disk = new Disk();
                readDisk(disk);
            }
        }

        ConfigBuilder builder = buildConfig();

        String device = getSelectedDevice().get();
        if (device.equals(DEVICE_FLUXFILE))
        {
        } else
        {
            builder.set("usb.serial", device);

            String drive = String.format("drive:%d", getSelectedDrive().get());
            builder.withFluxSource(drive);
            builder.withFluxSink(drive);
        }

        currentOperation.set(new ReadOperation()
                .setConfig(builder.build())
                .create()
                .observeOn(UiUtils.EDT)
                .subscribe(
                        this::handleLogMessage, e -> {
                            logQueue.add(new ErrorLogMessage(e.getMessage()));
                            currentOperation.fireChange(From.VIEW_MODEL);
                        }, () -> currentOperation.fireChange(From.VIEW_MODEL)));
    }

    void onRereadDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
    }

    void onWriteDisk(ComponentDelegate<JButton, ActionEvent> delegate)
    {
    }

    void onEmergencyStop(ComponentDelegate<JButton, ActionEvent> delegate)
    {
    }

    void onUseFluxFile(ComponentDelegate<JButton, ActionEvent> delegate)
    {
    }

    private ConfigBuilder buildConfig()
    {
        Logger.setLogger(Consumers.nop());
        ConfigBuilder builder = new ConfigBuilder();
        builder.loadConfigFile("_global_options");
        builder.loadConfigFile(selectedFormat.get());

        for (Pair<String, String> e : getOptionsForFormat(selectedFormat.get()).get())
            builder.applyOption(e.first(), e.second());
        for (Pair<String, String> e : getOptionsForDevice().get())
            builder.applyOption(e.first(), e.second());

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

            default ->
            {
            }
        }
        logQueue.add(message);
    }
}
