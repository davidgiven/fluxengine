package com.cowlark.fluxengine.gui;

import javafx.application.Application;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.scene.layout.StackPane;
import javafx.stage.Stage;

/**
 * The FluxEngine GUI, ported from src/gui/main.cc.
 */
public class Gui extends Application
{
    public static void main(String[] args)
    {
        launch(Gui.class, args);
    }

    @Override
    public void start(Stage stage)
    {
        Label label = new Label("FluxEngine");
        StackPane root = new StackPane(label);
        Scene scene = new Scene(root, 800, 600);

        stage.setTitle("FluxEngine");
        stage.setScene(scene);
        stage.show();
    }
}
