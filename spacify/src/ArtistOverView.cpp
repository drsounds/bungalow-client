#include "ArtistOverView.h"

#include "SPHBoxElement.h"
#include "SPElement.h"
#include "SPViewElement.h"

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
