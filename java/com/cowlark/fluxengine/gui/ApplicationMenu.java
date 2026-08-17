package com.cowlark.fluxengine.gui;

import static swingtree.UIFactoryMethods.menu;
import static swingtree.UIFactoryMethods.menuItem;
import static swingtree.UIFactoryMethods.of;
import static swingtree.UIFactoryMethods.separator;

import swingtree.UI;
import swingtree.UIForMenuItem;
import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.JMenuItem;
import javax.swing.KeyStroke;
import javax.swing.UIManager;
import javax.swing.text.DefaultEditorKit;
import javax.swing.text.JTextComponent;
import java.awt.KeyboardFocusManager;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;

public class ApplicationMenu
{
    public static UI.MenuBar createMenu()
    {
        installMacAboutHandler();

        return of(new UI.MenuBar()).add(menu("File").add(menuItem("About FluxEngine...").onClick(it -> UiUtils.fireAction(
                                new AboutAction(),
                                it.getComponent())))
                        .add(separator())
                        .add(menuItem("Exit").onClick(it -> System.exit(0))))
                .add(menu("Edit").add(actionMenuItem("Cut",
                                "cut",
                                new DefaultEditorKit.CutAction(),
                                shortcut(KeyEvent.VK_X)))
                        .add(actionMenuItem("Copy",
                                "copy",
                                new DefaultEditorKit.CopyAction(),
                                shortcut(KeyEvent.VK_C)))
                        .add(actionMenuItem("Paste",
                                "paste",
                                new DefaultEditorKit.PasteAction(),
                                shortcut(KeyEvent.VK_V)))
                        .add(actionMenuItem("Delete",
                                "delete",
                                new DeleteAction(),
                                shortcut(KeyEvent.VK_DELETE))))
                .get(UI.MenuBar.class);

    }

    /* On macOS, wires the application menu's About item to AboutAction. The
     * com.apple.eawt API is macOS-only, so this is done reflectively to keep
     * the code compiling on other platforms. */
    private static void installMacAboutHandler()
    {
        if (!System.getProperty("os.name").toLowerCase().contains("mac"))
            return;

        try
        {
            Class<?> applicationClass = Class.forName("com.apple.eawt.Application");
            Class<?> aboutHandlerClass = Class.forName("com.apple.eawt.AboutHandler");

            Object application = applicationClass.getMethod("getApplication").invoke(null);
            Object handler =
                    java.lang.reflect.Proxy.newProxyInstance(ApplicationMenu.class.getClassLoader(),
                            new Class<?>[]{aboutHandlerClass},
                            (proxy, method, args) -> {
                                if (method.getName().equals("handleAbout"))
                                    new AboutAction().actionPerformed(null);
                                return null;
                            });

            applicationClass.getMethod("setAboutHandler", aboutHandlerClass)
                    .invoke(application, handler);
        } catch (ReflectiveOperationException e)
        {
            /* The Mac integration isn't available; ignore. */
        }
    }

    /* Returns a platform-standard menu accelerator KeyStroke for the given key
     * code (Cmd on macOS, Ctrl elsewhere). */
    static KeyStroke shortcut(int keyCode)
    {
        return KeyStroke.getKeyStroke(keyCode,
                Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx());
    }

    /* Returns the standard platform icon for the given action, or null if the
     * look-and-feel doesn't provide one. */
    static javax.swing.Icon actionIcon(String name)
    {
        return UIManager.getIcon("Actions." + name);
    }

    /* Builds a menu item bound to the given action, setting the label, icon,
     * and accelerator from the action's properties. */
    static UIForMenuItem<JMenuItem> actionMenuItem(
            String name,
            String iconName,
            Action action,
            KeyStroke keyStroke)
    {
        action.putValue(Action.NAME, name);
        javax.swing.Icon icon = actionIcon(iconName);
        if (icon != null)
            action.putValue(Action.SMALL_ICON, icon);
        action.putValue(Action.ACCELERATOR_KEY, keyStroke);
        return menuItem(name).peek(item -> item.setAction(action));
    }

    /* Returns the text component which currently has keyboard focus, if any. */
    static JTextComponent focusedTextComponent()
    {
        java.awt.Component focusOwner =
                KeyboardFocusManager.getCurrentKeyboardFocusManager().getFocusOwner();
        return focusOwner instanceof JTextComponent component ? component : null;
    }

    /* An action which deletes the selected content of the focused text
     * component. */
    static class DeleteAction extends AbstractAction
    {
        @Override
        public void actionPerformed(ActionEvent e)
        {
            JTextComponent component = focusedTextComponent();
            if (component == null)
                return;

            Action delete = component.getActionMap().get(DefaultEditorKit.deleteNextCharAction);
            if (delete != null)
                delete.actionPerformed(new ActionEvent(component,
                        ActionEvent.ACTION_PERFORMED,
                        null));
        }
    }

}