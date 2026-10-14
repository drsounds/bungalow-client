#ifndef SPACESERVER_H
#define SPACESERVER_H

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

#endif // SPACESERVER_H
