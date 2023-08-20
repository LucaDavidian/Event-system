#include "EventBus.hpp"
#include <string>
#include <iostream>

struct HitEvent
{
    const std::string mMessage;
    const int mDamage;
};

struct BonusEvent
{
    const int mBonus;
};

EventBus eventDispatcher;

class Player
{
public:
    Player() : mHealth(100), mConnection(eventDispatcher.SubscribeToEvent<HitEvent>(*this, &Player::IsHit)), mConnection2(eventDispatcher.SubscribeToEvent<BonusEvent>(*this, &Player::GetBonus)) {}

    ~Player() { eventDispatcher.UnsubscribeFromEvent(mConnection); }

    void IsHit(HitEvent event) { std::cout << event.mMessage << std::endl; mHealth -= event.mDamage; }
    void GetBonus(BonusEvent event) { mHealth += event.mBonus; }

    int GetHealth() const { return mHealth; }
private:
    int mHealth;
    Connection mConnection;
    Connection mConnection2;
};



int main(int argc, char **argv)
{
    {
        Player p1;
        Player p2;

        std::cout << "p1 health: " << p1.GetHealth() << std::endl;

        eventDispatcher.EnqueueEvent(HitEvent{"damage", 10});
        eventDispatcher.EnqueueEvent(HitEvent{"damage + 2", 10});
        
        eventDispatcher.DispatchQueuedEvents();

        std::cout << "p1 health: " << p1.GetHealth() << std::endl;

        eventDispatcher.EnqueueEvent(BonusEvent{40});

        eventDispatcher.DispatchQueuedEvents();

        std::cout << "p1 health: " << p1.GetHealth() << std::endl;

        eventDispatcher.TriggerEvent<HitEvent>("critical", 50);

        std::cout << "p1 health: " << p1.GetHealth() << std::endl;
    }

    eventDispatcher.DispatchQueuedEvents();

    return 0;
}