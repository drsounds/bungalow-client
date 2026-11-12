#include "ConfigView.h"
#include "BungalowApp.h"
#include "SPTextElement.h"
#include "SPHBoxElement.h"
#include "SPVBoxElement.h"
#include "SPInputElement.h"
#include "SPEventArgs.h"
#include "SPWindowElement.h"
#include "SPColor.h"
#include "SPSkin.h"
#include "SPModernSkin.h"
#include "SPClassicSkin.h"
#include "SPFrutigerAeroSkin.h"
#include "SPAppManager.h"
#include "SPAeroColorSlider.h"
#include "SPGraphicsContext.h"
#include "PlatformUi.h"
#include "DownloadManager.h"
#include "playlist/Upsl.h"

#include <nlohmann/json.hpp>
#include <cstdint>

#include <fstream>
#include <sstream>

namespace space {
namespace views {

static const SPColor kSelectedColor   = SPColor( 60, 100, 220, 220);
static const SPColor kDeselectedColor = SPColor(160, 160, 160, 255);

// ── helpers ───────────────────────────────────────────────────────────────────

static void applyToggleButton(SPButtonElement* btn, bool active,
                               const string& onLabel, const string& offLabel)
{
    if (!btn) return;
    btn->setInnerText(active ? onLabel : offLabel);
    btn->setBackgroundColor(active ? kSelectedColor : kDeselectedColor);
}

static SPTextElement* makeLabel(SPElement* parent, const string& text, int height) {
    auto *lbl = new SPTextElement(parent);
    lbl->setInnerText(text);
    lbl->setHeight(height);
    parent->appendChild(lbl);
    return lbl;
}

static SPButtonElement* makeButton(SPElement* parent, const string& label,
                                   int width, int height) {
    auto *btn = new SPButtonElement(parent);
    btn->setInnerText(label);
    btn->setWidth(width);
    btn->setHeight(height);
    parent->appendChild(btn);
    return btn;
}

static SPDropdownElement* makeDropdown(SPElement* parent, int width, int height) {
    auto *dropdown = new SPDropdownElement(parent);
    dropdown->setWidth(width);
    dropdown->setHeight(height);
    parent->appendChild(dropdown);
    return dropdown;
}

// ── constructor ───────────────────────────────────────────────────────────────

ConfigView::ConfigView() {}

ConfigView::ConfigView(SPElement *parent) : SPViewElement(parent) {
    this->clipView = false;
    this->setUri("bungalow:config");
    this->setPadding({ 16, 16, 16, 16 });
    this->setGap(8);

    makeLabel(this, "Settings", 36);

    // ── Appearance section ─────────────────────────────────────────────────
    makeLabel(this, "Appearance", 20);

    auto *modeRow = new SPHBoxElement(this);
    modeRow->setHeight(28);
    modeRow->setGap(6);
    this->appendChild(modeRow);

    m_lightButton = makeButton(modeRow, "Light Mode", 90, 28);
    m_lightButton->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onLightClicked),
        this, m_lightButton
    );

