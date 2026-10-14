#pragma once

#include <boost/json/object.hpp>
#include <string>

namespace space {

struct SPTemplateRequest {
    std::string method = "GET";
    std::string uri;
    std::string action;
    boost::json::object data;
};

class SPTemplate {
public:
    static std::string render(const std::string& source,
                              const SPTemplateRequest& request = {},
                              const boost::json::object& model = {});
};

}
