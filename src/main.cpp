#include "Movie.h"
#include "MovieDatabase.h"
#include "Watchlist.h"
#include "RatingsManager.h"
#include "Recommender.h"
#include "SimilarityGraph.h"

#include <iostream>
#include <iomanip>
#include <limits>
#include <string>

namespace {

const std::string MOVIES_CSV = "data/movies.csv";
const std::string RATINGS_FILE = "data/ratings.csv";
const std::string WATCHLIST_FILE = "data/watchlist.txt";

void printHeader(const std::string& title) {
    std::cout << "\n==================== " << title << " ====================\n";
}

void printMenu() {
    printHeader("SCREAM MATCH");
    std::cout <<
        "1. Browse movies\n"
        "2. Search by title\n"
        "3. Filter movies by preferences\n"
        "4. Get recommendations\n"
        "5. View watchlist\n"
        "6. Add / remove a movie from watchlist\n"
        "7. Rate a movie\n"
        "8. Find similar movies (graph)\n"
        "9. Exit\n"
        "Choose an option: ";
}

int readInt(const std::string& prompt) {
    std::cout << prompt;
    int value;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a number: ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

double readDouble(const std::string& prompt) {
    std::cout << prompt;
    double value;
    while (!(std::cin >> value)) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a number: ";
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void printMovieList(const std::vector<Movie*>& movies) {
    if (movies.empty()) {
        std::cout << "(no movies to show)\n";
        return;
    }
    std::cout << std::left << std::setw(32) << "TITLE" << std::setw(6) << "YEAR"
              << std::setw(15) << "SUBGENRE" << std::setw(6) << "TIME"
              << "  I/G/S  RATING\n";
    std::cout << std::string(90, '-') << "\n";
    for (auto* m : movies) {
        std::cout << std::left << std::setw(32) << m->getTitle()
                   << std::setw(6) << m->getYear()
                   << std::setw(15) << m->getSubgenre()
                   << std::setw(6) << (std::to_string(m->getRuntime()) + "m")
                   << "  " << m->getIntensity() << "/" << m->getGore() << "/" << m->getSuspense();
        if (m->hasUserRating()) {
            std::cout << "   " << std::fixed << std::setprecision(1) << m->getUserRating() << "*";
        }
        std::cout << "\n";
    }
}

MovieDatabase::SortKey chooseSortKey() {
    std::cout << "Sort by: 1) Year  2) Rating  3) Title  4) Runtime\n";
    int choice = readInt("Choice: ");
    switch (choice) {
        case 1: return MovieDatabase::SortKey::Year;
        case 2: return MovieDatabase::SortKey::Rating;
        case 4: return MovieDatabase::SortKey::Runtime;
        default: return MovieDatabase::SortKey::Title;
    }
}

void browseMovies(MovieDatabase& db) {
    std::vector<Movie*> all;
    for (auto& m : db.getAllMoviesMutable()) all.push_back(&m);

    auto key = chooseSortKey();
    std::string dir = readLine("Ascending or descending? (a/d): ");
    bool asc = (dir.empty() || dir[0] == 'a' || dir[0] == 'A');

    MovieDatabase::sortMovies(all, key, asc);
    printMovieList(all);
}

void searchMovies(MovieDatabase& db) {
    std::string query = readLine("Enter title (or part of it): ");
    auto results = db.searchByTitle(query);
    std::cout << "\nFound " << results.size() << " match(es):\n";
    printMovieList(results);
}

void filterMovies(MovieDatabase& db) {
    MovieDatabase::FilterCriteria criteria;

    std::cout << "\nAll subgenres: ";
    for (const auto& g : db.getAllSubgenres()) std::cout << g << " | ";
    std::cout << "\n";

    std::string genre = readLine("Subgenre (leave blank for any): ");
    if (!genre.empty()) criteria.subgenre = genre;

    std::string yearRange = readLine("Minimum year (leave blank for any): ");
    if (!yearRange.empty()) criteria.minYear = std::stoi(yearRange);

    std::string maxGoreStr = readLine("Maximum gore level 1-10 (leave blank for any): ");
    if (!maxGoreStr.empty()) criteria.maxGore = std::stoi(maxGoreStr);

    std::string maxIntensityStr = readLine("Maximum intensity 1-10 (leave blank for any): ");
    if (!maxIntensityStr.empty()) criteria.maxIntensity = std::stoi(maxIntensityStr);

    std::string minSuspenseStr = readLine("Minimum suspense 1-10 (leave blank for any): ");
    if (!minSuspenseStr.empty()) criteria.minSuspense = std::stoi(minSuspenseStr);

    auto results = db.filter(criteria);
    std::cout << "\n" << results.size() << " movie(s) match your filters:\n";

    auto key = chooseSortKey();
    MovieDatabase::sortMovies(results, key, true);
    printMovieList(results);
}

void getRecommendations(MovieDatabase& db) {
    UserProfile profile = Recommender::buildProfile(db);
    if (profile.isEmpty()) {
        std::cout << "\nYou haven't rated any movies 4 stars or higher yet.\n"
                     "Rate a few favorites first, then come back for personalized picks!\n";
        return;
    }

    std::cout << "\nYour taste profile: favors " << profile.favoriteSubgenre()
              << " horror | avg intensity " << std::fixed << std::setprecision(1) << profile.avgIntensity
              << " | avg gore " << profile.avgGore
              << " | avg suspense " << profile.avgSuspense << "\n";

    int n = readInt("How many recommendations? ");
    auto recs = Recommender::topRecommendations(db, profile, n);

    printHeader("TOP RECOMMENDATIONS");
    for (const auto& r : recs) {
        std::cout << "\n" << r.movie->getTitle() << " (" << r.movie->getYear() << ") - "
                  << r.movie->getSubgenre() << " | Match: " << std::fixed << std::setprecision(0)
                  << r.score << "%\n  " << r.explanation << "\n";
    }
}

void viewWatchlist(MovieDatabase& db, Watchlist& watchlist) {
    printHeader("WATCHLIST");
    if (watchlist.getTitles().empty()) {
        std::cout << "Your watchlist is empty.\n";
        return;
    }
    std::vector<Movie*> movies;
    for (const auto& title : watchlist.getTitles()) {
        if (Movie* m = db.findByTitle(title)) movies.push_back(m);
    }
    printMovieList(movies);
}

void manageWatchlist(MovieDatabase& db, Watchlist& watchlist) {
    std::string title = readLine("Enter movie title: ");
    Movie* m = db.findByTitle(title);
    if (!m) {
        std::cout << "Movie not found. Try Search first to check the exact title.\n";
        return;
    }

    if (watchlist.contains(m->getTitle())) {
        watchlist.remove(m->getTitle());
        std::cout << "Removed \"" << m->getTitle() << "\" from your watchlist.\n";
    } else {
        watchlist.add(m->getTitle());
        std::cout << "Added \"" << m->getTitle() << "\" to your watchlist.\n";
    }
    watchlist.save();
}

void rateMovie(MovieDatabase& db, RatingsManager& ratings) {
    std::string title = readLine("Enter movie title: ");
    Movie* m = db.findByTitle(title);
    if (!m) {
        std::cout << "Movie not found. Try Search first to check the exact title.\n";
        return;
    }

    double rating = readDouble("Your rating for \"" + m->getTitle() + "\" (1.0-5.0): ");
    if (rating < 1.0 || rating > 5.0) {
        std::cout << "Rating must be between 1 and 5.\n";
        return;
    }

    ratings.rate(db, m->getTitle(), rating);
    ratings.save();
    std::cout << "Saved your rating of " << rating << " for \"" << m->getTitle() << "\".\n";
}

void findSimilarMovies(MovieDatabase& db, const SimilarityGraph& graph) {
    std::string title = readLine("Enter a movie title to find similar movies: ");
    Movie* m = db.findByTitle(title);
    if (!m) {
        std::cout << "Movie not found. Try Search first to check the exact title.\n";
        return;
    }

    auto neighbors = graph.getNeighbors(m->getTitle(), 8);
    printHeader("MOVIES SIMILAR TO \"" + m->getTitle() + "\"");
    if (neighbors.empty()) {
        std::cout << "No strongly similar movies found in the graph (try lowering the edge threshold).\n";
        return;
    }
    for (const auto& [neighborTitle, weight] : neighbors) {
        Movie* nm = db.findByTitle(neighborTitle);
        std::cout << neighborTitle;
        if (nm) std::cout << " (" << nm->getYear() << ", " << nm->getSubgenre() << ")";
        std::cout << " - similarity " << std::fixed << std::setprecision(0) << (weight * 100) << "%\n";
    }
}

} // namespace

int main() {
    MovieDatabase db;
    if (!db.loadFromCSV(MOVIES_CSV)) {
        std::cerr << "Failed to load movie database from " << MOVIES_CSV << ". Exiting.\n";
        return 1;
    }

    RatingsManager ratings;
    ratings.load(RATINGS_FILE, db);

    Watchlist watchlist;
    watchlist.load(WATCHLIST_FILE);

    SimilarityGraph graph;
    graph.build(db, 0.6);

    std::cout << "Loaded " << db.size() << " movies into the Scream Match.\n";

    bool running = true;
    while (running) {
        printMenu();
        int choice = readInt("");
        switch (choice) {
            case 1: browseMovies(db); break;
            case 2: searchMovies(db); break;
            case 3: filterMovies(db); break;
            case 4: getRecommendations(db); break;
            case 5: viewWatchlist(db, watchlist); break;
            case 6: manageWatchlist(db, watchlist); break;
            case 7: rateMovie(db, ratings); break;
            case 8: findSimilarMovies(db, graph); break;
            case 9:
                std::cout << "Stay safe out there. Goodbye!\n";
                running = false;
                break;
            default:
                std::cout << "Invalid option, try again.\n";
        }
    }

    return 0;
}
