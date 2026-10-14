package se.spacify.app.spot.controls;

import java.awt.BorderLayout;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;

import se.spacify.controls.Panel;
import se.spacify.controls.TextField;

import se.spacify.navigation.ViewStack;

import se.spacify.ui.AppHeader;

public class Spot09AppHeader extends AppHeader {
    protected TextField uriField;

	protected TextField searchField;

    private Panel center;
    public Spot09AppHeader(ViewStack viewStack) {
        super(viewStack);
        getComponent().setLayout(new BoxLayout(getComponent(), BoxLayout.LINE_AXIS));
        getComponent().setBorder(BorderFactory.createEmptyBorder(8, 16, 8, 16));

		backBtn = makeNavButton("◄");
		backBtn.setDiameter(48);
		backBtn.setPrimary(true);
		backBtn.getComponent().setEnabled(false);
		add(backBtn);
		forwardBtn = makeNavButton("►");
		forwardBtn.setDiameter(36);
		forwardBtn.getComponent().setEnabled(false);
		add(forwardBtn);

		backBtn.getComponent().addActionListener((ActionEvent e) -> viewStack.back());
		forwardBtn.getComponent().addActionListener((ActionEvent e) -> viewStack.forward());
		uriField = new TextField("spacify:home");
		uriField.getComponent().setFont(uriField.getComponent().getFont().deriveFont(12f));
		uriField.getComponent().setPreferredSize(new Dimension(260, 28));
		uriField.getComponent().addActionListener(e -> viewStack.navigate(uriField.getComponent().getText().trim()));

		searchField = new TextField();
		searchField.getComponent().putClientProperty("JTextField.placeholderText", "Search...");
		searchField.getComponent().setPreferredSize(new Dimension(180, 28));
		searchField.getComponent().addActionListener(e -> {
			String q = searchField.getComponent().getText().trim();
			if (!q.isEmpty()) {
				String encoded = URLEncoder.encode(q, StandardCharsets.UTF_8);
				viewStack.navigate("spacify:search?q=" + encoded);
			}
		});
        add(searchField);

		center = new Panel(new BorderLayout());
		center.getComponent().setOpaque(false);
		add(center);

		uriField.setVisible(false);

    }
	@Override
	public void onNavigate(String uri, boolean canGoBack, boolean canGoForward) {
		backBtn.getComponent().setEnabled(canGoBack);
		forwardBtn.getComponent().setEnabled(canGoForward);
		if (uri != null)
			uriField.getComponent().setText(uri);
	}
    
}
