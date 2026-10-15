#include "SPString.h"

#include <iostream>

int main() {
	if (!space::startsWith("spacify:site:home", "spacify:") ||
		space::startsWith("space:", "spacify:") ||
		!space::endsWith("example.spacify.se", ".spacify.se") ||
		space::endsWith("example.invalid", ".spacify.se") ||
		space::split("one::three", ':') !=
			std::vector<std::string>{"one", "", "three"}) {
		std::cerr << "String utility behavior was incorrect\n";
		return 1;
	}
	return 0;
}
