---
title: Templates
sidebar_position: 3
---

# Templates

View markup can be a **Lua template**: XUL with Lua expressions and control
flow mixed in, in the style of ASP or JSP. `space::SPTemplate` compiles the
template to Lua, runs it and returns the rendered XUL. Elements then render
that XUL with `setInnerXUL`.

This is the C++ counterpart of the Java client's
[Spider templates](../java-client/spider-templates.md), with the same syntax.

## Syntax

```xml
<view>
  <page title="${model.title}">
    <text>${method} ${uri} ${action} ${data.name}</text>
    % for i,3 do
    <text>${i}</text>
    % end
    % if data.enabled then
    <text>${model.nested.label}</text>
    % end
  </page>
</view>
```

* **`${expr}`** is a Lua expression. Its `tostring()` value is inserted into
  the markup.
* **Lines starting with `%`** are Lua statements (`% if x then`, `% end`, …).
* **`% for i,N do`** is shorthand for the numeric loop `for i=1,N do`.

Internally, each line is converted to elua's `<% … %>` and `<%= … %>` form
(`elua/`) and loaded into a Lua state.

## Variables

| Variable | Value |
|----------|-------|
| `method` | `"GET"` when loading, `"POST"` on a postback |
| `uri` | the requested URI |
| `action` | the postback action (the clicked button's `onclick`), or `""` |
| `data` | the posted form fields, as a Lua table |
| `model` | the model passed by the caller |

JSON arrays become 1-based Lua tables. With `modelGlobals`, each field of the
model is also a variable of its own, so `${name}` reads `model.name`. Native
apps use this; see [Native apps](../apps/native-apps.md).

```cpp
std::string xul = space::SPTemplate::render(
    source, "GET", "bungalow:start", "", data, model, /*modelGlobals*/ false);
```

## Functions

| Function | Result |
|----------|--------|
| `escape(value)` | `value` with `& < > " '` as entities, so data cannot be read as markup |
| `link(uri, text)` | `<link uri="uri">text</link>` with both escaped; `text` defaults to the URI |

Text bodies can hold links and simple formatting; see
[Rich text](./elements-and-xul.md#rich-text-and-links):

```xml
<text>By ${link("bungalow:artist:" .. model.entity.isni, model.entity.name)}</text>
<text>Read more on <a href="https://example.com">the <b>site</b></a>.</text>
```

## Rendering a URI: the `src` attribute

Setting an element's `src` to a `bungalow:` URI renders the template for that
route into the element. The catch-all `SpaceView` uses this for every URI no
other view claims:

```
bungalow:start          → views/start.xml
bungalow:artist:<isni>  → views/artist.xml
anything else           → views/error.xml
```

For a `GET`, the element:

1. asks the [URI store](./uri-store.md) to resolve the URI,
2. renders straight away with what the store already has (in memory or in the
   disk cache),
3. subscribes to the URI and renders again whenever the entity changes.

The template model contains:

| Field | Value |
|-------|-------|
| `model.entity` | the URI's entity data from the store, or `nil` |
| `model.status` | `Empty`, `Cached`, `Loading`, `Fresh` or `Error` |
| `model.now` | the current local time, `YYYY-MM-DD HH:MM:SS` |

So `views/artist.xml` can show `${model.entity.name}` at once from the cache
and update when the library returns fresh data.

## Postbacks

A `<button onclick="refresh">` posts back: the element's form data (named
`<input>` fields, collected by `getFormData()`) is sent as a `POST` with
`command` set to the button's `onclick` value (`submit` if it has none). The template runs again with
`method == "POST"` and `action == "refresh"`, and the result is reconciled into
the existing elements.

## The Spider prototype

`spider/` holds an earlier request/view prototype (`spider::Spider`,
`SpiderView`) that used elua directly. `SPTemplate`, the `src` attribute and
`SPNativeApp` replace it.
