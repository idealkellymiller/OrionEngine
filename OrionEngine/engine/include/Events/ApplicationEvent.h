#pragma once

#include "Event.h"
#include <sstream>

namespace Orion
{
    class ORION_API WindowResizeEvent : public Event
    {
        public:
            // constructor
            WindowResizeEvent(unsigned int width, unsigned int height) :
                Width(width), Height(height) {}

            unsigned int GetWidth() const { return Width; }
            unsigned int GetHeight() const { return Height; }
            
            EVENT_CLASS_TYPE(WindowResize)
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "WindowResizeEvent: " << "Width- " << GetWidth() << ", Height- " << GetHeight();
                return ss.str();
            }

        private:
            unsigned int Width, Height;
    };

    class ORION_API WindowCloseEvent : public Event
    {
        public: 
            // constructor
            WindowCloseEvent() = default;

            EVENT_CLASS_TYPE(WindowClose)

            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "WindowCloseEvent - closing window";
                return ss.str();
            }
    };
}