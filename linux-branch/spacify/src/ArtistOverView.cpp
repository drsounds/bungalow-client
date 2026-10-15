#include "ArtistOverView.h"

#include "../../space/src/SPHBoxElement.h"
#include "../../space/src/SPElement.h"
#include "../../space/src/SPViewElement.h"

namespace space {
ArtistOverView::ArtistOverView(SPElement *parent)
 : SPViewElement(parent)
{
    //ctor
    auto *header = new SPHBoxElement(this);
    header->setIntegerAttribute("height", 128);



}

ArtistOverView::~ArtistOverView()
{
    //dtor
}
}
