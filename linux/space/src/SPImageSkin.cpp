//
// Created by Alexander Forselius on 2025-12-11.
//

#include "SPImageSkin.h"

#include <iostream>

#include "SPImage.h"

using namespace space;

SPImageSkin::SPImageSkin(const string& bitmap) : SPSkin() {
    // cout << bitmap << "\r\n";
    auto *image = new SPImage(bitmap);
    // cout << "Sizeof pixels: " << sizeof(image->pixels) << "\r\n";
    this->image = image;
    // cout << "Sizeof pixels: " << sizeof(image->pixels) << "\r\n";
    this->colors.insert(pair<string, pixel *>("body.background.color", this->image->getPixelPtr(0, 0)));

    (this->colors)["body.foreground.color"] = this->image->getPixelPtr(1, 0);
    (this->colors)["::selected.background.color"] = this->image->getPixelPtr(7, 0);
    (this->colors)["::selected.foreground.color"] = this->image->getPixelPtr(8, 0);
    (this->slices)["#header.background.image"] = new slice(image, 214, 0, 27, 56, 1, 0, 0, 0);
    (this->slices)["tab.background.image::active"] = new slice(image, 0, 11, 32, 23, 1, 1, 1, 1);
    (this->slices)["tabbar.background.image"] = new slice(image, 97, 1, 1, 22, 0, 0, 0, 0);
    (this->slices)["button.background.image"] = new slice(image, 128, 1, 71, 19, 3, 13, 13, 3);
    (this->slices)["table::columnheader.background.image"] = new slice(image, 200, 63, 18, 18, 0, 0, 0, 0);
    (this->colors)["table::nth-child(2n).background.color"] = this->image->getPixelPtr(22, 0);
    (this->colors)["treeitem::selected.background.color"] = this->image->getPixelPtr(6, 0);
    (this->colors)["treeitem::selected.foreground.color"] = this->image->getPixelPtr(7, 0);
    (this->colors)["treeview.foreground.color"] = this->image->getPixelPtr(1, 0);
    (this->colors)["treeitem::focus.foreground.color"] = this->image->getPixelPtr(2, 0);
    (this->colors)["treeview.background.color"] = this->image->getPixelPtr(3, 0);
    //  (*this->images)["button.background.image::active"] = this->image->sliceImage(128, 21, 71, 19, 3, 13, 13, 3);
}
SPImage *SPImageSkin::getImage(string selector, string def) {
    if (!this->hasImage(selector)) {
        return NULL;
    }
    return this->images[selector];
}