#ifndef EBAY_H
#define EBAY_H

#include <string>
#include <vector>
#include <unordered_map>

namespace ebay
{
    // Represents a single bid placed on an item
    class Bid
    {
    public:
        Bid(const std::string &u, double a) : user(u), amount(a) {}
        std::string user;
        double amount;

        bool operator<(const Bid &other) const { return amount < other.amount; }
    };

    // Represents a single auction listing
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

        // Adds a bid if valid; sets win=true if bidAmount >= binPrice
        void addBid(const std::string userName, const double bidAmount);

        // Forces the auction closed with the given winner (used when timer expires)
        void forceWin(const std::string& winnerName, double amount)
        {
            if (win) return;
            win        = true;
            winner     = winnerName;
            highestBid = amount;
            bids.push_back(Bid(winnerName, amount));
        }

    private:
        std::string      item_name;
        std::string      seller;
        std::string      winner;
        bool             win;
        double           sPrice;
        double           binPrice;
        int              expiration;  // Unix timestamp
        double           highestBid;
        std::vector<Bid> bids;
    };

    // Represents a registered user
    class User
    {
    public:
        User();
        ~User();

        void setUsername(const std::string &u) { username = u; }
        void startSelling(Items* item);         // sets isSeller=true and fills selling slot
        void clearSelling();                    // frees the selling slot
        void addHistory(const Items &add);      // inserts into history[30], evicts oldest if full
        void addInterest(Items* item)           { interest.push_back(item); }
        void removeInterest(Items* item);

        std::string getUsername() const { return username; }
        bool        getIsSeller() const { return isSeller; }
        Items*      getSelling()  const { return selling; }
        bool        interested(const std::string& name) const;

    private:
        std::string         username;
        bool                isSeller;
        Items*              history[30]; // fixed array, max 30 entries
        Items*              selling;     // one active listing at a time
        std::vector<Items*> interest;    // watchlist
    };

    // Central class — owns all users, listings, and logs
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
