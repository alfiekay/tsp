#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <functional>

#include <QApplication>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QGraphicsLayout>

#include "header.hpp"

using namespace std::chrono;

typedef std::vector<Point> vP;

struct Diagnostics {
    vP route;
    float length;
    float duration;
};

void drawRoute(vP& route, std::string filename);
float calculateRouteLength(vP& route);
Diagnostics diagnoseFunction(const std::function<vP(vP&)>& fn, vP& points);
QLineSeries* makeSeries(const std::vector<Diagnostics>& diagnostics, const QString& name, bool plotTime,
                        bool stopAtZero);
void drawChart(const std::vector<QLineSeries*>& allSeries, const QString& yTitle, const QString& filename,
               double yMax);
void drawTime(const std::vector<Diagnostics>& BFDiagnostics, const std::vector<Diagnostics>& NNDiagnostics,
              const std::vector<Diagnostics>& CIDiagnostics, const std::vector<Diagnostics>& NMDiagnostics,
              const std::vector<Diagnostics>& CHDiagnostics);
void drawLength(const std::vector<Diagnostics>& BFDiagnostics, const std::vector<Diagnostics>& NNDiagnostics,
                const std::vector<Diagnostics>& CIDiagnostics, const std::vector<Diagnostics>& NMDiagnostics,
                const std::vector<Diagnostics>& CHDiagnostics);

int main(int argc, char* argv[]) {
    // boilerplate
    std::cout << std::fixed;
    std::srand(std::time(nullptr));
    QApplication app(argc, argv);
    // constants
    int RANGE = 100;
    int STEP = 1;

    std::cout << "Range and Step:\n";
    std::cin >> RANGE;
    std::cin >> STEP;

    std::vector<Diagnostics> BFDiagnostics;
    std::vector<Diagnostics> NNDiagnostics;
    std::vector<Diagnostics> CIDiagnostics;
    std::vector<Diagnostics> NMDiagnostics;
    std::vector<Diagnostics> CHDiagnostics;

    for (int i = 5; i <= RANGE; i += STEP) { std::cout << '#'; } // loading bar
    std::cout << '\n';
    for (int i = 5; i <= RANGE; i += STEP) {
        auto points = generatePoints(i);
        NNDiagnostics.push_back(diagnoseFunction(nearestNeighbour, points));
        CIDiagnostics.push_back(diagnoseFunction(cheapestInsertion, points));
        NMDiagnostics.push_back(diagnoseFunction(nearestMerge, points));
        CHDiagnostics.push_back(diagnoseFunction(convexHullInsertion, points));
        if (i < 10) { BFDiagnostics.push_back(diagnoseFunction(bruteForce, points)); }
        else { BFDiagnostics.push_back({NNDiagnostics.back().route, static_cast<float>(0.712 * sqrt(i * 100)), 0}); }
        std::cout << '~' << std::flush;
    }

    drawTime(BFDiagnostics, NNDiagnostics, CIDiagnostics, NMDiagnostics, CHDiagnostics);
    drawLength(BFDiagnostics, NNDiagnostics, CIDiagnostics, NMDiagnostics, CHDiagnostics);

    // drawRoute(BFDiagnostics[0].route, "BruteForce");
    // drawRoute(NNDiagnostics.back().route, "NearestNeighbour");
    // drawRoute(CIDiagnostics.back().route, "CheapestInsertion");
    // drawRoute(NMDiagnostics.back().route, "NearestMerge");
    // drawRoute(CHDiagnostics.back().route, "ConvexHullInsertion");

    return app.exec();
}

void drawRoute(vP& route, const std::string filename) {
    auto* series = new QLineSeries;
    for (const auto& point : route) { series->append({point.x, point.y}); }

    series->setName("Route");
    series->setPointsVisible(true);
    series->setMarkerSize(4);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(QString::fromStdString(filename));
    chart->legend()->hide();
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setBackgroundRoundness(0);
    // chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* xAxis = new QValueAxis;
    auto* yAxis = new QValueAxis;
    xAxis->setRange(-0.5, 10.5);
    yAxis->setRange(-0.5, 10.5);
    xAxis->setLabelsVisible(false);
    yAxis->setLabelsVisible(false);
    chart->addAxis(xAxis, Qt::AlignBottom);
    chart->addAxis(yAxis, Qt::AlignLeft);

    series->attachAxis(xAxis);
    series->attachAxis(yAxis);

    auto* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->resize(700, 700);
    chartView->show();

    QPixmap pixmap = chartView->grab();
    const QString QFilename = QString::fromStdString(filename) + ".png";
    pixmap.save(QFilename, "PNG");
}

float calculateRouteLength(vP& route) {
    float totalDistance = 0;
    for (int i = 0; i < route.size(); i++) { totalDistance += findDistance(route[i], route[(i + 1) % route.size()]); }
    return totalDistance;
}

