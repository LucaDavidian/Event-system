#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "event.h"
#include <memory>
#include <map>
#include <typeindex>
#include <vector>

struct Event {};

class EventHandlerBase
{
public:
	virtual ~EventHandlerBase() = default;

	virtual void Invoke(Event &) = 0;
};

template <typename TEvent, typename TObject>
class EventHandler : public EventHandlerBase
{
	friend class EventBus;

public:
	EventHandler(TObject &object, void (TObject::*handler)(TEvent&)) : mHandler(handler), mObject(object) {}

	void Invoke(Event &event) override
	{
		TEvent &tEvent = static_cast<TEvent&>(event);

		(mObject.*mHandler)(tEvent);
	}
private:
	void (TObject::*mHandler)(TEvent&);
	TObject &mObject;
};

class EventBus
{
public:
	~EventBus()
	{
		//for (auto &&it : mEventHandlers)
		//	for (auto &&eventHandler : it.second)
		//		delete eventHandler;
	}

	template <typename TEvent, typename TObject>
	void SubscribeToEvent(TObject &object, void (TObject::*handler)(TEvent &event))
	{
		//mEventHandlers[typeid(TEvent)].push_back(new EventHandler<TEvent, TObject>(object, handler));
		mEventHandlers[typeid(TEvent)].push_back(std::make_unique<EventHandler<TEvent, TObject>>(object, handler));
	}

	template <typename TEvent, typename TObject>
	void UnsubscribeFromEvent(TObject &object, void (TObject::*handler)(TEvent &&event))
	{
		auto it = mEventHandlers.find(typeid(TEvent));

		if (it != mEventHandlers.end())
		{
			std::vector<std::unique_ptr<EventHandlerBase>> &handlers = it->second;

			handlers.erase(std::remove_if(handlers.begin(), handlers.end(), [object, handler](TObject &otherObject, void (TObject::*otherHandler)(TEvent &&event)) {
				return object == otherObject && handler == otherHandler;
			}));
		}
	}

	template <typename TEvent>
	void DispatchEvent(TEvent &&event)
	{
		auto it = mEventHandlers.find(typeid(TEvent));

		if (it != mEventHandlers.end())
			for (auto &&eventHandler : it->second)
				eventHandler->Invoke(std::forward<TEvent>(event));
	}

	template <typename TEvent, typename... Args>
	void DispatchEvent(Args&&... args)
	{
		auto it = mEventHandlers.find(typeid(TEvent));

		if (it != mEventHandlers.end())
			for (auto &&eventHandler : it->second)
			{
				TEvent e{ {}, std::forward<Args>(args)... };
				eventHandler->Invoke(e);
			}
	}

private:
	//std::map<std::type_index, std::vector<EventHandlerBase*>> mEventHandlers;
	std::map<std::type_index, std::vector<std::unique_ptr<EventHandlerBase>>> mEventHandlers;
};

#endif // EVENTBUS_H
