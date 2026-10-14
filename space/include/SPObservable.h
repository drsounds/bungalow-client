#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include "libspace.h"

#include <list>
#include <string>
#include <vector>

using namespace std;
namespace space {
    class SPElement;
    class SPEventArgs;
    class SPObserver;
    class SPObject;
    class SPObservable
    {
        public:
            SPObservable();
            void dispatchEvent(const string& event, SPEventArgs *data, SPObject *sender);
            virtual int notify(const string &event, SPObject *sender,
                               SPEventArgs *data);
            void addEventListener(const string& event, const SPCallback& callback, SPElement* responder, SPElement* sender);
            virtual ~SPObservable();
        protected:
            vector<SPObserver *> observers;
        private:
    };
}

#endif // OBSERVABLE_H
