#pragma once
#include "../../space/src/SPContainerViewElement.h"

namespace space {
    class SPElement;
    namespace views {
        class TopListView : public SPContainerViewElement
        {
            public:
                TopListView();
                TopListView(SPElement *parent);
                virtual ~TopListView();
                bool acceptsUri(const string& uri) override;
                void navigate(const string& uri) override;
                string getType() override;
                virtual void scroll(int scrollX, int scrollY, int x, int y) override;
        };
    }
}

