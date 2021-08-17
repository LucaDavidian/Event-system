#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <vector>
#include <type_traits>
#include "../../delegates/delegates (virtual dispatch) with connections/signal.hpp"

inline int GetUniqueEventId()
{
    static int id = 0;

    return id++;
}

template <typename Event>
int GetEventId()
{
    static int eventID = GetUniqueEventId();

    return eventID;
}

class EventBus
{
private:
    struct EventHandlerBase
    {
        virtual ~EventHandlerBase() = default;

        virtual void DispatchQueuedEvents() = 0;
        virtual void ClearEventQueue() = 0;
    };

    template <typename Event>
    class EventHandler : public EventHandlerBase
    {
        friend class EventBus;
    public:
        template <typename... Args>
        void TriggerEvent(Args&&... args)
        {
            Event event{std::forward<Args>(args)...};
            mSignal(event);
        }

        // SFINAE implementation
        template <typename... Args, typename U = Event, typename = typename std::enable_if<std::is_aggregate<U>::value>::type>
        void EnqueueEvent(Args&&... args) 
        {
            mEventQueue.push_back(Event{std::forward<Args>(args)...});
        }

        template <typename... Args, typename U = Event, typename = typename std::enable_if<!std::is_aggregate<U>::value>::type, typename = void>
        void EnqueueEvent(Args&&... args) 
        {
            mEventQueue.emplace_back(std::forward<Args>(args)...); 
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

        void DispatchQueuedEvents() override
        {
            for (auto &event : mEventQueue)
                mSignal(event);

            ClearEventQueue();
        }

        void ClearEventQueue()
        {
            mEventQueue.clear();
        }

    private:
        Signal<void(Event&)> mSignal;
        std::vector<Event> mEventQueue;
    };

public:
    template <typename Event, typename... Args>
    void TriggerEvent(Args&&... args);

    template <typename Event>
    void TriggerEvent(Event &&event);   

    template <typename Event, typename... Args>
    void EnqueueEvent(Args&&... args);

    template <typename Event>
    void EnqueueEvent(Event &&event);
    
    template <typename... Event>
    void DispatchQueuedEvents() const;

    void DispatchQueuedEvents() const;

    template <typename... Event>
    void ClearEventQueues();

    void ClearEventQueues();

    // C++17 constexpr-if implementation
    // template <typename... Event>
    // void ClearEventQueues()
    // {
    //     if constexpr (sizeof...(Event) != 0)
    //         (GetEventHandler<Event>().ClearEvents(), ...);
    //     else  // sizeof...(Event) == 0
    //         for (std::size_t i = 0; i < mEventHandlers.size(); i++)
    //             if (mEventHandlers[i])
    //                 mEventHandlers[i]->ClearEvents();
    // }

    // template <typename Event, typename T>
    // Connection SubscribeToEvent(T &instance, void (T::*ptrToMemFun)(Event&))
    // {
    //     return GetEventHandler<Event>().mSignal.Bind(instance, ptrToMemFun);
    // }

    // template <typename Event, typename T>
    // Connection SubscribeToEvent(T &instance, void (T::*ptrToConstMemFun)(Event&) const)
    // {
    //     return GetEventHandler<Event>().mSignal.Bind(instance, ptrToConstMemFun);
    // }

    template <typename Event, typename T, typename PtrToMemFun>
    Connection SubscribeToEvent(T &instance, PtrToMemFun ptrToMemFun)
    {
        return GetEventHandler<Event>().mSignal.Bind(instance, ptrToMemFun);
    }

    template <typename Event, typename T>
    Connection SubscribeToEvent(T &&funObj)
    {
        return GetEventHandler<Event>().mSignal.Bind(std::forward<T>(funObj));
    }

    void UnsubscribeFromEvent(Connection connection)
    {
        connection.Disconnect();
    }
private:
    template <typename Event>
    EventHandler<Event> &GetEventHandler(); 

    std::vector<EventHandlerBase*> mEventHandlers;
};

template <typename Event>
EventBus::EventHandler<Event> &EventBus::GetEventHandler()
{
    const auto eventHandlerIndex = GetEventId<Event>();

    if (eventHandlerIndex >= mEventHandlers.size())
        mEventHandlers.push_back(new EventHandler<Event>);

    return static_cast<EventHandler<Event>&>(*mEventHandlers[eventHandlerIndex]);
}

template <typename Event, typename... Args>
void EventBus::TriggerEvent(Args&&... args)
{
    GetEventHandler<Event>().TriggerEvent(std::forward<Args>(args)...);
}

template <typename Event>
void EventBus::TriggerEvent(Event &&event)
{
    GetEventHandler<typename std::remove_reference<Event>::type>().TriggerEvent(std::forward<Event>(event));
}

template <typename Event, typename... Args>
void EventBus::EnqueueEvent(Args&&... args)
{
    GetEventHandler<Event>().EnqueueEvent(std::forward<Args>(args)...);
}

template <typename Event>
void EventBus::EnqueueEvent(Event &&event)
{
    GetEventHandler<typename std::remove_reference<Event>::type>().EnqueueEvent(std::forward<Event>(event));
}

template <typename... Event>
void EventBus::DispatchQueuedEvents() const
{
    (GetEventHandler<Event>().DispatchEvents(), ...);
}

void EventBus::DispatchQueuedEvents() const
{
    for (EventHandlerBase *eventHandler : mEventHandlers)
        if (eventHandler)
            eventHandler->DispatchQueuedEvents();
}

template <typename... Events>
void EventBus::ClearEventQueues()
{
    (GetEventHandler<Events>().ClearEventQueue(), ...);
}

void EventBus::ClearEventQueues()
{
    for (auto eventHandler : mEventHandlers)
        if (eventHandler)
            eventHandler->ClearEventQueue(); 
}

#endif  // EVENT_BUS_H