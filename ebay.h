#ifndef EBAY_H
#define EBAY_H
#include <string>
#include <vector>
#include <unordered_map>

namespace ebay
{
    class Bid
    {
    public:
        Bid(const std::string &u, double a) : user(u), amount(a) {}
        std::string user;
        double amount;

        bool operator<(const Bid &other) const { return amount < other.amount; }
    };

    class Items
    {
    public:
        Items() : item_name(""), seller(""), winner(""), win(false),
                  sPrice(0.0), binPrice(0.0), expiration(0), highestBid(0.0) {}
        ~Items() {}

        void setItem(const std::string &name) { item_name = name; }
        void setSeller(const std::string &s)  { seller = s; }
        void setPrices(const double s, const double b);
        void setTime(int t)                   { expiration = t; }

        std::string      getItem()       const { return item_name; }
        std::string      getSeller()     const { return seller; }
        double           getPrice_S()    const { return sPrice; }
        double           getPrice_B()    const { return binPrice; }
        int              getTime()       const { return expiration; }
        double           getHighestBid() const { return highestBid; }
        bool             isWon()         const { return win; }
        std::string      getWinner()     const { return winner; }
        std::vector<Bid> getBids()       const { return bids; }

        void addBid(const std::string userName, const double bidAmount);

    private:
        std::string      item_name;
        std::string      seller;
        std::string      winner;
        bool             win;
        double           sPrice;
        double           binPrice;
        int              expiration;
        double           highestBid;
        std::vector<Bid> bids;
    };

    class User
    {
    public:
        User();
        ~User();

        void setUsername(const std::string &u) { username = u; }
        void startSelling(Items* item);
        void clearSelling();
        void addHistory(const Items &add);
        void addInterest(Items* item) { interest.push_back(item); }
        void removeInterest(Items* item);

        std::string getUsername() const { return username; }
        bool        getIsSeller() const { return isSeller; }
        Items*      getSelling()  const { return selling; }
        bool        interested(const std::string& name) const;

    private:
        std::string          username;
        bool                 isSeller;
        Items*               history[30];
        Items*               selling;
        std::vector<Items*>  interest;
    };

    class Market
    {
    public:
        Market() {}
        ~Market();

        void addItem(const Items &i);
        void addUser(const User &u);
        void addLog(const std::string &l);

        std::vector<Items>&                    getListings();
        std::unordered_map<std::string, User>& getUsers();
        std::vector<std::string>&              getLogs();
        User*                                  getUser(const std::string& username);
        Items*                                 findItem(const std::string& item_name);

    private:
        std::vector<Items>                    active_listings;
        std::unordered_map<std::string, User> users;
        std::vector<std::string>              logs;
    };
}

#endif
