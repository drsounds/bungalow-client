#include "SPServer.h"

#include <string>

using namespace std;

namespace space {
SPServer::SPServer()
{
    //ctor
}

void SPServer::request(string method, string uri, void *data, space_server_reply *reply) {
    if (method == "GET") {
        if (uri == "bungalow:internal:home") {

        }
    }
}
SPServer::~SPServer()
{
    //dtor
}
}