#pragma once
#include "SPNode.h"
#ifdef _MSC_VER
#include <boost/config/compiler/visualc.hpp>
#endif
#include <boost/property_tree/ptree.hpp>
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
         boost::property_tree::ptree *pt;
        map<string, string> selectors;
    private:
};
}

