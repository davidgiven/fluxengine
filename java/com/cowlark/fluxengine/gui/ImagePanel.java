package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.data.Disk;
import com.formdev.flatlaf.util.UIScale;
import org.exbin.auxiliary.binary_data.EmptyBinaryData;
import org.exbin.bined.CodeCharactersCase;
import org.exbin.bined.swing.basic.CodeArea;
import sprouts.From;
import sprouts.Viewable;
import javax.swing.JPanel;
import javax.swing.UIManager;
import javax.swing.border.LineBorder;
import java.awt.BorderLayout;
import java.awt.Font;
import java.nio.charset.StandardCharsets;

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
        codeArea.setBorder(new LineBorder(UIManager.getColor("TextArea.background"), 3));
        codeArea.setCodeFont(font);
        codeArea.setContentData(new EmptyBinaryData());
        codeArea.setCharset(StandardCharsets.ISO_8859_1);
        codeArea.setCodeCharactersCase(CodeCharactersCase.LOWER);
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
