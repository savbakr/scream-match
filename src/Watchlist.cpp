#include "Watchlist.h"
#include <fstream>
#include <algorithm>
#include <cctype>

std::string Watchlist::toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

bool Watchlist::load(const std::string& filepath) {
    filepath_ = filepath;
    order_.clear();
    lookup_.clear();

    std::ifstream file(filepath);
    if (!file.is_open()) return false; // file may not exist on first run

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        order_.push_back(line);
        lookup_.insert(toLower(line));
    }
    return true;
}

bool Watchlist::save() const {
    std::ofstream file(filepath_);
    if (!file.is_open()) return false;
    for (const auto& t : order_) file << t << "\n";
    return true;
}

bool Watchlist::add(const std::string& title) {
    if (contains(title)) return false;
    order_.push_back(title);
    lookup_.insert(toLower(title));
    return true;
}

bool Watchlist::remove(const std::string& title) {
    if (!contains(title)) return false;
    lookup_.erase(toLower(title));
    order_.erase(std::remove_if(order_.begin(), order_.end(),
                    [&](const std::string& t) { return toLower(t) == toLower(title); }),
                 order_.end());
    return true;
}

bool Watchlist::contains(const std::string& title) const {
    return lookup_.find(toLower(title)) != lookup_.end();
}
