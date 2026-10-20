#include "SPString.h"

#include <iostream>

int main() {
	if (!space::startsWith("bungalow:site:home", "bungalow:") ||
		space::startsWith("space:", "bungalow:") ||
		!space::endsWith("example.bungalow.se", ".bungalow.se") ||
		space::endsWith("example.invalid", ".bungalow.se") ||
		space::split("one::three", ':') !=
			std::vector<std::string>{"one", "", "three"}) {
		std::cerr << "String utility behavior was incorrect\n";
		return 1;
	}
	return 0;
}
