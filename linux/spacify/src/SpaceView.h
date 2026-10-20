#pragma once
#include "../../space/src/SPSpiderViewElement.h"
#include <string>
using namespace std;
namespace space {
    namespace views {
        class SpaceView : public SPSpiderViewElement
        {
            public:
                SpaceView();
                explicit SpaceView(SPElement *parent);
                virtual ~SpaceView();
                virtual string getType() override;
                virtual void navigate(const string&) override;
                bool acceptsUri(const string&) override;
            protected:
            private:
        };
    }
}
