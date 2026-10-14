# Spacify Client

Spacify Client is a C++ desktop application that interacts with spacify.se services.

It uses an own written GUI framework called **Space** which is a lightweight abstract GUI framework that aims to create an alternate protocol for serving content over the internet

## Building

Required libraries:

* Boost
* Lua
* libpng
* Expat

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
