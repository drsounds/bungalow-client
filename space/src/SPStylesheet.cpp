#include "SPStylesheet.h"

#include <fstream>
#include <iostream>
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


        std::ifstream file("../../skin.json");
        if (file)
            pt = nlohmann::json::parse(file);

    }
    catch (std::exception const& e)
    {
        std::cerr << e.what() << std::endl;
    }

    //ctor
}

string SPStylesheet::get(string entity, string prop) {
    return pt.at(entity).at(prop).get<string>();
}

SPStylesheet::~SPStylesheet()
{
    //dtor
}
}
