#ifndef STYLESHEET_H
#define STYLESHEET_H
#include "SPNode.h"
#include <nlohmann/json.hpp>
#include <string>
namespace space {
class SPStylesheet;

class Selector : SPNode {
public:
    Selector();
    virtual ~Selector();
};

class SPStylesheet
{

    public:
        SPStylesheet();
        virtual ~SPStylesheet();
        string get(string entity, string prop);
    protected:
        nlohmann::json pt = nlohmann::json::object();
        map<string, string> selectors;
    private:
};
}

#endif // STYLESHEET_H
