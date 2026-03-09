#include <iostream>
#include <ctime>
#include "eBay.h"

extern ebay::Market market;
extern void loadMarket();
extern void saveMarket();

int main()
{
    loadMarket();
    std::cout << "Content-type: application/json\n";
    std::cout << "Access-Control-Allow-Origin: *\n\n";

    std::vector<ebay::Items>& listings = market.getListings();
    int now = (int)std::time(NULL);

    // Check for expired auctions and assign winner to highest bidder
    for (size_t i = 0; i < listings.size(); i++)
    {
        ebay::Items& item = listings[i];
        if (item.isWon() || item.getTime() > now) continue;

        // Find highest bidder from bid list
        std::vector<ebay::Bid> bids = item.getBids();
        std::string topBidder = "";
        double topAmount = item.getPrice_S();
        for (size_t j = 0; j < bids.size(); j++)
        {
            if (bids[j].user != "__restore__" && bids[j].amount >= topAmount)
            {
                topAmount = bids[j].amount;
                topBidder = bids[j].user;
            }
        }

        // Force win by calling addBid with a very high amount to trigger BIN path
        if (!topBidder.empty())
        {
            item.forceWin(topBidder, topAmount);
            ebay::User* seller = market.getUser(item.getSeller());
            if (seller) seller->clearSelling();
            market.addLog(topBidder + " won " + item.getItem());
        }
    }

    std::cout << "[";
    bool first = true;
    for (size_t i = 0; i < listings.size(); i++)
    {
        ebay::Items& item = listings[i];
        if (!first) std::cout << ",";
        first = false;

        std::cout << "{"
            << "\"item_name\":\""      << item.getItem()      << "\","
            << "\"sellerUsername\":\"" << item.getSeller()    << "\","
            << "\"sPrice\":"           << item.getPrice_S()   << ","
            << "\"binPrice\":"         << item.getPrice_B()   << ","
            << "\"highestBid\":"       << item.getHighestBid()<< ","
            << "\"expiration\":"       << item.getTime()      << ","
            << "\"win\":"              << (item.isWon() ? "true" : "false") << ","
            << "\"winner\":\""         << item.getWinner()    << "\","
            << "\"bids\":[";

        std::vector<ebay::Bid> bids = item.getBids();
        bool firstBid = true;
        for (size_t j = 0; j < bids.size(); j++)
        {
            if (bids[j].user == "__restore__") continue;
            if (!firstBid) std::cout << ",";
            firstBid = false;
            std::cout << "{\"user\":\"" << bids[j].user << "\",\"amount\":" << bids[j].amount << "}";
        }
        std::cout << "]}";
    }
    std::cout << "]";

    saveMarket();
    return 0;
}
