#pragma once
#include "Movie.h"
#include "MovieDatabase.h"
#include <string>
#include <vector>
#include <unordered_map>

// connects movies based on how similar they are
class SimilarityGraph {
public:
    // build connections between similar movies
    void build(MovieDatabase& db, double edgeThreshold = 0.6);

    // find movies most similar to a title
    std::vector<std::pair<std::string, double>> getNeighbors(const std::string& title, int topK = 10) const;

    // find recommendations starting from selected movies
    std::vector<std::pair<std::string, double>> recommendFromSeeds(
        const std::vector<std::string>& seedTitles, int topK = 10, int maxHops = 2) const;

    static double similarity(const Movie& a, const Movie& b);

private:
    // stores each movie's similar movies and their scores
    std::unordered_map<std::string, std::vector<std::pair<std::string, double>>> adjacency_;
    // stores the original title formatting
    std::unordered_map<std::string, std::string> canonicalTitle_;

    static std::string toLower(const std::string& s);
};
