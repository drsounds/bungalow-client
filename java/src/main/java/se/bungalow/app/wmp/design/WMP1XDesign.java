package se.bungalow.app.wmp.design;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.design.Design;
import se.bungalow.app.wmp.chrome.WMP1XChrome;
import se.bungalow.app.wmp.skin.WMP10Skin;
import se.bungalow.skinning.Skin;
import se.bungalow.ui.chrome.Chrome;

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
