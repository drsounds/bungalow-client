#pragma once

#include <string>
using namespace std;
namespace space {
class SPSpaceServer
{
    public:
        SPSpaceServer();
        virtual ~SPSpaceServer();
        void request(string method, string uri, void *data);
    protected:
    private:
};
}

