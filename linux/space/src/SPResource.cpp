#include "SPResource.h"
namespace space {
    SPResource::SPResource(const string& uri)
    {
        //ctor
        this->uri = uri;
    }
    SPResource::SPResource() {

    }
    string SPResource::get(const string& key, const string& defaultValue) {
        string ret = defaultValue;
        if (key == "uri") {
            ret = this->uri;
        } else if (key == "name") {
            ret = name;
        } else if (this->attributes.count(key)) {
            ret = this->attributes[key];
        }
        return ret;
    }
    void SPResource::set(const string& key, const string& value) {
        if (key == "name") {
            this->name = value;
        } else {
            this->attributes[key] = value;
        }
    }
}