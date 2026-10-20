package se.bungalow.aspect;

public interface Aspect {
    public String getId();
    public String getName();
    public void onRegister(AspectManager<? extends Aspect> aspectManager);
}
