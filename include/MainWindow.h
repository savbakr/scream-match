#pragma once
#include <QMainWindow>
#include <QCloseEvent>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <vector>

#include "MovieDatabase.h"
#include "Watchlist.h"
#include "RatingsManager.h"
#include "Recommender.h"
#include "SimilarityGraph.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onSearchTextChanged(const QString& text);
    void onApplyFilters();
    void onClearFilters();
    void onSortChanged();
    void onBrowseRowSelected();
    void onToggleWatchlist();
    void onSubmitRating();
    void onFindSimilar();

    void onWatchlistRowSelected();
    void onRemoveFromWatchlist();

    void onGenerateRecommendations();
    void onRecommendationRowSelected();

private:
    MovieDatabase db_;
    Watchlist watchlist_;
    RatingsManager ratings_;
    SimilarityGraph graph_;

    Movie* selectedMovie_ = nullptr;
    std::string selectedWatchlistTitle_;

    std::vector<Movie*> currentBrowseList_;
    std::vector<Recommendation> currentRecommendations_;

    QLineEdit* searchBox_;
    QComboBox* genreFilter_;
    QSpinBox* minYearSpin_;
    QSpinBox* maxGoreSpin_;
    QSpinBox* maxIntensitySpin_;
    QSpinBox* minSuspenseSpin_;
    QComboBox* sortKeyBox_;
    QComboBox* sortDirBox_;
    QTableWidget* browseTable_;

    QLabel* detailsLabel_;
    QPushButton* watchlistButton_;
    QDoubleSpinBox* ratingSpin_;
    QPushButton* rateButton_;
    QPushButton* similarButton_;
    QTextEdit* similarResultsBox_;

    QTableWidget* watchlistTable_;
    QPushButton* removeWatchlistButton_;

    QSpinBox* numRecsSpin_;
    QPushButton* generateRecsButton_;
    QTableWidget* recsTable_;
    QTextEdit* recExplanationBox_;

    QWidget* buildBrowseTab();
    QWidget* buildWatchlistTab();
    QWidget* buildRecommendationsTab();

    void populateGenreFilter();
    void refreshBrowseTable();
    void refreshWatchlistTable();
    void showDetails(Movie* m);
    void applyStyleSheet();
};