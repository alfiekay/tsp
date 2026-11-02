#include <iostream>
#include <algorithm>

#include "header.hpp"

struct Data {
    int x;
    float y;
};

Data findClosestPoints(const std::vector<Point>& points, int index) {
    float bestDistance = 100;
    int bestIndex = index + 1; // -1 means "no point found"
    for (int i = index + 1; i < static_cast<int>(points.size()) - 1; i++) {
        float distance = findDistance(points[index], points[i]);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    if (bestIndex == -1) { std::cout << "ERROR\n"; }
    return Data{bestIndex, bestDistance};
}

std::vector<Point> nearestNeighbour(std::vector<Point>& points) {
    float bestDistance = 100 * points.size();
    std::vector<Point> bestRoute;
    for (int i = 0; i < points.size(); i++) { // starting points
        float totalDistance = 0;
        std::vector<Point> newPoints(points.begin(), points.end());
        std::rotate(newPoints.begin(), newPoints.begin() + i, newPoints.begin() + i + 1); // move ith point to start

        for (int j = 0; j < points.size(); j++) {
            auto data = findClosestPoints(newPoints, j);
            int newPointIndex = data.x;
            totalDistance += data.y;

            std::rotate(newPoints.begin() + j + 1, newPoints.begin() + newPointIndex,
                        newPoints.begin() + newPointIndex + 1);
        }

        totalDistance += findDistance(newPoints[0], newPoints.back());
        newPoints.push_back(newPoints.front()); // connect into hamiltonian loop

        if (totalDistance < bestDistance) {
            bestDistance = totalDistance;
            bestRoute = newPoints;
        }
    }
    return bestRoute;
}

bool operator<(const Point& a, const Point& b) { // for points algorithm use
    if (a.x != b.x) return a.x < b.x;
    return a.y < b.y;
}

std::vector<std::vector<Point>> permutations(std::vector<Point> points) {
    std::sort(points.begin(), points.end());
    std::vector<Point> localPoints(points.begin() + 1, points.end());
    std::vector<std::vector<Point>> perms;
    do {
        localPoints.insert(localPoints.begin(), points[0]);
        localPoints.insert(localPoints.end(), points[0]);
        perms.push_back(localPoints);
        localPoints.erase(localPoints.begin());
        localPoints.pop_back();
    }
    while (std::next_permutation(localPoints.begin(), localPoints.end()));

    return perms;
}

std::vector<Point> bruteForce(std::vector<Point>& points) {
    auto perms = permutations(points);

    std::vector<Point> optimalRoute;
    float optimalDistance = 20 * perms[0].size();

    for (const auto& route : perms) {
        float distance = 0;
        for (int i = 0; i < route.size() - 1; i++) { distance += findDistance(route[i], route[i + 1]); }
        if (distance < optimalDistance) {
            optimalDistance = distance;
            optimalRoute = route;
        }
    }
    return optimalRoute;
}

struct PartialRoutes {
    std::vector<Point> oldPoints;
    std::vector<Point> newPoints;
};

PartialRoutes initialiseRoute(std::vector<Point>& points) {
    std::vector<Point> oldPoints(points.begin(), points.end());
    std::vector<Point> newPoints;
    struct Indices {
        int i;
        int j;
    } indices;

    float bestDistance = 100;
    float distance;

    for (int i = 0; i < oldPoints.size() - 1; i++) {
        for (int j = i + 1; j < oldPoints.size(); j++) {
            distance = findDistance(oldPoints[i], oldPoints[j]);
            if (distance < bestDistance) {
                bestDistance = distance;
                indices = {i, j};
            }
        }
    }

    newPoints = {oldPoints[indices.i], oldPoints[indices.j]};
    oldPoints.erase(oldPoints.begin() + indices.j);
    oldPoints.erase(oldPoints.begin() + indices.i);
    return {oldPoints, newPoints};
}

std::vector<Point> cheapestInsertion(std::vector<Point>& points) {
    auto partialRoute = initialiseRoute(points);
    std::vector<Point> oldPoints = partialRoute.oldPoints;
    std::vector<Point> newPoints = partialRoute.newPoints;
    int oldPointIndex;
    int newPointIndex;
    while (!oldPoints.empty()) {
        float bestDistance = 100 * points.size();
        for (int i = 0; i < oldPoints.size(); i++) {
            for (int j = 0; j < newPoints.size(); j++) {
                const Point& a = newPoints[j];
                const Point& b = newPoints[(j + 1) % newPoints.size()];
                float extra = (findDistance(oldPoints[i], a)
                        + findDistance(oldPoints[i], b))
                    - findDistance(a, b);

                if (extra < bestDistance) {
                    bestDistance = extra;
                    oldPointIndex = i;
                    newPointIndex = j;
                }
            }
        }
        newPoints.insert(newPoints.begin() + newPointIndex + 1, oldPoints[oldPointIndex]);
        oldPoints.erase(oldPoints.begin() + oldPointIndex);
    }
    newPoints.push_back(newPoints[0]);
    return newPoints;
}
