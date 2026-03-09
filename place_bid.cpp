#include <iostream>
#include <string>
#include <cstdlib>
#include "eBay.h"

extern ebay::Market market;
extern void loadMarket();
extern void saveMarket();

// Extracts a field value from a URL-encoded POST body (e.g. "key=value&key2=value2")
std::string parseField(const std::string& body, const std::string& field)
{
    std::string key = field + "=";
    size_t pos = body.find(key);
    if (pos == std::string::npos) return "";
    pos += key.length();
    size_t end = body.find("&", pos);
    if (end == std::string::npos) end = body.length();
    return body.substr(pos, end - pos);
}

int main()
{
    loadMarket();
    std::cout << "Content-type: application/json\n";
    std::cout << "Access-Control-Allow-Origin: *\n\n";

    // Read POST body from stdin
    std::string body = "";
    char* lenStr = getenv("CONTENT_LENGTH");
    if (lenStr != nullptr)
    {
        int len = std::stoi(lenStr);
        body.resize(len);
        std::cin.read(&body[0], len);
    }

    std::string item_name = parseField(body, "item_name");
    std::string userName  = parseField(body, "userName");
    std::string amountStr = parseField(body, "bidAmount");

    if (item_name.empty() || userName.empty() || amountStr.empty())
    {
        std::cout << "{\"success\":false,\"message\":\"Missing required fields.\"}";
        saveMarket();
    return 0;
    }

    double bidAmount = std::stod(amountStr);

    ebay::User* user = market.getUser(userName);
    if (user == nullptr)
    {
        std::cout << "{\"success\":false,\"message\":\"User not found.\"}";
        saveMarket();
    return 0;
    }

    ebay::Items* item = market.findItem(item_name);
    if (item == nullptr)
    {
        std::cout << "{\"success\":false,\"message\":\"Item not found.\"}";
        saveMarket();
    return 0;
    }

    if (item->getSeller() == userName)
    {
        std::cout << "{\"success\":false,\"message\":\"You cannot bid on your own listing.\"}";
        saveMarket();
    return 0;
    }

    if (item->isWon())
    {
        std::cout << "{\"success\":false,\"message\":\"This item has already been won.\"}";
        saveMarket();
    return 0;
    }

    double prevHigh = item->getHighestBid();

    // Calls Items::addBid — handles both regular bids and Buy It Now
    item->addBid(userName, bidAmount);

    if (item->getHighestBid() > prevHigh || item->isWon())
    {
        user->addHistory(*item);
        market.addLog(userName + " bid on " + item_name);

        if (item->isWon())
        {
            // Free the seller's listing slot
            ebay::User* seller = market.getUser(item->getSeller());
            if (seller != nullptr) seller->clearSelling();

            std::cout << "{\"success\":true,\"message\":\"Bought for $"
                      << item->getHighestBid() << "!\",\"won\":true}";
        }
        else
        {
            std::cout << "{\"success\":true,\"message\":\"Bid placed!\",\"won\":false}";
        }
    }
    else
    {
        std::cout << "{\"success\":false,\"message\":\"Bid must exceed current highest bid of $"
                  << prevHigh << ".\"}";
    }

    saveMarket();
    return 0;
}
