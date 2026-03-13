#pragma once

#include "Event.h"

namespace Orion 
{
    class MouseMovedEvent : public Event
    {
        // constructor
        MouseMovedEvent(float x, float y) : MouseX(x), MouseY(y) {}

        inline float GetX() const { return MouseX; }
        inline float GetY() const { return MouseY; }  

        EVENT_CLASS_TYPE(MouseMoved)

        std::string ToString() const override
        {
	        std::stringstream ss;
	        ss << "MouseMovedEvent: " << GetX() << ", " << GetY();
	        return ss.str();
        }

        private:
            float MouseX;
            float MouseY;
    };

    class MouseScrolledEvent : public Event
    {
        // constructor
        MouseScrolledEvent(float xOffset, float yOffset) : XOffset(xOffset), YOffset(yOffset) {}

        inline float GetXOffset() const { return XOffset; }
        inline float GetYOffset() const { return YOffset; }

        EVENT_CLASS_TYPE(MouseScrolled)

        std::string ToString() const override
        {
	        std::stringstream ss;
	        ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
	        return ss.str();        
        }

        private:
            float XOffset;
            float YOffset;
    };

    class MouseBtnEvent : public Event
    {
        public:
            inline int GetMouseBtn() const { return MouseBtn; }

        protected:
            int MouseBtn;
            // constructor
            MouseBtnEvent(int button) : MouseBtn(button) {}
    };

    class MouseBtnPressedEvent : MouseBtnEvent
    {
        // constructor
        MouseBtnPressedEvent(int button) : MouseBtnEvent(button) {}

        EVENT_CLASS_TYPE(MouseButtonPressed)

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonPressedEvent: " << GetMouseBtn();
            return ss.str();
        }

    };

    class MouseBtnReleasedEvent : MouseBtnEvent
    {
        // constructor
        MouseBtnReleasedEvent(int button) : MouseBtnEvent(button) {}

        EVENT_CLASS_TYPE(MouseButtonReleased)

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "MouseButtonReleasedEvent: " << GetMouseBtn();
            return ss.str();
        }
    };

}