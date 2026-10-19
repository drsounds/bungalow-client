#ifndef CONFIGVIEW_H
#define CONFIGVIEW_H

#include "SPViewElement.h"
#include "SPColorChooser.h"
#include "SPAeroColorSlider.h"
#include "SPButtonElement.h"
#include "SPInputElement.h"
#include "SPElement.h"
#include "SPVBoxElement.h"
#include "SPFrutigerAeroSkin.h"
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace space {
namespace views {

class ConfigView : public SPViewElement {
public:
    ConfigView();
    explicit ConfigView(SPElement *parent);
    ~ConfigView() override;

    string getType() override { return "configview"; }

    bool acceptsUri(const string& uri) override;
    void navigate(const string& uri) override;
    void setVisible(bool value) override;

    static string getSettingsPath();
    void loadSettings();
    void saveSettings();

    int onSaveClicked(SPObject* sender, SPEventArgs* e);
    int onLightClicked(SPObject* sender, SPEventArgs* e);
    int onDarkClicked(SPObject* sender, SPEventArgs* e);
    int onGlossyClicked(SPObject* sender, SPEventArgs* e);
    int onBrowseDownloadsClicked(SPObject* sender, SPEventArgs* e);
    int onBrowseMusicClicked(SPObject* sender, SPEventArgs* e);
    int onDeveloperModeClicked(SPObject* sender, SPEventArgs* e);
    int onClassicSkinClicked(SPObject* sender, SPEventArgs* e);
    int onModernSkinClicked(SPObject* sender, SPEventArgs* e);
    int onFrutigerAeroSkinClicked(SPObject* sender, SPEventArgs* e);
    int onBundleSkinClicked(SPObject* sender, SPEventArgs* e);
    int onFullWidthAddressBarClicked(SPObject* sender, SPEventArgs* e);

    // Aero header-style callbacks
    int onAeroHeaderSolidClicked(SPObject* sender, SPEventArgs* e);
    int onAeroHeaderGradientClicked(SPObject* sender, SPEventArgs* e);
    int onAeroHeaderGlossyClicked(SPObject* sender, SPEventArgs* e);
    int onAeroHeaderAccentGlossyClicked(SPObject* sender, SPEventArgs* e);
    int onAeroHeaderDarkGlossyClicked(SPObject* sender, SPEventArgs* e);

    // Button style callbacks
    int onButtonFlatClicked(SPObject* sender, SPEventArgs* e);
    int onButtonGradientClicked(SPObject* sender, SPEventArgs* e);
    int onButtonModernGradientClicked(SPObject* sender, SPEventArgs* e);
    int onButtonAquaClicked(SPObject* sender, SPEventArgs* e);

private:
    SPColorChooser*  m_colorChooser           = nullptr;
    SPButtonElement* m_saveButton             = nullptr;
    SPButtonElement* m_lightButton            = nullptr;
    SPButtonElement* m_darkButton             = nullptr;
    SPButtonElement* m_glossyButton           = nullptr;
    SPInputElement*  m_downloadsFolderInput   = nullptr;
    SPInputElement*  m_musicFolderInput       = nullptr;
    SPButtonElement* m_browseDownloadsBtn     = nullptr;
    SPButtonElement* m_browseMusicBtn         = nullptr;
    SPButtonElement* m_developerModeButton    = nullptr;
    SPButtonElement* m_classicSkinButton      = nullptr;
    SPButtonElement* m_modernSkinButton       = nullptr;
    SPButtonElement* m_frutigerAeroSkinButton = nullptr;
    // Skins from app bundles other than the default modern skin, with their skin type.
    std::vector<std::pair<SPButtonElement*, string>> m_bundleSkinButtons;
    SPButtonElement* m_fullWidthAddressBarButton = nullptr;

    // Aero header style buttons
    SPButtonElement* m_aeroHeaderSolidBtn        = nullptr;
    SPButtonElement* m_aeroHeaderGradientBtn     = nullptr;
    SPButtonElement* m_aeroHeaderGlossyBtn       = nullptr;
    SPButtonElement* m_aeroHeaderAccentGlossyBtn = nullptr;
    SPButtonElement* m_aeroHeaderDarkGlossyBtn   = nullptr;

    // Button style picker
    SPButtonElement* m_btnStyleFlatBtn          = nullptr;
    SPButtonElement* m_btnStyleGradientBtn      = nullptr;
    SPButtonElement* m_btnStyleModernGradBtn    = nullptr;
    SPButtonElement* m_btnStyleAquaBtn          = nullptr;

    // Aero colour sliders
    SPAeroColorSlider* m_hueSlider        = nullptr;
    SPAeroColorSlider* m_saturationSlider = nullptr;
    SPAeroColorSlider* m_lightnessSlider  = nullptr;

    // Aero section visibility container (hidden unless frutiger-aero skin selected)
    SPVBoxElement* m_aeroSection = nullptr;

    short  m_pendingScheme             = SP_COLOR_SCHEME_LIGHT;
    bool   m_pendingGlossy             = false;
    bool   m_pendingDeveloperMode      = false;
    string m_pendingSkinType           = "modern";
    bool   m_pendingFullWidthAddressBar = true;

    // Aero-specific pending values
    float           m_pendingAeroHue         = 200.0f;
    float           m_pendingAeroSaturation  = 0.0f;
    float           m_pendingAeroLightness   = 0.50f;
    AeroHeaderStyle m_pendingAeroHeaderStyle = kAeroHeaderGlossy;

    SPButtonStyle   m_pendingButtonStyle     = kButtonAqua;

    void updateModeButtons();
    void updateGlossyButton();
    void updateDeveloperModeButton();
    void updateSkinButtons();
    void updateFullWidthAddressBarButton();
    void updateAeroHeaderStyleButtons();
    void updateAeroSectionVisibility();
    void updateButtonStyleButtons();
    void syncSliderContext();
};

} // namespace views
} // namespace space

#endif // CONFIGVIEW_H
