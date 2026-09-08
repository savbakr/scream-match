#include "MainWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <sstream>
#include <QPixmap>

namespace {
const char* MOVIES_CSV = "data/movies.csv";
const char* RATINGS_FILE = "data/ratings.csv";
const char* WATCHLIST_FILE = "data/watchlist.txt";
constexpr int NO_MIN_YEAR = 1900;
constexpr int NO_MAX_LEVEL = 10;
constexpr int NO_MIN_LEVEL = 0;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    if (!db_.loadFromCSV(MOVIES_CSV)) {
        QMessageBox::critical(this, "Scream Match",
            "Could not load data/movies.csv.\nMake sure the app runs with the project's data/ folder next to it.");
    }
    ratings_.load(RATINGS_FILE, db_);
    watchlist_.load(WATCHLIST_FILE);
    graph_.build(db_, 0.6);

    setWindowTitle("Scream Match");
    resize(1100, 700);

    auto* central = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);

    QPixmap logoPixmap("data/logo.png");
    if (!logoPixmap.isNull()) {
        auto* logoLabel = new QLabel();
        logoLabel->setFixedHeight(160);
        logoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        logoLabel->setAlignment(Qt::AlignCenter);
        logoLabel->setScaledContents(false);

        QPixmap scaled = logoPixmap.scaledToHeight(160, Qt::SmoothTransformation);
        logoLabel->setPixmap(scaled);
        centralLayout->addWidget(logoLabel);
    }

    auto* tabs = new QTabWidget(this);
    tabs->addTab(buildBrowseTab(), "Browse && Search");
    tabs->addTab(buildWatchlistTab(), "Watchlist");
    tabs->addTab(buildRecommendationsTab(), "Recommendations");
    centralLayout->addWidget(tabs);
    setCentralWidget(central);

    connect(tabs, &QTabWidget::currentChanged, this, [this](int) {
        refreshWatchlistTable();
    });

    applyStyleSheet();
    populateGenreFilter();

    currentBrowseList_.clear();
    for (auto& m : db_.getAllMoviesMutable()) currentBrowseList_.push_back(&m);
    onSortChanged(); // sort movies by title

    refreshWatchlistTable();

    statusBar()->showMessage(
        QString("Loaded %1 movies into the Scream Match.").arg(db_.size()));
}


// Browse and Search tab


