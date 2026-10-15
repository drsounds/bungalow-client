//
// Created by Alexander Forselius on 2025-12-03.
//

#ifndef SPACIFYCLIENT_APP_H
#define SPACIFYCLIENT_APP_H
#include <string>

#include "SPObservable.h"

#include <queue>

using namespace std;

namespace space
{
    class SPWindowElement;
    class SPEvent;
    class SPStore;
    class SPSkin;
    class SPGraphicsContext;
    class SPSubscription;
    class Space : public SPObservable
    {
    protected:
        //Spider *m_spider;
        SPStore *m_store;
        SPGraphicsContext *m_graphicsContext;
        vector<SPWindowElement *> m_windows;
        queue<SPEvent> m_events;
        queue<SPMessage> m_messages;
    public:
        /*spider::Spider *getSpider() {
            return m_spider;
        }*/
        SPSubscription *subscribe(string uri);
        vector<SPWindowElement *>& getWindows() {
            return m_windows;
        }
        queue<SPEvent>& getEvents() {
            return m_events;
        }
        queue<SPMessage>& getMessages() {
            return m_messages;
        }
        void setWindows(const vector<SPWindowElement *>& value) {
            this->m_windows = value;
        }
        SPStore *getStore() {
            return m_store;
        }
        void setStore(SPStore *value) {
            m_store = value;
        }
        void addWindow(SPWindowElement *window);
        SPGraphicsContext *getGraphicsContext()
        {
            return this->m_graphicsContext;
        }
        void setGraphicsContext(SPGraphicsContext *value) {
            this->m_graphicsContext = value;
        }
        virtual int notify(const string &event, SPObject *sender,
                           SPEventArgs *data) override;
        Space();
        SPWindowElement * createWindow(SPSkin *skin);
        SPMessage dequeueNextMessage();
        void postMessage(const SPMessage& message);
        void processEvent(const SPEvent& event);
        void processMessage(const SPMessage& message);
    };
}

#endif //SPACIFYCLIENT_APP_H