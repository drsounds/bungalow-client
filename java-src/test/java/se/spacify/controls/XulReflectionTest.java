package se.spacify.controls;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.List;

import javax.swing.JProgressBar;

import org.junit.Test;

/**
 * Fully-qualified XUL tags are inflated by reflection (Android-style): a dotted tag
 * names a class, which is used directly if it is a {@link Control} or wrapped in a
 * {@link ComponentControl} if it is a raw Swing/AWT component.
 */
public class XulReflectionTest {

    @Test
    public void dottedTagInstantiatesControlSubclassByReflection() {
        Panel host = new Panel();
        host.setInnerXul("<root><se.spacify.controls.Label>Hi</se.spacify.controls.Label></root>");

        List<Control<?>> children = host.getChildren();
        assertEquals(1, children.size());
        assertTrue("expected a Label", children.get(0) instanceof Label);
        assertEquals("Hi", ((Label) children.get(0)).getComponent().getText());
    }

    @Test
    public void dottedTagWrapsRawComponentInComponentControl() {
        Panel host = new Panel();
        host.setInnerXul("<root><javax.swing.JProgressBar/></root>");

        List<Control<?>> children = host.getChildren();
        assertEquals(1, children.size());
        assertTrue("raw component should be wrapped", children.get(0) instanceof ComponentControl);
        assertTrue(children.get(0).getComponent() instanceof JProgressBar);
    }

    @Test
    public void shortTagsStillUseTheBuiltinVocabulary() {
        Panel host = new Panel();
        host.setInnerXul("<root><button>Go</button><vbox/></root>");

        List<Control<?>> children = host.getChildren();
        assertEquals(2, children.size());
        assertTrue(children.get(0) instanceof Button);
        assertTrue(children.get(1) instanceof VBox);
    }

    @Test
    public void unknownDottedTagFailsLoudly() {
        Panel host = new Panel();
        try {
            host.setInnerXul("<root><com.example.Nope/></root>");
            org.junit.Assert.fail("expected an exception for a missing class");
        } catch (IllegalArgumentException expected) {
            // message names the offending element
            assertTrue(expected.getMessage().contains("com.example.Nope"));
        }
    }
}
