#include "PlayerView.h"

#include <cstdint>
#include "SPTextElement.h"
#include "SPVBoxElement.h"
#include "SPWindowElement.h"
#include "SPFrutigerAeroSkin.h"
#include "SPGraphicsContext.h"
#include "SPVisualization.h"
#include "SPPlayerCanvasImplementationElement.h"
#include "ConfigView.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

namespace space {
namespace views {

PlayerView::PlayerView() {}

PlayerView::PlayerView(SPElement* parent) : SPViewElement(parent) {
    this->clipView = false;
    this->setUri("bungalow:player");

    // ── Top: canvas slot (flex, fills available space) ────────────────────────
    m_canvas = new SPPlayerCanvasElement(this);
    m_canvas->setFlex(1);
    this->appendChild(m_canvas);

    // Default implementation: music visualizer.
    m_vizImpl = new SPMusicVisualizerCanvasElement(m_canvas);
    m_canvas->setImplementation(m_vizImpl);

    // Rebuild the parameter panel whenever the active implementation changes.
    m_canvas->addEventListener(
        "implementationChanged",
        reinterpret_cast<SPCallback>(&PlayerView::onImplementationChanged),
        this,
        nullptr
    );

    // ── Visualizer parameter panel (rebuilt dynamically) ──────────────────────
    m_paramPanel = new SPVBoxElement(this);
    m_paramPanel->setBackgroundColor(SPColor(25, 25, 25, 255));
    m_paramPanel->setPadding({ 8, 12, 8, 12 });
    m_paramPanel->setGap(4);
    m_paramPanel->setHeight(0); // hidden until the active impl provides params
    this->appendChild(m_paramPanel);

    // ── Bottom: colour control bar ────────────────────────────────────────────
    // Height = 3 × (label 16 + slider 20) + 5 × gap 4 + padding top 12 + bottom 12 = 152
    auto* colorBar = new SPVBoxElement(this);
    colorBar->setPadding({ 12, 16, 12, 16 });
    colorBar->setGap(4);
    colorBar->setHeight(252);
    colorBar->setBackgroundColor(SPColor(30, 30, 30, 255));
    this->appendChild(colorBar);

    // Hue
    auto* hueLabel = new SPTextElement(colorBar);
    hueLabel->setInnerText("Hue");
    hueLabel->setHeight(16);
    colorBar->appendChild(hueLabel);

    m_hueSlider = new SPAeroColorSlider(colorBar, SPAeroColorSlider::Type::Hue);
    m_hueSlider->setHeight(20);
    colorBar->appendChild(m_hueSlider);
    m_hueSlider->onChange = [this](float v) {
        m_hue = v;
        syncSliderContext();
        applyLive();
        saveSettings();
    };

    // Saturation
    auto* satLabel = new SPTextElement(colorBar);
    satLabel->setInnerText("Saturation");
    satLabel->setHeight(16);
    colorBar->appendChild(satLabel);

    m_saturationSlider = new SPAeroColorSlider(colorBar, SPAeroColorSlider::Type::Saturation);
    m_saturationSlider->setHeight(20); 
    colorBar->appendChild(m_saturationSlider);
    m_saturationSlider->onChange = [this](float v) {
        m_saturation = v;
        syncSliderContext();
        applyLive();
        saveSettings();
    };

    // Lightness
    auto* lightnessLabel = new SPTextElement(colorBar);
    lightnessLabel->setInnerText("Lightness");
    lightnessLabel->setHeight(16);
    colorBar->appendChild(lightnessLabel);

    m_lightnessSlider = new SPAeroColorSlider(colorBar, SPAeroColorSlider::Type::Lightness);
    m_lightnessSlider->setHeight(20);
    m_lightnessSlider->setFlex(1);
    colorBar->appendChild(m_lightnessSlider);
    m_lightnessSlider->onChange = [this](float v) {
        m_lightness = v;
        syncSliderContext();
        applyLive();
        saveSettings();
    };

    loadSettings();
}

PlayerView::~PlayerView() {}

// ── URI handling ──────────────────────────────────────────────────────────────

bool PlayerView::acceptsUri(const std::string& uri) {
    return uri == "bungalow:player" || uri.rfind("bungalow:player:", 0) == 0;
}

void PlayerView::setCanvasImplementation(SPPlayerCanvasImplementationElement* impl) {
    if (m_canvas)
        m_canvas->setImplementation(impl);
}

void PlayerView::setVisible(bool value) {
    SPViewElement::setVisible(value);
    if (m_canvas)
        m_canvas->setVisible(value);
    // Settings may have been changed from ConfigView since we last loaded.
    if (value)
        loadSettings();
}

void PlayerView::navigate(const std::string& uri) {
    this->setUri(uri);
}

// ── Settings ──────────────────────────────────────────────────────────────────

void PlayerView::loadSettings() {
    std::ifstream file(views::ConfigView::getSettingsPath());
    if (file.is_open()) {
        std::ostringstream ss;
        ss << file.rdbuf();
        file.close();
        try {
            nlohmann::json obj = nlohmann::json::parse(ss.str());

            if (obj.contains("colorScheme"))
                m_scheme = static_cast<short>(obj.at("colorScheme").get<std::int64_t>());
            if (obj.contains("glossy"))
                m_glossy = obj.at("glossy").get<bool>();
            if (obj.contains("skinType"))
                m_skinType = obj.at("skinType").get<std::string>();
            if (obj.contains("aeroHue"))
                m_hue = static_cast<float>(obj.at("aeroHue").get<double>());
            if (obj.contains("aeroSaturation"))
                m_saturation = static_cast<float>(obj.at("aeroSaturation").get<double>());
            if (obj.contains("aeroLightness"))
                m_lightness = static_cast<float>(obj.at("aeroLightness").get<double>());
            if (obj.contains("aeroHeaderStyle"))
                m_headerStyle = static_cast<AeroHeaderStyle>(
                    obj.at("aeroHeaderStyle").get<std::int64_t>());
            if (obj.contains("developerMode"))
                m_developerMode = obj.at("developerMode").get<bool>();
            if (obj.contains("fullWidthAddressBar"))
                m_fullWidthAddressBar = obj.at("fullWidthAddressBar").get<bool>();
            if (obj.contains("tintColor"))
                m_tintColor = obj.at("tintColor").get<std::string>();
            if (obj.contains("downloadsFolder"))
                m_downloadsFolder = obj.at("downloadsFolder").get<std::string>();
            if (obj.contains("musicFolder"))
                m_musicFolder = obj.at("musicFolder").get<std::string>();
        } catch (...) {}
    }

    if (m_hueSlider)        m_hueSlider->setValue(m_hue);
    if (m_saturationSlider) m_saturationSlider->setValue(m_saturation);
    if (m_lightnessSlider)  m_lightnessSlider->setValue(m_lightness);
    syncSliderContext();
    applyLive();
}

void PlayerView::saveSettings() {
    // Merge into the current file rather than rewriting it from our cached
    // copy, which may be stale (ConfigView saves to the same file) and does
    // not cover every key (e.g. buttonStyle).
    nlohmann::json obj = nlohmann::json::object();
    {
        std::ifstream in(views::ConfigView::getSettingsPath());
        if (in.is_open()) {
            std::ostringstream ss;
            ss << in.rdbuf();
            try {
                nlohmann::json existing = nlohmann::json::parse(ss.str());
                if (existing.is_object()) obj = std::move(existing);
            } catch (...) {}
        }
    }
    obj["aeroHue"]        = static_cast<double>(m_hue);
    obj["aeroSaturation"] = static_cast<double>(m_saturation);
    obj["aeroLightness"]  = static_cast<double>(m_lightness);

    std::ofstream file(views::ConfigView::getSettingsPath());
    if (file.is_open())
        file << obj.dump();
}

void PlayerView::applyLive() {
    auto* win = this->getWindowElement();
    if (!win) return;

    auto* skin = this->getSkin();
    if (skin != nullptr) {
        skin->setHue(m_hue);
        skin->setSaturation(m_saturation);
        skin->setLightness(m_lightness);
        win->setTintColor(SPColor::fromHSL(m_hue, m_saturation, m_lightness));
    }

    auto* gc = win->createGraphics();
    win->invalidateRegion({ 0, 0, gc->getWidth(), gc->getHeight() });
}

void PlayerView::syncSliderContext() {
    if (m_saturationSlider) m_saturationSlider->setLinkedHue(m_hue);
    if (m_lightnessSlider) {
        m_lightnessSlider->setLinkedHue(m_hue);
        m_lightnessSlider->setLinkedSat(m_saturation);
    }
}

// ── Visualizer parameter panel ────────────────────────────────────────────────

int PlayerView::onImplementationChanged(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    rebuildParamPanel();
    return 0;
}

void PlayerView::rebuildParamPanel() {
    if (!m_paramPanel) return;

    m_paramPanel->clear();

    SPPlayerCanvasImplementationElement* impl =
        m_canvas ? m_canvas->getImplementation() : nullptr;
    std::vector<SPVisualizationParam>* params =
        impl ? impl->getVisualizationParams() : nullptr;

    if (!params || params->empty()) {
        m_paramPanel->setHeight(0);
        this->pack();
        return;
    }

    // Children layout: [name] [label + ctrl] × N
    // Total height = padding(8+8) + name(16) + gap×(2N) + N×(label(16)+ctrl(20))
    //              = 32 + 44×N
    m_paramPanel->setHeight(32 + 44 * (int)params->size());

    // ── Implementation name header ────────────────────────────────────────────
    auto* nameLabel = new SPTextElement(m_paramPanel);
    nameLabel->setInnerText(impl->getName());
    nameLabel->setHeight(16);
    nameLabel->setForegroundColor(SPColor(200, 200, 200, 255));
    nameLabel->setIntegerAttribute("size", 11);
    m_paramPanel->appendChild(nameLabel);

    // ── One control per parameter ─────────────────────────────────────────────
    for (size_t pi = 0; pi < params->size(); ++pi) {
        auto& param = (*params)[pi];

        auto* lbl = new SPTextElement(m_paramPanel);
        lbl->setInnerText(param.label);
        lbl->setHeight(16);
        lbl->setForegroundColor(SPColor(160, 160, 160, 255));
        lbl->setIntegerAttribute("size", 10);
        m_paramPanel->appendChild(lbl);

        if (std::holds_alternative<SPVisualizationParam::FloatParam>(param.data)) {
            auto& fp = std::get<SPVisualizationParam::FloatParam>(param.data);
            float range      = fp.max - fp.min;
            float normalised = (range > 0.0f) ? (fp.value - fp.min) / range : 0.0f;

            auto* slider = new SPAeroColorSlider(m_paramPanel,
                                                  SPAeroColorSlider::Type::Lightness);
            slider->setHeight(20);
            slider->setValue(normalised);
            // Capture by index into the stable params vector owned by the implementation.
            slider->onChange = [params, pi](float v) {
                auto& fp2 = std::get<SPVisualizationParam::FloatParam>(
                    (*params)[pi].data);
                fp2.value = fp2.min + v * (fp2.max - fp2.min);
            };
            m_paramPanel->appendChild(slider);

        } else if (std::holds_alternative<SPVisualizationParam::BoolParam>(param.data)) {
            auto& bp = std::get<SPVisualizationParam::BoolParam>(param.data);
            auto* val = new SPTextElement(m_paramPanel);
            val->setHeight(20);
            val->setForegroundColor(SPColor(130, 200, 130, 255));
            val->setIntegerAttribute("size", 10);
            val->setInnerText(bp.value ? "Enabled" : "Disabled");
            m_paramPanel->appendChild(val);

        } else if (std::holds_alternative<SPVisualizationParam::EnumParam>(param.data)) {
            auto& ep = std::get<SPVisualizationParam::EnumParam>(param.data);
            auto* val = new SPTextElement(m_paramPanel);
            val->setHeight(20);
            val->setForegroundColor(SPColor(130, 180, 220, 255));
            val->setIntegerAttribute("size", 10);
            if (!ep.options.empty())
                val->setInnerText(ep.options[ep.value % ep.options.size()]);
            m_paramPanel->appendChild(val);

        } else if (std::holds_alternative<SPVisualizationParam::ColorParam>(param.data)) {
            auto& cp = std::get<SPVisualizationParam::ColorParam>(param.data);
            auto* swatch = new SPTextElement(m_paramPanel);
            swatch->setHeight(20);
            swatch->setBackgroundColor(cp.value);
            swatch->setInnerText("");
            m_paramPanel->appendChild(swatch);
        }
    }

    this->pack();
    this->invalidate();
}

} // namespace views
} // namespace space
