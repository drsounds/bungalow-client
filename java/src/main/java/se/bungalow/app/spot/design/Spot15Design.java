package se.bungalow.app.spot.design;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.design.Design;
import se.bungalow.app.spot.skin.Spot15Skin;
import se.bungalow.app.spot.chrome.Spot15Chrome;
import se.bungalow.skinning.Skin;

import se.bungalow.ui.chrome.Chrome;

public class Spot15Design implements Design {
    private Spot15Skin skin;
    private Spot15Chrome chrome;

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Spot 15";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
       
    }

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "spot15";
    }

    @Override
    public Chrome getChrome() {
        // TODO Auto-generated method stub
        return chrome == null ? chrome = new Spot15Chrome() : chrome;
    }

    @Override
    public Skin getSkin() {
        // TODO Auto-generated method stub
        return skin == null ? skin = new Spot15Skin() : skin;
    }
    
}
