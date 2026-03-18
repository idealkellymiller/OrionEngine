#pragma once

#include "EngineCore.h"
#include <functional>
#include <string>
#include <sstream>

namespace Orion {

    enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

    // macro to assign and define event getters for each type of event 
    #define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::##type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

    class ORION_API Event 
    {
        friend class EventDispatcher;
        public:
            virtual EventType GetEventType() const = 0;
            virtual const char* GetName() const = 0;
            virtual std::string ToString() const { return GetName(); }
            bool Handled = false;
    };

    // EventDispatcher dispatches events based on their type
    class EventDispatcher
    {
        // T is a placeholder for an Event
        template<typename T> 
        using EventFn = std::function<bool(T&)>;
        public:
            EventDispatcher(Event& event) : m_Event(event) 
            {
            }

            template<typename T>
            bool Dispatch(EventFn<T> func)
            {
                // make sure event's type matches EventFn T type
                if (m_Event.GetEventType() == T::GetStaticType())
                {
                    // dispatch to the corresponding function for this type of event
                    m_Event.Handled = func(*(T*)&m_Event);
                    return true;
                }
                return false;
            }

        private:
            Event& m_Event;
    };

    // for logging
    inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

}
