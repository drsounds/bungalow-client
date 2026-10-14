package se.spacify.controls;

import javax.swing.JSlider;

/**
 * A slider control wrapping a {@link JSlider}. Reach the widget through
 * {@link #getComponent()}.
 */
public class Slider extends Control<JSlider> {

	public Slider() {
		this.component = new JSlider();
	}

	public Slider(int min, int max, int value) {
		this.component = new JSlider(min, max, value);
	}
}
