#include "SPLinkElement.h"
namespace space {
SPLinkElement::SPLinkElement(SPElement *parent)
 : SPTextElement(parent)
{
    //ctor
	this->setTagName("link");
}

string SPLinkElement::getLinkUri()
{
    const string uri = this->getStringAttribute("uri", "");
    return uri.empty() ? this->getStringAttribute("href", "") : uri;
}

SPRichText *SPLinkElement::getRichText()
{
    SPRichText *inner = SPTextElement::getRichText();
    const string uri = this->getLinkUri();
    if (uri.empty())
        return inner;
    // The body may have markup of its own: <link uri="…"><b>Queen</b></link>.
    const string content = inner ? inner->getMarkup() : SPRichText::escape(this->getInnerText());
    if (content.empty())
        return nullptr;
    m_linkText.setMarkup(SPRichText::wrapLink(uri, content));
    return &m_linkText;
}

SPLinkElement::~SPLinkElement()
{
    //dtor
}
}
