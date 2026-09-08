#include "MovieDatabase.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

std::string MovieDatabase::toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

void MovieDatabase::rebuildIndex() {
    titleIndex_.clear();
    for (size_t i = 0; i < movies_.size(); ++i) {
        titleIndex_[toLower(movies_[i].getTitle())] = i;
    }
}

bool MovieDatabase::loadFromCSV(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: could not open " << filepath << "\n";
        return false;
    }

    movies_.clear();
    std::string line;
    bool isHeader = true;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (isHeader) { isHeader = false; continue; } // skip header row

        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> tokens;
        while (std::getline(ss, field, ',')) {
            tokens.push_back(field);
        }
        if (tokens.size() < 7) continue; // skip rows that have data missing

        try {
            std::string title = tokens[0];
            int year = std::stoi(tokens[1]);
            std::string subgenre = tokens[2];
            int runtime = std::stoi(tokens[3]);
            int intensity = std::stoi(tokens[4]);
            int gore = std::stoi(tokens[5]);
            int suspense = std::stoi(tokens[6]);

            movies_.emplace_back(title, year, subgenre, runtime, intensity, gore, suspense);
        } catch (const std::exception& e) {
            std::cerr << "Skipping malformed row: " << line << "\n";
        }
    }

    rebuildIndex();
    return !movies_.empty();
}

Movie* MovieDatabase::findByTitle(const std::string& title) {
    auto it = titleIndex_.find(toLower(title));
    if (it == titleIndex_.end()) return nullptr;
    return &movies_[it->second];
}

std::vector<Movie*> MovieDatabase::searchByTitle(const std::string& query) {
    std::vector<Movie*> results;
    std::string q = toLower(query);
    for (auto& m : movies_) {
        if (toLower(m.getTitle()).find(q) != std::string::npos) {
            results.push_back(&m);
        }
    }
    return results;
}

std::vector<Movie*> MovieDatabase::filter(const FilterCriteria& c) {
    std::vector<Movie*> results;
    for (auto& m : movies_) {
        if (c.subgenre && toLower(m.getSubgenre()) != toLower(*c.subgenre)) continue;
        if (c.minYear && m.getYear() < *c.minYear) continue;
        if (c.maxYear && m.getYear() > *c.maxYear) continue;
        if (c.minRuntime && m.getRuntime() < *c.minRuntime) continue;
        if (c.maxRuntime && m.getRuntime() > *c.maxRuntime) continue;
        if (c.maxIntensity && m.getIntensity() > *c.maxIntensity) continue;
        if (c.maxGore && m.getGore() > *c.maxGore) continue;
        if (c.minSuspense && m.getSuspense() < *c.minSuspense) continue;
        results.push_back(&m);
    }
    return results;
}

void MovieDatabase::sortMovies(std::vector<Movie*>& movies, SortKey key, bool ascending) {
    // keep movies with matching values in the same order
    std::stable_sort(movies.begin(), movies.end(), [&](const Movie* a, const Movie* b) {
        double av, bv;
        switch (key) {
            case SortKey::Year:    av = a->getYear();           bv = b->getYear();           break;
            case SortKey::Rating:  av = a->getAvgUserRating();  bv = b->getAvgUserRating();  break;
            case SortKey::Runtime: av = a->getRuntime();        bv = b->getRuntime();        break;
            case SortKey::Title:   default:
                return ascending ? (a->getTitle() < b->getTitle()) : (a->getTitle() > b->getTitle());
        }
        return ascending ? (av < bv) : (av > bv);
    });
}

std::set<std::string> MovieDatabase::getAllSubgenres() const {
    std::set<std::string> genres;
    for (const auto& m : movies_) genres.insert(m.getSubgenre());
    return genres;
}