    m_darkButton = makeButton(modeRow, "Dark Mode", 90, 28);
    m_darkButton->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onDarkClicked),
        this, m_darkButton
    );

    
    m_glossyButton = makeButton(this, "Glossy: On", 110, 28);
    m_glossyButton->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onGlossyClicked),
        this, m_glossyButton
    );

    // ── Button Style section ───────────────────────────────────────────────
    makeLabel(this, "Button Style", 20);

    m_buttonStyleDropdown = makeDropdown(this, 160, 28);
    m_buttonStyleDropdown->setOptions({
        { std::to_string(kButtonFlat),           "Flat"            },
        { std::to_string(kButtonGradient),       "Gradient"        },
        { std::to_string(kButtonModernGradient), "Modern Gradient" },
        { std::to_string(kButtonAqua),           "Aqua"            },
    });
    m_buttonStyleDropdown->onChange = [this](const string& value) {
        m_pendingButtonStyle = static_cast<SPButtonStyle>(std::stoi(value));
        this->invalidate();
    };

    // ── Tint Color section ─────────────────────────────────────────────────
    //makeLabel(this, "Tint Color", 20);

    m_colorChooser = new SPColorChooser(this);
    // this->appendChild(m_colorChooser);
    // When a tint swatch is picked, sync H+S into the Aero sliders.
    // Lightness is kept unchanged so the user's brightness setting is preserved.
    m_colorChooser->onChange = [this](const SPColor& c) {
        float h, s, l;
        c.toHSL(h, s, l);
        m_pendingAeroHue        = h;
        m_pendingAeroSaturation = s;
        if (m_hueSlider)        m_hueSlider->setValue(h);
        if (m_saturationSlider) m_saturationSlider->setValue(s);
        syncSliderContext();
        this->invalidate();
    };

    // ── Skin section ───────────────────────────────────────────────────────
    makeLabel(this, "Skin", 20);

    m_skinDropdown = makeDropdown(this, 160, 28);
    m_skinDropdown->addOption("classic",       "Classic");
    m_skinDropdown->addOption("modern",        "Modern");
    m_skinDropdown->addOption("frutiger-aero", "Frutiger Aero");
    // XML skins (SPXmlSkin) installed as app bundles, e.g. "Modern Skin" (modern.skin)
    for (const SPAppExtension* ext : SPAppManager::instance().getExtensions("Skin"))
        m_skinDropdown->addOption(ext->identifier, ext->getName());
    m_skinDropdown->onChange = [this](const string& value) {
        m_pendingSkinType = value;
        updateAeroSectionVisibility();
        this->invalidate();
    };

    // ── Frutiger Aero options (hidden unless aero skin active) ─────────────
    m_aeroSection = new SPVBoxElement(this);
    m_aeroSection->setGap(8);
    this->appendChild(m_aeroSection);

    makeLabel(m_aeroSection, "Aero Header / Footer Style", 18);

    auto* headerStyleRow = new SPHBoxElement(m_aeroSection);
    headerStyleRow->setHeight(28);
    headerStyleRow->setGap(4);
    this->appendChild(headerStyleRow);

    m_aeroHeaderSolidBtn = makeButton(headerStyleRow, "Solid", 56, 28);
    m_aeroHeaderSolidBtn->addEventListener("click",
        reinterpret_cast<SPCallback>(&ConfigView::onAeroHeaderSolidClicked),
        this, m_aeroHeaderSolidBtn);

    m_aeroHeaderGradientBtn = makeButton(headerStyleRow, "WMP9", 56, 28);
    m_aeroHeaderGradientBtn->addEventListener("click",
        reinterpret_cast<SPCallback>(&ConfigView::onAeroHeaderGradientClicked),
        this, m_aeroHeaderGradientBtn);

    m_aeroHeaderGlossyBtn = makeButton(headerStyleRow, "WMP10", 60, 28);
    m_aeroHeaderGlossyBtn->addEventListener("click",
        reinterpret_cast<SPCallback>(&ConfigView::onAeroHeaderGlossyClicked),
        this, m_aeroHeaderGlossyBtn);

    m_aeroHeaderAccentGlossyBtn = makeButton(headerStyleRow, "WMP11b", 68, 28);
    m_aeroHeaderAccentGlossyBtn->addEventListener("click",
        reinterpret_cast<SPCallback>(&ConfigView::onAeroHeaderAccentGlossyClicked),
        this, m_aeroHeaderAccentGlossyBtn);

    m_aeroHeaderDarkGlossyBtn = makeButton(headerStyleRow, "WMP11", 60, 28);
    m_aeroHeaderDarkGlossyBtn->addEventListener("click",
        reinterpret_cast<SPCallback>(&ConfigView::onAeroHeaderDarkGlossyClicked),
        this, m_aeroHeaderDarkGlossyBtn);

    makeLabel(m_aeroSection, "Hue", 18);
    m_hueSlider = new SPAeroColorSlider(m_aeroSection, SPAeroColorSlider::Type::Hue);
    m_hueSlider->setHeight(20);
    //this->appendChild(m_hueSlider);
    m_hueSlider->onChange = [this](float v) {
        m_pendingAeroHue = v;
        syncSliderContext();
        // Mirror current hue+sat into color chooser so the selected swatch tracks
        if (m_colorChooser)
            m_colorChooser->setSelectedColor(
                SPColor::fromHSL(m_pendingAeroHue, m_pendingAeroSaturation, 0.55f, 255));
        this->invalidate();
    };

    makeLabel(m_aeroSection, "Saturation", 18);
    m_saturationSlider = new SPAeroColorSlider(m_aeroSection, SPAeroColorSlider::Type::Saturation);
    m_saturationSlider->setHeight(20);
    //this->appendChild(m_saturationSlider);
    m_saturationSlider->onChange = [this](float v) {
        m_pendingAeroSaturation = v;
        syncSliderContext();
        if (m_colorChooser)
            m_colorChooser->setSelectedColor(
                SPColor::fromHSL(m_pendingAeroHue, m_pendingAeroSaturation, 0.55f, 255));
        this->invalidate();
    };

    makeLabel(m_aeroSection, "Lightness", 18);
    m_lightnessSlider = new SPAeroColorSlider(m_aeroSection, SPAeroColorSlider::Type::Lightness);
    m_lightnessSlider->setHeight(20);
    //this->appendChild(m_lightnessSlider);
    m_lightnessSlider->onChange = [this](float v) {
        m_pendingAeroLightness = v;
        this->invalidate();
    };

    // ── Address bar section ────────────────────────────────────────────────
    makeLabel(this, "Address Bar", 20);

    m_fullWidthAddressBarButton = makeButton(this, "Full Width: On", 140, 28);
    m_fullWidthAddressBarButton->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onFullWidthAddressBarClicked),
        this, m_fullWidthAddressBarButton
    );

    // ── Links section ──────────────────────────────────────────────────────
    makeLabel(this, "Web Links", 20);

    m_forceExternalLinksButton = makeButton(this, "Open in System Browser: Off", 220, 28);
    m_forceExternalLinksButton->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onForceExternalLinksClicked),
        this, m_forceExternalLinksButton
    );

    // ── Developer section ──────────────────────────────────────────────────
    makeLabel(this, "Developer", 20);

    m_developerModeButton = makeButton(this, "Developer Mode: Off", 160, 28);
    m_developerModeButton->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onDeveloperModeClicked),
        this, m_developerModeButton
    );

    // ── Save button ────────────────────────────────────────────────────────
    m_saveButton = makeButton(this, "Save Changes", 120, 28);
    m_saveButton->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onSaveClicked),
        this, m_saveButton
    );

    // ── Downloads Folder section ───────────────────────────────────────────
    makeLabel(this, "Downloads Folder", 20);

    auto* dlRow = new SPHBoxElement(this);
    dlRow->setHeight(28);
    dlRow->setGap(6);
    this->appendChild(dlRow);

    m_downloadsFolderInput = new SPInputElement(dlRow);
    m_downloadsFolderInput->setFlex(1);
    m_downloadsFolderInput->setHeight(28);
    dlRow->appendChild(m_downloadsFolderInput);

    m_browseDownloadsBtn = makeButton(dlRow, "Browse…", 72, 28);
    m_browseDownloadsBtn->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onBrowseDownloadsClicked),
        this, m_browseDownloadsBtn
    );

    // ── Music Folder section ───────────────────────────────────────────────
    makeLabel(this, "Music Folder", 20);

    auto* musicRow = new SPHBoxElement(this);
    musicRow->setHeight(28);
    musicRow->setGap(6);
    this->appendChild(musicRow);

    m_musicFolderInput = new SPInputElement(musicRow);
    m_musicFolderInput->setFlex(1);
    m_musicFolderInput->setHeight(28);
    musicRow->appendChild(m_musicFolderInput);

    m_browseMusicBtn = makeButton(musicRow, "Browse…", 72, 28);
    m_browseMusicBtn->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&ConfigView::onBrowseMusicClicked),
        this, m_browseMusicBtn
    );

    // ── Federated Handle section ───────────────────────────────────────────
    // New playlists are shared as musik:federated:user:<handle>:playlist:<id>.
    makeLabel(this, "Federated Handle (@user@instance)", 20);

    m_federatedHandleInput = new SPInputElement(this);
    m_federatedHandleInput->setId("federatedHandleInput");
    m_federatedHandleInput->setHeight(28);
    this->appendChild(m_federatedHandleInput);

    loadSettings();
}

