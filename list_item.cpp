#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include "eBay.h"

extern ebay::Market market;
extern void loadMarket();
extern void saveMarket();

// Extracts a value from a simple JSON body without a full parser.
// Handles both string values ("key":"value") and numeric values ("key":123)
std::string parseJson(const std::string& body, const std::string& field)
{
    std::string key = "\"" + field + "\":";
    size_t pos = body.find(key);
    if (pos == std::string::npos) return "";
    pos += key.length();

    while (pos < body.length() && body[pos] == ' ') pos++;

    if (body[pos] == '"')
    {
        pos++;
        size_t end = body.find("\"", pos);
        if (end == std::string::npos) return "";
        return body.substr(pos, end - pos);
    }
    else
    {
        size_t end = body.find_first_of(",}", pos);
        if (end == std::string::npos) end = body.length();
        return body.substr(pos, end - pos);
    }
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

    std::string item_name   = parseJson(body, "item_name");
    std::string username    = parseJson(body, "username");
    std::string sPriceStr   = parseJson(body, "sPrice");
    std::string binPriceStr = parseJson(body, "binPrice");
    std::string expireStr   = parseJson(body, "expiration");

    if (item_name.empty() || username.empty() || sPriceStr.empty())
    {
        std::cout << "{\"success\":false,\"message\":\"Missing required fields.\"}";
        saveMarket();
    return 0;
    }

    double sPrice   = std::stod(sPriceStr);
    double binPrice = binPriceStr.empty() ? 0.0 : std::stod(binPriceStr);
    double expHours = expireStr.empty()   ? 0.5 : std::stod(expireStr);

    ebay::User* user = market.getUser(username);
    if (user == nullptr)
    {
        std::cout << "{\"success\":false,\"message\":\"User not found.\"}";
        saveMarket();
    return 0;
    }

    if (user->getSelling() != nullptr)
    {
        std::cout << "{\"success\":false,\"message\":\"Already have an active listing. One item at a time.\"}";
        saveMarket();
    return 0;
    }

    // Build the item using Items setters
    ebay::Items item;
    item.setItem(item_name);
    item.setSeller(username);
    item.setPrices(sPrice, binPrice);
    item.setTime((int)std::time(nullptr) + (int)(expHours * 3600));

    market.addItem(item);

    // Point user's selling slot at the stored item in active_listings
    ebay::Items* stored = market.findItem(item_name);
    if (stored != nullptr) user->startSelling(stored);

    market.addLog(username + " listed " + item_name);

    std::cout << "{\"success\":true,\"message\":\"Item listed!\"}";
    saveMarket();
    return 0;
}
