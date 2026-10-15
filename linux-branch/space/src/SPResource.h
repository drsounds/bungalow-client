#pragma once
#include <string>
#include <vector>
#include <map>

#include "SPObject.h"

using namespace std;
namespace space {
class SPResource : public SPObject
{
public:
    string id;
    string type;
    string uri;
    string name;
    string collection_uri;
    vector<string> creator_uris;
    map<string, string> attributes;

    explicit SPResource(const string& uri);
    SPResource();
    virtual string get(const string& key, const string& defaultValue);
    virtual void set(const string& key, const string& value);
};
}