ConfigView::~ConfigView() {}

// ── URI handling ──────────────────────────────────────────────────────────────

bool ConfigView::acceptsUri(const string& uri) {
    return uri == "bungalow:config" || uri.rfind("bungalow:config", 0) == 0;
}

void ConfigView::navigate(const string& uri) {
    this->setUri(uri);
}

void ConfigView::setVisible(bool value) {
    SPViewElement::setVisible(value);
    // Refresh pending values from disk so saving here doesn't overwrite
    // colour changes made elsewhere (e.g. the PlayerView sliders).
    if (value)
        loadSettings();
}

// ── Settings path ─────────────────────────────────────────────────────────────

string ConfigView::getSettingsPath() {
    return platform::userDataDir() + "/settings.json";
}

string ConfigView::getFederatedHandle() {
    std::ifstream file(getSettingsPath());
    if (file.is_open()) {
        std::ostringstream ss;
        ss << file.rdbuf();
        try {
            nlohmann::json obj = nlohmann::json::parse(ss.str());
            if (obj.contains("federatedHandle") && obj.at("federatedHandle").is_string()) {
                const string handle = obj.at("federatedHandle").get<string>();
                if (playlist::Upsl::isValidHandle(handle)) return handle;
            }
        } catch (...) {}
    }
    return playlist::Upsl::defaultHandle();
}

