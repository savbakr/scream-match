#pragma once
#include "MovieDatabase.h"
#include <string>
#include <unordered_map>

// saves and loads the user's movie ratings
class RatingsManager {
public:
    bool load(const std::string& filepath, MovieDatabase& db);
    bool save() const;

    bool rate(MovieDatabase& db, const std::string& title, double rating);

    const std::unordered_map<std::string, double>& getRatings() const { return ratings_; }
    bool hasRated(const std::string& title) const;

private:
    std::string filepath_;
    std::unordered_map<std::string, double> ratings_; // stores ratings by lowercase title

    static std::string toLower(const std::string& s);
};
