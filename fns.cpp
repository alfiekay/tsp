#include <vector>
#include <cmath>

#include "header.hpp"

typedef std::vector<Point> vP;

float findDistance(const Point p1, const Point p2) { return std::hypot(p1.x - p2.x, p1.y - p2.y); }

vP generatePoints(int num) {
    vP points;
    for (int i = 0; i < num; i++) {
        Point point;
        point.x = (rand() % 1000) / 100.0f;
        point.y = (rand() % 1000) / 100.0f;
        points.push_back(point);
    }
    return points;
}
