package se.bungalow.design;

import se.bungalow.aspect.Aspect;
import se.bungalow.skinning.Skin;

import se.bungalow.ui.chrome.Chrome;

public interface Design extends Aspect { 
    public String getId();
    public Chrome getChrome();
    public Skin getSkin();
}
