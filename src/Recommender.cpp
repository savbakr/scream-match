#include "Recommender.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>
#include <sstream>

std::string UserProfile::favoriteSubgenre() const {
    std::string best;
    int bestCount = -1;
    for (const auto& [genre, count] : subgenreCounts) {
        if (count > bestCount) {
            bestCount = count;
            best = genre;
        }
    }
    return best;
}

UserProfile Recommender::buildProfile(MovieDatabase& db) {
    UserProfile profile;
    double intensitySum = 0, goreSum = 0, suspenseSum = 0;

    for (const auto& m : db.getAllMovies()) {
        // a movie is liked if the user rates it 4+ stars
        if (m.hasUserRating() && m.getUserRating() >= 4.0) {
            profile.subgenreCounts[m.getSubgenre()]++;
            intensitySum += m.getIntensity();
            goreSum += m.getGore();
            suspenseSum += m.getSuspense();
            profile.likedCount++;
        }
    }

    if (profile.likedCount > 0) {
        profile.avgIntensity = intensitySum / profile.likedCount;
        profile.avgGore = goreSum / profile.likedCount;
        profile.avgSuspense = suspenseSum / profile.likedCount;
    }
    return profile;
}

double Recommender::matchScore(const Movie& m, const UserProfile& profile) {
    if (profile.isEmpty()) return 50.0; // default values before the user rates a movie

    // give points (up to 40) for the user's favorite subgenre
    double subgenreScore = 0.0;
    auto it = profile.subgenreCounts.find(m.getSubgenre());
    if (it != profile.subgenreCounts.end()) {
        double share = static_cast<double>(it->second) / profile.likedCount;
        subgenreScore = 40.0 * share;
        subgenreScore = std::min(subgenreScore, 40.0);
        // give liked subgenres at least 20 points
        subgenreScore = std::max(subgenreScore, 20.0);
    }

    // add points based on similar intensity, gore, and suspense levels
    auto closeness = [](double a, double b) {
        double dist = std::fabs(a - b);
        return std::max(0.0, (9.0 - dist) / 9.0) * 20.0;
    };

    double score = subgenreScore
                 + closeness(m.getIntensity(), profile.avgIntensity)
                 + closeness(m.getGore(), profile.avgGore)
                 + closeness(m.getSuspense(), profile.avgSuspense);

    return std::min(100.0, score);
}

std::string Recommender::explain(const Movie& m, const UserProfile& profile) {
    if (profile.isEmpty()) {
        return "General pick - rate a few movies to get personalized recommendations.";
    }

    std::ostringstream reason;
    reason << "Recommended because you";

    std::vector<std::string> clauses;

    auto it = profile.subgenreCounts.find(m.getSubgenre());
    if (it != profile.subgenreCounts.end()) {
        std::ostringstream c;
        c << " prefer " << m.getSubgenre() << " horror";
        clauses.push_back(c.str());
    }

    if (m.getGore() <= profile.avgGore - 2) clauses.push_back(" tend to enjoy lower-gore films");
    else if (m.getGore() >= profile.avgGore + 2) clauses.push_back(" have shown a taste for graphic gore");

    if (m.getSuspense() >= profile.avgSuspense + 1) clauses.push_back(" gravitate toward slow-burn suspense");

    if (m.getIntensity() <= profile.avgIntensity - 2) clauses.push_back(" seem to prefer gentler intensity levels");
    else if (m.getIntensity() >= profile.avgIntensity + 2) clauses.push_back(" seek out high-intensity scares");

    if (clauses.empty()) {
        reason << " have rated similar movies highly.";
        return reason.str();
    }

    for (size_t i = 0; i < clauses.size(); ++i) {
        if (i > 0 && i == clauses.size() - 1) reason << ", and";
        else if (i > 0) reason << ",";
        reason << clauses[i];
    }
    reason << ".";
    return reason.str();
}

std::vector<Recommendation> Recommender::topRecommendations(
        MovieDatabase& db, const UserProfile& profile, int topN,
        const std::vector<std::string>& excludeTitles) {

    std::unordered_set<std::string> exclude;
    for (const auto& t : excludeTitles) exclude.insert(t);

    // sort recommendations by their match score (highest first)
    auto cmp = [](const Recommendation& a, const Recommendation& b) {
        return a.score < b.score;
    };
    std::priority_queue<Recommendation, std::vector<Recommendation>, decltype(cmp)> heap(cmp);

    for (auto& m : db.getAllMoviesMutable()) {
        if (m.hasUserRating()) continue;               // skip what's already rated
        if (exclude.count(m.getTitle())) continue;

        double score = matchScore(m, profile);
        heap.push({&m, score, explain(m, profile)});
    }

    std::vector<Recommendation> results;
    while (!heap.empty() && static_cast<int>(results.size()) < topN) {
        results.push_back(heap.top());
        heap.pop();
    }
    return results;
}
