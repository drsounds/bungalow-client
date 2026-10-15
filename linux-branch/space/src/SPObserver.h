#pragma once
#include "libspace.h"

namespace space {
    class SPNode;
    class SPObserver {
        string m_event;
        SPCallback m_callback;
        SPElement* m_responder;
    	SPElement *m_sender;
    public:
        SPElement* getResponder() const {
            return this->m_responder;
        }
        string getEvent() {
            return this->m_event;
        }
        SPCallback getCallback() const {
            return this->m_callback;
        }
        SPObserver(const string& event, const SPCallback& callback, SPElement* responder, SPElement *sender) {
            this->m_event = event;
            this->m_callback = callback;
            this->m_responder = responder;
        	this->m_sender = sender;
        }
        ~SPObserver() = default;
    	SPElement *getSender() const {
    		return m_sender;
    	}
    	void setSender(SPElement *value) {
    		this->m_sender = value;
    	}
    };
}
