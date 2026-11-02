---
title: Settings and files
sidebar_position: 2
---

# Settings and files

## User data directory

Bungalow keeps its data in the user data directory,
`space::platform::userDataDir()`. On Windows this is `%APPDATA%\Bungalow`.

| Path | Contents |
|------|----------|
| `settings.json` | settings (below) |
| `bungalow.db` | the library database, unless `resources/bungalow.db` exists next to the executable |
| `cache/uris/` | the URI store's JSON cache, one file per URI |
| `favicons/` | cached favicons for bookmarked sites and apps |
| `cef/` | the CEF profile and cache (WTL with CEF) |

## Folders the user can add apps to

See [App bundles](../apps/bundles.md#search-paths):

```
~/Bungalow
~/Bungalow/Apps
~/Documents/Bungalow
~/Documents/Bungalow/Apps
```

## `settings.json`

Written by the Settings view (`bungalow:config`, `ConfigView`) and read at
startup:

| Key | Type | Default | Meaning |
|-----|------|---------|---------|
| `skinType` | string | `"modern"` | `modern`, `classic`, `frutiger-aero`, or a skin bundle id |
| `colorScheme` | number | `1` (dark) | `0` default, `1` dark, `2` light |
| `tintColor` | string | | tint colour |
| `glossy` | bool | `false` | glossy shine on buttons and bars |
| `buttonStyle` | number | `3` (Aqua) | `0` flat, `1` gradient, `2` modern gradient, `3` Aqua |
| `aeroHue` | number | `200` | skin hue, 0–360 |
| `aeroSaturation` | number | `0` | skin saturation, 0–1 |
| `aeroLightness` | number | `0.5` | skin lightness, 0–1 |
| `aeroHeaderStyle` | number | `2` (glossy) | Frutiger Aero header style, `0`–`4` |
| `fullWidthAddressBar` | bool | `true` | show the address field in the header |
| `developerMode` | bool | `false` | DevTools button for apps and sites |

Despite the `aero` prefix, the hue, saturation and lightness apply to every
skin.

## Files next to the executable

| Path | Contents |
|------|----------|
| `views/app.xml` | the shell layout |
| `views/start.xml`, `views/artist.xml`, `views/error.xml` | templates for the catch-all view |
| `views/skin.json` | default colours |
| `icons/*.svg` | sidebar icons |
| `Apps/` | bundled app bundles |
| `resources/bungalow.db` | optional bundled database |
| CEF runtime files | copied after each build when CEF is enabled |