QWidget* MainWindow::buildBrowseTab() {
    auto* page = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(page);

    // Search bar
    auto* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel("Search:"));
    searchBox_ = new QLineEdit();
    searchBox_->setPlaceholderText("Type a title, e.g. \"Hereditary\"...");
    searchLayout->addWidget(searchBox_);
    mainLayout->addLayout(searchLayout);

    // Filters
    auto* filterBox = new QGroupBox("Filters");
    auto* filterLayout = new QHBoxLayout(filterBox);

    genreFilter_ = new QComboBox();
    genreFilter_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    minYearSpin_ = new QSpinBox();
    minYearSpin_->setRange(NO_MIN_YEAR, 2030);
    minYearSpin_->setSpecialValueText("Any");
    maxGoreSpin_ = new QSpinBox();
    maxGoreSpin_->setRange(NO_MIN_LEVEL, NO_MAX_LEVEL);
    maxGoreSpin_->setValue(NO_MAX_LEVEL);
    maxIntensitySpin_ = new QSpinBox();
    maxIntensitySpin_->setRange(NO_MIN_LEVEL, NO_MAX_LEVEL);
    maxIntensitySpin_->setValue(NO_MAX_LEVEL);
    minSuspenseSpin_ = new QSpinBox();
    minSuspenseSpin_->setRange(NO_MIN_LEVEL, NO_MAX_LEVEL);

    auto addFiltered = [&](const QString& label, QWidget* w) {
        auto* v = new QVBoxLayout();
        v->addWidget(new QLabel(label));
        v->addWidget(w);
        filterLayout->addLayout(v);
    };
    addFiltered("Subgenre", genreFilter_);
    addFiltered("Min Year", minYearSpin_);
    addFiltered("Max Gore", maxGoreSpin_);
    addFiltered("Max Intensity", maxIntensitySpin_);
    addFiltered("Min Suspense", minSuspenseSpin_);

    auto* applyBtn = new QPushButton("Apply Filters");
    auto* clearBtn = new QPushButton("Clear Filters");
    filterLayout->addWidget(applyBtn);
    filterLayout->addWidget(clearBtn);
    mainLayout->addWidget(filterBox);

    // Sort controls
    auto* sortLayout = new QHBoxLayout();
    sortLayout->addWidget(new QLabel("Sort by:"));
    sortKeyBox_ = new QComboBox();
    sortKeyBox_->addItems({"Title", "Year", "Rating", "Runtime"});
    sortKeyBox_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    sortKeyBox_->view()->setMinimumWidth(120);
    sortLayout->addWidget(sortKeyBox_);
    sortDirBox_ = new QComboBox();
    sortDirBox_->addItems({"Ascending", "Descending"});
    sortDirBox_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    sortDirBox_->view()->setMinimumWidth(120);
    sortLayout->addWidget(sortDirBox_);
    sortLayout->addStretch();
    mainLayout->addLayout(sortLayout);

    // Movie table and details
    auto* splitter = new QSplitter(Qt::Horizontal);

    browseTable_ = new QTableWidget();
    browseTable_->setColumnCount(6);
    browseTable_->setHorizontalHeaderLabels({"Title", "Year", "Subgenre", "Runtime", "I/G/S", "Your Rating"});
    browseTable_->horizontalHeader()->setStretchLastSection(true);
    browseTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    browseTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    splitter->addWidget(browseTable_);

    auto* detailsPanel = new QWidget();
    auto* detailsLayout = new QVBoxLayout(detailsPanel);
    detailsLabel_ = new QLabel("Select a movie to see details.");
    detailsLabel_->setWordWrap(true);
    detailsLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    detailsLabel_->setObjectName("detailsLabel");
    detailsLayout->addWidget(detailsLabel_);

    watchlistButton_ = new QPushButton("Add to Watchlist");
    watchlistButton_->setEnabled(false);
    detailsLayout->addWidget(watchlistButton_);

    auto* rateLayout = new QHBoxLayout();
    ratingSpin_ = new QDoubleSpinBox();
    ratingSpin_->setRange(1.0, 5.0);
    ratingSpin_->setSingleStep(0.5);
    ratingSpin_->setValue(4.0);
    ratingSpin_->setEnabled(false);
    rateButton_ = new QPushButton("Rate");
    rateButton_->setEnabled(false);
    rateLayout->addWidget(ratingSpin_);
    rateLayout->addWidget(rateButton_);
    detailsLayout->addLayout(rateLayout);

    similarButton_ = new QPushButton("Find Similar Movies");
    similarButton_->setEnabled(false);
    detailsLayout->addWidget(similarButton_);

    similarResultsBox_ = new QTextEdit();
    similarResultsBox_->setReadOnly(true);
    similarResultsBox_->setPlaceholderText("Similar movies will appear here.");
    detailsLayout->addWidget(similarResultsBox_);

    splitter->addWidget(detailsPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    connect(searchBox_, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyFilters);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearFilters);
    connect(sortKeyBox_, &QComboBox::currentTextChanged, this, &MainWindow::onSortChanged);
    connect(sortDirBox_, &QComboBox::currentTextChanged, this, &MainWindow::onSortChanged);
    connect(browseTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::onBrowseRowSelected);
    connect(watchlistButton_, &QPushButton::clicked, this, &MainWindow::onToggleWatchlist);
    connect(rateButton_, &QPushButton::clicked, this, &MainWindow::onSubmitRating);
    connect(similarButton_, &QPushButton::clicked, this, &MainWindow::onFindSimilar);

    return page;
}

void MainWindow::populateGenreFilter() {
    genreFilter_->addItem("Any Subgenre");
    for (const auto& g : db_.getAllSubgenres()) {
        genreFilter_->addItem(QString::fromStdString(g));
    }
}

void MainWindow::refreshBrowseTable() {
    browseTable_->setRowCount(static_cast<int>(currentBrowseList_.size()));
    for (int row = 0; row < static_cast<int>(currentBrowseList_.size()); ++row) {
        Movie* m = currentBrowseList_[row];
        browseTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(m->getTitle())));
        browseTable_->setItem(row, 1, new QTableWidgetItem(QString::number(m->getYear())));
        browseTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(m->getSubgenre())));
        browseTable_->setItem(row, 3, new QTableWidgetItem(QString("%1m").arg(m->getRuntime())));
        browseTable_->setItem(row, 4, new QTableWidgetItem(
            QString("%1/%2/%3").arg(m->getIntensity()).arg(m->getGore()).arg(m->getSuspense())));
        browseTable_->setItem(row, 5, new QTableWidgetItem(
            m->hasUserRating() ? QString::number(m->getUserRating(), 'f', 1) : QString("-")));
    }
    browseTable_->resizeColumnsToContents();
}