Diagnostics diagnoseFunction(const std::function<vP(vP&)>& fn, vP& points) {
    auto start = high_resolution_clock::now();
    vP solution = fn(points);
    auto stop = high_resolution_clock::now();
    float duration = (duration_cast<microseconds>(stop - start)).count();
    float length = calculateRouteLength(solution);
    return {solution, length, duration};
}

// Builds one line on a chart: x = number of points, y = either time or length.
// stopAtZero is for brute force, whose entries past the cut-off have no real time.
QLineSeries* makeSeries(const std::vector<Diagnostics>& diagnostics, const QString& name, bool plotTime,
                        bool stopAtZero) {
    auto* series = new QLineSeries;
    for (const auto& d : diagnostics) {
        float value = plotTime ? d.duration : d.length;
        if (stopAtZero && value == 0) { break; }
        series->append(static_cast<float>(d.route.size()) - 1.0, value);
    }
    series->setName(name);
    return series;
}

// Puts the series on a chart with the usual axes, shows it and saves it as a png.
// yMax <= 0 means "let Qt choose".
void drawChart(const std::vector<QLineSeries*>& allSeries, const QString& yTitle, const QString& filename,
               double yMax) {
    auto* chart = new QChart;
    for (auto* series : allSeries) { chart->addSeries(series); }
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    auto* xAxis = new QValueAxis;
    auto* yAxis = new QValueAxis;
    xAxis->setLabelFormat("%d");
    yAxis->setLabelFormat("%d");

    // axes made by hand don't scale to the data (Qt defaults to 0-10), so work out the ranges ourselves
    double xMin = 0;
    double xMaxData = 0;
    double yMaxData = 0;
    bool first = true;
    for (auto* series : allSeries) {
        for (const QPointF& point : series->points()) {
            if (first || point.x() < xMin) { xMin = point.x(); }
            if (first || point.x() > xMaxData) { xMaxData = point.x(); }
            if (first || point.y() > yMaxData) { yMaxData = point.y(); }
            first = false;
        }
    }
    xAxis->setRange(xMin, std::max(xMaxData, xMin + 1));
    yAxis->setRange(0, yMax > 0 ? yMax : std::max(yMaxData, 1.0));
    xAxis->applyNiceNumbers();
    yAxis->applyNiceNumbers();
    xAxis->setTitleText("Number of Points");
    yAxis->setTitleText(yTitle);
    chart->addAxis(xAxis, Qt::AlignBottom);
    chart->addAxis(yAxis, Qt::AlignLeft);
    for (auto* series : allSeries) {
        series->attachAxis(xAxis);
        series->attachAxis(yAxis);
    }
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setBackgroundRoundness(0);

    auto* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->resize(700, 700);
    chartView->show();

    QPixmap pixmap = chartView->grab();
    pixmap.save(filename + ".png", "PNG");
}

void drawTime(const std::vector<Diagnostics>& BFDiagnostics, const std::vector<Diagnostics>& NNDiagnostics,
              const std::vector<Diagnostics>& CIDiagnostics, const std::vector<Diagnostics>& NMDiagnostics,
              const std::vector<Diagnostics>& CHDiagnostics) {
    drawChart({makeSeries(BFDiagnostics, "BruteForce", true, true),
               makeSeries(NNDiagnostics, "NearestNeighbour", true, false),
               makeSeries(CIDiagnostics, "CheapestInsertion", true, false),
               makeSeries(NMDiagnostics, "NearestMerge", true, false),
               makeSeries(CHDiagnostics, "ConvexHullInsertion", true, false)},
              "Time Taken to Compute (microseconds)", "ComputationTime", 0);
}

void drawLength(const std::vector<Diagnostics>& BFDiagnostics, const std::vector<Diagnostics>& NNDiagnostics,
                const std::vector<Diagnostics>& CIDiagnostics, const std::vector<Diagnostics>& NMDiagnostics,
                const std::vector<Diagnostics>& CHDiagnostics) {
    // y axis goes up to the longest final route
    float yMax = std::max({NNDiagnostics.back().length, CIDiagnostics.back().length, NMDiagnostics.back().length,
                           CHDiagnostics.back().length});

    drawChart({makeSeries(BFDiagnostics, "BruteForce(Estimation)", false, false),
               makeSeries(NNDiagnostics, "NearestNeighbour", false, false),
               makeSeries(CIDiagnostics, "CheapestInsertion", false, false),
               makeSeries(NMDiagnostics, "NearestMerge", false, false),
               makeSeries(CHDiagnostics, "ConvexHullInsertion", false, false)},
              "Length (Arbitrary units)", "RouteLength", yMax);
}