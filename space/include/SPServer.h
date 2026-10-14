#ifndef SERVER_H
#define SERVER_H

#include <string>
using namespace std;

namespace space {
   typedef void (*space_server_reply)(int code, string data);
class SPServer
{
    public:
        SPServer();
        virtual ~SPServer();
        void request(string method, string uri, void *data, space_server_reply *reply);
};
}
#endif // SERVER_H