void MainWindow::onSearchTextChanged(const QString& text) {
    if (text.trimmed().isEmpty()) {
        currentBrowseList_.clear();
        for (auto& m : db_.getAllMoviesMutable()) currentBrowseList_.push_back(&m);
    } else {
        currentBrowseList_ = db_.searchByTitle(text.toStdString());
    }
    onSortChanged(); // re-sort and refresh
}

void MainWindow::onApplyFilters() {
    MovieDatabase::FilterCriteria criteria;
    QString genre = genreFilter_->currentText();
    if (genre != "Any Subgenre") criteria.subgenre = genre.toStdString();
    if (minYearSpin_->value() > NO_MIN_YEAR) criteria.minYear = minYearSpin_->value();
    if (maxGoreSpin_->value() < NO_MAX_LEVEL) criteria.maxGore = maxGoreSpin_->value();
    if (maxIntensitySpin_->value() < NO_MAX_LEVEL) criteria.maxIntensity = maxIntensitySpin_->value();
    if (minSuspenseSpin_->value() > NO_MIN_LEVEL) criteria.minSuspense = minSuspenseSpin_->value();

    currentBrowseList_ = db_.filter(criteria);
    searchBox_->clear();
    onSortChanged();
}

void MainWindow::onClearFilters() {
    genreFilter_->setCurrentIndex(0);
    minYearSpin_->setValue(NO_MIN_YEAR);
    maxGoreSpin_->setValue(NO_MAX_LEVEL);
    maxIntensitySpin_->setValue(NO_MAX_LEVEL);
    minSuspenseSpin_->setValue(NO_MIN_LEVEL);
    searchBox_->clear();
    currentBrowseList_.clear();
    for (auto& m : db_.getAllMoviesMutable()) currentBrowseList_.push_back(&m);
    onSortChanged();
}

void MainWindow::onSortChanged() {
    MovieDatabase::SortKey key;
    QString keyText = sortKeyBox_->currentText();
    if (keyText == "Year") key = MovieDatabase::SortKey::Year;
    else if (keyText == "Rating") key = MovieDatabase::SortKey::Rating;
    else if (keyText == "Runtime") key = MovieDatabase::SortKey::Runtime;
    else key = MovieDatabase::SortKey::Title;

    bool ascending = (sortDirBox_->currentText() == "Ascending");
    MovieDatabase::sortMovies(currentBrowseList_, key, ascending);
    refreshBrowseTable();
}

void MainWindow::showDetails(Movie* m) {
    selectedMovie_ = m;
    if (!m) {
        detailsLabel_->setText("Select a movie to see details.");
        watchlistButton_->setEnabled(false);
        ratingSpin_->setEnabled(false);
        rateButton_->setEnabled(false);
        similarButton_->setEnabled(false);
        return;
    }

    std::ostringstream oss;
    oss << "<h3>" << m->getTitle() << " (" << m->getYear() << ")</h3>"
        << "<b>Subgenre:</b> " << m->getSubgenre() << "<br>"
        << "<b>Runtime:</b> " << m->getRuntime() << " minutes<br>"
        << "<b>Intensity:</b> " << m->getIntensity() << "/10<br>"
        << "<b>Gore:</b> " << m->getGore() << "/10<br>"
        << "<b>Suspense:</b> " << m->getSuspense() << "/10<br>";
    if (m->hasUserRating()) {
        oss << "<b>Your rating:</b> " << QString::number(m->getUserRating(), 'f', 1).toStdString() << "&#9733;<br>";
    }
    detailsLabel_->setText(QString::fromStdString(oss.str()));

    watchlistButton_->setEnabled(true);
    watchlistButton_->setText(watchlist_.contains(m->getTitle()) ? "Remove from Watchlist" : "Add to Watchlist");
    ratingSpin_->setEnabled(true);
    rateButton_->setEnabled(true);
    similarButton_->setEnabled(true);
    similarResultsBox_->clear();
}

void MainWindow::onBrowseRowSelected() {
    auto selected = browseTable_->selectionModel()->selectedRows();
    if (selected.isEmpty()) { showDetails(nullptr); return; }
    int row = selected.first().row();
    if (row < 0 || row >= static_cast<int>(currentBrowseList_.size())) { showDetails(nullptr); return; }
    showDetails(currentBrowseList_[row]);
}

void MainWindow::onToggleWatchlist() {
    if (!selectedMovie_) return;
    if (watchlist_.contains(selectedMovie_->getTitle())) {
        watchlist_.remove(selectedMovie_->getTitle());
        watchlistButton_->setText("Add to Watchlist");
    } else {
        watchlist_.add(selectedMovie_->getTitle());
        watchlistButton_->setText("Remove from Watchlist");
    }
    watchlist_.save();
    refreshWatchlistTable();
}

