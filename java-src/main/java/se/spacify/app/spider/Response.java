package se.spacify.app.spider;

import org.w3c.dom.Element;

public class Response {
    private Element xml;
    private String text;

    public String getText() {
        return text;
    }

    public Element getXml() {
        return xml;
    }
    private Request request;

    public Request getRequest() {
        return request;
    }

}