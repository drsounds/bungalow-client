//
// Created by Alexander Forselius on 2025-12-11.
//

#ifndef BUNGALOWCLIENT_SPIMAGESKIN_H
#define BUNGALOWCLIENT_SPIMAGESKIN_H
#include "SPSkin.h"

namespace space
{
    class SPImageSkin : SPSkin
    {
        explicit SPImageSkin(const string& bitmap);
        SPImage *image;
        SPImage *getImage(string selector, string def);
    };
}

#endif //BUNGALOWCLIENT_SPIMAGESKIN_H