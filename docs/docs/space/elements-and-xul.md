---
title: Elements and XUL
sidebar_position: 2
---

# Elements and XUL

Space describes UI in **XUL-like XML**. Tags map to `SP*Element` classes, and
attributes map to element properties. The markup is parsed by `SPXmlParser`
(built on Expat).

## The shell: `views/app.xml`

The main window is itself XUL:

```xml
<document>
  <appheader id="header" height="120">
    <button id="backButton">&lt;</button>
    <button id="forwardButton">&gt;</button>
    <input id="searchInput" />
    <input id="addressInput" />
  </appheader>
  <hbox flex="1" id="body">
    <vbox width="300" id="leftSidebar">
      <tabbar id="leftTabBar" height="28" />
      <treeview id="treeview" width="228" flex="1" />
    </vbox>
    <splitter />
    <viewstack flex="1" id="viewstack" />
  </hbox>
  <appfooter id="footer" height="30" />
</document>
```

`ui::MainWindow` loads it with `loadXUL("views/app.xml")`, finds the parts with
`getElementById()` and wires them up in `layout()`. Elements that `MainWindow`
creates itself are returned from its `createElement()` override.

## Content tags

These tags are available in view markup and templates (`SPElement::createElement`):

| Tag | Element | Notes |
|-----|---------|-------|
| `<view>` | tab-bar view | its `<page>` children become tabs |
| `<page>` | `SPPageElement` | one tab; `title` is the caption, `id` the tab id (`#fragment`) |
| `<section>` | `SPSectionElement` | a titled block |
| `<vbox>` / `<hbox>` | `SPVBoxElement` / `SPHBoxElement` | column / row layout |
| `<splitter>` | `SPSplitterElement` | a draggable divider |
| `<text>` | `SPTextElement` | text, from the element body |
| `<img>` | `SPImageElement` | an image |
| `<button>` | `SPButtonElement` | `onclick` posts back the named action |
| `<input>` | `SPInputElement` | a named field is sent with postbacks |
| `<checkbox>` | `SPCheckBoxElement` | a check box |

Shell-level elements such as `<appheader>`, `<appfooter>`, `<tabbar>`,
`<treeview>` and `<viewstack>` are used in `app.xml`. The framework also has
tables (`SPTableViewElement` with `SPTableDataSource`), dropdowns, a seek bar,
the now-playing bar, a colour chooser and the Aero colour slider, which views
create in code.

## Common attributes

| Attribute | Meaning |
|-----------|---------|
| `id` | element id, for `getElementById()` |
| `width`, `height` | size in pixels, or percent (`50%`) |
| `flex` | share of the remaining space in a box |
| `gap` | space between children |
| `bgcolor`, `fgcolor` | background and foreground colour (`#rrggbb`) |
| `src` | a `bungalow:` URI or file to render into the element; see [Templates](./templates.md) |
| `entity`, `entity-field` | bind the element's text to a field of a URI entity; see [URI store](./uri-store.md) |
| `name` | field name for inputs and buttons in postbacks |
| `onclick` | the action a button posts back |
| `overflow` | `auto` scrolls content taller than the element, with the mouse wheel and a [skinned scrollbar](./skins.md#scrollbars); `hidden` (the default, except for views and the sidebar) clips it |

Changing `bgcolor`, `fgcolor`, `flex`, `gap`, `width`, `height`, `src`,
`entity` or `overflow` at run time updates the element (`attributechanged`).

The mouse wheel scrolls the innermost `overflow="auto"` element under the
pointer, and its ancestors once it has reached its end. Only vertical
scrolling is supported.

## Example

```xml
<view>
    <page title="Overview" id="overview">
        <text id="testtext">First view</text>
        <text bgcolor="#00ff00" id="time">Current time is ${os.time()}</text>
        <button id="test_button" name="refresh" onclick="refresh">Test</button>
    </page>
    <page title="Test" id="test">
        <text>Second view</text>
    </page>
</view>
```

This is `views/start.xml`, shown at `bungalow:start`. The two pages become tabs,
and `bungalow:start#test` opens the second one.

## Errors

If XUL fails to load, `SPElement::s_xulErrorHandler` is called. The application
sets it to show an error message. Unknown routes render `views/error.xml`.
