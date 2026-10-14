package se.spacify.app.spider;

import java.util.HashMap;
import java.util.Map;

public class Request {
    private Object text;
    public Object getText() {
        return text;
    }
    
    private Object data;
    public Object getData() {
        return data;
    }
    
    private String method = "GET";
    public String getMethod() {
        return method;
    }
    private String uri = ""; // uri: spacify:
    public String getUri() {
        return uri;
    }
    private Map<String, Object> headers = new HashMap<>();
    public Map<String, Object> getHeaders() {
        return headers;
    }
    public Request(String method, String uri, Map<String, Object> headers, String text, Object data) {
        this.method = method;
        this.uri = uri;
        this.headers = headers;
        this.text = text;
        this.data = data;
    }
}