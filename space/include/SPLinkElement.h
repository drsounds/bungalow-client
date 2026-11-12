#ifndef LINKELEMENT_H
#define LINKELEMENT_H
#include "SPTextElement.h"
#include "SPRichText.h"

namespace space {
class SPElement;
/**
 * A text element that is a link as a whole:
 *
 *   <link uri="bungalow:artist:1">Queen</link>
 *   <a href="https://example.com">Example</a>
 *
 * A click navigates to its uri (or href) attribute. Inside the body of
 * another text element the same tags make part of that text a link instead;
 * see SPElement::setInnerHTML().
 */
class SPLinkElement : public SPTextElement
{
    public:
        SPLinkElement(SPElement *parent);
        virtual ~SPLinkElement();
        virtual string getType() override {
            return "link";
        }
        /// The uri attribute, or href when there is none.
        string getLinkUri();
    protected:
        SPRichText *getRichText() override;
    private:
        SPRichText m_linkText;
};
}

#endif // LINKELEMENT_H
