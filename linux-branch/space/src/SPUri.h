#pragma once
#include "libspace.h"
#include <string>
#include <vector>

using namespace std;

namespace space {
class SPUri {
	vector<string> segments;
	string address;
	string uri;
	string protocol;
	string entity;
	string id;
	string hash;
	string host;
public:
	vector<string> getSegments() {
		return segments;
	}
	string getAddress() {
		return address;
	}
	string getProtocol() {
		return protocol;
	}
	string getEntity() {
		return entity;
	}
	string getId() {
		return id;
	}
	string getHash() {
		if (hash.empty()) {
			return "overview";
		}
		return hash;
	}
    string getUri() {
    	return uri;
    }
	explicit SPUri(const string& uri);
	~SPUri();
    string toString();
};
}

