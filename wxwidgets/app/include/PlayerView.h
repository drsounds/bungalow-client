#pragma once

#include "SPViewElement.h"
#include "SPAeroColorSlider.h"
#include "SPFrutigerAeroSkin.h"
#include <SPPlayerCanvasElement.h>
#include "SPMusicVisualizerCanvasElement.h"
#include "SPVBoxElement.h"
#include "SPTextElement.h"
#include <string>

namespace space {
namespace views {

class PlayerView : public SPViewElement {
public:
    PlayerView();
    explicit PlayerView(SPElement* parent);
    ~PlayerView() override;

    std::string getType() override { return "playerview"; }

    bool acceptsUri(const std::string& uri) override;
    void navigate(const std::string& uri) override;
    void setVisible(bool value) override;

    /// Swap the active canvas implementation.
    /// PlayerView owns all implementation instances; the caller should not
    /// delete them.
    void setCanvasImplementation(SPPlayerCanvasImplementationElement* impl);

private:
    SPPlayerCanvasElement*          m_canvas        = nullptr;
    SPMusicVisualizerCanvasElement* m_vizImpl       = nullptr;  // default impl, owned here
    SPVBoxElement*                  m_paramPanel    = nullptr;
    SPAeroColorSlider*              m_hueSlider     = nullptr;
    SPAeroColorSlider*              m_saturationSlider = nullptr;
    SPAeroColorSlider*              m_lightnessSlider  = nullptr;

    // Full settings state — all keys preserved on save
    short           m_scheme              = SP_COLOR_SCHEME_LIGHT;
    bool            m_glossy              = true;
    std::string     m_skinType            = "frutiger-aero";
    AeroHeaderStyle m_headerStyle         = kAeroHeaderGlossy;
    float           m_hue                 = 200.0f;
    float           m_saturation          = 0.70f;
    float           m_lightness           = 0.50f;
    bool            m_developerMode       = false;
    bool            m_fullWidthAddressBar = true;
    std::string     m_tintColor;
    std::string     m_downloadsFolder;
    std::string     m_musicFolder;

    void loadSettings();
    void saveSettings();
    void applyLive();
    void syncSliderContext();

    /// Rebuild the parameter panel from the active canvas implementation's
    /// visualization params.  Called when the implementation changes.
    void rebuildParamPanel();

    /// Called when SPPlayerCanvasElement dispatches "implementationChanged".
    int onImplementationChanged(SPObject* sender, SPEventArgs* args);
};

} // namespace views
} // namespace space
