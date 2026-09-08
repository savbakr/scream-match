#pragma once
#include <string>
#include <vector>
#include <unordered_set>

// stores the user's watchlist
class Watchlist {
public:
    bool load(const std::string& filepath);
    bool save() const;

    bool add(const std::string& title);
    bool remove(const std::string& title);
    bool contains(const std::string& title) const;

    const std::vector<std::string>& getTitles() const { return order_; }
    size_t size() const { return order_.size(); }

private:
    std::string filepath_;
    std::vector<std::string> order_;          // keeps movies in the order added
    std::unordered_set<std::string> lookup_;  // checks if a movie is already added

    static std::string toLower(const std::string& s);
};