bool ConfigView::getForceExternalLinks() {
    std::ifstream file(getSettingsPath());
    if (file.is_open()) {
        std::ostringstream ss;
        ss << file.rdbuf();
        try {
            nlohmann::json obj = nlohmann::json::parse(ss.str());
            if (obj.contains("forceExternalLinks") && obj.at("forceExternalLinks").is_boolean())
                return obj.at("forceExternalLinks").get<bool>();
        } catch (...) {}
    }
    return false;
}

// ── Button state helpers ──────────────────────────────────────────────────────

void ConfigView::updateModeButtons() {
    if (m_lightButton)
        m_lightButton->setBackgroundColor(
            m_pendingScheme == SP_COLOR_SCHEME_LIGHT ? kSelectedColor : kDeselectedColor);
    if (m_darkButton)
        m_darkButton->setBackgroundColor(
            m_pendingScheme == SP_COLOR_SCHEME_DARK ? kSelectedColor : kDeselectedColor);
}

void ConfigView::updateGlossyButton() {
    applyToggleButton(m_glossyButton, m_pendingGlossy, "Glossy: On", "Glossy: Off");
}

void ConfigView::updateAeroHeaderStyleButtons() {
    auto set = [&](SPButtonElement* btn, AeroHeaderStyle style) {
        if (btn) btn->setBackgroundColor(
            m_pendingAeroHeaderStyle == style ? kSelectedColor : kDeselectedColor);
    };
    set(m_aeroHeaderSolidBtn,        kAeroHeaderSolid);
    set(m_aeroHeaderGradientBtn,     kAeroHeaderGradient);
    set(m_aeroHeaderGlossyBtn,       kAeroHeaderGlossy);
    set(m_aeroHeaderAccentGlossyBtn, kAeroHeaderAccentGlossy);
    set(m_aeroHeaderDarkGlossyBtn,   kAeroHeaderDarkGlossy);
}

void ConfigView::updateAeroSectionVisibility() {
    if (!m_aeroSection) return;
    bool show = (m_pendingSkinType == "frutiger-aero");
    m_aeroSection->setVisible(show);
}

void ConfigView::syncSliderContext() {
    if (m_saturationSlider) m_saturationSlider->setLinkedHue(m_pendingAeroHue);
    if (m_lightnessSlider) {
        m_lightnessSlider->setLinkedHue(m_pendingAeroHue);
        m_lightnessSlider->setLinkedSat(m_pendingAeroSaturation);
    }
}

