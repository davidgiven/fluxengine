package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.data.Disk;
import com.formdev.flatlaf.util.UIScale;
import org.exbin.auxiliary.binary_data.EmptyBinaryData;
import org.exbin.bined.swing.basic.CodeArea;
import sprouts.From;
import sprouts.Viewable;
import javax.swing.JLabel;
import javax.swing.JPanel;
import java.awt.BorderLayout;
import java.awt.Font;

/**
 * A panel which displays the raw bytes of the loaded disk image in a hex
 * editor, ported from the original imager's image view.
 */
public class ImagePanel extends JPanel
{
    private final ImagerViewModel model;
    private final CodeArea codeArea;

    public ImagePanel(ImagerViewModel model)
    {
        this.model = model;
        setLayout(new BorderLayout());

        Font font = new Font(Font.MONOSPACED, Font.PLAIN, UIScale.scale(14));

        codeArea = new CodeArea();
        codeArea.setCodeFont(font);
        codeArea.setContentData(new EmptyBinaryData());
        add(codeArea, BorderLayout.CENTER);

        Viewable.cast(model.getDisk()).onChange(From.VIEW_MODEL, it -> updateContent());
        updateContent();
    }

    private void updateContent()
    {
        Disk disk = model.getDisk().get();
        codeArea.setContentData(new ImageBinaryData(disk));
    }

}
