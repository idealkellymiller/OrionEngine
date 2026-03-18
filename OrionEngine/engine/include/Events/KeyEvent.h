#pragma once

#include "Event.h"

namespace Orion {

    class ORION_API KeyEvent : public Event 
    {
        public:
            inline int GetKeyCode() const { return KeyCode; }

        protected:
            int KeyCode;
            // constructor
            KeyEvent(int keycode) : KeyCode(keycode) {}
    };

    class ORION_API KeyPressedEvent : public KeyEvent
    {
        public:
            // constructor
            KeyPressedEvent(int keycode, int repeatCnt) : KeyEvent(keycode), RepeatCount(repeatCnt) {}

            inline int GetRepeatCount() const { return RepeatCount; }

            EVENT_CLASS_TYPE(KeyPressed)

            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyPressedEvent: " << GetKeyCode() << " (repeated " << GetRepeatCount() << " times)";
                return ss.str();
            }
        
        private:
            int RepeatCount;
    };

    class ORION_API KeyReleasedEvent : public KeyEvent
    {
        public:
            // constructor
            KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}

            EVENT_CLASS_TYPE(KeyReleased)

            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyReleasedEvent: " << GetKeyCode();
                return ss.str();
            }
    };

}