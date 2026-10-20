//
// Created by Alexander Forselius on 2025-12-11.
//

#pragma once 
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
