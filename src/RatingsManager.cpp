#include "RatingsManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

std::string RatingsManager::toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

bool RatingsManager::load(const std::string& filepath, MovieDatabase& db) {
    filepath_ = filepath;
    ratings_.clear();

    std::ifstream file(filepath);
    if (!file.is_open()) return false; // file may not exist

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto commaPos = line.rfind(',');
        if (commaPos == std::string::npos) continue;

        std::string title = line.substr(0, commaPos);
        double rating;
        try {
            rating = std::stod(line.substr(commaPos + 1));
        } catch (...) {
            continue;
        }

        ratings_[toLower(title)] = rating;
        if (Movie* m = db.findByTitle(title)) {
            m->setUserRating(rating);
        }
    }
    return true;
}

bool RatingsManager::save() const {
    std::ofstream file(filepath_);
    if (!file.is_open()) return false;
    for (const auto& [title, rating] : ratings_) {
        file << title << "," << rating << "\n";
    }
    return true;
}

bool RatingsManager::rate(MovieDatabase& db, const std::string& title, double rating) {
    Movie* m = db.findByTitle(title);
    if (!m) return false;

    m->setUserRating(rating);
    ratings_[toLower(m->getTitle())] = rating; // use lowercase titles for lookup
    return true;
}

bool RatingsManager::hasRated(const std::string& title) const {
    return ratings_.find(toLower(title)) != ratings_.end();
}
