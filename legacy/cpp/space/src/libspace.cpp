#include "libspace.h"

#include <sstream>
#include <string>
#include <boost/json.hpp>

namespace space {
/*
	SPMessage::SPMessage() {
	}

	boost::json::value SPMessage::toJson() {
		boost::json::object obj;
		obj["predicate"] = this->predicate;
		obj["object_uri"] = this->object_uri;
		obj["attribute"] = this->attribute;
		obj["value"] = this->value;
		obj["data"] = this->data;
		return obj;
	}
	SPMessage::SPMessage(const string& json) {
		boost::json::value v = boost::json::parse(json);
		this->predicate = v.at("predicate").as_string().c_str();
		this->object_uri = v.at("object_uri").as_string().c_str();
		this->attribute = v.at("attribute").as_string().c_str();
		this->value = v.at("value").as_string().c_str();
		this->data = v.at("data").as_object();
	}
*/
    int htmlToColor(string str) {
        string str2 = str.substr(1, str.length() -1);
        stringstream c(str2);
        unsigned int value = 0;
        c >> hex >> value;
        return value;
    }


}



