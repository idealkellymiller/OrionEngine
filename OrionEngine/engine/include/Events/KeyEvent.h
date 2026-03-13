#pragma once

#include "Event.h"

namespace Orion {

    class KeyEvent : public Event 
    {
        public:
            inline int GetKeyCode() const { return KeyCode; }

        protected:
            int KeyCode;
            // constructor
            KeyEvent(int keycode) : KeyCode(keycode) {}
    };

    class KeyPressedEvent : KeyEvent
    {
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

    class KeyReleasedEvent : KeyEvent
    {
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