#pragma once
#include <map>
#include <string>
#include "SPResponse.h"
#include <map>
using namespace std;
namespace space {
/**
 * Request handler for Space
 */
class ISPRequestHandler
{
    public:
        ISPRequestHandler();
        virtual ~ISPRequestHandler();

        /**
         * Returns whether this handles this type of URI
         **/
        virtual bool handlesUri(string uri) {
            return false;
        }

        /**
         * Processes request
         **/
        virtual SPResponse *processRequest(string method, string uri, const std::map<string, string>& headers, void *payload) {
            return NULL;
        }

    protected:
    private:
};
}
