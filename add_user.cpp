#include <iostream>
#include <string>
#include <cstdlib>
#include "eBay.h"

extern ebay::Market market;
extern void loadMarket();
extern void saveMarket();

// Extracts a field value from a URL-encoded POST body
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

    std::string username = parseField(body, "username");

    if (username.empty())
    {
        std::cout << "{\"success\":false,\"message\":\"Username is required.\"}";
        saveMarket();
    return 0;
    }

    // Return success silently if user already exists
    if (market.getUser(username) != nullptr)
    {
        std::cout << "{\"success\":true,\"message\":\"User already exists.\"}";
        saveMarket();
    return 0;
    }

    ebay::User user;
    user.setUsername(username);
    market.addUser(user);
    market.addLog("User " + username + " registered.");

    std::cout << "{\"success\":true,\"message\":\"User added.\"}";
    saveMarket();
    return 0;
}
