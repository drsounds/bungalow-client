package se.spacify.graphics;

import java.awt.*;
import java.awt.geom.*;
import java.awt.image.ColorModel;

/**
 * A wrapper for RadialGradientPaint that handles non-uniform scaling (stretching)
 * and aspect ratio adjustments automatically.
 */
public class StretchableRadialGradient implements Paint {

    private final Point2D center;
    private final float radius;
    private final float[] fractions;
    private final Color[] colors;
    private final MultipleGradientPaint.CycleMethod cycleMethod;
    private final MultipleGradientPaint.ColorSpaceType colorSpace;
    
    // Stretching parameters
    private final double scaleX;
    public double getScaleX() {
        return scaleX;
    }

    private final double scaleY;
    public double getScaleY() {
        return scaleY;
    }

    private final double rotationInRadians;

    public StretchableRadialGradient(
            Point2D center, 
            float radius, 
            float[] fractions, 
            Color[] colors,
            double scaleX, 
            double scaleY, 
            double rotationInRadians) {
        
        this.center = center;
        this.radius = radius;
        this.fractions = fractions;
        this.colors = colors;
        this.scaleX = scaleX;
        this.scaleY = scaleY;
        this.rotationInRadians = rotationInRadians;
        this.cycleMethod = MultipleGradientPaint.CycleMethod.NO_CYCLE;
        this.colorSpace = MultipleGradientPaint.ColorSpaceType.SRGB;
    }

    @Override
    public PaintContext createContext(ColorModel cm, Rectangle deviceBounds, Rectangle2D userBounds, 
                                      AffineTransform xform, RenderingHints hints) {
        
        // 1. Build the internal stretching transform relative to (0,0)
        AffineTransform stretchTransform = new AffineTransform();
        
        // Move to the intended center in user-space
        stretchTransform.translate(center.getX(), center.getY());
        
        // Apply rotation if requested
        if (rotationInRadians != 0) {
            stretchTransform.rotate(rotationInRadians);
        }
        
        // Apply the non-uniform stretchingstretchTransform.scale(scaleX, scaleY);

        // 2. Combine our custom stretch transform with the existing Graphics2D transform
        AffineTransform combinedTransform = new AffineTransform(xform);
        combinedTransform.concatenate(stretchTransform);

        // 3. Instantiate a standard RadialGradientPaint centered at (0,0).
        // The combined transform will warp it into an ellipse and place it perfectly.
        RadialGradientPaint internalGradient = new RadialGradientPaint(
            new Point2D.Float(0, 0), 
            radius, 
            center, fractions, 
            colors, 
            cycleMethod, 
            colorSpace, 
            combinedTransform
        );

        // 4. Delegate the actual pixel rendering context to the native engine
        return internalGradient.createContext(cm, deviceBounds, userBounds, new AffineTransform(), hints);
    }

    @Override
    public int getTransparency() {
        return Transparency.TRANSLUCENT;
    }
}