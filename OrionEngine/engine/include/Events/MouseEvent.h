#pragma once

#include "Event.h"

namespace Orion 
{
    class ORION_API MouseMovedEvent : public Event
    {
        public: 
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

    class ORION_API MouseScrolledEvent : public Event
    {
        public:
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

    class ORION_API MouseBtnEvent : public Event
    {
        public:
            inline int GetMouseBtn() const { return MouseBtn; }

        protected:
            int MouseBtn;
            // constructor
            MouseBtnEvent(int button) : MouseBtn(button) {}
    };

    class ORION_API MouseBtnPressedEvent : public MouseBtnEvent
    {
        public:
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

    class ORION_API MouseBtnReleasedEvent : public MouseBtnEvent
    {
        public: 
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