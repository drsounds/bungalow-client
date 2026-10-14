//
// Created by Alexander Forselius on 2025-12-08.
//

#ifndef SPACIFYCLIENT_SPNAVIGATEEVENTARGS_H
#define SPACIFYCLIENT_SPNAVIGATEEVENTARGS_H

#include "SPEventArgs.h"
#include "SPUri.h"

namespace space
{
    class SPObject;
    class SPNavigateEventArgs : public SPEventArgs
    {
    	SPUri *uri;
    public:
        explicit SPNavigateEventArgs(const string& url);
        virtual ~SPNavigateEventArgs();
        string getUrl() {
            return this->url;
        }
    	SPUri *getUri() {
	        return uri;
        }
        private:
        string url;
    };
} // space

#endif //SPACIFYCLIENT_SPNAVIGATEEVENTARGS_H