#ifndef USERVIEW_H
#define USERVIEW_H
#include "SPViewElement.h"
#include <string>
using namespace std;
namespace space {
    class SPHeaderElement;
    class SPElement;
    namespace views {
        class UserView : public SPViewElement
        {
            public:
                UserView();
                UserView(SPElement *parent);
                virtual ~UserView();
                bool acceptsUri(const string& uri) override;
                void navigate(const string& uri) override;
            protected:
                SPHeaderElement *header;
            private:
        };
    }
}
#endif // USERVIEW_H
