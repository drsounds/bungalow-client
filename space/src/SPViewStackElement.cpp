#include "SPViewStackElement.h"

#include <iostream>

#include "WhatsNewView.h"
#include <regex>
#include "SPWindowElement.h"
#include "PlayQueueView.h"
#include "SPUri.h"

#include <vector>
#include <boost/algorithm/string/predicate.hpp>
inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
namespace space {
    void SPViewStackElement::appendChild(SPNode *child) {
    	SPNode::appendChild(child);
        SPViewElement *view = (SPViewElement *)child;
        string type = view->getType();
        // cout << "valid view: " << type << endl;
        ((SPElement *)child)->setVisible(false);
        view->setFlex( 1);
    	this->pack();
    }
    SPViewStackElement::SPViewStackElement() {
    }

    SPViewStackElement::SPViewStackElement(SPElement *parent)
     : SPVBoxElement(parent) {
    	this->setTagName("viewstack");

        this->activeView = nullptr;
    }
    void SPViewStackElement::pack() {
        SPVBoxElement::pack();
    }
    /**
     * Main navigation handler inside spacify. Records the current URI in
     * the history stack before navigating, and clears the forward stack.
     **/
    void SPViewStackElement::navigate(const string& value) {
        if (!currentUri.empty() && currentUri != value) {
            history.push(currentUri);
            while (!future.empty()) future.pop();
        }
        currentUri = value;
        navigateInternal(value);
    }

    void SPViewStackElement::goBack() {
        if (history.empty()) return;
        future.push(currentUri);
        currentUri = history.top();
        history.pop();
        navigateInternal(currentUri, NavigationIntent::Back);
    }

    void SPViewStackElement::goForward() {
        if (future.empty()) return;
        history.push(currentUri);
        currentUri = future.top();
        future.pop();
        navigateInternal(currentUri, NavigationIntent::Forward);
    }

    bool SPViewStackElement::canGoBack() const {
        return !history.empty();
    }

    bool SPViewStackElement::canGoForward() const {
        return !future.empty();
    }

    string SPViewStackElement::getCurrentUri() const {
        return currentUri;
    }

    void SPViewStackElement::navigateInternal(const string& value, NavigationIntent intent) {
        SPUri uri(value);

        // Find the most-specific (last) view that accepts this URI.
        // Catch-all views (e.g. SpaceView) are added first; specific views
        // are added later and therefore win when multiple views match.
        SPViewElement *matchedView = nullptr;
        for (auto* node : this->getChildNodes()) {
            SPViewElement *view = static_cast<SPViewElement *>(node);
            if (view->acceptsUri(uri.getAddress()))
                matchedView = view;
        }

        // Show the matched view and hide all others.
        for (auto* node : this->getChildNodes()) {
            SPViewElement *view = static_cast<SPViewElement *>(node);
            if (view == matchedView) {
                view->setVisible(true);
                if (intent == NavigationIntent::Back)
                    view->navigateBack(uri.getAddress());
                else if (intent == NavigationIntent::Forward)
                    view->navigateForward(uri.getAddress());
                else
                    view->navigate(uri.getAddress());
                view->setHash(uri.getHash());
                view->setFlex(1);
                activeView = view;
            } else {
                view->setVisible(false);
            }
        }

        this->invalidate();
        this->pack();
    }
}
