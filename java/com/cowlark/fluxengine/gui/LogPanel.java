package com.cowlark.fluxengine.gui;

import static swingtree.UI.of;
import static swingtree.UI.scrollPane;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;
import com.cowlark.fluxengine.core.PrintingLogRenderer;
import io.reactivex.rxjava3.core.Observable;
import io.reactivex.rxjava3.disposables.Disposable;
import sprouts.Var;
import sprouts.Vars;
import sprouts.Viewables;
import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.SwingUtilities;
import java.awt.Font;
import java.io.OutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;

public class LogPanel extends JPanel
{
    private final ImagerViewModel model;
    private final JTextArea textArea;
    private final LogRenderer printingLogRenderer;

    public LogPanel(ImagerViewModel model)
    {
        this.model = model;
        textArea = new JTextArea();
        textArea.setEditable(false);
        textArea.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 16));

        of(this).withLayout("fill, insets 5").add("grow, push", scrollPane().add(of(textArea)));

        printingLogRenderer = new PrintingLogRenderer(getPrintStream());

        Viewables.cast(model.getLogQueue()).onChange(it -> {
            Vars<LogMessage> queue = model.getLogQueue();
            while (queue.isNotEmpty())
                printingLogRenderer.add(model.getLogQueue().popFirst().get());
        });
    }

    /* Appends the given text to the log, scrolling to show it. */
    public void add(String text)
    {
        textArea.append(text);
        textArea.setCaretPosition(textArea.getDocument().getLength());
    }

    /* Returns a PrintStream which appends whatever is printed to it to the
     * log, on the Swing event thread. */
    public PrintStream getPrintStream()
    {
        return new PrintStream(new OutputStream()
        {
            @Override
            public void write(int b)
            {
                write(new byte[]{(byte) b}, 0, 1);
            }

            @Override
            public void write(byte[] b, int off, int len)
            {
                String text = new String(b, off, len, StandardCharsets.UTF_8);
                SwingUtilities.invokeLater(() -> add(text));
            }
        });
    }

    /* Appends the text contained in each incoming panel to the log, on the
     * Swing event thread. */
    public void observe(Observable<LogMessage> messages)
    {
        Disposable disposable = messages.subscribe(
                panel -> SwingUtilities.invokeLater(() -> printingLogRenderer.add(panel)));
    }
}
