---
title: Native apps
sidebar_position: 2
---

# Native apps

A **native app** is an app bundle with `"AppType": "Native"`. It is a small
Lua MVC application: **routes** match `bungalow:` URIs, **controllers** are Lua
functions that return a model, and **views** are XUL
[templates](../space/templates.md). The result is rendered with Space elements,
so a native app looks like the rest of the client and needs no browser.

It is run by `space::SPNativeApp` (`space/include/SPNativeApp.h`) and shown by
`SPNativeAppElement` inside `AppView`.

## Example: `Apps/native`

```
Apps/native/
├── manifest.json
├── routes.json                   bundle routes (empty here)
└── feature/test/
    ├── routes.json
    ├── index.lua                 controller
    └── index.xml                 view
```

`manifest.json`:

```json
{
    "BundleType": "Application",
    "BundleIdentifier": "native",
    "AppName": { "en": "Native" },
    "AppType": "Native",
    "Features": {
        "test": { "Routes": "feature/test/routes.json" }
    }
}
```

`feature/test/routes.json`:

```json
{
    "routes": [
        {
            "path": "^bungalow:app:native:(?P<name>[a-zA-Z0-9\\-_]+)$",
            "view": "index.xml",
            "controller": "index.lua"
        }
    ]
}
```

`feature/test/index.lua`:

```lua
function index(request)
    return {
        method = request.method,
        name = request.arguments[0]
    }
end
```

`feature/test/index.xml`:

```xml
<view>
    <text>${method}</text>
    <text>Hello, ${name}!</text>
</view>
```

Navigating to `bungalow:app:native:world` shows "GET" and "Hello, world!".

## Routes

Routes come from two places:

1. `<bundle>/routes.json`, with paths relative to the bundle,
2. the file each `Features.<name>.Routes` in the manifest points to, with paths
   relative to that file's folder.

The bundle's own routes come first. **The first route whose `path` matches the
whole URI handles it.** `path` is a regular expression and may use named groups
(`(?P<name>…)`). A `view` or `controller` without an extension gets `.xml` or
`.lua`.

## Controllers

The controller file is loaded and its function **named after the file** is
called (`index.lua` → `index`). It receives a request table:

| Field | Value |
|-------|-------|
| `method` | `"GET"` or `"POST"` |
| `uri` | the requested URI |
| `action` | the postback action (the posted `command`) |
| `data` | the posted form fields |
| `arguments` | regex captures: by index from `0`, and by group name |

The table the controller returns is the view's **model**. Its fields are also
template variables, so `${name}` reads `model.name`.

Instead of a model, a controller can return:

* `redirect(uri)` to navigate somewhere else, for example after handling a
  `POST`,
* `render(view, model)` to render another view, named like a route's view.

## State and sandboxing

* The app keeps **one Lua state for its lifetime**, and each controller file
  has its own environment in it. Controllers keep state between requests.
* Controllers run in a **sandbox**: the safe standard libraries, `require()` of
  `.lua` modules inside the bundle, and only the extra libraries listed in the
  manifest's `ApiPermissions` (`"io"`, `"os"`).
* Each request has an **instruction limit** (about 50 million Lua
  instructions by default, `setInstructionLimit()`). A runaway controller fails
  instead of hanging the app.

## Errors

`SPNativeApp` throws `std::runtime_error` if a routes file is invalid, no route
matches, or a controller or view fails. `AppView` then shows an error view in
place of the app.

`space/tests/SPNativeAppTest.cpp` covers routing, captures, redirects, the
sandbox and the instruction limit.
