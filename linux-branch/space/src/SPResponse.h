#pragma once
#include <string>
using namespace std;

namespace space {
class SPResponse
{
    public:
        SPResponse();
        virtual ~SPResponse();

        string content_type;
        int status;


    protected:
    private:
};
}
