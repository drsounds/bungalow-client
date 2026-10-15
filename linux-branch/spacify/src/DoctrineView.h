#pragma once
#include "../../space/src/SPContainerViewElement.h"
namespace space {
    class SPElement;
    class SPTextElement;
    namespace views {
        class DoctrineView : public SPContainerViewElement
        {
            public:
                DoctrineView(SPElement *parent);
                virtual ~DoctrineView();
                void navigate(const string& uri) override;
                bool acceptsUri(const string& uri) override;
            protected:
                SPTextElement *text;
            private:
        };
    }
}
