package se.spacify.app.playlist.controller;

import java.io.IOException;
import java.util.Map;

import se.spacify.app.playlist.upsl.Upsl;
import se.spacify.app.spider.Request;
import se.spacify.app.spider.controller.Controller;
import se.spacify.net.Uri;


public class PlaylistController extends Controller {

    @Override
    public boolean acceptsUri(Uri uri) {
        // TODO Auto-generated method stub
        return Upsl.isUpsl(uri != null ? uri.toString() : null);
    }

    @Override
    protected String template(Request request) {
        // TODO Auto-generated method stub
       try {
        return Controller.getResourceFileAsString("views/generic_header.xml");
       } catch (IOException e) {
        // TODO Auto-generated catch block
        e.printStackTrace();
        return "<vbox></vbox>";
       }
    }

    /**
     * Decode the request's UPSL link (RFC-0001) into the model the template renders:
     * the playlist {@code name}, its {@code description} and the originating {@code uri}.
     * A non-UPSL request yields an empty model.
     */
    @Override
    protected Map<String, Object> data(Request request) {
        Upsl upsl = Upsl.parse(request.getUri());
        if (upsl == null) {
            return Map.of();
        }
        return Map.of(
            "name", upsl.name(),
            "uri", upsl.uri(),
            "description", upsl.description());
    }

}