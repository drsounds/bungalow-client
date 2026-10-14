#ifndef REQUEST_H
#define REQUEST_H

#include <map>
#include <string>
#include <vector>

#include "ISPRequestHandler.h"

using namespace std;

namespace space {
static vector<ISPRequestHandler *> handlers;
class SPRequest
{
    public:
        SPRequest();
        virtual ~SPRequest();

        ISPRequestHandler *getMatchingHandlerForUri(string uri);

        string method;
        string uri;
        std::map<string, string> headers;
        void *data;
        int status;
        SPResponse *request(string method, string uri, const std::map<string, string>& headers, void *data);

        SPResponse *response;

        void requestAsync(string method, string uri, const std::map<string, string>& headers, void *data);

    protected:
    private:
};
}

#endif // REQUEST_H
