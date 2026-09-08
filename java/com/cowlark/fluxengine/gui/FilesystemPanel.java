package com.cowlark.fluxengine.gui;

import static swingtree.UI.of;
import static swingtree.UIFactoryMethods.scrollPane;

import lombok.With;
import sprouts.HasId;
import sprouts.Tuple;
import sprouts.Val;
import swingtree.UI;
import javax.swing.JPanel;
import java.awt.BorderLayout;

public class FilesystemPanel extends JPanel
{
    private final ImagerViewModel model;


    interface FsNode extends HasId<String>
    {
        String name();
    }

    @With
    record Dir(String id, String name, Tuple<FsNode> entries) implements FsNode
    {
    }

    @With
    record Doc(String id, String name, String body) implements FsNode
    {
    }

    public FilesystemPanel(ImagerViewModel model)
    {
        this.model = model;
        setLayout(new BorderLayout());

        //        Font font = new Font(Font.MONOSPACED, Font.PLAIN, UIScale.scale(14));

        Val<FsNode> fileSystem =
                Val.of(new Dir("id", "name", Tuple.of(new Doc("doc", "doc", "doc"))));

        of(this).withLayout("fill, insets 5").add(
                "grow, push", scrollPane().add(UI.tree(
                                fileSystem, conf -> conf.nodesOf(
                                                Dir.class,
                                                it -> it
                                                        .children(Dir::entries, Dir::withEntries)
                                                        .text(Dir::name, Dir::withName))
                                        //                                                        .icon(d -> Icons.FOLDER)
                                        .nodesOf(Doc.class, it -> it.text(Doc::name)))
                        //.withSelection(selectedPath)                           //
                        // Var<Tuple<String>>: a PATH of ids, two-way
                        .withRootVisible(false).withInitialExpansionDepth(2)));

    }
}
