---
layout: default
title: Spider Templates
nav_order: 6
---

# Spider templates

**Spider** is a server-style template engine for building view content. Instead of
assembling a control tree by hand in Java, a plugin writes ASP/JSP-style markup —
XUL-like XML with Lua interpolation — and Spider turns it into a live tree of
[`Control`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/Control.java)s. A button click
**posts back**, re-runs the template, and re-renders, with widget state preserved.

It is the same idea as a web framework (request → controller → template → markup →
page), pointed at a Swing control tree instead of a browser.

Types: [`Spider`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/Spider.java),
[`Controller`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/controller/Controller.java),
[`Request`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/Request.java) /
[`Response`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/Response.java),
[`TemplateEngine`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/template/TemplateEngine.java),
[`LuaPreprocessor`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/template/LuaPreprocessor.java),
[`Postbacks`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/Postbacks.java),
[`SpiderView`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/views/SpiderView.java).

## The pipeline

```
 template (XUL + Lua)                                         on-screen
 ────────────────────                                         ─────────
   <view> … ${expr} …          LuaPreprocessor   Lua chunk      Control tree
   % for i,10 do        ─────► (directives →  ─► (LuaJ runs  ─► (setInnerXul
   …                            Lua source)      it → XML)       reconciles)
   % end                                            │              │
                                                    ▼              ▼
                                             org.w3c.dom.Element  SpiderView /
                                             (parsed markup)      TabBarView
```

1. A [`Controller`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/controller/Controller.java)
   supplies a **template** (a string, often loaded from a resource file).
2. [`LuaPreprocessor`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/template/LuaPreprocessor.java)
   compiles the template into a **Lua chunk** that emits markup.