void MainWindow::onSubmitRating() {
    if (!selectedMovie_) return;
    ratings_.rate(db_, selectedMovie_->getTitle(), ratingSpin_->value());
    ratings_.save();
    showDetails(selectedMovie_);
    refreshBrowseTable();
    statusBar()->showMessage(
        QString("Saved your rating of %1 for \"%2\".")
            .arg(ratingSpin_->value())
            .arg(QString::fromStdString(selectedMovie_->getTitle())), 4000);
}

void MainWindow::onFindSimilar() {
    if (!selectedMovie_) return;
    auto neighbors = graph_.getNeighbors(selectedMovie_->getTitle(), 8);
    if (neighbors.empty()) {
        similarResultsBox_->setPlainText("No strongly similar movies found in the graph.");
        return;
    }
    QString text;
    for (const auto& [title, weight] : neighbors) {
        Movie* nm = db_.findByTitle(title);
        text += QString::fromStdString(title);
        if (nm) text += QString(" (%1, %2)").arg(nm->getYear()).arg(QString::fromStdString(nm->getSubgenre()));
        text += QString(" - similarity %1%\n").arg(static_cast<int>(weight * 100));
    }
    similarResultsBox_->setPlainText(text);
}


// Watchlist tab


QWidget* MainWindow::buildWatchlistTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    watchlistTable_ = new QTableWidget();
    watchlistTable_->setColumnCount(4);
    watchlistTable_->setHorizontalHeaderLabels({"Title", "Year", "Subgenre", "Your Rating"});
    watchlistTable_->horizontalHeader()->setStretchLastSection(true);
    watchlistTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    watchlistTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(watchlistTable_, 1);

    removeWatchlistButton_ = new QPushButton("Remove Selected from Watchlist");
    removeWatchlistButton_->setEnabled(false);
    layout->addWidget(removeWatchlistButton_);

    connect(watchlistTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::onWatchlistRowSelected);
    connect(removeWatchlistButton_, &QPushButton::clicked, this, &MainWindow::onRemoveFromWatchlist);

    return page;
}

void MainWindow::refreshWatchlistTable() {
    const auto& titles = watchlist_.getTitles();
    watchlistTable_->setRowCount(static_cast<int>(titles.size()));
    for (int row = 0; row < static_cast<int>(titles.size()); ++row) {
        Movie* m = db_.findByTitle(titles[row]);
        watchlistTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(titles[row])));
        watchlistTable_->setItem(row, 1, new QTableWidgetItem(m ? QString::number(m->getYear()) : ""));
        watchlistTable_->setItem(row, 2, new QTableWidgetItem(m ? QString::fromStdString(m->getSubgenre()) : ""));
        watchlistTable_->setItem(row, 3, new QTableWidgetItem(
            (m && m->hasUserRating()) ? QString::number(m->getUserRating(), 'f', 1) : QString("-")));
    }
    watchlistTable_->resizeColumnsToContents();
    removeWatchlistButton_->setEnabled(false);
}

void MainWindow::onWatchlistRowSelected() {
    auto selected = watchlistTable_->selectionModel()->selectedRows();
    if (selected.isEmpty()) { removeWatchlistButton_->setEnabled(false); return; }
    int row = selected.first().row();
    if (row < 0 || row >= static_cast<int>(watchlist_.getTitles().size())) return;
    selectedWatchlistTitle_ = watchlist_.getTitles()[row];
    removeWatchlistButton_->setEnabled(true);
}

void MainWindow::onRemoveFromWatchlist() {
    if (selectedWatchlistTitle_.empty()) return;
    watchlist_.remove(selectedWatchlistTitle_);
    watchlist_.save();
    refreshWatchlistTable();
    if (selectedMovie_ && selectedMovie_->getTitle() == selectedWatchlistTitle_) {
        watchlistButton_->setText("Add to Watchlist");
    }
    selectedWatchlistTitle_.clear();
}


// Recommendations tab


