package se.spacify.app.spider;

import java.util.HashMap;
import java.util.Map;
import java.util.function.Consumer;

import javax.swing.JButton;

import se.spacify.controls.Button;
import se.spacify.controls.Control;
import se.spacify.controls.TextField;

/**
 * Postback wiring shared by Spider-rendered surfaces. After a template is rendered
 * into a control tree, {@link #bind bind} attaches a click handler to every button
 * that declares {@code onclick}, and {@link #collectInput collectInput} gathers the
 * current values of named input fields to send back with the request.
 *
 * <p>Buttons are bound at most once even across re-renders: the Spider reconciler
 * reuses control instances, so a {@code spider.bound} client property marks an
 * already-wired button. The handler reads the {@code onclick} attribute live, so a
 * reused button always posts back its latest action.
 */
public final class Postbacks {

    /** Marks a button whose postback listener is already attached. */
    private static final String BOUND = "spider.bound";

    private Postbacks() {
    }

    /** Wire every {@code onclick} button under {@code root} to call {@code onAction} with its action name. */
    public static void bind(Control<?> root, Consumer<String> onAction) {
        for (Control<?> child : root.getChildren()) {
            if (child instanceof Button button && child.getAttribute("onclick", null) != null) {
                JButton widget = button.getComponent();
                if (widget.getClientProperty(BOUND) == null) {
                    widget.putClientProperty(BOUND, Boolean.TRUE);
                    widget.addActionListener(e -> onAction.accept(child.getAttribute("onclick", "").toString()));
                }
            }
            bind(child, onAction);
        }
    }

    /** Collect the current value of every named input field under {@code root}. */
    public static Map<String, Object> collectInput(Control<?> root) {
        Map<String, Object> input = new HashMap<>();
        collectInto(root, input);
        return input;
    }

    private static void collectInto(Control<?> control, Map<String, Object> input) {
        for (Control<?> child : control.getChildren()) {
            if (child instanceof TextField field) {
                String name = inputName(child);
                if (name != null) {
                    input.put(name, field.getComponent().getText());
                }
            }
            collectInto(child, input);
        }
    }

    /** An input's key: its control name, else its {@code name}, else its {@code id} attribute. */
    private static String inputName(Control<?> control) {
        if (control.getName() != null) {
            return control.getName();
        }
        Object name = control.getAttribute("name", control.getAttribute("id", null));
        return name != null ? name.toString() : null;
    }
}
