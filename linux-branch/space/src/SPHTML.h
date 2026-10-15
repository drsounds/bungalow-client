#pragma once
#include <string>
using namespace std;
namespace space {
class SPHTML
{

    public:
        string innerText;
        string uri;
        int position;

        SPHTML(string html);
        virtual ~SPHTML();
    protected:
    private:
};
}

