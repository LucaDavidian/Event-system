#include "EventDispatcher.hpp"
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

extern EventDispatcher eventDispatcher;

class Player
{
public:
    Player() : mHealth(100), mConnection(eventDispatcher.FollowEvent<HitEvent>(*this, &Player::IsHit)), mConnection2(eventDispatcher.FollowEvent<BonusEvent>(*this, &Player::GetBonus)) {}

    ~Player() { eventDispatcher.UnfollowEvent(mConnection); }

    void IsHit(HitEvent &event) { std::cout << event.mMessage << std::endl; mHealth -= event.mDamage; }
    void GetBonus(BonusEvent &event) { mHealth += event.mBonus; }

    int GetHealth() const { return mHealth; }
private:
    int mHealth;
    Connection mConnection;
    Connection mConnection2;
};

EventDispatcher eventDispatcher;

int main(int argc, char **argv)
{
    {
        Player p1;

        std::cout << "p1 health: " << p1.GetHealth() << std::endl;

        eventDispatcher.EnqueueEvent(HitEvent{"damage", 10});
        eventDispatcher.EnqueueEvent(HitEvent{"damage + 2", 10});
        
        eventDispatcher.DispatchEvents();

        std::cout << "p1 health: " << p1.GetHealth() << std::endl;

        eventDispatcher.EnqueueEvent(BonusEvent{40});

        eventDispatcher.DispatchEvents();

        std::cout << "p1 health: " << p1.GetHealth() << std::endl;
    }

    eventDispatcher.DispatchEvents();

    return 0;
}