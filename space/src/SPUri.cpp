#include "SPUri.h"
#include "SPString.h"

namespace space {
    SPUri::SPUri(const string& uri) {
        this->uri = uri;
        vector<string> uri_parts = split(uri, '#');
        if (uri_parts.size() > 1) {
            this->hash = uri_parts[1];
        }
        this->address = uri_parts[0];
        vector<string> uri_segments = split(this->address, ':');
        this->protocol = uri_parts[0];
        this->host = uri_segments[1];
        this->segments = vector(uri_segments.begin() + 2, uri_segments.end());
    }

    SPUri::~SPUri() {
    }

    string SPUri::toString() {
        return this->uri;
    }
}
