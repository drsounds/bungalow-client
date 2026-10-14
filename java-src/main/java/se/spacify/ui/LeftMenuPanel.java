package se.spacify.ui;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.BoxLayout;

import se.spacify.controls.Panel;
import se.spacify.controls.VerticalPanel;

public class LeftMenuPanel extends Panel {

	private Panel topSpacePanel;
	private VerticalPanel menuPanel;
	private Panel bottomSpacePanel;

	public LeftMenuPanel() {
		getComponent().setLayout(new BoxLayout(getComponent(), BoxLayout.PAGE_AXIS));
		getComponent().setOpaque(false);
		topSpacePanel = new Panel();
		topSpacePanel.getComponent().setOpaque(false);
		topSpacePanel.getComponent().setMinimumSize(new Dimension(0, 2500));
		topSpacePanel.getComponent().setMaximumSize(new Dimension(Short.MAX_VALUE, 2500));
		add(topSpacePanel);
		menuPanel = new VerticalPanel();
		add(menuPanel);
		bottomSpacePanel = new Panel();
		bottomSpacePanel.getComponent().setOpaque(false);
		bottomSpacePanel.getComponent().setMinimumSize(new Dimension(0, 2500));
		bottomSpacePanel.getComponent().setMaximumSize(new Dimension(Short.MAX_VALUE, 2500));
		add(bottomSpacePanel);
	}

	@Override
	protected void paintSurface(Graphics g) {
		Graphics2D g2 = (Graphics2D) g.create();
		getSkin().paintLeftMenuPanel(this, g2);
		g2.dispose();
	}
}
