package se.bungalow.ui.theme;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.design.Design;
import se.bungalow.app.Application;
import se.bungalow.skinning.Skin;
import se.bungalow.ui.chrome.Chrome;

public class Theme implements Aspect {
    private Application plugin;
    public Application getApplication() {
        return plugin;
    }
    public void setApplication(Application plugin) {
        this.plugin = plugin;
    }
    public String getId() {
        return "theme";
    }
    public String getName() {
        return "Base Theme";
    }
    private Design design;
    public Design getDesign() {
        return design;
    }
    public void setDesign(Design design) {
        this.design = design;
    }
    private Skin skin;
    public Skin getSkin() {
        return skin;
    }
    public void setSkin(Skin skin) {
        this.skin = skin;
    }
    private Chrome chrome;

    public Chrome getChrome() {
        return chrome;
    }
    public void setChrome(Chrome chrome) {
        this.chrome = chrome;
    }
    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
    }
}
