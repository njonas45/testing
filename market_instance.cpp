#include "eBay.h"
#include <fstream>
#include <sstream>
#include <vector>

ebay::Market market;

static const std::string STATE_FILE = "/tmp/ebay_market.dat";

static std::string esc(const std::string& s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '|') out += "\\p";
        else if (s[i] == '\n') out += "\\n";
        else out += s[i];
    }
    return out;
}

static std::string unesc(const std::string& s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            if (s[i+1] == 'p') { out += '|'; i++; }
            else if (s[i+1] == 'n') { out += '\n'; i++; }
            else out += s[i];
        }
        else out += s[i];
    }
    return out;
}

void saveMarket()
{
    std::ofstream f(STATE_FILE);
    if (!f) return;

    // Save users
    std::unordered_map<std::string, ebay::User>& users = market.getUsers();
    std::unordered_map<std::string, ebay::User>::iterator uit;
    for (uit = users.begin(); uit != users.end(); ++uit)
        f << "USER|" << esc(uit->first) << "\n";

    // Save listings
    std::vector<ebay::Items>& listings = market.getListings();
    for (size_t i = 0; i < listings.size(); i++)
    {
        ebay::Items& item = listings[i];
        f << "ITEM"
          << "|" << esc(item.getItem())
          << "|" << esc(item.getSeller())
          << "|" << item.getPrice_S()
          << "|" << item.getPrice_B()
          << "|" << item.getTime()
          << "|" << item.getHighestBid()
          << "|" << (item.isWon() ? "1" : "0")
          << "|" << esc(item.getWinner())
          << "\n";

        // Save each bid for this item
        std::vector<ebay::Bid> bids = item.getBids();
        for (size_t j = 0; j < bids.size(); j++)
        {
            if (bids[j].user == "__restore__") continue;
            f << "BID"
              << "|" << esc(item.getItem())
              << "|" << esc(bids[j].user)
              << "|" << bids[j].amount
              << "\n";
        }
    }

    // Save selling relationships
    for (uit = users.begin(); uit != users.end(); ++uit)
    {
        if (uit->second.getSelling() != NULL)
            f << "SELLING|" << esc(uit->first) << "|" << esc(uit->second.getSelling()->getItem()) << "\n";
    }

    // Save logs
    std::vector<std::string>& logs = market.getLogs();
    for (size_t i = 0; i < logs.size(); i++)
        f << "LOG|" << esc(logs[i]) << "\n";
}

void loadMarket()
{
    std::ifstream f(STATE_FILE);
    if (!f) return;

    std::string line;
    while (std::getline(f, line))
    {
        if (line.empty()) continue;

        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string tok;
        while (std::getline(ss, tok, '|'))
            parts.push_back(tok);

        if (parts.empty()) continue;

        if (parts[0] == "USER" && parts.size() >= 2)
        {
            ebay::User u;
            u.setUsername(unesc(parts[1]));
            market.addUser(u);
        }
        else if (parts[0] == "ITEM" && parts.size() >= 8)
        {
            ebay::Items item;
            item.setItem(unesc(parts[1]));
            item.setSeller(unesc(parts[2]));
            item.setPrices(std::stod(parts[3]), std::stod(parts[4]));
            item.setTime(std::stoi(parts[5]));
            double hb      = std::stod(parts[6]);
            bool   won     = (parts[7] == "1");
            std::string winner = (parts.size() >= 9) ? unesc(parts[8]) : "";
            // Restore highestBid via internal bid if not won
            if (hb > 0 && !won)
                item.addBid("__restore__", hb);
            if (won && !winner.empty())
                item.forceWin(winner, hb);
            market.addItem(item);
        }
        else if (parts[0] == "BID" && parts.size() >= 4)
        {
            // Restore actual bids onto the item
            ebay::Items* item = market.findItem(unesc(parts[1]));
            if (item && !item->isWon())
                item->addBid(unesc(parts[2]), std::stod(parts[3]));
        }
        else if (parts[0] == "SELLING" && parts.size() >= 3)
        {
            ebay::User*  user = market.getUser(unesc(parts[1]));
            ebay::Items* item = market.findItem(unesc(parts[2]));
            if (user && item) user->startSelling(item);
        }
        else if (parts[0] == "LOG" && parts.size() >= 2)
        {
            market.addLog(unesc(parts[1]));
        }
    }
}
