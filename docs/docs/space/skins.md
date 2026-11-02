---
title: Skins
sidebar_position: 5
---

# Skins

The look of every element comes from the active **skin**, an `SPSkin` set on
the window element. Elements ask the skin to draw them. Anything the skin
doesn't handle falls back to the element's own drawing.

## The `SPSkin` interface

```cpp
class SPSkin {
public:
    // Tint applied to the whole skin (the HSL sliders)
    void setHue(float h);        // 0–360
    void setSaturation(float s); // 0–1
    void setLightness(float l);  // 0–1

    // Derived palette
    SPColor bg(short scheme) const;   // bgLight() / bgDark()
    SPColor accent() const;
    SPColor tabStart() const;
    SPColor tabEnd() const;
    SPColor textColor(short scheme) const;

    // Drawing hooks
    virtual bool drawElement(SPGraphicsContext*, SPElement*, const string& state, const sp_rectangle&);
    virtual sp_rectangle getContentBounds(SPElement*, const sp_rectangle&);
    virtual std::optional<SPColor> getTextColor(SPElement*, const string& state);
    virtual void drawTableRow(…);
    virtual void drawTableHeader(…);
    virtual void drawTabBar(…);
    virtual void drawTabBarTab(…);
    virtual void drawTreeItem(…);
    virtual void drawSectionHeader(…);

    virtual void setGlossy(bool);
    virtual void setButtonStyle(SPButtonStyle); // flat, gradient, modern gradient, aqua
};
```

`drawElement()` returns `true` when the skin painted the element. States are
`normal`, `hover` and `active`, and every colour can differ between the dark
and light colour schemes.

## Built-in skins

| `skinType` | Class | Look |
|------------|-------|------|
| `modern` (default) | `SPModernSkin` | flat, procedurally drawn |
| `classic` | `SPClassicSkin` | classic bevelled controls |
| `frutiger-aero` | `SPFrutigerAeroSkin` | Vista-era glass, inspired by Windows Media Player |

`SPFrutigerAeroSkin` derives its whole palette from hue, saturation and
lightness. Its header and footer have five styles that follow the Windows Media
Player versions:

| Style | Inspired by |
|-------|-------------|
| `kAeroHeaderSolid` | flat |
| `kAeroHeaderGradient` | WMP 9 |
| `kAeroHeaderGlossy` | WMP 10 |
| `kAeroHeaderAccentGlossy` | WMP 11 beta |
| `kAeroHeaderDarkGlossy` | WMP 11 final |

Button styles (`SPButtonStyle`): `kButtonFlat`, `kButtonGradient`,
`kButtonModernGradient` and `kButtonAqua` (the default, a Mac OS X Aqua-style
gel). The glossy setting adds a separate shine layer on top of any style.

## Skin bundles (`skin.xml`)

Any other `skinType` is looked up as a **skin extension in an app bundle**
(`SPAppManager::createSkin()`). The bundled `Apps/modern` app contains one:

```
Apps/modern/
├── manifest.json                       BundleType: Application
└── skins/modern/
    ├── manifest.json                   BundleType: Skin, BundleIdentifier: modern.skin
    ├── skin.xml
    └── images/skin.png
```

`createSkin()` accepts the full identifier (`modern.skin`) or the short form
(`modern`). If the bundle is missing, the app falls back to `SPModernSkin`.

A `skin.xml` (`SPXmlSkin`) declares resources and selectors. An excerpt from
`Apps/modern/skins/modern/skin.xml`:

```xml
<skin>
  <name>Modern Skin</name>
  <resources>
    <image src="sp://modern/skins/modern/images/skin.png" id="skin" />
    <color name="button">
      <state mode="dark"  foreground="#1a1a1a" />
      <state mode="light" foreground="#1a1a1a" />
    </color>
  </resources>
  <selector match="sp-button">
    <property name="skin" value="url(#skin)" slice="33,81;79,95" nine-patch="5,4,5,4" />
    <property name="color" value="$button" />
  </selector>
  <selector match="sp-button" state="active">
    <property name="skin" value="url(#skin)" slice="33,97;79,111" nine-patch="5,4,5,4" />
    <property name="color" value="$button-active" />
  </selector>
</skin>
```

Selectors match an element's type with an `sp-` prefix (`sp-button`,
`sp-view`, `sp-viewstack`, `sp-tabbarview`, …).

| Property | Value |
|----------|-------|
| `background` | `$color` or `#rrggbb` for a solid fill, or `url(#imageId)` |
| `skin` / `background` with `slice="x1,y1;x2,y2"` and `nine-patch="l,t,r,b"` | a nine-patch sprite from the sheet |
| `padding` | `l,t,r,b` content inset (`getContentBounds`) |
| `color` | label colour for buttons and check boxes |
| `radius` | rounds solid backgrounds |

A selector with `mode="dark"` or `mode="light"` takes precedence over one
without a mode in that colour scheme and is ignored in the other. Sprites and
colours are tinted by the skin's hue, saturation and lightness. Opt a resource
out with `tint="false"`, or set a minimum saturation with
`tint-saturation="0.35"`. An image can declare `brightness="0.4"` for a darker
variant of the same sheet. Anything the stylesheet doesn't style falls back to
`SPModernSkin`.

## Settings

The skin and its parameters are saved in `settings.json` and applied at startup:
`skinType`, `colorScheme`, `tintColor`, `glossy`, `buttonStyle`, `aeroHue`,
`aeroSaturation`, `aeroLightness` and `aeroHeaderStyle`. The hue, saturation
and lightness sliders (in Now Playing and Settings) apply to every skin. See
[Settings and files](../reference/settings-and-files.md).

When the skin, colour scheme or tint changes, `SPGraphicsContext::themeChanged()`
lets the backend drop any assets it rendered in advance.
