# Spacify Client

Spacify Client is a C++ desktop application that interacts with spacify.se services.

It uses an own written GUI framework called **Space** which is a lightweight abstract GUI framework that aims to create an alternate protocol for serving content over the internet

## Building

Required libraries:

* Boost.JSON
* Lua
* libpng
* Expat
* wxWidgets with the WebView component enabled

On Windows, the wxWidgets WebView backend also requires the Microsoft Edge
WebView2 Runtime to be installed.

```bash
cmake .
make
```

## Usage

TBA

## License

MIT
