package se.spacify.app.wmp.design;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.design.Design;
import se.spacify.app.wmp.chrome.WMP1XChrome;
import se.spacify.app.wmp.skin.WMP10Skin;
import se.spacify.skinning.Skin;
import se.spacify.ui.chrome.Chrome;

public class WMP1XDesign implements Design {
    Chrome chrome;
    Skin skin;

    public WMP1XDesign() {
        chrome = new WMP1XChrome();
        skin = new WMP10Skin();
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "WMP 1X";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
         
    }

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "wmp1x";
    }

    @Override
    public Chrome getChrome() {
        // TODO Auto-generated method stub
        return chrome;
    }

    @Override
    public Skin getSkin() {
        // TODO Auto-generated method stub
        return skin;
    }
    
}