// ── Load / Save ───────────────────────────────────────────────────────────────

void ConfigView::loadSettings() {
    m_pendingScheme = SP_COLOR_SCHEME_LIGHT;
    m_pendingGlossy = true;

    std::ifstream file(getSettingsPath());
    if (file.is_open()) {
        std::ostringstream ss;
        ss << file.rdbuf();
        file.close();

        try {
            nlohmann::json obj = nlohmann::json::parse(ss.str());

            if (obj.contains("tintColor") && m_colorChooser) {
                string hex = obj.at("tintColor").get<string>();
                m_colorChooser->setSelectedColor(SPColor(hex));
            }
            if (obj.contains("colorScheme"))
                m_pendingScheme = static_cast<short>(obj.at("colorScheme").get<std::int64_t>());
            if (obj.contains("glossy"))
                m_pendingGlossy = obj.at("glossy").get<bool>();
            if (obj.contains("developerMode"))
                m_pendingDeveloperMode = obj.at("developerMode").get<bool>();
            if (obj.contains("skinType"))
                m_pendingSkinType = obj.at("skinType").get<string>();
            if (obj.contains("fullWidthAddressBar"))
                m_pendingFullWidthAddressBar = obj.at("fullWidthAddressBar").get<bool>();
            if (obj.contains("forceExternalLinks"))
                m_pendingForceExternalLinks = obj.at("forceExternalLinks").get<bool>();
            if (obj.contains("downloadsFolder") && m_downloadsFolderInput)
                m_downloadsFolderInput->setValue(
                    obj.at("downloadsFolder").get<string>());
            if (obj.contains("musicFolder") && m_musicFolderInput)
                m_musicFolderInput->setValue(
                    obj.at("musicFolder").get<string>());

            // Frutiger Aero colour params
            if (obj.contains("aeroHue"))
                m_pendingAeroHue = static_cast<float>(obj.at("aeroHue").get<double>());
            if (obj.contains("aeroSaturation"))
                m_pendingAeroSaturation = static_cast<float>(obj.at("aeroSaturation").get<double>());
            if (obj.contains("aeroLightness"))
                m_pendingAeroLightness = static_cast<float>(obj.at("aeroLightness").get<double>());
            if (obj.contains("aeroHeaderStyle"))
                m_pendingAeroHeaderStyle = static_cast<AeroHeaderStyle>(
                    obj.at("aeroHeaderStyle").get<std::int64_t>());
            if (obj.contains("buttonStyle"))
                m_pendingButtonStyle = static_cast<SPButtonStyle>(
                    obj.at("buttonStyle").get<std::int64_t>());
        } catch (...) {}
    }

    // Apply loaded values to slider widgets
    if (m_hueSlider)        m_hueSlider->setValue(m_pendingAeroHue);
    if (m_saturationSlider) m_saturationSlider->setValue(m_pendingAeroSaturation);
    if (m_lightnessSlider)  m_lightnessSlider->setValue(m_pendingAeroLightness);
    syncSliderContext();
    // Sync color chooser to reflect the loaded HSL values
    if (m_colorChooser)
        m_colorChooser->setSelectedColor(
            SPColor::fromHSL(m_pendingAeroHue, m_pendingAeroSaturation, 0.55f, 255));

    updateModeButtons();
    updateGlossyButton();
    if (m_buttonStyleDropdown)
        m_buttonStyleDropdown->setSelectedValue(std::to_string(m_pendingButtonStyle));
    if (m_skinDropdown)
        m_skinDropdown->setSelectedValue(m_pendingSkinType);
    updateDeveloperModeButton();
    updateFullWidthAddressBarButton();
    updateForceExternalLinksButton();
    updateAeroHeaderStyleButtons();
    updateAeroSectionVisibility();

    if (m_downloadsFolderInput && m_downloadsFolderInput->getValue().empty())
        m_downloadsFolderInput->setValue(DownloadManager::getDownloadsFolder());
    if (m_musicFolderInput && m_musicFolderInput->getValue().empty())
        m_musicFolderInput->setValue(DownloadManager::getMusicFolder());
    if (m_federatedHandleInput)
        m_federatedHandleInput->setValue(getFederatedHandle());
}

