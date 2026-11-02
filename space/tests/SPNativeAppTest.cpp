#include "SPNativeApp.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <crtdbg.h>
#endif

namespace fs = std::filesystem;

static bool contains(const std::string &text, const std::string &value) {
	if (text.find(value) != std::string::npos)
		return true;
	std::cerr << "Rendered view is missing: " << value << "\nin: " << text << '\n';
	return false;
}

static bool throws(const std::function<void()> &action, const std::string &message) {
	try {
		action();
	} catch (const std::exception &error) {
		if (std::string(error.what()).find(message) != std::string::npos)
			return true;
		std::cerr << "Expected an error containing \"" << message << "\", got: " << error.what() << '\n';
		return false;
	}
	std::cerr << "Expected an error containing \"" << message << "\"\n";
	return false;
}

static void write(const fs::path &file, const std::string &text) {
	fs::create_directories(file.parent_path());
	std::ofstream(file, std::ios::binary) << text;
}

static space::SPAppBundle bundleAt(const fs::path &dir) {
	std::ifstream in(dir / "manifest.json");
	std::ostringstream manifest;
	manifest << in.rdbuf();
	space::SPAppBundle bundle;
	bundle.manifest = nlohmann::json::parse(manifest.str());
	bundle.identifier = dir.filename().u8string();
	bundle.path = dir.u8string();
	return bundle;
}

static int run();

int main() {
#ifdef _WIN32
	// Report debug CRT failures on stderr instead of a dialog that hangs ctest.
	for (int type : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT}) {
		_CrtSetReportMode(type, _CRTDBG_MODE_FILE);
		_CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
	}
#endif
	try {
		return run();
	} catch (const std::exception &error) {
		std::cerr << "Unexpected error: " << error.what() << '\n';
		return 1;
	}
}

