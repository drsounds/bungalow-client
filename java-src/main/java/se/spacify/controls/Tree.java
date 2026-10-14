package se.spacify.controls;

import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Rectangle;

import javax.swing.JTree;
import javax.swing.tree.TreeNode;

import se.spacify.ui.theme.ThemeManager;

/**
 * A tree control wrapping a {@link JTree}, painted by the active skin via
 * {@link se.spacify.skinning.Skin#paintTree}. Reach the widget through
 * {@link #getComponent()}.
 */
public class Tree extends Control<JTree> {

	protected class Surface extends JTree {
		private static final long serialVersionUID = 1L;
		Surface() { super(); }
		Surface(TreeNode root) { super(root); }
		@Override
		protected void paintComponent(Graphics g) {
			Tree.this.paintSurface(g, this);
		}
		void superPaint(Graphics g) { super.paintComponent(g); }
	}

	public Tree() {
		this.component = new Surface();
	}

	public Tree(TreeNode root) {
		this.component = new Surface(root);
	}

	protected void paintSurface(Graphics g, JTree tree) {
		Graphics2D g2 = (Graphics2D) g.create();
		getSkin().paintTree(this, g2);
		g2.dispose();
		// Selection highlight spanning the full row width (a plain JTree only fills
		// behind the label). The cell renderer paints transparently on top.
		int[] selected = tree.getSelectionRows();
		if (selected != null) {
			g.setColor(ThemeManager.getAccentForegroundColor());
			for (int row : selected) {
				Rectangle b = tree.getRowBounds(row);
				if (b != null) g.fillRect(0, b.y, tree.getWidth(), b.height);
			}
		}
		((Surface) component).superPaint(g);
	}
}
