CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
SRC_DIR := src
OBJ_DIR := obj
BIN := Scream_Match

SOURCES := $(SRC_DIR)/main.cpp \
           $(SRC_DIR)/MovieDatabase.cpp \
           $(SRC_DIR)/Watchlist.cpp \
           $(SRC_DIR)/RatingsManager.cpp \
           $(SRC_DIR)/Recommender.cpp \
           $(SRC_DIR)/SimilarityGraph.cpp
# Note: src/MainWindow.cpp and src/main_gui.cpp are the Qt GUI and are built
# separately via CMake/CLion (see CMakeLists.txt), not by this Makefile.
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: all
	./$(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN)