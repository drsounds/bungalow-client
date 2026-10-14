package se.spacify.design;

import se.spacify.aspect.Aspect;
import se.spacify.skinning.Skin;

import se.spacify.ui.chrome.Chrome;

public interface Design extends Aspect { 
    public String getId();
    public Chrome getChrome();
    public Skin getSkin();
}
