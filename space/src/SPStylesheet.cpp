#include "SPStylesheet.h"

#include <iostream>
#include <boost/json.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace space {

Selector::Selector()
 : SPNode() {
 }
Selector::~Selector() {

}

SPStylesheet::SPStylesheet()
{

    try
    {
        // send your JSON above to the parser below, but populate ss first


        boost::property_tree::read_json("../../skin.json", *pt);

    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << std::endl;
    }

    //ctor
}

string SPStylesheet::get(string entity, string prop) {
    boost::property_tree::ptree tree = this->pt->get_child((string)entity.c_str());
    string val = tree.get_child((string)prop.c_str()).get_value<std::string>();
    return val;
}

SPStylesheet::~SPStylesheet()
{
    //dtor
}
}
