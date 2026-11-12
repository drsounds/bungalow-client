package se.bungalow.app.spot.design;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.design.Design;
import se.bungalow.app.spot.skin.Spot09Skin;
import se.bungalow.app.spot.chrome.Spot09Chrome;
import se.bungalow.skinning.Skin;

import se.bungalow.ui.chrome.Chrome;

public class Spot09Design implements Design {
    private Spot09Skin skin;
    private Spot09Chrome chrome;

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "Spot 09";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
       
    }

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "spot09";
    }

    @Override
    public Chrome getChrome() {
        // TODO Auto-generated method stub
        return chrome == null ? chrome = new Spot09Chrome() : chrome;
    }

    @Override
    public Skin getSkin() {
        // TODO Auto-generated method stub
        return skin == null ? skin = new Spot09Skin() : skin;
    }
    
}
