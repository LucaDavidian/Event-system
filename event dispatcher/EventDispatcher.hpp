#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include <vector>
#include <type_traits>
#include "../../delegates/delegates (virtual dispatch) with connections/signal.hpp"

static int GetUniqueId()
{
    static int id = 0;

    return id++;
}

template <typename Event>
static int GetEventId()
{
    static int eventID = GetUniqueId();

    return eventID;
}

class EventDispatcher
{
private:
    struct EventHandlerBase
    {
        virtual ~EventHandlerBase() = default;

        virtual void DispatchEvents() = 0;
        virtual void ClearEvents() = 0;
    };

    template <typename Event>
    struct EventHandler : public EventHandlerBase
    {
        template <typename... Args>
        void TriggerEvent(Args&&... args)
        {
            Event event{std::forward<Args>(args)...};
            mSignal(event);
        }

        template <typename... Args, typename U = Event, typename = typename std::enable_if<std::is_aggregate<U>::value>::type>
        void EnqueueEvent(Args&&... args) 
        {
            mEvents.push_back(Event{std::forward<Args>(args)...});
        }

        template <typename... Args, typename U = Event, typename = typename std::enable_if<!std::is_aggregate<U>::value>::type, typename = void>
        void EnqueueEvent(Args&&... args) 
        {
            mEvents.emplace_back(std::forward<Args>(args)...); 
        }

        // C++17 constexpr-if implementation
        // template <typename... Args>
        // void EnqueueEvent(Args&&... args) 
        // {
        //     if constexpr (std::is_aggregate_v<Event>)
        //         mEvents.push_back(Event{std::forward<Args>(args)...}); 
        //     else
        //         mEvents.emplace_back(std::forward<Args>(args)...); 
        // }

        void DispatchEvents() override
        {
            for (auto &event : mEvents)
                mSignal(event);

            mEvents.clear();
        }

        void ClearEvents()
        {
            mEvents.clear();
        }

        Signal<void(Event&)> mSignal;
        std::vector<Event> mEvents;
    };

public:
    template <typename Event>
    EventHandler<Event> &GetEventHandler(); 

    template <typename Event, typename... Args>
    void TriggerEvent(Args&&... args);

    template <typename Event>
    void TriggerEvent(Event &&event);   

    template <typename Event, typename... Args>
    void EnqueueEvent(Args&&... args);

    template <typename Event>
    void EnqueueEvent(Event &&event);
    
    void DispatchEvents() const;

    template <typename Event>
    void DispatchEvents() const;

    template <typename... Event, typename = std::enable_if_t<sizeof...(Event) != 0U>>
    void ClearEvents();

    template <typename... Event, typename = std::enable_if_t<sizeof...(Event) == 0U>, typename = void>
    void ClearEvents();

    // C++17 constexpr-if implementation
    // template <typename... Event>
    // void ClearEvents()
    // {
    //     if constexpr (sizeof...(Event) != 0)
    //         (GetEventHandler<Event>().ClearEvents(), ...);
    //     else  // sizeof...(Event) == 0
    //         for (std::size_t i = 0; i < mEventHandlers.size(); i++)
    //             if (mEventHandlers[i])
    //                 mEventHandlers[i]->ClearEvents();
    // }

    template <typename Event, typename T>
    Connection FollowEvent(T &instance, void (T::*ptrToMemFun)(Event&))
    {
        return GetEventHandler<Event>().mSignal.Bind(instance, ptrToMemFun);
    }

    template <typename Event, typename T>
    Connection FollowEvent(T &instance, void (T::*ptrToConstMemFun)(Event&) const)
    {
        return GetEventHandler<Event>().mSignal.Bind(instance, ptrToConstMemFun);
    }

    template <typename Event, typename T>
    Connection FollowEvent(T &&funObj)
    {
        return GetEventHandler<Event>().mSignal.Bind(std::forward<T>(funObj));
    }

    void UnfollowEvent(Connection connection)
    {
        connection.Disconnect();
    }

private:
    std::vector<EventHandlerBase*> mEventHandlers;
};

template <typename Event>
EventDispatcher::EventHandler<Event> &EventDispatcher::GetEventHandler()
{
    const auto eventIndex = GetEventId<Event>();

    if (eventIndex >= mEventHandlers.size())
        mEventHandlers.push_back(new EventHandler<Event>);

    return static_cast<EventHandler<Event>&>(*mEventHandlers[eventIndex]);
}

template <typename Event, typename... Args>
void EventDispatcher::TriggerEvent(Args&&... args)
{
    GetEventHandler<Event>().TriggerEvent(std::forward<Args>(args)...);
}

template <typename Event>
void EventDispatcher::TriggerEvent(Event &&event)
{
    GetEventHandler<typename std::decay<Event>::type>().TriggerEvent(std::forward<Event>(event));
}

template <typename Event, typename... Args>
void EventDispatcher::EnqueueEvent(Args&&... args)
{
    GetEventHandler<Event>().EnqueueEvent(std::forward<Args>(args)...);
}

template <typename Event>
void EventDispatcher::EnqueueEvent(Event &&event)
{
    GetEventHandler<typename std::decay<Event>::type>().EnqueueEvent(std::forward<Event>(event));
}

void EventDispatcher::DispatchEvents() const
{
    for (EventHandlerBase *eventHandler : mEventHandlers)
        if (eventHandler)
            eventHandler->DispatchEvents();
}

template <typename Event>
void EventDispatcher::DispatchEvents() const
{
    GetEventHandler<Event>().DispatchEvents();
}

template <typename... Events, typename>
void EventDispatcher::ClearEvents()
{
    (GetEventHandler<Events>().ClearEvents(), ...);
}

template <typename... Event, typename, typename>
void EventDispatcher::ClearEvents()
{
    for (std::size_t i = 0; i < mEventHandlers.size(); i++)
        if (mEventHandlers[i])
            mEventHandlers[i]->ClearEvents();
}

#endif  // EVENT_DISPATCHER_H