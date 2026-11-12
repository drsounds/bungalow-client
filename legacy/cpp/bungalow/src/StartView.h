#pragma once
#include "../../space/src/SPSpiderViewElement.h"
#include <string>
using namespace std;
namespace space {
    namespace views {
        class StartView : public SPSpiderViewElement
        {
            public:
                StartView();
                explicit StartView(SPElement *parent);
                void onLoad(SPObject *sender, SPEventArgs *e);
                virtual ~StartView();
                virtual string getType() override;
            protected:
            private:
        };
    }
}
