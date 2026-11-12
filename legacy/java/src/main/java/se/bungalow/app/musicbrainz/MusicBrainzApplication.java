package se.bungalow.app.musicbrainz;

import se.bungalow.aspect.Aspect;
import se.bungalow.aspect.AspectManager;
import se.bungalow.app.Application;
import se.bungalow.app.ApplicationContext;
import se.bungalow.app.ApplicationSetting;

import java.util.List;

/**
 * Built-in plugin contributing the MusicBrainz catalogue. Registers a
 * {@link MusicBrainzService} (the discovery aspect,
 * {@link se.bungalow.app.catalogue.service.MusicCatalogueService}) so search and
 * MBID/ISRC/ISNI lookups can resolve against the MusicBrainz web Service.
 *
 * <p>Exposes a single {@code contact} setting folded into the API
 * {@code User-Agent}, which MusicBrainz asks callers to supply.
 */
public class MusicBrainzApplication extends Application {

    /** Settings key for the contact URL/email sent in the MusicBrainz User-Agent. */
    public static final String SETTING_CONTACT = "contact";

    @Override
    public void onActivate(ApplicationContext ctx) {
        String contact = ctx.settings().getString(SETTING_CONTACT);
        ctx.registerService(new MusicBrainzService(contact));
    }

    @Override
    public List<ApplicationSetting> getSettingsSchema() {
        return List.of(
            ApplicationSetting.string(SETTING_CONTACT,
                "Contact (email or URL, sent to MusicBrainz in the User-Agent)", "")
        );
    }

    @Override
    public String getId() {
        // TODO Auto-generated method stub
        return "musicbrainz";
    }

    @Override
    public String getName() {
        // TODO Auto-generated method stub
        return "MusicBrainz";
    }

    @Override
    public void onRegister(AspectManager<? extends Aspect> aspectManager) {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'onRegister'");
    }
}
