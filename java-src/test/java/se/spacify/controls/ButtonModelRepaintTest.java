package se.spacify.controls;

import static org.junit.Assert.assertEquals;

import java.util.concurrent.atomic.AtomicInteger;

import javax.swing.JButton;

import org.junit.Test;

public class ButtonModelRepaintTest {

    @Test
    public void modelChangesTriggerRepaintCallbacksForCustomButtonUi() {
        AtomicInteger repaintCount = new AtomicInteger();
        Button button = new Button("Test") {
            @Override
            public void repaint() {
                repaintCount.incrementAndGet();
                super.repaint();
            }
        };

        JButton component = button.getComponent();
        component.getModel().setPressed(true);
        component.getModel().setPressed(false);
        component.getModel().setRollover(true);
        component.getModel().setRollover(false);

        assertEquals(4, repaintCount.get());
    }
}
