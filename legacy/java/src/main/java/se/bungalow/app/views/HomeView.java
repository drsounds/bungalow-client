package se.bungalow.app.views;

import se.bungalow.navigation.View;
import se.bungalow.navigation.ViewStack;

import javax.swing.*;
import java.awt.*;

public class HomeView extends View {


    public HomeView(ViewStack viewStack) {
        super(viewStack);
        getComponent().setLayout(new BorderLayout());
        getComponent().setOpaque(false);

        JLabel label = new JLabel("Home", SwingConstants.CENTER);
        label.setFont(label.getFont().deriveFont(Font.BOLD, 24f));
        label.setForeground(Color.WHITE);
        getComponent().add(label, BorderLayout.CENTER);
    }

    @Override
    public boolean acceptsUri(String uri) {
        return uri != null && uri.matches("bungalow:(home|//)");
    }   

    @Override
    public void navigate(String uri) {}


    @Override
    public String getName() { return "Home"; }
}