void ConfigView::saveSettings() {
    if (!m_colorChooser) return;

    SPColor color = m_colorChooser->getSelectedColor();

    // Merge into the existing file so keys owned elsewhere (showPlayQueue) survive.
    nlohmann::json obj = nlohmann::json::object();
    {
        std::ifstream in(getSettingsPath());
        if (in.is_open()) {
            std::ostringstream ss;
            ss << in.rdbuf();
            try {
                nlohmann::json existing = nlohmann::json::parse(ss.str());
                if (existing.is_object()) obj = std::move(existing);
            } catch (...) {}
        }
    }
    obj["tintColor"]            = color.toHTMLColor();
    obj["colorScheme"]          = m_pendingScheme;
    obj["glossy"]               = m_pendingGlossy;
    obj["developerMode"]        = m_pendingDeveloperMode;
    obj["skinType"]             = m_pendingSkinType;
    obj["fullWidthAddressBar"]  = m_pendingFullWidthAddressBar;
    obj["forceExternalLinks"]   = m_pendingForceExternalLinks;
    obj["aeroHue"]              = static_cast<double>(m_pendingAeroHue);
    obj["aeroSaturation"]       = static_cast<double>(m_pendingAeroSaturation);
    obj["aeroLightness"]        = static_cast<double>(m_pendingAeroLightness);
    obj["aeroHeaderStyle"]      = static_cast<int>(m_pendingAeroHeaderStyle);
    obj["buttonStyle"]          = static_cast<int>(m_pendingButtonStyle);
    if (m_downloadsFolderInput && !m_downloadsFolderInput->getValue().empty())
        obj["downloadsFolder"] = m_downloadsFolderInput->getValue();
    if (m_musicFolderInput && !m_musicFolderInput->getValue().empty())
        obj["musicFolder"] = m_musicFolderInput->getValue();
    // An invalid handle is not saved; the field shows the current one again on load.
    if (m_federatedHandleInput && playlist::Upsl::isValidHandle(m_federatedHandleInput->getValue()))
        obj["federatedHandle"] = m_federatedHandleInput->getValue();

    std::ofstream file(getSettingsPath());
    if (file.is_open()) {
        file << obj.dump();
        file.close();
    }

    // Apply all settings instantly
    auto *win = this->getWindowElement();
    if (!win) return;

    win->setTintColor(SPColor::fromHSL(m_pendingAeroHue, m_pendingAeroSaturation, m_pendingAeroLightness));
    win->setColorScheme(m_pendingScheme);

    SPSkin* newSkin = nullptr;
    if (m_pendingSkinType == "frutiger-aero") {
        auto* aero = new SPFrutigerAeroSkin();
        aero->setHeaderStyle(m_pendingAeroHeaderStyle);
        aero->setGlossy(m_pendingGlossy);
        aero->setButtonStyle(m_pendingButtonStyle);
        newSkin = aero;
    } else if (m_pendingSkinType == "classic") {
        newSkin = new SPClassicSkin();
        newSkin->setGlossy(m_pendingGlossy);
        newSkin->setButtonStyle(m_pendingButtonStyle);
    } else {
        // "modern" is the procedural skin; anything else is a skin.xml bundle,
        // falling back to the procedural skin if the bundle is missing.
        if (m_pendingSkinType != "modern")
            newSkin = SPAppManager::instance().createSkin(m_pendingSkinType);
        if (!newSkin) newSkin = new SPModernSkin();
        newSkin->setGlossy(m_pendingGlossy);
        newSkin->setButtonStyle(m_pendingButtonStyle);
    }
    // Hue/saturation/lightness apply to every skin (same as at startup).
    newSkin->setHue(m_pendingAeroHue);
    newSkin->setSaturation(m_pendingAeroSaturation);
    newSkin->setLightness(m_pendingAeroLightness);
    win->setSkin(newSkin);

    if (auto* a = BungalowApp::current()) {
        a->setDeveloperMode(m_pendingDeveloperMode);
        a->setFullWidthAddressBar(m_pendingFullWidthAddressBar);
    }

    auto *gc = win->createGraphics();
    win->invalidateRegion({0, 0, gc->getWidth(), gc->getHeight()});
}

