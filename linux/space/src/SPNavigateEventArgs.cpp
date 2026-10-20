//
// Created by Alexander Forselius on 2025-12-08.
//

#include "SPNavigateEventArgs.h"

#include "SPObject.h"

namespace space
{
    SPNavigateEventArgs::SPNavigateEventArgs(const string& url)
    {
        this->url = url;
    	this->uri = new SPUri(url);
    }

    SPNavigateEventArgs::~SPNavigateEventArgs()
    {
    	delete this->uri;
    }
} // space