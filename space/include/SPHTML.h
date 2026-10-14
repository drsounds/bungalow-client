#ifndef HTML_H
#define HTML_H
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

#endif // HTML_H
