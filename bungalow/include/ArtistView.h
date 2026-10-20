#ifndef ARTISTVIEW_H
#define ARTISTVIEW_H
#include "SPTabBarViewElement.h"

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
#endif // ARTISTVIEW_H
