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
void drawTime(const std::vector<Diagnostics>& BFDiagnostics, const std::vector<Diagnostics>& NNDiagnostics,
              const std::vector<Diagnostics>& CIDiagnostics);
void drawLength(const std::vector<Diagnostics>& BFDiagnostics, const std::vector<Diagnostics>& NNDiagnostics,
                const std::vector<Diagnostics>& CIDiagnostics);

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

    for (int i = 5; i <= RANGE; i += STEP) { std::cout << '#'; }
    std::cout << '\n';
    for (int i = 5; i <= RANGE; i += STEP) {
        auto points = generatePoints(i);
        NNDiagnostics.push_back(diagnoseFunction(nearestNeighbour, points));
        CIDiagnostics.push_back(diagnoseFunction(cheapestInsertion, points));
        if (i < 10) { BFDiagnostics.push_back(diagnoseFunction(bruteForce, points)); }
        else { BFDiagnostics.push_back({NNDiagnostics.back().route, static_cast<float>(0.712 * sqrt(i * 100)), 0}); }
        std::cout << '%' << std::flush;
    }

    drawTime(BFDiagnostics, NNDiagnostics, CIDiagnostics);
    drawLength(BFDiagnostics, NNDiagnostics, CIDiagnostics);

    // drawRoute(BFDiagnostics[0].route, "BruteForce");
    // drawRoute(NNDiagnostics.back().route, "NearestNeighbour");
    // drawRoute(CIDiagnostics.back().route, "CheapestInsertion");

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

void drawTime(const std::vector<Diagnostics>& BFDiagnostics, const std::vector<Diagnostics>& NNDiagnostics,
              const std::vector<Diagnostics>& CIDiagnostics) {
    auto* series1 = new QLineSeries;
    for (const auto& route : BFDiagnostics) {
        if (route.duration == 0) { break; }
        series1->append({static_cast<float>(route.route.size()) - 1.0, route.duration});
    }
    series1->setName("BruteForce");

    auto* series2 = new QLineSeries;
    for (const auto& route : NNDiagnostics) {
        series2->append({static_cast<float>(route.route.size()) - 1.0, route.duration});
    }
    series2->setName("NearestNeighbour");

    auto* series3 = new QLineSeries;
    for (const auto& route : CIDiagnostics) {
        series3->append({static_cast<float>(route.route.size()) - 1.0, route.duration});
    }
    series3->setName("CheapestInsertion");

    auto* chart = new QChart;
    chart->addSeries(series1);
    chart->addSeries(series2);
    chart->addSeries(series3);
    // chart->setTitle(QString::fromStdString("Computation Time of Different TSP Algorithms"));
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    // chart->createDefaultAxes();

    auto* xAxis = new QValueAxis;
    auto* yAxis = new QValueAxis;
    xAxis->setLabelFormat("%d");
    yAxis->setLabelFormat("%d");
    // xAxis->setRange(0, 10);
    // yAxis->setRange(0, 10);
    xAxis->setTitleText("Number of Points");
    yAxis->setTitleText("Time Taken to Compute (milliseconds)");
    chart->addAxis(xAxis, Qt::AlignBottom);
    chart->addAxis(yAxis, Qt::AlignLeft);
    series2->attachAxis(xAxis);
    series2->attachAxis(yAxis);
    series3->attachAxis(xAxis);
    series3->attachAxis(yAxis);
    series1->attachAxis(xAxis);
    series1->attachAxis(yAxis);

    auto* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->resize(700, 700);
    chartView->show();

    QPixmap pixmap = chartView->grab();
    const QString QFilename = QString::fromStdString("ComputationTime") + ".png";
    pixmap.save(QFilename, "PNG");
}

void drawLength(const std::vector<Diagnostics>& BFDiagnostics, const std::vector<Diagnostics>& NNDiagnostics,
                const std::vector<Diagnostics>& CIDiagnostics) {
    auto* series1 = new QLineSeries;
    for (const auto& route : BFDiagnostics) {
        if (route.length == 0) { break; }
        series1->append({static_cast<float>(route.route.size()) - 1.0, route.length});
    }
    series1->setName("BruteForce(Estimation)");

    auto* series2 = new QLineSeries;
    for (const auto& route : NNDiagnostics) {
        series2->append({static_cast<float>(route.route.size()) - 1.0, route.length});
    }
    series2->setName("NearestNeighbour");

    auto* series3 = new QLineSeries;
    for (const auto& route : CIDiagnostics) {
        series3->append({static_cast<float>(route.route.size()) - 1.0, route.length});
    }
    series3->setName("CheapestInsertion");

    auto* chart = new QChart;
    chart->addSeries(series1);
    chart->addSeries(series2);
    chart->addSeries(series3);
    // chart->setTitle(QString::fromStdString("Length of Different TSP Algorithms"));
    chart->layout()->setContentsMargins(0, 0, 0, 0);

    // chart->createDefaultAxes();

    auto* xAxis = new QValueAxis;
    auto* yAxis = new QValueAxis;
    xAxis->setLabelFormat("%d");
    yAxis->setLabelFormat("%d");
    // xAxis->setRange(0, 10);
    yAxis->setRange(0, std::max(NNDiagnostics.back().length, CIDiagnostics.back().length));
    xAxis->setTitleText("Number of Points");
    yAxis->setTitleText("Length (Arbitrary units)");
    chart->addAxis(xAxis, Qt::AlignBottom);
    chart->addAxis(yAxis, Qt::AlignLeft);
    series1->attachAxis(xAxis);
    series1->attachAxis(yAxis);
    series2->attachAxis(xAxis);
    series2->attachAxis(yAxis);
    series3->attachAxis(xAxis);
    series3->attachAxis(yAxis);

    auto* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->resize(700, 700);
    chartView->show();

    QPixmap pixmap = chartView->grab();
    const QString QFilename = QString::fromStdString("RouteLength") + ".png";
    pixmap.save(QFilename, "PNG");
}
