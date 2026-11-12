#pragma once

#include "../../space/src/SPButtonElement.h"
#include "../../space/src/SPContainerViewElement.h"

namespace space {
    class SPElement;
    namespace views {
        class WhatsNewView : public SPContainerViewElement
        {
            public:
                WhatsNewView();
                WhatsNewView(SPElement *parent);
                virtual ~WhatsNewView();
                void navigate(const string& uri) override;
                bool acceptsUri(const string& uri) override;
                string getType() override {
                    return "whatsnewview";
                }

                SPButtonElement * button;
        };
    }
}

