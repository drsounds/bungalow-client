package se.spacify.content;

import se.spacify.net.Uri;

public class Intent {
    private Context context;
    public Context getContext() {
        return context;
    }
    public void setContext(Context context) {
        this.context = context;
    }
    public static final String ACTION_VIEW = "ACTION_VIEW";
    public static final String ACTION_PLAY = "ACTION_PLAY";
    private String action;
    public String getAction() {
        return action;
    }
    public void setAction(String action) {
        this.action = action;
    }
    private Uri data;
    public Uri getData() {
        return data;
    }
    public void setData(Uri data) {
        this.data = data;
    }
}
