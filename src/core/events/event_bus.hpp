#pragma once

struct event
{
	virtual ~event() = default;
};

class base_event_handler
{
public:
	virtual ~base_event_handler() = default;

	virtual void Call(event& E) = 0;

	void Execute(event& E)
	{
		Call(E);
	}
};

template<typename owner, typename event_type>
class event_handler : public base_event_handler
{
	typedef void (owner::*callback_function)(event_type&);

public:
	event_handler(owner* NewOwner, callback_function NewCallback)
	{
		Owner = NewOwner;
		Callback = NewCallback;
	}

	~event_handler() override = default;

	void Call(event& E)
	{
		std::invoke(Callback, Owner, static_cast<event_type&>(E));
	}

	void Execute(event_type& E)
	{
		Call(static_cast<event&>(E));
	}

private:
	owner* Owner;
	callback_function Callback;
};

typedef std::list<std::unique_ptr<base_event_handler>> handler_list;
struct event_bus
{
	// TODO: multimap?
	std::unordered_map<std::type_index, std::unique_ptr<handler_list>> EventList;
	std::vector<std::function<void()>> EventsQueue;

	void Reset()
	{
		EventList.clear();
        EventsQueue.clear();
	}

	template<typename owner, typename event_type>
	void Subscribe(owner* Owner, void (owner::*CallbackFunction)(event_type& E))
	{
		if(!EventList[std::type_index(typeid(event_type))].get())
		{
			EventList[std::type_index(typeid(event_type))] = std::make_unique<handler_list>();
		}

		EventList[std::type_index(typeid(event_type))]->push_back(std::make_unique<event_handler<owner, event_type>>(Owner, CallbackFunction));
	}

	template<typename event_type, typename... args>
	void Emit(args&&... Args)
	{
		handler_list* Handlers = EventList[std::type_index(typeid(event_type))].get();
		if(Handlers)
		{
			event_type Event(std::forward<args>(Args)...);
			for(std::unique_ptr<base_event_handler>& Handler : *Handlers)
			{
				base_event_handler* Handle = Handler.get();
				EventsQueue.push_back([=](){Handle->Execute(const_cast<event_type&>(Event));});
			}
		}
	}

    void DispatchEvents()
    {
        for (auto& EventFunction : EventsQueue)
        {
            EventFunction();
        }
    }
};
