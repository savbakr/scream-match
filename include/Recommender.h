#pragma once
#include "Movie.h"
#include "MovieDatabase.h"
#include <string>
#include <vector>
#include <unordered_map>

// stores the user's horror movie preferences
struct UserProfile {
    std::unordered_map<std::string, int> subgenreCounts; // number liked in each subgenre
    double avgIntensity = 5.0;
    double avgGore = 5.0;
    double avgSuspense = 5.0;
    int likedCount = 0;

    std::string favoriteSubgenre() const;
    bool isEmpty() const { return likedCount == 0; }
};

struct Recommendation {
    Movie* movie;
    double score;          // 0-100 match score
    std::string explanation;
};

class Recommender {
public:
    // build preferences from movies rated 4+ stars
    static UserProfile buildProfile(MovieDatabase& db);

    // calculate how well a movie matches the user
    static double matchScore(const Movie& m, const UserProfile& profile);

    // explain why a movie was recommended
    static std::string explain(const Movie& m, const UserProfile& profile);

    // find recommendations with the highest scores
    static std::vector<Recommendation> topRecommendations(
        MovieDatabase& db, const UserProfile& profile, int topN,
        const std::vector<std::string>& excludeTitles = {});
};
