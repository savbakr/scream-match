#pragma once
#include "Movie.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <set>

// stores and searches the movie collection
class MovieDatabase {
public:
    // load movie information from a CSV file
    bool loadFromCSV(const std::string& filepath);

    const std::vector<Movie>& getAllMovies() const { return movies_; }
    std::vector<Movie>& getAllMoviesMutable() { return movies_; }

    // find a movie using its full title
    Movie* findByTitle(const std::string& title);

    // find movies containing part of a title
    std::vector<Movie*> searchByTitle(const std::string& query);

    struct FilterCriteria {
        std::optional<std::string> subgenre;
        std::optional<int> minYear;
        std::optional<int> maxYear;
        std::optional<int> minRuntime;
        std::optional<int> maxRuntime;
        std::optional<int> maxIntensity;
        std::optional<int> maxGore;
        std::optional<int> minSuspense;
    };
    std::vector<Movie*> filter(const FilterCriteria& criteria);

    enum class SortKey { Year, Rating, Title, Runtime };
    static void sortMovies(std::vector<Movie*>& movies, SortKey key, bool ascending = true);

    std::set<std::string> getAllSubgenres() const;

    size_t size() const { return movies_.size(); }

private:
    std::vector<Movie> movies_;
    std::unordered_map<std::string, size_t> titleIndex_; // connects lowercase titles to their positions

    static std::string toLower(const std::string& s);
    void rebuildIndex();
};
