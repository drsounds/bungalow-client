#pragma once
#include "../../space/src/SPTabBarViewElement.h"

namespace space {
    class SPTabBarViewElement;
    class SPElement;
    class ArtistView : public SPTabBarViewElement
    {
        public:
            ArtistView(SPElement *parent);
            virtual ~ArtistView();
            virtual string getType() override {
                return "artistview";
            }
        protected:
        private:
    };
}
