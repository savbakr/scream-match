#pragma once
#include <string>
#include <sstream>
#include <iomanip>

// stores a horror movie's information and ratings
class Movie {
public:
    Movie() = default;

    Movie(std::string title, int year, std::string subgenre, int runtime,
          int intensity, int gore, int suspense)
        : title_(std::move(title)), year_(year), subgenre_(std::move(subgenre)),
          runtime_(runtime), intensity_(intensity), gore_(gore), suspense_(suspense) {}

    // Getters
    const std::string& getTitle() const { return title_; }
    int getYear() const { return year_; }
    const std::string& getSubgenre() const { return subgenre_; }
    int getRuntime() const { return runtime_; }
    int getIntensity() const { return intensity_; }
    int getGore() const { return gore_; }
    int getSuspense() const { return suspense_; }

    double getAvgUserRating() const {
        return ratingCount_ == 0 ? 0.0 : ratingSum_ / ratingCount_;
    }
    int getRatingCount() const { return ratingCount_; }

    // user rating functions
    double getUserRating() const { return userRating_; }
    bool hasUserRating() const { return userRating_ > 0.0; }
    void setUserRating(double r) {
        // replace previous rating
        if (userRating_ > 0.0) {
            ratingSum_ -= userRating_;
            ratingCount_--;
        }
        userRating_ = r;
        ratingSum_ += r;
        ratingCount_++;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << std::left << std::setw(32) << title_
            << std::setw(6) << year_
            << std::setw(15) << subgenre_
            << std::setw(6) << (std::to_string(runtime_) + "m")
            << " I:" << intensity_ << " G:" << gore_ << " S:" << suspense_;
        if (hasUserRating()) {
            oss << "  YourRating:" << std::fixed << std::setprecision(1) << userRating_;
        }
        return oss.str();
    }

private:
    std::string title_;
    int year_ = 0;
    std::string subgenre_;
    int runtime_ = 0;      // minutes
    int intensity_ = 0;    // 1-10
    int gore_ = 0;         // 1-10
    int suspense_ = 0;     // 1-10

    double ratingSum_ = 0.0;
    int ratingCount_ = 0;
    double userRating_ = 0.0; // 0 = unrated
};
