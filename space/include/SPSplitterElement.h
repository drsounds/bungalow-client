#ifndef SPLITTERELEMENT_H
#define SPLITTERELEMENT_H
#include "SPElement.h"
namespace space {
class SPSplitterElement : public SPElement
{
    public:
		SPSplitterElement(SPElement *parent);
		void paint(int x, int y, int width, int height, SPGraphicsContext* c) override;
        virtual ~SPSplitterElement();

        /// The element whose width is adjusted when this splitter is dragged
        /// horizontally. The splitter is expected to sit immediately after it.
        void setTarget(SPElement* target) { m_target = target; }
        void setMinWidth(int w) { m_minWidth = w; }
        void setMaxWidth(int w) { m_maxWidth = w; }

        void mouseDown(SPObject* sender, SPEventArgs* e) override;
        void mouseUp(SPObject* sender, SPEventArgs* e) override;
    protected:
    private:
        SPElement* m_target    = nullptr;
        bool       m_dragging  = false;
        int        m_dragStartX = 0;
        int        m_startWidth = 0;
        int        m_minWidth   = 150;
        int        m_maxWidth   = 600;

        void onMouseMove(SPObject* sender, SPEventArgs* e);
};
}

#endif // SPLITTERELEMENT_H
