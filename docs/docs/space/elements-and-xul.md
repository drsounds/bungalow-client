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
| `<text>` | `SPTextElement` | text, from the element body; may hold [links and formatting](#rich-text-and-links) |
| `<link>` / `<a>` | `SPLinkElement` | text that navigates to its `uri` (or `href`) when clicked |
| `<img>` | `SPImageElement` | an image, from `src`; with a `uri` it is a link |
| `<button>` | `SPButtonElement` | `onclick` posts back the named action |
| `<input>` | `SPInputElement` | a named field is sent with postbacks |
| `<checkbox>` | `SPCheckBoxElement` | a check box |

Shell-level elements such as `<appheader>`, `<appfooter>`, `<tabbar>`,
`<treeview>` and `<viewstack>` are used in `app.xml`. The framework also has
tables (`SPTableViewElement` with `SPTableDataSource`), dropdowns, a seek bar,
the now-playing bar, a colour chooser and the Aero colour slider, which views
create in code.

## Rich text and links

A text body, a table cell and any label set with `setInnerHTML()` can use a
small subset of HTML (`SPRichText`):

| Markup | Meaning |
|--------|---------|
| `<link uri="…">text</link>`, `<a href="…">text</a>` | a link; a click navigates to the URI |
| `<b>`, `<strong>`, `<i>`, `<em>`, `<u>` | bold, italic, underlined |
| `<br />` | a line break (lines do not wrap by themselves) |
| `<img src="…" uri="…" width="…" height="…" />` | an image in the line; `uri` makes it a link |
| `&amp;` `&lt;` `&gt;` `&quot;` `&apos;` `&nbsp;` `&#N;` | entities |

```xml
<text>By <link uri="bungalow:artist:1">Queen</link> on <i>A Night at the Opera</i></text>
```

In XUL, a body that mixes text with these tags becomes one label, and
whitespace collapses as in HTML. A `<link>` or `<a>` that stands on its own
between other elements is an element of its own.

A click on a link asks the window to navigate (`SPElement::activateLink`). In
the client, `MainWindow::navigate` sends a `bungalow:` URI to the view stack
and an `http(s)` URL to a site view, or to the system browser when there is no
embedded browser, the URL is not `https`, or **Settings → Web Links → Open in
System Browser** (`forceExternalLinks`) is on. Links take the `linkcolor`
attribute, show a hand cursor and are underlined under the pointer.

In code:

```cpp
label->setInnerHTML("By " + SPRichText::link("bungalow:artist:1", artistName));
```

Plain `setInnerText()` never reads markup. Data that goes into markup must be
escaped, with `SPRichText::escape()` / `link()` in C++ or `escape()` / `link()`
in a [template](./templates.md#functions), or a name such as `<b>` would be
read as a tag.

**Tables** draw a cell as rich text when the data source returns markup from
`getMarkupForCell(column, row)`. The music tables do this for the Artists and
Release columns, so each artist and the release open their library page. A
click on a link does not select or play the row.

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
