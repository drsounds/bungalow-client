package se.spacify.aspect;

public interface Aspect {
    public String getId();
    public String getName();
    public void onRegister(AspectManager<? extends Aspect> aspectManager);
}
