#include "SPRequest.h"
namespace space {
SPRequest::SPRequest()
{
    //ctor
}
ISPRequestHandler *SPRequest::getMatchingHandlerForUri(string uri) {
    for (std::vector<ISPRequestHandler *>::iterator it = space::handlers.begin(); it != space::handlers.end(); ++it)
    {
        ISPRequestHandler *handler = (ISPRequestHandler *)*it;
        if (handler->handlesUri(uri)) {
            return handler;
        }
    }
    return nullptr;
}
SPResponse *SPRequest::request(string method, string uri, const std::map<string, string>& headers, void *data) {
    ISPRequestHandler *handler = this->getMatchingHandlerForUri(uri);
    SPResponse *response = handler->processRequest(method, uri, headers, data);
    return response;
}
SPRequest::~SPRequest()
{
    //dtor
}
}
