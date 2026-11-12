#pragma once
#include "SPViewElement.h"

namespace space {
    class SPElement;
    class SPContainerViewElement : public SPViewElement
    {
        public:
            SPContainerViewElement(SPElement *parent);
            SPContainerViewElement();
            virtual ~SPContainerViewElement();
    		void navigate(const string &uri) override;
    		bool acceptsUri(const string& uri) override;
        protected:
        private:
    };
    
}

