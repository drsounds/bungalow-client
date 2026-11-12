package se.bungalow.skinning;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Paint;

import javax.swing.JComponent;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.controls.Button;
import se.bungalow.controls.Control;
import se.bungalow.controls.GlassPanel;
import se.bungalow.controls.GlossyButton;
import se.bungalow.controls.SplitPane.SplitPaneDivider;
import se.bungalow.controls.TabButton;
import se.bungalow.controls.Table;
import se.bungalow.controls.TextField;
import se.bungalow.controls.ToolBar;
import se.bungalow.controls.ToolButton;
import se.bungalow.controls.Tree;
import se.bungalow.controls.VerticalPanel;
import se.bungalow.controls.Panel;
import se.bungalow.navigation.ViewStack;
import se.bungalow.app.Application;
import se.bungalow.ui.LeftLibraryMenu;
import se.bungalow.ui.LeftMenuPanel;

public abstract class Skin implements Aspect {
	protected Application plugin;
	public Application getApplication() {
		return plugin;
	}
	public float getFloatValue(Control<? extends JComponent> control, String key, float defaultValue) {
		return defaultValue;
	}
	public int getIntValue(Control<? extends JComponent> control, String key, int defaultValue) {
		return defaultValue;
	}
	public String getStringValue(Control<? extends JComponent> control, String key, String defaultValue) {
		return defaultValue;
	}
	public Color getColorValue(Control<? extends JComponent> control, String key, Color defaultValue) {
		return defaultValue;
	}
	public Paint getPaintValue(Control<? extends JComponent> control, String key, Paint defaultValue) {
		return defaultValue;
	}

	public abstract String getId(); 
	public void paintText(JComponent control, Graphics2D g2, String text, int x, int y) {
		g2.drawString(text, x, y);
	}
	public void paintTopBar(Panel control, Graphics2D g2) {}
	public void paintHeader(Panel header, Graphics2D g2) {}
	public void paintFooter(Panel footer, Graphics2D g2) {}
	public void paintTabButton(TabButton button, Graphics2D g2) {}
	public void paintGlossyButton(GlossyButton control, Graphics2D g2, int x, int y, int d) {}
	public void paintPlaylist(Panel control, Graphics2D g2) {}
	public void paintGlassPanel(GlassPanel control, Graphics2D g2) {}
	public void paintToolBar(ToolBar control, Graphics2D g2) {}
	public void paintTableHeader(Table table, int width, int height, Graphics2D g2)  {}
	public void paintToolButton(ToolButton control, Graphics2D g2) {}
	public void paintVerticalPanel(VerticalPanel verticalPanel, Graphics2D g2) {}
	public void onRegister(SkinManager manager) {}
	public void paintButton(Button control, Graphics2D g2, boolean hovered, boolean pressed) {}
	public void paintViewStack(ViewStack control, Graphics2D g2) {}
    public void paintTextField(TextField textField, Graphics2D g2) {}
    public void paintSplitPaneDivider(SplitPaneDivider splitPaneDivider, Graphics2D g2) {}
	public void paintLeftLibraryMenu(LeftLibraryMenu control, Graphics2D g2) {}
	public void paintLeftMenuPanel(LeftMenuPanel control, Graphics2D g2) {}
	public void paintTree(Tree tree, Graphics2D g2) {}
	public void paintControl(Control<? extends JComponent> control, Graphics2D g2) {}

	public void onRegister(AspectManager<? extends Aspect> aspectManager) {
	
	}
}
