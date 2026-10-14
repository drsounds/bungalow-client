package se.spacify.app.spot.design;

import se.spacify.aspect.Aspect;
import se.spacify.aspect.AspectManager;
import se.spacify.design.Design;
import se.spacify.app.spot.skin.Spot09Skin;
import se.spacify.app.spot.chrome.Spot09Chrome;
import se.spacify.skinning.Skin;

import se.spacify.ui.chrome.Chrome;

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
