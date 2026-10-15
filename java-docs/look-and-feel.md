---
layout: default
title: Look & Feel
nav_order: 7
---

# Look & feel: Design = Chrome + Skin

The entire appearance is pluggable too. A **Design** is a premix of a **Chrome**
(the window shell and layout) and a **Skin** (how individual controls are
painted). Designs are contributed by plugins and selected at runtime in Settings,
so the same content can be shown as a Windows-Media-Player shell or a Spotify-like
shell without touching any view.

Types: [`Design`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/design/Design.java),
[`Chrome`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/chrome/Chrome.java),
[`Skin`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/skinning/Skin.java),
[`Theme`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/theme/Theme.java),
[`Taste`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/theme/Taste.java),
[`ThemeManager`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/theme/ThemeManager.java).

## The pieces

```
Design  ── getChrome() ─▶  Chrome   the window shell: header, sidebar split,
   │                                 center ViewStack, Now Playing, footer
   └────── getSkin()  ─▶   Skin     Graphics2D painters for controls
                                     (top bar, header, footer, buttons, tables…)
```

- **[`Design`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/design/Design.java)** — an `Aspect`
  with `getChrome()` + `getSkin()`. e.g.
  [`WMP1XDesign`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/plugin/wmp/design/WMP1XDesign.java)
  = WMP1X Chrome + WMP9 Skin; `SpotDesign` = Spot Chrome + Spot Skin.
- **[`Chrome`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/chrome/Chrome.java)** — a `Panel`
  that, in `build()`, assembles the shell: `appHeader`, the left split holding the
  sidebar + a `centerPanel` wrapping the shared `ViewStack`, the main split with
  the Now Playing panel, and the `appFooter`. Different chromes lay these out
  differently (WMP's tabbed header vs Spot's slim header).
- **[`Skin`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/skinning/Skin.java)** — abstract painter:
  `paintHeader`, `paintFooter`, `paintTopBar`, `paintGlossyButton`,
  `paintTableHeader`, `paintGlassPanel`, … Controls call into the active Skin in
  their `paintComponent`, so a skin restyles every control at once.
- **[`Theme`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/theme/Theme.java)** — may also carry
  a design/skin/chrome, letting a theme override the look wholesale.

## Taste vs ThemeManager (the colour model)

There are two colour holders, and the split is worth understanding:

- **[`Taste`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/theme/Taste.java)** — the *per-window
  model* the UI writes to: tint HSL (hue/saturation/lightness), dark mode, accent
  colour, display toggles, and the active theme/design/skin/chrome. The Settings
  sliders and `ConfigManager` read/write the `Taste`.
- **[`ThemeManager`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/ui/theme/ThemeManager.java)** —
  the *live global palette* everything renders from: `getBackground()`,
  `getTintColor()`, `getForeground()`, the Nimbus `UIDefaults`, and a list of
  change listeners (~10 components repaint on change).

`Taste`'s setters **forward into `ThemeManager`** (which recomputes the palette
and repaints every listener) and then fire the `Taste`'s own listeners (theme
rebuild + config save). Without that bridge a tint change would reach only the
few elements that read the `Taste` directly — so this forwarding is what makes a
tint apply app-wide.

```
slider → Taste.setHue(h) ──┬─▶ ThemeManager.setHue(h) → applyToDefaults() → repaint all listeners
                           └─▶ Taste.notify_()        → rebuild theme + ConfigManager.save()
```

## Selecting & persisting a Design

- `MainWindow.setDesign(design)` sets the Taste's design, swaps the Skin, then
  `installChrome()` calls `chrome.build()` and adds it to the window's CENTER.
- The Chrome is handed the **shared** `ViewStack` and `LeftLibraryMenu` before
  `build()`, so the plugin-contributed sidebar and the live view survive a Design
  switch.
- `pickInitialDesign()` chooses the persisted design, else `wmp1x` (default),
  else `spot`, else the first registered Design.
- The Settings panel lists every registered Design and applies the chosen one
  live; the selection is persisted as `design.id` in `~/.spacify/settings.properties`.

## Why this matters for the ecosystem

Look-and-feel being *just another plugin contribution* means the visual identity
of the "open music OS" is community-ownable: a Frutiger-Aero glass Design, a
retro WMP Design, and a modern streaming Design can all ship as plugins and be
switched at runtime, while every content view stays exactly the same.