static int run() {
	const fs::path dir = fs::temp_directory_path() / "bungalow-native-app-test";
	fs::remove_all(dir);
	write(dir / "manifest.json", R"({
		"AppType": "Native",
		"Features": { "count": { "Routes": "feature/count/routes.json" } }
	})");
	write(dir / "routes.json", R"({"routes": [
		{"path": "^bungalow:app:scratch:sandbox$", "view": "views/sandbox",
		 "controller": "controllers/sandbox"},
		{"path": "^bungalow:app:scratch:flow:([\\w-]+)$", "view": "views/flow",
		 "controller": "controllers/flow"}
	]})");
	write(dir / "controllers" / "flow.lua", R"(
function flow(request)
  local step = request.arguments[0]
  if step == "redirect" then return redirect("bungalow:app:scratch:flow:done") end
  if step == "other" then return render("views/other", { word = "rendered" }) end
  if step == "escape" then return render("../outside") end
  if step == "spin" then while true do end end
  if step == "spin-coroutine" then coroutine.wrap(function() while true do end end)() end
  return { word = step }
end
)");
	write(dir / "views" / "flow.xml", "<view><text>${word}</text></view>");
	write(dir / "views" / "other.xml", "<view><text>other ${word}</text></view>");
	write(dir / "helpers" / "text.lua",
		  "local M = {}\nfunction M.shout(s) return string.upper(s) .. '!' end\nreturn M\n");
	write(dir / "controllers" / "sandbox.lua", R"(
local count = 0
function sandbox(request)
  count = count + 1
  local escaped = pcall(require, "/outside")
  return {
    count = count,
    io = io and "io" or "no-io",
    load = load and "load" or "no-load",
    execute = os.execute and "execute" or "no-execute",
    shout = require("helpers.text").shout(request.action),
    escaped = escaped and "escaped" or "contained",
    method = request.method,
    name = request.data.name,
    items = { "a", "b" },
  }
end
)");
	write(dir / "views" / "sandbox.xml",
		  "<view><text>${count} ${io} ${load} ${execute} ${shout} ${escaped} ${method} ${name} ${items[2]}</text></view>");
	write(dir / "feature" / "count" / "routes.json", R"({"routes": [
		{"path": "^bungalow:app:scratch:count:(\\d+):(?:x|y):(?P<word>\\w+)$", "view": "count.xml",
		 "controller": "count.lua"}
	]})");
	write(dir / "feature" / "count" / "count.lua",
		  "function count(request)\n"
		  "  return { first = request.arguments[0], word = request.arguments.word, second = request.arguments[1] }\n"
		  "end\n");
	write(dir / "feature" / "count" / "count.xml", "<view><text>${first} ${word} ${second}</text></view>");

	auto bundle = bundleAt(dir);
	if (!space::SPNativeApp::isNative(bundle)) {
		std::cerr << "AppType Native was not recognized\n";
		return 1;
	}
	space::SPNativeApp app(bundle);
	if (!throws([&] { app.handle("GET", "bungalow:app:scratch:missing"); }, "No route matches"))
		return 1;

	// Sandboxed, with state kept between requests and POST data reaching it.
	if (!contains(app.handle("GET", "bungalow:app:scratch:sandbox").xul,
				  "<text>1 no-io no-load no-execute ! contained GET  b</text>"))
		return 1;
	if (!contains(app.handle("POST", "bungalow:app:scratch:sandbox", {{"command", "save"}, {"name", "Ada"}}).xul,
				  "<text>2 no-io no-load no-execute SAVE! contained POST Ada b</text>"))
		return 1;

	// Feature routes resolve relative to their folder; captures are 0-based
	// and named, and non-capturing groups do not shift them.
	if (!contains(app.handle("GET", "bungalow:app:scratch:count:7:x:hi").xul, "<text>7 hi hi</text>"))
		return 1;

	// Controllers can redirect or render another view, inside the app.
	const auto redirected = app.handle("POST", "bungalow:app:scratch:flow:redirect");
	if (redirected.redirect != "bungalow:app:scratch:flow:done" || !redirected.xul.empty()) {
		std::cerr << "redirect() did not produce a redirect: " << redirected.redirect << '\n';
		return 1;
	}
	if (!contains(app.handle("GET", "bungalow:app:scratch:flow:other").xul, "<text>other rendered</text>"))
		return 1;
	if (!throws([&] { app.handle("GET", "bungalow:app:scratch:flow:escape"); }, "renders a view outside the app"))
		return 1;

	// Runaway controllers, also in coroutines, stop at the instruction limit
	// without breaking later requests.
	app.setInstructionLimit(1'000'000);
	if (!throws([&] { app.handle("GET", "bungalow:app:scratch:flow:spin"); }, "instruction limit") ||
		!throws([&] { app.handle("GET", "bungalow:app:scratch:flow:spin-coroutine"); }, "instruction limit"))
		return 1;
	if (!contains(app.handle("GET", "bungalow:app:scratch:flow:done").xul, "<text>done</text>"))
		return 1;

	// ApiPermissions unlock libraries.
	bundle.manifest["ApiPermissions"] = {"io", "os"};
	space::SPNativeApp permitted(bundle);
	if (!contains(permitted.handle("GET", "bungalow:app:scratch:sandbox").xul, "<text>1 io no-load execute"))
		return 1;

	// Routes files may not point outside the app.
	bundle.manifest["Features"]["count"]["Routes"] = "../elsewhere/routes.json";
	if (!throws([&] { space::SPNativeApp escaping(bundle); }, "inside the app"))
		return 1;

	// Controller errors surface with the controller's file name.
	write(dir / "controllers" / "sandbox.lua", "function sandbox(request) error('boom') end\n");
	space::SPNativeApp failing(bundleAt(dir));
	if (!throws([&] { failing.handle("GET", "bungalow:app:scratch:sandbox"); }, "controllers/sandbox.lua:1: boom"))
		return 1;

	fs::remove_all(dir);
	return 0;
}