// ── Standard callbacks ────────────────────────────────────────────────────────

int ConfigView::onSaveClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    saveSettings();
    return 0;
}

int ConfigView::onLightClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingScheme = SP_COLOR_SCHEME_LIGHT;
    updateModeButtons();
    this->invalidate();
    return 0;
}

int ConfigView::onDarkClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingScheme = SP_COLOR_SCHEME_DARK;
    updateModeButtons();
    this->invalidate();
    return 0;
}

int ConfigView::onGlossyClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingGlossy = !m_pendingGlossy;
    updateGlossyButton();
    this->invalidate();
    return 0;
}

int ConfigView::onBrowseDownloadsClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    std::string current = m_downloadsFolderInput
        ? std::string(m_downloadsFolderInput->getValue())
        : platform::homeDir() + "/Downloads";

    std::string chosen = platform::showFolderDialog("Choose Downloads Folder", current);
    if (!chosen.empty() && m_downloadsFolderInput)
        m_downloadsFolderInput->setValue(chosen);
    return 0;
}

void ConfigView::updateDeveloperModeButton() {
    applyToggleButton(m_developerModeButton, m_pendingDeveloperMode,
                      "Developer Mode: On", "Developer Mode: Off");
}

int ConfigView::onDeveloperModeClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingDeveloperMode = !m_pendingDeveloperMode;
    updateDeveloperModeButton();
    this->invalidate();
    return 0;
}

void ConfigView::updateFullWidthAddressBarButton() {
    applyToggleButton(m_fullWidthAddressBarButton, m_pendingFullWidthAddressBar,
                      "Full Width: On", "Full Width: Off");
}

void ConfigView::updateForceExternalLinksButton() {
    applyToggleButton(m_forceExternalLinksButton, m_pendingForceExternalLinks,
                      "Open in System Browser: On", "Open in System Browser: Off");
}

int ConfigView::onForceExternalLinksClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingForceExternalLinks = !m_pendingForceExternalLinks;
    updateForceExternalLinksButton();
    this->invalidate();
    return 0;
}

int ConfigView::onFullWidthAddressBarClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingFullWidthAddressBar = !m_pendingFullWidthAddressBar;
    updateFullWidthAddressBarButton();
    this->invalidate();
    return 0;
}

int ConfigView::onBrowseMusicClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    std::string current = m_musicFolderInput
        ? std::string(m_musicFolderInput->getValue())
        : platform::homeDir() + "/Music";

    std::string chosen = platform::showFolderDialog("Choose Music Folder", current);
    if (!chosen.empty() && m_musicFolderInput)
        m_musicFolderInput->setValue(chosen);
    return 0;
}

// ── Aero header style callbacks ───────────────────────────────────────────────

int ConfigView::onAeroHeaderSolidClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingAeroHeaderStyle = kAeroHeaderSolid;
    updateAeroHeaderStyleButtons();
    this->invalidate();
    return 0;
}

int ConfigView::onAeroHeaderGradientClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingAeroHeaderStyle = kAeroHeaderGradient;
    updateAeroHeaderStyleButtons();
    this->invalidate();
    return 0;
}

int ConfigView::onAeroHeaderGlossyClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingAeroHeaderStyle = kAeroHeaderGlossy;
    updateAeroHeaderStyleButtons();
    this->invalidate();
    return 0;
}

int ConfigView::onAeroHeaderAccentGlossyClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingAeroHeaderStyle = kAeroHeaderAccentGlossy;
    updateAeroHeaderStyleButtons();
    this->invalidate();
    return 0;
}

int ConfigView::onAeroHeaderDarkGlossyClicked(SPObject* /*sender*/, SPEventArgs* /*e*/) {
    m_pendingAeroHeaderStyle = kAeroHeaderDarkGlossy;
    updateAeroHeaderStyleButtons();
    this->invalidate();
    return 0;
}

} // namespace views
} // namespace space