3. [`TemplateEngine`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/template/TemplateEngine.java)
   runs the chunk on an embedded **Lua interpreter** ([LuaJ](#dependency)) with the
   request bound in, then parses the emitted XML into a `org.w3c.dom.Element`.
4. The view renders the element into a control tree via
   [`Control.setInnerXul`](#rendering-controlsetinnerxul), and wires up postbacks.

## Template syntax

A template is XML markup with two directive forms layered on top — the classic
"Lua preprocessor" model:

```xml
<view>
    <page id="overview" title="Overview">
        <vbox>
            <text>${os.date("%Y-%m-%d")}</text>
            % for i,10 do
            <text>${i}</text>
            % end
            <button onclick="refresh">Refresh</button>
        </vbox>
    </page>
</view>
```

- **`${expr}` — interpolation.** `expr` is evaluated as a Lua expression and its
  `tostring()` spliced into the markup. `${os.date("%Y-%m-%d")}` renders today's
  date; `${i}` renders the loop variable.
- **`%`-prefixed lines — control flow**, emitted as raw Lua. `% end`, `% if x
  then`, etc. pass through verbatim. The one piece of custom sugar is the **count
  loop** `% for i,10 do`, which the preprocessor rewrites to the Lua numeric loop
  `for i=1,10 do` (var from 1 to the count). Everything between it and `% end` is
  emitted once per iteration.

The compiled chunk accumulates markup into a buffer and `return`s the concatenated
string, so the engine can run it and parse the result as XML. Literal markup is
emitted as escaped Lua string literals, so quotes and brackets in your XML are safe.

### Request bindings

The running chunk sees these globals, derived from the
[`Request`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/Request.java), so a single
template can branch on what was asked:

| Global   | Value |
|----------|-------|
| `method` | `"GET"` on a load, `"POST"` on a postback |
| `uri`    | the request URI |
| `action` | the postback action name (the clicked button's `onclick`), or `""` |
| `data`   | a Lua table of the request's input fields (see [postbacks](#postbacks)) |

## Controllers and routing

A [`Controller`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/controller/Controller.java)
handles the URIs it accepts and supplies the template:

```java
public abstract class Controller {
    public abstract boolean acceptsUri(Uri uri);      // "is this URI mine?"
    protected abstract String template(Request req);  // the Lua template (from file/string)
    public Element process(Request req) { … }          // renders template → Element
}
```

[`Spider`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/Spider.java) is the registry +
router. `process(request)` parses the URI and dispatches to the **first registered
controller** that accepts it — the same "first match wins" rule the
[`ViewStack`](navigation-and-views.html) uses for views:

```java
public Element process(Request request) {
    Uri uri = Uri.parse(request.getUri());
    for (Controller c : controllers)
        if (c.acceptsUri(uri)) return c.process(request);
    return null;                                        // no handler → nothing rendered
}
```

## Rendering: `Control.setInnerXul`

Markup becomes widgets through
[`Control.setInnerXul(Element)`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/Control.java)
(string overload also provided). Each tag maps to a control:

| Tag | Control | Notes |
|-----|---------|-------|
| `<button>` | [`Button`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/Button.java) | text from the element body; `onclick` → [postback](#postbacks) |
| `<text>` / `<label>` / `<img>` | [`Label`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/Label.java) | body text becomes the label |
| `<input>` | [`TextField`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/TextField.java) | a named field is collected on postback |
| `<hbox>` / `<vbox>` | [`HBox`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/HBox.java) / [`VBox`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/VBox.java) | row / column layout |
| `<view>` | [`TabbedPane`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/TabbedPane.java) | children become tabs |
| `<page>` | [`Panel`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/Panel.java) | a tab; its `title`/`label` is the tab caption |
| `<element>` / anything else | `Panel` | a plain container |
| `<a.b.C>` (dotted) | the class `a.b.C` | inflated by reflection — see below |

**Fully-qualified tags (Android-style).** A tag containing a `.` is treated as a
class name and instantiated by reflection, exactly like a custom-view tag in
Android layout XML. The class needs a public no-arg constructor and may be either a
[`Control`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/Control.java) (used directly) or a
raw Swing/AWT `Component` (wrapped in a
[`ComponentControl`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/controls/ComponentControl.java)):

```xml
<se.spacify.controls.GlossyButton>Buy</se.spacify.controls.GlossyButton>
<javax.swing.JProgressBar/>
```

This lets a template mount any control or widget without first teaching the short
vocabulary about it. The class is resolved via the **thread context classloader**
first (so a control supplied by a separately-loaded plugin jar resolves), falling
back to the framework's own loader for the built-ins. A missing class or one with no
usable constructor fails loudly with an `IllegalArgumentException` naming the
offending element.

### Reconciliation (React-style)

`setInnerXul` does **not** rebuild the tree on every render. It reconciles the new
element list against the children produced by the previous render, the way React
diffs: a child whose **tag matches** the control already at that position is
**reused** (its attributes, text and subtree updated in place); only a differing
tag forces a replacement, and trailing controls the new tree dropped are removed.

That is what makes a postback cheap and seamless — the selected tab, caret
position, scroll offset and focus survive a re-render because the underlying Swing
widgets are the same instances.

## Postbacks

A `<button onclick="refresh">` is wired to **post back** by
[`Postbacks.bind`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/Postbacks.java): clicking
it re-issues the request as a `POST` whose `action` is the `onclick` value, carrying
the current input fields (collected by `Postbacks.collectInput` from named
`<input>` controls) as the request `data`. The controller re-runs its template —
now with `method == "POST"` and `action == "refresh"` visible as
[globals](#request-bindings) — and the view re-renders.

Buttons are bound **once even across re-renders**: because the reconciler reuses
control instances, a `spider.bound` client-property marks an already-wired button,
and the listener reads the `onclick` attribute live so a reused button always posts
back its latest action.

## Views: `SpiderView` and `TabBarView`

Spider content is mounted by a [navigation `View`](navigation-and-views.html) so the
`ViewStack` can route a `spacify:` URI to it. Two render shapes exist:

- **[`SpiderView`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/spider/views/SpiderView.java)** —
  the general-purpose surface. It wraps the rendered root so the root element
  *itself* becomes its single reconciled child (a `<view>` root becomes an inner
  `TabbedPane`), then binds postbacks. Use it to drop arbitrary Spider markup into
  a screen.
- **[`TabBarView`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/navigation/TabBarView.java)** — a view
  that *is* a tab bar. Rendering a `<view>`'s `<page>` children straight into its
  `TabbedPane` makes each page a tab. `TabBarView.navigate` also reads a URI
  **`#fragment`** and selects the tab whose id matches it, so
  `spacify:testapp#overview` deep-links to the "overview" tab.

> The `<view>` ⇒ tab-bar, `<page>` ⇒ tab mapping is why a Spider template's
> top-level shape mirrors the on-screen tab structure.

## Worked example: the Test App

[`se.spacify.app.testapp`](https://github.com/drsounds/bungalow/tree/main/src/main/java/se/spacify/app/testapp) is a hello-world
plugin wiring all of the above to `spacify:testapp`:

- **Template** —
  [`views/test.xml`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/testapp/views/test.xml), the
  markup shown [above](#template-syntax). It ships next to the code under
  `src/main/java`; the build copies `**/*.xml` onto the classpath (see the
  `<resources>` block in [`pom.xml`](https://github.com/drsounds/bungalow/blob/main/pom.xml)) so it can be loaded as a resource.
- **Controller** —
  [`TestController`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/testapp/controller/TestController.java)
  accepts `spacify:testapp` (and `…#fragment`) and returns `test.xml` as its
  template.
- **View** —
  [`TestAppView`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/testapp/views/TestAppView.java)
  extends `TabBarView`; on `navigate` it asks its `Spider` to process the request
  and renders the pages as tabs, binding the refresh button to a postback.
- **Plugin** —
  [`TestApplication`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/testapp/TestApplication.java)
  registers the view and a "Test App" sidebar node pointing at `spacify:testapp`,
  and is listed in
  [`BuiltinApplicationRegistry`](https://github.com/drsounds/bungalow/blob/main/src/main/java/se/spacify/app/BuiltinApplicationRegistry.java).

Click "Test App" in the sidebar → the view renders the date, the `1..10` loop and
the button; click **Refresh** → it posts back and re-renders in place.

## Dependency

Spider runs Lua on **[LuaJ](https://github.com/luaj/luaj)** (`org.luaj:luaj-jse`, a
pure-Java Lua 5.2 interpreter — no native code), declared in
[`pom.xml`](https://github.com/drsounds/bungalow/blob/main/pom.xml). The engine builds a fresh
`JsePlatform.standardGlobals()` sandbox per render, so the standard Lua library
(`os`, `string`, `table`, `math`, …) is available inside `${…}` expressions and
`%` lines.
