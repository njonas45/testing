#include "eBay.h"

using namespace ebay;

void Items::setPrices(const double s, const double b)
{
    sPrice   = s;
    binPrice = b;
}

void Items::addBid(const std::string userName, const double bidAmount)
{
    if (win)
        return;

    if (binPrice > 0 && bidAmount >= binPrice)
    {
        win        = true;
        winner     = userName;
        highestBid = binPrice;
        Bid newBid = Bid(userName, binPrice);
        bids.push_back(newBid);
    }
    else if (bidAmount >= highestBid)
    {
        highestBid = bidAmount;
        Bid newBid = Bid(userName, bidAmount);
        bids.push_back(newBid);
    }
    else
    {
        return;
    }
}

User::User()
{
    username = "";
    isSeller = false;
    selling  = nullptr;

    for(int i = 0; i < 30; i++)
        history[i] = nullptr;
}

User::~User()
{
    selling = nullptr;

    for(int i = 0; i < 30; i++)
    {
        delete history[i];
        history[i] = nullptr;
    }
}

void User::startSelling(Items* item)
{
    isSeller = true;
    selling  = item;
}

void User::clearSelling()
{
    selling = nullptr;
}

void User::addHistory(const Items& add)
{
    for(int i = 0; i < 30; i++)
    {
        if(history[i] == nullptr)
        {
            history[i] = new Items(add);
            return;
        }
    }

    delete history[0];
    for(int i = 0; i < 29; i++)
        history[i] = history[i+1];

    history[29] = new Items(add);
}

bool User::interested(const std::string& name) const
{
    size_t i = 0;
    while(i < interest.size())
    {
        if(interest[i] != nullptr && interest[i]->getItem() == name)
            return true;
        i++;
    }
    return false;
}

void User::removeInterest(Items* item)
{
    size_t i = 0;
    while(i < interest.size())
    {
        if(interest[i] == item)
        {
            interest.erase(interest.begin() + i);
            return;
        }
        i++;
    }
}

Market::~Market()
{
    // std::vector and std::unordered_map manage their own memory
}

void Market::addItem(const Items &i)
{
    active_listings.push_back(i);
}

void Market::addUser(const User &u)
{
    users[u.getUsername()] = u;
}

void Market::addLog(const std::string &l)
{
    logs.push_back(l);
}

std::vector<Items>& Market::getListings()
{
    return active_listings;
}

std::unordered_map<std::string, User>& Market::getUsers()
{
    return users;
}

std::vector<std::string>& Market::getLogs()
{
    return logs;
}

User* Market::getUser(const std::string& username)
{
    auto it = users.find(username);
    if (it == users.end()) return nullptr;
    return &it->second;
}

Items* Market::findItem(const std::string& item_name)
{
    for (Items& item : active_listings)
    {
        if (item.getItem() == item_name)
            return &item;
    }
    return nullptr;
}
