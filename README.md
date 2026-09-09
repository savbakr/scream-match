# Scream Match

A horror movie finder and recommendation system, built in C++17 with both a terminal interface and a Qt GUI.

## Features

- **Browse & search** — full movie list with live substring search by title
- **Filters** — subgenre, minimum year, max gore, max intensity, min suspense
- **Sorting** — by title, year, rating, or runtime, ascending or descending
- **Watchlist** — save movies you want to watch, persisted between sessions
- **Ratings** — rate any movie 1-5 stars, saved locally
- **Personalized recommendations** — scores every unrated movie against your taste profile (built from movies you've rated 4+ stars) and explains *why* each one was picked, e.g. "Recommended because you prefer Psychological horror and tend to enjoy lower-gore films"
- **Similarity graph** — find movies similar to any title based on subgenre and intensity/gore/suspense closeness

## Concepts demonstrated

- Classes and object-oriented design (`Movie`, `MovieDatabase`, `Watchlist`, `RatingsManager`, `Recommender`, `SimilarityGraph`)
- `vector` and `unordered_map`/`unordered_set` for storage and fast lookups
- File I/O for loading the movie CSV and persisting ratings/watchlist
- Searching, filtering, and sorting algorithms
- A `priority_queue` (max-heap) for ranking top recommendations
- A weighted graph (adjacency list) connecting similar movies, with BFS-based traversal

## Building & running

### Terminal version
```bash
make run
```

### GUI version (requires Qt)
```bash
brew install qt
cmake -B cmake-build-debug -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build cmake-build-debug --target ScreamMatchGui
./cmake-build-debug/ScreamMatchGui
```
Or open the project in CLion, select the `ScreamMatchGui` run configuration, and click Run.

## Project structure

```
include/    — class headers
src/        — implementations + both entry points (main.cpp, main_gui.cpp)
data/       — movie database (CSV) and app logo
```