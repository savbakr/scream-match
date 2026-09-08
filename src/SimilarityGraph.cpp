#include "SimilarityGraph.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>

std::string SimilarityGraph::toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

double SimilarityGraph::similarity(const Movie& a, const Movie& b) {
    double subgenreBonus = (a.getSubgenre() == b.getSubgenre()) ? 0.4 : 0.0;

    auto closeness = [](int x, int y) {
        double dist = std::fabs(static_cast<double>(x - y));
        return std::max(0.0, (9.0 - dist) / 9.0); // convert the difference to a similarity score
    };

    double attrAvg = (closeness(a.getIntensity(), b.getIntensity())
                     + closeness(a.getGore(), b.getGore())
                     + closeness(a.getSuspense(), b.getSuspense())) / 3.0;

    return subgenreBonus + 0.6 * attrAvg; // return a score [0,1]
}

void SimilarityGraph::build(MovieDatabase& db, double edgeThreshold) {
    adjacency_.clear();
    canonicalTitle_.clear();

    const auto& movies = db.getAllMovies();
    for (const auto& m : movies) {
        canonicalTitle_[toLower(m.getTitle())] = m.getTitle();
        adjacency_[toLower(m.getTitle())]; // add the movie even if it has no matches
    }

    // compare each movie with every other movie
    for (size_t i = 0; i < movies.size(); ++i) {
        for (size_t j = i + 1; j < movies.size(); ++j) {
            double sim = similarity(movies[i], movies[j]);
            if (sim >= edgeThreshold) {
                std::string ti = toLower(movies[i].getTitle());
                std::string tj = toLower(movies[j].getTitle());
                adjacency_[ti].emplace_back(tj, sim);
                adjacency_[tj].emplace_back(ti, sim);
            }
        }
    }

    // sort matches from most to least similar
    for (auto& [title, neighbors] : adjacency_) {
        std::sort(neighbors.begin(), neighbors.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
    }
}

std::vector<std::pair<std::string, double>> SimilarityGraph::getNeighbors(
        const std::string& title, int topK) const {
    std::vector<std::pair<std::string, double>> results;
    auto it = adjacency_.find(toLower(title));
    if (it == adjacency_.end()) return results;

    for (const auto& [neighborLower, weight] : it->second) {
        auto nameIt = canonicalTitle_.find(neighborLower);
        results.emplace_back(nameIt != canonicalTitle_.end() ? nameIt->second : neighborLower, weight);
        if (static_cast<int>(results.size()) >= topK) break;
    }
    return results;
}

std::vector<std::pair<std::string, double>> SimilarityGraph::recommendFromSeeds(
        const std::vector<std::string>& seedTitles, int topK, int maxHops) const {

    std::unordered_set<std::string> seeds;
    for (const auto& t : seedTitles) seeds.insert(toLower(t));

    std::unordered_map<std::string, double> accumulated;

    // search for similar movies starting from each selected movie
    for (const auto& seed : seeds) {
        if (adjacency_.find(seed) == adjacency_.end()) continue;

        std::unordered_map<std::string, int> hopsSeen;
        std::queue<std::string> q;
        q.push(seed);
        hopsSeen[seed] = 0;

        while (!q.empty()) {
            std::string current = q.front();
            q.pop();
            int hop = hopsSeen[current];
            if (hop >= maxHops) continue;

            auto it = adjacency_.find(current);
            if (it == adjacency_.end()) continue;

            for (const auto& [neighbor, weight] : it->second) {
                if (hopsSeen.count(neighbor)) continue; // skip movies already checked
                hopsSeen[neighbor] = hop + 1;
                q.push(neighbor);

                if (!seeds.count(neighbor)) {
                    double decay = 1.0 / (hop + 1); // closer movies get higher score
                    accumulated[neighbor] += weight * decay;
                }
            }
        }
    }

    std::vector<std::pair<std::string, double>> ranked(accumulated.begin(), accumulated.end());
    std::sort(ranked.begin(), ranked.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (static_cast<int>(ranked.size()) > topK) ranked.resize(topK);

    // restore original title formatting
    for (auto& [title, score] : ranked) {
        auto it = canonicalTitle_.find(title);
        if (it != canonicalTitle_.end()) title = it->second;
    }
    return ranked;
}
