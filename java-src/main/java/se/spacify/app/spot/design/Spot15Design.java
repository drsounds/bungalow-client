package se.spacify.app.spot.design;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.design.Design;
import se.spacify.app.spot.skin.Spot15Skin;
import se.spacify.app.spot.chrome.Spot15Chrome;
import se.spacify.skinning.Skin;

import se.spacify.ui.chrome.Chrome;

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
