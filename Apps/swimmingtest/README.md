# Swimming Test

A test app for `"AppType": "Swimming"`: a React Router app rendered natively
by [swimming](https://github.com/drsounds/swimming). It has a click counter,
a clock driven by JS timers, and routes that Bungalow URIs select:

- `bungalow:app:swimmingtest` shows `/`
- `bungalow:app:swimmingtest:about` shows `/about`

`dist/bundle.js` is checked in so the app runs without a Node toolchain. The
`swimming-*` packages are not on npm yet, so rebuild it inside a swimming
checkout, where npm workspaces resolve them:

```sh
git clone https://github.com/drsounds/swimming.git
mkdir swimming/examples/swimmingtest
cp -r package.json vite.config.ts src swimming/examples/swimmingtest/
cd swimming && npm install && npm run build -w swimmingtest
cp examples/swimmingtest/dist/bundle.js <this directory>/dist/
```

The test `bungalow-swimming-app-test` (`space/tests/SPSwimmingAppTest.cpp`)
loads this bundle.
