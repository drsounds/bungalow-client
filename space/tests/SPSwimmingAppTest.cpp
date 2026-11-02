#include "SPSwimmingApp.h"

#include "swimming/Loader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using space::SPAppBundle;
using space::SPSwimmingApp;

namespace {

int failures = 0;

void check(bool condition, const std::string &message) {
	if (!condition) {
		std::cerr << "FAILED: " << message << "\n";
		++failures;
	}
}

void texts(const swimming::LayoutBox &b, std::string &out) {
	if (b.text) out += *b.text;
	for (const auto &c : b.children) texts(c, out);
}

std::string read(const swimming::Bridge &bridge) {
	std::string out;
	texts(bridge.engine().layout()->root, out);
	return out;
}

/// Centers of the boxes with tag, in layout order.
void centers(const swimming::LayoutBox &b, const std::string &tag, float ox, float oy,
             std::vector<std::pair<float, float>> &out) {
	const float x = ox + b.x, y = oy + b.y;
	if (b.tag == tag) out.emplace_back(x + b.w / 2, y + b.h / 2);
	for (const auto &c : b.children) centers(c, tag, x, y, out);
}

bool contains(const std::string &haystack, const std::string &needle) {
	return haystack.find(needle) != std::string::npos;
}

void entryIsResolvedInsideTheBundle() {
	const fs::path dir = fs::temp_directory_path() / "bungalow-swimming-app-test";
	fs::remove_all(dir);
	fs::create_directories(dir / "dist");
	std::ofstream(dir / "dist" / "bundle.js") << "";
	std::ofstream(dir / "index.html") << "";

	SPAppBundle bundle;
	bundle.identifier = "entry";
	bundle.path = dir.u8string();
	bundle.manifest = {{"AppType", "Swimming"}};
	check(SPSwimmingApp::isSwimming(bundle), "AppType Swimming is recognised");
	check(fs::path(SPSwimmingApp::entryPath(bundle)) == dir / "dist" / "bundle.js", "Main defaults to dist/bundle.js");

	bundle.manifest["Main"] = "index.html";
	check(fs::path(SPSwimmingApp::entryPath(bundle)) == dir / "index.html", "Main names the entry");

	for (const char *main : {"missing.js", "../other/index.html", "..", "."}) {
		bundle.manifest["Main"] = main;
		bool threw = false;
		try {
			SPSwimmingApp::entryPath(bundle);
		} catch (const std::runtime_error &error) {
			threw = contains(error.what(), main);
		}
		check(threw, std::string("a missing or escaping Main is reported: ") + main);
	}

	bundle.manifest = {{"AppType", "Native"}};
	check(!SPSwimmingApp::isSwimming(bundle), "other app types are not Swimming apps");
}

/// The test app in Apps/swimmingtest, a React Router app built with
/// vite-plugin-swimming.
void theTestAppRendersClicksTicksAndRoutes() {
	const fs::path root = fs::u8path(BUNGALOW_SOURCE_DIR) / "Apps" / "swimmingtest";
	std::ifstream manifest(root / "manifest.json");
	SPAppBundle bundle;
	bundle.identifier = "swimmingtest";
	bundle.path = root.u8string();
	bundle.manifest = nlohmann::json::parse(manifest);
	check(SPSwimmingApp::isSwimming(bundle), "the test app is a Swimming app");

	swimming::Bridge bridge(800, 600);
	swimming::runSource(bridge, SPSwimmingApp::entryPath(bundle));
	check(contains(read(bridge), "Swimming Test"), "the home page renders: " + read(bridge));
	check(contains(read(bridge), "Clicked 0 times"), "the counter starts at zero: " + read(bridge));

	std::vector<std::pair<float, float>> buttons;
	centers(bridge.engine().layout()->root, "button", 0, 0, buttons);
	check(buttons.size() == 2, "the home page has two buttons");
	if (buttons.size() != 2) return;

	bridge.pointer("click", buttons[0].first, buttons[0].second);
	check(contains(read(bridge), "Clicked 1 times"), "clicks reach React handlers: " + read(bridge));

	bridge.tick(1000);
	check(contains(read(bridge), "Running for 1 s"), "JS timers run when ticked: " + read(bridge));

	bridge.pointer("click", buttons[1].first, buttons[1].second);
	check(contains(read(bridge), "land here directly"), "the app navigates itself: " + read(bridge));

	bridge.replaceRoute("/");
	check(contains(read(bridge), "Swimming Test"), "routes reach React Router: " + read(bridge));
	bridge.replaceRoute("/about");
	check(contains(read(bridge), "land here directly"), "the about route: " + read(bridge));
	bridge.replaceRoute("/nowhere");
	check(contains(read(bridge), "No page for /nowhere"), "unknown routes: " + read(bridge));
}

} // namespace

int main() {
	try {
		entryIsResolvedInsideTheBundle();
		theTestAppRendersClicksTicksAndRoutes();
	} catch (const std::exception &error) {
		std::cerr << "FAILED: " << error.what() << "\n";
		return 1;
	}
	return failures == 0 ? 0 : 1;
}