QWidget* MainWindow::buildRecommendationsTab() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("How many recommendations?"));
    numRecsSpin_ = new QSpinBox();
    numRecsSpin_->setRange(1, 20);
    numRecsSpin_->setValue(5);
    topRow->addWidget(numRecsSpin_);
    generateRecsButton_ = new QPushButton("Generate Recommendations");
    topRow->addWidget(generateRecsButton_);
    topRow->addStretch();
    layout->addLayout(topRow);

    auto* splitter = new QSplitter(Qt::Vertical);

    recsTable_ = new QTableWidget();
    recsTable_->setColumnCount(4);
    recsTable_->setHorizontalHeaderLabels({"Title", "Year", "Subgenre", "Match %"});
    recsTable_->horizontalHeader()->setStretchLastSection(true);
    recsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    recsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    splitter->addWidget(recsTable_);

    recExplanationBox_ = new QTextEdit();
    recExplanationBox_->setReadOnly(true);
    recExplanationBox_->setPlaceholderText("Select a recommendation to see why it was picked.");
    splitter->addWidget(recExplanationBox_);

    layout->addWidget(splitter, 1);

    connect(generateRecsButton_, &QPushButton::clicked, this, &MainWindow::onGenerateRecommendations);
    connect(recsTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::onRecommendationRowSelected);

    return page;
}

void MainWindow::onGenerateRecommendations() {
    UserProfile profile = Recommender::buildProfile(db_);
    if (profile.isEmpty()) {
        recsTable_->setRowCount(0);
        recExplanationBox_->setPlainText(
            "You haven't rated any movies 4 stars or higher yet.\n"
            "Rate a few favorites on the Browse tab, then come back for personalized picks!");
        return;
    }

    currentRecommendations_ = Recommender::topRecommendations(db_, profile, numRecsSpin_->value());
    recsTable_->setRowCount(static_cast<int>(currentRecommendations_.size()));
    for (int row = 0; row < static_cast<int>(currentRecommendations_.size()); ++row) {
        const auto& rec = currentRecommendations_[row];
        recsTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(rec.movie->getTitle())));
        recsTable_->setItem(row, 1, new QTableWidgetItem(QString::number(rec.movie->getYear())));
        recsTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(rec.movie->getSubgenre())));
        recsTable_->setItem(row, 3, new QTableWidgetItem(QString::number(static_cast<int>(rec.score))));
    }
    recsTable_->resizeColumnsToContents();

    QString header = QString("Taste profile: favors %1 horror | avg intensity %2 | avg gore %3 | avg suspense %4\n\n")
        .arg(QString::fromStdString(profile.favoriteSubgenre()))
        .arg(profile.avgIntensity, 0, 'f', 1)
        .arg(profile.avgGore, 0, 'f', 1)
        .arg(profile.avgSuspense, 0, 'f', 1);
    recExplanationBox_->setPlainText(header + "Select a row above to see why it was recommended.");
}

void MainWindow::onRecommendationRowSelected() {
    auto selected = recsTable_->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;
    int row = selected.first().row();
    if (row < 0 || row >= static_cast<int>(currentRecommendations_.size())) return;
    recExplanationBox_->setPlainText(QString::fromStdString(currentRecommendations_[row].explanation));
}


// Misc


void MainWindow::closeEvent(QCloseEvent* event) {
    ratings_.save();
    watchlist_.save();
    QMainWindow::closeEvent(event);
}

void MainWindow::applyStyleSheet() {
    setStyleSheet(R"(
        QMainWindow, QWidget { background-color: #1b1416; color: #e8e0e0; font-size: 13px; }
        QTabWidget::pane { border: 1px solid #3a2226; }
        QTabBar::tab {
            background: #2a1a1d; color: #cbb9ba; padding: 8px 16px;
            border: 1px solid #3a2226; border-bottom: none;
        }
        QTabBar::tab:selected { background: #7a1f2b; color: white; }
        QGroupBox {
            border: 1px solid #3a2226; border-radius: 4px; margin-top: 8px; padding-top: 8px;
            font-weight: bold; color: #cbb9ba;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
        QPushButton {
            background-color: #7a1f2b; color: white; border: none;
            border-radius: 4px; padding: 6px 12px; font-weight: bold;
        }
        QPushButton:hover { background-color: #96262f; }
        QPushButton:disabled { background-color: #4a3a3c; color: #8a7a7c; }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit {
            background-color: #2a1a1d; color: #e8e0e0; border: 1px solid #3a2226;
            border-radius: 3px; padding: 4px;
        }
        QTableWidget {
            background-color: #211517; alternate-background-color: #2a1a1d;
            gridline-color: #3a2226; selection-background-color: #7a1f2b;
        }
        QHeaderView::section {
            background-color: #3a2226; color: #e8e0e0; padding: 4px; border: none;
        }
        #detailsLabel { font-size: 14px; }
        QStatusBar { color: #cbb9ba; }
    )");
    browseTable_->setAlternatingRowColors(true);
    watchlistTable_->setAlternatingRowColors(true);
    recsTable_->setAlternatingRowColors(true);
}
