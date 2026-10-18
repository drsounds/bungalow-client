# Bungalow Client

BungalownClient is a C++ desktop application.

It uses an own written GUI framework called **Space** which is a lightweight abstract GUI framework that aims to create an alternate protocol for serving content over the internet

## Building

Required libraries:

* nlohmann-json
* Lua
* libpng
* Expat
* wxWidgets with the WebView component enabled (for the wxWidgets backend)

On Windows, the wxWidgets WebView backend also requires the Microsoft Edge
WebView2 Runtime to be installed.

The default UI backend is wxWidgets. On Windows, select `win32` or `wtl` with
`-DBUNGALOW_UI_BACKEND=win32` or `-DBUNGALOW_UI_BACKEND=wtl`. The WTL backend
also requires the WTL headers, supplied with `-DWTL_INCLUDE_DIR=<path>`.

```bash
cmake .
make
```

## Usage

TBA

## License

MIT
